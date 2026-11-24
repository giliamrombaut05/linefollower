/*
  LINEFOLLOWER - FINAAL PROGRAMMA  (ESP32-S3 SuperMini)

  Bouwt voort op het basisprogramma (zelfde commando's, EEPROM, interrupt en wifi) en voegt toe:
    - QTR-8A uitlezen met kalibratie (wit / zwart) en normalisatie
    - positie van de lijn berekenen (gewogen gemiddelde van de 8 sensoren)
    - PID-regeling die de twee motoren aanstuurt via de DRV8833

  STATUS: eerste versie, nog niet op de baan getest. Kp, Ki, Kd, power en diff moeten nog
  afgesteld worden (zie readme.md in deze map).

  Pinnen (conform schema):
    sensoren S1..S8 = GPIO1, 2, 5, 6, 7, 8, 9, 10  (allemaal ADC1, werkt ook met wifi aan)
    motor M1 (links)  : IN1 = GPIO13, IN2 = GPIO12
    motor M2 (rechts) : IN3 = GPIO4,  IN4 = GPIO3
    EEPROM            : SDA = GPIO43 (TX), SCL = GPIO44 (RX), adres 0x50
    start/stop knop   : GPIO11 (naar GND, INPUT_PULLUP)

  Seriele commando's (115200 baud, eindigen met Enter):
    help | get | sensors | start | stop | save | defaults
    debug on|off
    set kp|ki|kd <waarde>
    set power <0-255>        maximale motorkracht
    set diff <0-1>           hoe sterk de correctie het snelheidsverschil bepaalt
    set cycle <100-100000>   cyclustijd in microseconden
    set invert <0|1>         draait de stuurrichting om
    calibrate white          sensorbalk boven wit houden
    calibrate black          sensorbalk boven zwart houden

  Instellingen Arduino IDE: "ESP32S3 Dev Module", USB CDC On Boot = Enabled.
  Wifi: netwerk "LineFollower", webpagina op 192.168.4.1.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

// Wifi-netwerk van de robot
#define WIFI_SSID "LineFollower"
#define WIFI_PASS "GiliamWouter"

// ============================
// Pinnen en constanten
// ============================
const uint8_t PIN_SDA  = 43;   // TX
const uint8_t PIN_SCL  = 44;   // RX
const uint8_t PIN_KNOP = 11;
const uint8_t PIN_SENSOR[8] = {1, 2, 5, 6, 7, 8, 9, 10};   // S1 .. S8
const uint8_t PIN_IN1 = 13;    // motor M1 (links)
const uint8_t PIN_IN2 = 12;
const uint8_t PIN_IN3 = 4;     // motor M2 (rechts)
const uint8_t PIN_IN4 = 3;
const int     PIN_LED = -1;    // zet op een vrije GPIO voor een knipperende led (-1 = geen)

uint8_t        eepromAddr   = 0x50;   // wordt automatisch gezocht (0x50 - 0x57)
const uint16_t EEPROM_START = 0;
const uint32_t EEPROM_MAGIC = 0x4C464C32;   // "LFL2"

const unsigned long DEBOUNCE_MS = 30;   // flank moet minstens zo lang laag blijven
const unsigned long RELEASE_MS  = 50;   // knop moet zo lang stabiel los zijn

const int CAL_SAMPLES       = 64;    // metingen per kalibratie
const int CAL_MIN_VERSCHIL  = 150;   // minimaal verschil wit/zwart om een sensor als gekalibreerd te zien

// ============================
// Parameters (staan in de EEPROM)
// ============================
struct __attribute__((packed)) Params {
  float    kp;
  float    ki;
  float    kd;
  float    diff;
  uint32_t cycleTime;     // microseconden
  uint16_t power;         // 0 - 255
  uint8_t  debug;
  uint8_t  invert;
  uint16_t calMin[8];     // gemeten waarde boven wit
  uint16_t calMax[8];     // gemeten waarde boven zwart
};

struct __attribute__((packed)) Stored {
  uint32_t magic;
  Params   p;
  uint32_t checksum;
};

static_assert(sizeof(Stored) <= 64, "Stored moet in een EEPROM-pagina (64 bytes) passen");

Params params;

// ============================
// Toestand
// ============================
WebServer server(80);

bool runRobot = false;
bool eepromOk = false;
bool dirty    = false;           // parameters gewijzigd, nog niet opgeslagen
String lastMessage = "";

uint32_t cycleCount = 0;
unsigned long lastCycleStart = 0;
unsigned long lastPidUs = 0;

int   rawVal[8];
float normVal[8];
float pos = 0, lastPos = 0;      // positie van de lijn: -1 (links) .. +1 (rechts)
bool  lijnGezien = false;

float integral = 0, prevE = 0, dFilt = 0, corr = 0;
bool  havePrev = false;
int   motorL = 0, motorR = 0;

String serialLine = "";

volatile bool knopFlag = false;
volatile unsigned long flankTijd = 0;

// ============================
// Hulpfuncties
// ============================
float clampf(float v, float lo, float hi) {
  if (!(v >= lo)) return lo;     // vangt ook NaN op
  if (v > hi) return hi;
  return v;
}

long clampl(long v, long lo, long hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

bool isNumber(const String& s) {
  if (s.length() == 0) return false;
  bool punt = false;
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '-' && i == 0 && s.length() > 1) continue;
    if (c == '.' && !punt) { punt = true; continue; }
    if (c < '0' || c > '9') return false;
  }
  return true;
}

// ============================
// EEPROM (I2C, 24C256)
// ============================
bool eepromDetect() {
  // A0..A2 bepalen het adres (0x50 - 0x57): zoek het eerste dat antwoordt
  for (uint8_t a = 0x50; a <= 0x57; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      eepromAddr = a;
      Serial.print("EEPROM gevonden op adres 0x");
      Serial.println(a, HEX);
      return true;
    }
  }
  return false;
}

// len mag niet over een paginagrens (64 bytes) gaan
bool eepromWrite(uint16_t addr, const uint8_t* data, size_t len) {
  Wire.beginTransmission(eepromAddr);
  Wire.write((uint8_t)(addr >> 8));
  Wire.write((uint8_t)(addr & 0xFF));
  Wire.write(data, len);
  if (Wire.endTransmission() != 0) return false;

  // ACK-polling: wacht tot de schrijfcyclus klaar is (max 20 ms)
  unsigned long t = millis();
  while (millis() - t < 20) {
    Wire.beginTransmission(eepromAddr);
    if (Wire.endTransmission() == 0) return true;
    delay(1);
  }
  return false;
}

bool eepromRead(uint16_t addr, uint8_t* data, size_t len) {
  Wire.beginTransmission(eepromAddr);
  Wire.write((uint8_t)(addr >> 8));
  Wire.write((uint8_t)(addr & 0xFF));
  if (Wire.endTransmission() != 0) return false;

  uint8_t got = Wire.requestFrom((uint8_t)eepromAddr, (uint8_t)len);
  if (got != len) return false;
  for (size_t i = 0; i < len; i++) data[i] = Wire.read();
  return true;
}

uint32_t calcChecksum(const Params& p) {
  const uint8_t* b = (const uint8_t*)&p;
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < sizeof(Params); i++) {
    h ^= b[i];
    h *= 16777619u;
  }
  return h;
}

void setDefaults() {
  params.kp = 1.00f;
  params.ki = 0.00f;
  params.kd = 0.02f;
  params.diff = 0.80f;
  params.cycleTime = 2000;
  params.power = 120;
  params.debug = 0;
  params.invert = 0;
  for (int i = 0; i < 8; i++) {
    params.calMin[i] = 0;
    params.calMax[i] = 4095;
  }
}

void validateParams() {
  params.kp = clampf(params.kp, 0.0f, 50.0f);
  params.ki = clampf(params.ki, 0.0f, 50.0f);
  params.kd = clampf(params.kd, 0.0f, 100.0f);
  params.diff = clampf(params.diff, 0.0f, 1.0f);
  params.power = (uint16_t)clampl(params.power, 0, 255);
  params.cycleTime = (uint32_t)clampl((long)params.cycleTime, 100, 100000);
  params.debug = params.debug ? 1 : 0;
  params.invert = params.invert ? 1 : 0;
  for (int i = 0; i < 8; i++) {
    params.calMin[i] = (uint16_t)clampl(params.calMin[i], 0, 4095);
    params.calMax[i] = (uint16_t)clampl(params.calMax[i], 0, 4095);
  }
}

bool saveParams() {
  if (!eepromOk) return false;
  Stored s;
  s.magic = EEPROM_MAGIC;
  s.p = params;
  s.checksum = calcChecksum(s.p);
  return eepromWrite(EEPROM_START, (const uint8_t*)&s, sizeof(s));
}

void loadParams() {
  Stored s;
  bool ok = eepromOk &&
            eepromRead(EEPROM_START, (uint8_t*)&s, sizeof(s)) &&
            s.magic == EEPROM_MAGIC &&
            s.checksum == calcChecksum(s.p);

  if (ok) {
    params = s.p;
    validateParams();
    Serial.println("Parameters geladen uit EEPROM");
  } else {
    setDefaults();
    dirty = true;                // standaardwaarden meteen wegschrijven
    Serial.println("Geen geldige data in EEPROM: standaardwaarden gebruikt");
  }
}

// Schrijft enkel als er iets gewijzigd is en de robot niet rijdt
void flushParams() {
  if (!dirty || runRobot) return;
  dirty = false;
  if (!eepromOk) {
    Serial.println("WAARSCHUWING: EEPROM niet gevonden, niet opgeslagen");
    return;
  }
  if (saveParams()) Serial.println("Parameters opgeslagen in EEPROM");
  else              Serial.println("FOUT: opslaan in EEPROM mislukt");
}

// ============================
// Motoren (DRV8833)
// ============================
// snelheid: 0 - 255 (enkel vooruit)
void driveMotor(uint8_t pin1, uint8_t pin2, int snelheid) {
  analogWrite(pin1, snelheid);
  analogWrite(pin2, 0);
}

void motorsStop() {
  analogWrite(PIN_IN1, 0);
  analogWrite(PIN_IN2, 0);
  analogWrite(PIN_IN3, 0);
  analogWrite(PIN_IN4, 0);
  motorL = 0;
  motorR = 0;
}

// ============================
// Sensoren
// ============================
bool isCalibrated() {
  for (int i = 0; i < 8; i++) {
    if ((int)params.calMax[i] < (int)params.calMin[i] + CAL_MIN_VERSCHIL) return false;
  }
  return true;
}

void readSensors() {
  for (int i = 0; i < 8; i++) {
    int r = analogRead(PIN_SENSOR[i]);
    rawVal[i] = r;
    int lo = params.calMin[i];
    int hi = params.calMax[i];
    if (hi - lo < CAL_MIN_VERSCHIL) { lo = 0; hi = 4095; }   // niet gekalibreerd
    normVal[i] = clampf((float)(r - lo) / (float)(hi - lo), 0.0f, 1.0f);   // 0 = wit, 1 = zwart
  }
}

void computePosition() {
  float som = 0, gew = 0;
  for (int i = 0; i < 8; i++) {
    som += normVal[i];
    gew += normVal[i] * ((float)i - 3.5f);
  }
  if (som > 0.5f) {
    pos = gew / som / 3.5f;          // -1 .. +1
    lastPos = pos;
    lijnGezien = true;
  } else {
    pos = (lastPos >= 0) ? 1.0f : -1.0f;   // lijn kwijt: blijf draaien naar de laatste kant
    lijnGezien = false;
  }
}

// black = true: sensorbalk boven zwart, anders boven wit
bool calibrate(bool black) {
  if (runRobot) {
    lastMessage = "Stop de robot eerst.";
    return false;
  }
  uint32_t acc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  for (int n = 0; n < CAL_SAMPLES; n++) {
    for (int i = 0; i < 8; i++) acc[i] += analogRead(PIN_SENSOR[i]);
    delay(3);
  }
  for (int i = 0; i < 8; i++) {
    uint16_t gem = (uint16_t)(acc[i] / CAL_SAMPLES);
    if (black) params.calMax[i] = gem;
    else       params.calMin[i] = gem;
  }
  dirty = true;
  Serial.println(black ? "Kalibratie zwart klaar" : "Kalibratie wit klaar");
  if (!isCalibrated()) Serial.println("(nog niet beide kalibraties gedaan of verschil te klein)");
  return true;
}

// ============================
// Parameters instellen (gedeeld door serial en web)
// ============================
bool setParam(const String& key, const String& val) {
  if (!isNumber(val)) {
    lastMessage = "Ongeldige waarde: " + val;
    return false;
  }
  if      (key == "kp")    params.kp = clampf(val.toFloat(), 0.0f, 50.0f);
  else if (key == "ki")    params.ki = clampf(val.toFloat(), 0.0f, 50.0f);
  else if (key == "kd")    params.kd = clampf(val.toFloat(), 0.0f, 100.0f);
  else if (key == "diff")  params.diff = clampf(val.toFloat(), 0.0f, 1.0f);
  else if (key == "power" || key == "speed") params.power = (uint16_t)clampl(val.toInt(), 0, 255);
  else if (key == "cycle") params.cycleTime = (uint32_t)clampl(val.toInt(), 100, 100000);
  else if (key == "invert") params.invert = val.toInt() ? 1 : 0;
  else {
    lastMessage = "Onbekende parameter: " + key;
    return false;
  }
  dirty = true;
  return true;
}

void resetPid() {
  integral = 0;
  prevE = 0;
  dFilt = 0;
  corr = 0;
  havePrev = false;
}

void setRun(bool r) {
  runRobot = r;
  if (r) {
    cycleCount = 0;
    resetPid();
    if (!isCalibrated()) Serial.println("WAARSCHUWING: sensoren niet gekalibreerd (calibrate white / calibrate black)");
  } else {
    motorsStop();
  }
  Serial.println(r ? "ROBOT START" : "ROBOT STOP");
}

void setDebug(bool d) {
  params.debug = d ? 1 : 0;
  dirty = true;
  Serial.print("Debug = ");
  Serial.println(d ? "AAN" : "UIT");
}

// ============================
// Seriele commando's
// ============================
void printParams() {
  Serial.println("===== PARAMETERS =====");
  Serial.printf("Kp        = %.3f\n", params.kp);
  Serial.printf("Ki        = %.3f\n", params.ki);
  Serial.printf("Kd        = %.3f\n", params.kd);
  Serial.printf("Power     = %u\n", (unsigned)params.power);
  Serial.printf("Diff      = %.2f\n", params.diff);
  Serial.printf("Invert    = %u\n", (unsigned)params.invert);
  Serial.printf("CycleTime = %lu us\n", (unsigned long)params.cycleTime);
  Serial.printf("Debug     = %s\n", params.debug ? "AAN" : "UIT");
  Serial.printf("Status    = %s\n", runRobot ? "RUN" : "STOP");
  Serial.printf("EEPROM    = %s\n", eepromOk ? "OK" : "NIET GEVONDEN");
  Serial.printf("Gekalibreerd = %s\n", isCalibrated() ? "ja" : "nee");
  Serial.print("Wit       :");
  for (int i = 0; i < 8; i++) { Serial.print(' '); Serial.print(params.calMin[i]); }
  Serial.println();
  Serial.print("Zwart     :");
  for (int i = 0; i < 8; i++) { Serial.print(' '); Serial.print(params.calMax[i]); }
  Serial.println();
  Serial.println("======================");
}

void printSensors() {
  readSensors();
  computePosition();
  Serial.print("ruw      :");
  for (int i = 0; i < 8; i++) { Serial.print(' '); Serial.print(rawVal[i]); }
  Serial.println();
  Serial.print("genormeerd:");
  for (int i = 0; i < 8; i++) { Serial.print(' '); Serial.print(normVal[i], 2); }
  Serial.println();
  Serial.printf("positie = %.2f (%s)\n", pos, lijnGezien ? "lijn gezien" : "lijn kwijt");
}

void printHelp() {
  Serial.println("Commando's:");
  Serial.println("  help | get | sensors | start | stop | save | defaults");
  Serial.println("  debug on|off");
  Serial.println("  set kp|ki|kd <waarde>");
  Serial.println("  set power <0-255>");
  Serial.println("  set diff <0-1>");
  Serial.println("  set cycle <100-100000>   (us)");
  Serial.println("  set invert <0|1>");
  Serial.println("  calibrate white | calibrate black");
}

void executeCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  if (cmd.length() == 0) return;

  int sp = cmd.indexOf(' ');
  String name = (sp < 0) ? cmd : cmd.substring(0, sp);
  String rest = (sp < 0) ? "" : cmd.substring(sp + 1);
  rest.trim();

  if (name == "help") {
    printHelp();
  } else if (name == "get" || name == "status") {
    printParams();
  } else if (name == "sensors") {
    printSensors();
  } else if (name == "start") {
    setRun(true);
  } else if (name == "stop") {
    setRun(false);
  } else if (name == "debug") {
    if (rest == "on")       setDebug(true);
    else if (rest == "off") setDebug(false);
    else                    setDebug(!params.debug);
  } else if (name == "calibrate") {
    if (rest == "white")      { if (!calibrate(false)) Serial.println(lastMessage); }
    else if (rest == "black") { if (!calibrate(true))  Serial.println(lastMessage); }
    else Serial.println("Gebruik: calibrate white | calibrate black");
  } else if (name == "set") {
    int sp2 = rest.indexOf(' ');
    if (sp2 < 0) {
      Serial.println("Gebruik: set <kp|ki|kd|power|diff|cycle|invert> <waarde>");
    } else {
      String key = rest.substring(0, sp2);
      String val = rest.substring(sp2 + 1);
      val.trim();
      if (setParam(key, val)) {
        Serial.println("OK");
        if (runRobot) Serial.println("(wordt opgeslagen zodra de robot stopt)");
      } else {
        Serial.println(lastMessage);
      }
    }
  } else if (name == "save") {
    dirty = true;
    if (runRobot) Serial.println("Wordt opgeslagen zodra de robot stopt");
  } else if (name == "defaults") {
    setDefaults();
    dirty = true;
    Serial.println("Standaardwaarden ingesteld (kalibratie gewist)");
  } else {
    Serial.println("Onbekend commando, typ: help");
  }
}

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialLine.length() > 0) {
        executeCommand(serialLine);
        serialLine = "";
      }
    } else if (serialLine.length() < 64) {
      serialLine += c;
    }
  }
}

// ============================
// Externe interrupt (knop)
// ============================
// De interrupt meldt enkel dat er een dalende flank was.
// handleButton() controleert of de knop echt ingedrukt is en wacht daarna
// tot hij stabiel losgelaten is, zodat prellen bij het loslaten niet meetelt.
void IRAM_ATTR knopISR() {
  if (!knopFlag) {
    flankTijd = millis();
    knopFlag = true;
  }
}

void handleButton() {
  static bool vergrendeld = false;
  static unsigned long losSinds = 0;
  bool laag = (digitalRead(PIN_KNOP) == LOW);

  if (vergrendeld) {
    if (laag) {
      losSinds = 0;
    } else if (losSinds == 0) {
      losSinds = millis();
    } else if (millis() - losSinds >= RELEASE_MS) {
      vergrendeld = false;
      knopFlag = false;
      losSinds = 0;
    }
  } else if (knopFlag && millis() - flankTijd >= DEBOUNCE_MS) {
    knopFlag = false;
    if (laag) {
      vergrendeld = true;
      losSinds = 0;
      setRun(!runRobot);
    }
  }
}

// ============================
// Webpagina
// ============================
const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Line Follower</title>
  <style>
    body { font-family: Arial; max-width: 600px; margin: auto; padding: 20px; background: #f4f4f4; }
    .card { background: white; padding: 15px; border-radius: 12px; margin-bottom: 15px; box-shadow: 0 0 8px rgba(0,0,0,0.12); }
    .running { color: #28a745; font-weight: bold; }
    .stopped { color: #dc3545; font-weight: bold; }
    input { width: 100%; padding: 12px; font-size: 18px; margin: 5px 0 15px; box-sizing: border-box; }
    button { width: 100%; padding: 15px; font-size: 18px; margin-top: 8px; border: none; border-radius: 10px; font-weight: bold; color: white; cursor: pointer; }
    button:disabled { opacity: 0.4; cursor: not-allowed; }
    .save { background: #2196F3; } .start { background: #28a745; } .stop { background: #dc3545; } .debug { background: #ff9800; } .cal { background: #6c757d; }
    h1 { text-align: center; }
  </style>
</head>
<body>
  <h1>Line Follower</h1>

  <div class="card"><span id="msg" style="color:green;font-weight:bold;"></span>&nbsp;</div>

  <div class="card">
    <h2>Instellingen</h2>
    Kp [-]<input type="number" step="0.01" min="0" max="50" id="kp">
    Ki [-]<input type="number" step="0.01" min="0" max="50" id="ki">
    Kd [-]<input type="number" step="0.001" min="0" max="100" id="kd">
    Power [0 - 255]<input type="number" min="0" max="255" id="power">
    Diff [0 - 1]<input type="number" step="0.05" min="0" max="1" id="diff">
    Cycle Time [&micro;s]<input type="number" min="100" max="100000" id="cycle">
    <button class="save" id="save" onclick="save()">OPSLAAN</button>
  </div>

  <div class="card">
    <h2>Kalibratie</h2>
    <button class="cal" id="calw" onclick="cal('/calwhite')">KALIBREER WIT</button>
    <button class="cal" id="calb" onclick="cal('/calblack')">KALIBREER ZWART</button>
  </div>

  <div class="card">
    <h2>Robot Bediening</h2>
    <button class="start" onclick="cmd('/start')">START</button>
    <button class="stop" onclick="cmd('/stop')">STOP</button>
    <button class="debug" onclick="cmd('/debug')">DEBUG AAN / UIT</button>
  </div>

  <div class="card">
    <h2>Status</h2>
    <p><b>Status:</b> <span id="state">...</span></p>
    <p><b>Debug:</b> <span id="dbg">...</span></p>
    <p><b>Positie lijn [-1 .. 1]:</b> <span id="pos">0</span></p>
    <p><b>Gekalibreerd:</b> <span id="calst">...</span></p>
    <p><b>Cycli sinds start:</b> <span id="cyc">0</span></p>
    <p><b>EEPROM:</b> <span id="eep">...</span></p>
  </div>

  <script>
    let filled = false;
    const $ = id => document.getElementById(id);

    async function refresh() {
      try {
        const s = await (await fetch('/status')).json();
        $('state').textContent = s.run ? 'ROBOT ACTIEF' : 'ROBOT GESTOPT';
        $('state').className = s.run ? 'running' : 'stopped';
        $('dbg').textContent = s.debug ? 'AAN' : 'UIT';
        $('pos').textContent = s.pos;
        $('calst').textContent = s.cal ? 'ja' : 'nee';
        $('cyc').textContent = s.cycles;
        $('eep').textContent = s.eeprom ? 'OK' : 'NIET GEVONDEN';
        $('save').disabled = s.run;
        $('calw').disabled = s.run;
        $('calb').disabled = s.run;
        if (!filled) {
          $('kp').value = s.kp; $('ki').value = s.ki; $('kd').value = s.kd;
          $('power').value = s.power; $('diff').value = s.diff; $('cycle').value = s.cycle;
          filled = true;
        }
      } catch (e) {
        $('state').textContent = 'GEEN VERBINDING';
        $('state').className = 'stopped';
      }
    }

    async function cmd(path) { await fetch(path); refresh(); }

    async function cal(path) {
      const r = await fetch(path);
      $('msg').textContent = await r.text();
      refresh();
    }

    async function save() {
      const q = new URLSearchParams({
        kp: $('kp').value, ki: $('ki').value, kd: $('kd').value,
        power: $('power').value, diff: $('diff').value, cycle: $('cycle').value
      });
      const r = await fetch('/save?' + q);
      $('msg').textContent = await r.text();
      filled = false;
      refresh();
    }

    setInterval(refresh, 500);
    refresh();
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", PAGE);
}

void handleStatus() {
  String j;
  j.reserve(320);
  j += "{\"run\":";      j += runRobot ? "true" : "false";
  j += ",\"debug\":";    j += params.debug ? "true" : "false";
  j += ",\"kp\":";       j += String(params.kp, 3);
  j += ",\"ki\":";       j += String(params.ki, 3);
  j += ",\"kd\":";       j += String(params.kd, 3);
  j += ",\"power\":";    j += String((unsigned)params.power);
  j += ",\"diff\":";     j += String(params.diff, 2);
  j += ",\"cycle\":";    j += String((unsigned long)params.cycleTime);
  j += ",\"cycles\":";   j += String((unsigned long)cycleCount);
  j += ",\"pos\":";      j += String(pos, 2);
  j += ",\"cal\":";      j += isCalibrated() ? "true" : "false";
  j += ",\"eeprom\":";   j += eepromOk ? "true" : "false";
  j += "}";
  server.send(200, "application/json", j);
}

void handleSave() {
  if (runRobot) {
    server.send(409, "text/plain", "Stop de robot eerst.");
    return;
  }
  const char* keys[] = {"kp", "ki", "kd", "power", "diff", "cycle"};
  bool ok = true;
  for (int i = 0; i < 6; i++) {
    if (server.hasArg(keys[i])) {
      if (!setParam(String(keys[i]), server.arg(keys[i]))) { ok = false; break; }
    }
  }
  if (!ok) {
    server.send(400, "text/plain", lastMessage);
    return;
  }
  flushParams();
  server.send(200, "text/plain", eepromOk ? "Instellingen opgeslagen." : "Toegepast, maar EEPROM niet gevonden.");
}

void handleCalWhite() {
  if (calibrate(false)) server.send(200, "text/plain", "Kalibratie wit klaar.");
  else                  server.send(409, "text/plain", lastMessage);
}

void handleCalBlack() {
  if (calibrate(true)) server.send(200, "text/plain", "Kalibratie zwart klaar.");
  else                 server.send(409, "text/plain", lastMessage);
}

void handleStart() { setRun(true);  server.send(200, "text/plain", "OK"); }
void handleStop()  { setRun(false); server.send(200, "text/plain", "OK"); }
void handleDebug() { setDebug(!params.debug); server.send(200, "text/plain", "OK"); }

// ============================
// Cyclus: sensoren lezen, PID berekenen, motoren sturen
// ============================
void cycle() {
  cycleCount++;

  unsigned long nu = micros();
  float dt = (float)(nu - lastPidUs) / 1000000.0f;
  lastPidUs = nu;
  if (dt < 0.0001f) dt = 0.0001f;
  if (dt > 0.1f)    dt = 0.1f;

  readSensors();
  computePosition();

  if (runRobot) {
    float e = params.invert ? -pos : pos;
    integral = clampf(integral + e * dt, -1.0f, 1.0f);
    float d = havePrev ? (e - prevE) / dt : 0.0f;
    prevE = e;
    havePrev = true;
    dFilt += 0.2f * (d - dFilt);          // afgeleide licht uitmiddelen

    corr = clampf(params.kp * e + params.ki * integral + params.kd * dFilt, -1.0f, 1.0f);

    // corr > 0: lijn rechts -> linkermotor sneller, rechtermotor trager
    float pl = (float)params.power * (1.0f + params.diff * corr);
    float pr = (float)params.power * (1.0f - params.diff * corr);
    motorL = (int)clampl((long)pl, 0, 255);
    motorR = (int)clampl((long)pr, 0, 255);
    driveMotor(PIN_IN1, PIN_IN2, motorL);
    driveMotor(PIN_IN3, PIN_IN4, motorR);
  } else {
    motorsStop();
    corr = 0;
  }

  if (PIN_LED >= 0) {
    uint32_t halve = 500000UL / params.cycleTime;
    if (halve == 0) halve = 1;
    digitalWrite(PIN_LED, runRobot && ((cycleCount / halve) % 2 == 0));
  }

  if (params.debug) {
    uint32_t elke = 100000UL / params.cycleTime;      // ongeveer 10 x per seconde
    if (elke == 0) elke = 1;
    if (cycleCount % elke == 0) {
      Serial.printf("[debug] %s pos=%.2f corr=%.2f L=%d R=%d\n",
                    runRobot ? "RUN" : "STOP", pos, corr, motorL, motorR);
    }
  }
}

// ============================
// setup en loop
// ============================
void setup() {
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  motorsStop();

  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("Linefollower finaal programma");

  pinMode(PIN_KNOP, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_KNOP), knopISR, FALLING);

  if (PIN_LED >= 0) pinMode(PIN_LED, OUTPUT);

  analogReadResolution(12);              // 0 - 4095
  analogSetAttenuation(ADC_11db);        // grootste meetbereik

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);                 // 100 kHz: betrouwbaar met de interne pull-ups
  delay(50);
  for (int poging = 0; poging < 3 && !eepromOk; poging++) {
    eepromOk = eepromDetect();
    if (!eepromOk) delay(20);
  }
  if (!eepromOk) Serial.println("WAARSCHUWING: EEPROM niet gevonden (controleer SDA/SCL en A0-A2)");

  loadParams();
  validateParams();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  Serial.print("Wifi: ");
  Serial.print(WIFI_SSID);
  Serial.print("  IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/save", handleSave);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/debug", handleDebug);
  server.on("/calwhite", handleCalWhite);
  server.on("/calblack", handleCalBlack);
  server.begin();

  printParams();
  Serial.println("Typ 'help' voor de commando's");
  lastCycleStart = micros();
  lastPidUs = lastCycleStart;
}

void loop() {
  server.handleClient();
  handleSerial();
  handleButton();
  flushParams();

  unsigned long nu = micros();
  if ((unsigned long)(nu - lastCycleStart) >= params.cycleTime) {
    lastCycleStart = nu;
    cycle();
  }
}
