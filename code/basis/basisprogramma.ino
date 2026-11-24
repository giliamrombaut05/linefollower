/*
  BASISPROGRAMMA LINEFOLLOWER  (ESP32-S3 SuperMini)

  Samenwerkende onderdelen:
    - instelbare parameters via seriele commando's (Serial Monitor)
    - cyclustijd (niet-blokkerend, in microseconden)
    - debug aan/uit
    - start/stop (seriel, web en fysieke knop)
    - parameters in de externe EEPROM (I2C)
    - start/stop via externe interrupt (knop)
    - draadloze communicatie (eigen wifi-netwerk + webpagina)

  Pinnen (conform schema):
    SDA  = GPIO43 (TX)   SCL = GPIO44 (RX)     (EEPROM, adres 0x50: A0..A2 aan GND)
    KNOP = GPIO11    (tussen GPIO11 en GND, INPUT_PULLUP, dalende flank)

  Seriele commando's (115200 baud, eindigen met Enter):
    help                  toon deze lijst
    get                   toon alle parameters en status
    set kp|ki|kd <waarde>
    set power <0-255>     (ook: set speed)
    set cycle <100-100000>   (microseconden)
    debug on|off
    start | stop
    save                  parameters nu opslaan in EEPROM
    defaults              standaardwaarden terugzetten

  Instellingen bord (Arduino IDE): "ESP32S3 Dev Module", USB CDC On Boot = Enabled.
  PlatformIO: build_flags = -DARDUINO_USB_CDC_ON_BOOT=1
  Zonder USB CDC gebruikt Serial GPIO43/44 en botst dat met de EEPROM (SDA/SCL).

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
const uint8_t  PIN_SDA  = 43;   // TX
const uint8_t  PIN_SCL  = 44;   // RX
const uint8_t  PIN_KNOP = 11;
const int      PIN_LED  = -1;      // zet op een vrije GPIO voor een knipperende test-led (-1 = geen)

uint8_t        eepromAddr   = 0x50;   // wordt automatisch gezocht (0x50 - 0x57)
const uint16_t EEPROM_START = 0;
const uint32_t EEPROM_MAGIC = 0x4C464C31;   // "LFL1"

const unsigned long DEBOUNCE_MS = 30;   // flank moet minstens zo lang laag blijven
const unsigned long RELEASE_MS  = 50;   // knop moet zo lang stabiel los zijn

// ============================
// Parameters
// ============================
struct __attribute__((packed)) Params {
  float    kp;
  float    ki;
  float    kd;
  int32_t  maxSpeed;     // 0 - 255
  uint32_t cycleTime;    // microseconden
  uint8_t  debug;
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
bool ledState = false;

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
// EEPROM (I2C, 24C256 / FM24C256)
// ============================
bool eepromDetect() {
  // A0..A2 bepalen het adres (0x50 - 0x57): zoek het eerste dat antwoordt
  for (uint8_t a = 0x50; a <= 0x57; a++) {
    Wire.beginTransmission(a);
    uint8_t fout = Wire.endTransmission();
    if (fout == 0) {
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
  params.kp = 0.30f;
  params.ki = 0.00f;
  params.kd = 2.00f;
  params.maxSpeed = 180;
  params.cycleTime = 1000;
  params.debug = 0;
}

void validateParams() {
  params.kp = clampf(params.kp, 0.0f, 50.0f);
  params.ki = clampf(params.ki, 0.0f, 50.0f);
  params.kd = clampf(params.kd, 0.0f, 100.0f);
  params.maxSpeed = clampl(params.maxSpeed, 0, 255);
  params.cycleTime = clampl((long)params.cycleTime, 100, 100000);
  params.debug = params.debug ? 1 : 0;
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
  else if (key == "speed" || key == "power") params.maxSpeed = clampl(val.toInt(), 0, 255);
  else if (key == "cycle") params.cycleTime = clampl(val.toInt(), 100, 100000);
  else {
    lastMessage = "Onbekende parameter: " + key;
    return false;
  }
  dirty = true;
  return true;
}

void setRun(bool r) {
  runRobot = r;
  if (r) cycleCount = 0;
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
  Serial.printf("Kp        = %.2f\n", params.kp);
  Serial.printf("Ki        = %.2f\n", params.ki);
  Serial.printf("Kd        = %.2f\n", params.kd);
  Serial.printf("MaxSpeed  = %ld\n", (long)params.maxSpeed);
  Serial.printf("CycleTime = %lu us\n", (unsigned long)params.cycleTime);
  Serial.printf("Debug     = %s\n", params.debug ? "AAN" : "UIT");
  Serial.printf("Status    = %s\n", runRobot ? "RUN" : "STOP");
  Serial.printf("EEPROM    = %s\n", eepromOk ? "OK" : "NIET GEVONDEN");
  Serial.println("======================");
}

void printHelp() {
  Serial.println("Commando's:");
  Serial.println("  help | get | start | stop | save | defaults");
  Serial.println("  set kp|ki|kd <waarde>");
  Serial.println("  set power <0-255>   (ook: set speed)");
  Serial.println("  set cycle <100-100000>   (us)");
  Serial.println("  debug on|off");
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
  } else if (name == "start") {
    setRun(true);
  } else if (name == "stop") {
    setRun(false);
  } else if (name == "debug") {
    if (rest == "on")       setDebug(true);
    else if (rest == "off") setDebug(false);
    else                    setDebug(!params.debug);
  } else if (name == "set") {
    int sp2 = rest.indexOf(' ');
    if (sp2 < 0) {
      Serial.println("Gebruik: set <kp|ki|kd|power|cycle> <waarde>");
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
    Serial.println("Standaardwaarden ingesteld");
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
    .save { background: #2196F3; } .start { background: #28a745; } .stop { background: #dc3545; } .debug { background: #ff9800; }
    h1 { text-align: center; }
  </style>
</head>
<body>
  <h1>Line Follower</h1>

  <div class="card"><span id="msg" style="color:green;font-weight:bold;"></span>&nbsp;</div>

  <div class="card">
    <h2>PID Instellingen</h2>
    Kp [-]<input type="number" step="0.01" min="0" max="50" id="kp">
    Ki [-]<input type="number" step="0.01" min="0" max="50" id="ki">
    Kd [-]<input type="number" step="0.01" min="0" max="100" id="kd">
    Max Speed [0 - 255]<input type="number" min="0" max="255" id="speed">
    Cycle Time [&micro;s]<input type="number" min="100" max="100000" id="cycle">
    <button class="save" id="save" onclick="save()">OPSLAAN</button>
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
        $('cyc').textContent = s.cycles;
        $('eep').textContent = s.eeprom ? 'OK' : 'NIET GEVONDEN';
        $('save').disabled = s.run;
        if (!filled) {
          $('kp').value = s.kp; $('ki').value = s.ki; $('kd').value = s.kd;
          $('speed').value = s.speed; $('cycle').value = s.cycle;
          filled = true;
        }
      } catch (e) {
        $('state').textContent = 'GEEN VERBINDING';
        $('state').className = 'stopped';
      }
    }

    async function cmd(path) { await fetch(path); refresh(); }

    async function save() {
      const q = new URLSearchParams({
        kp: $('kp').value, ki: $('ki').value, kd: $('kd').value,
        speed: $('speed').value, cycle: $('cycle').value
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
  j.reserve(256);
  j += "{\"run\":";      j += runRobot ? "true" : "false";
  j += ",\"debug\":";    j += params.debug ? "true" : "false";
  j += ",\"kp\":";       j += String(params.kp, 2);
  j += ",\"ki\":";       j += String(params.ki, 2);
  j += ",\"kd\":";       j += String(params.kd, 2);
  j += ",\"speed\":";    j += String((long)params.maxSpeed);
  j += ",\"cycle\":";    j += String((unsigned long)params.cycleTime);
  j += ",\"cycles\":";   j += String((unsigned long)cycleCount);
  j += ",\"eeprom\":";   j += eepromOk ? "true" : "false";
  j += "}";
  server.send(200, "application/json", j);
}

void handleSave() {
  if (runRobot) {
    server.send(409, "text/plain", "Stop de robot eerst.");
    return;
  }
  const char* keys[] = {"kp", "ki", "kd", "speed", "cycle"};
  bool ok = true;
  for (int i = 0; i < 5; i++) {
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

void handleStart() { setRun(true);  server.send(200, "text/plain", "OK"); }
void handleStop()  { setRun(false); server.send(200, "text/plain", "OK"); }
void handleDebug() { setDebug(!params.debug); server.send(200, "text/plain", "OK"); }

// ============================
// Cyclus: hier komt later de linefollower
// ============================
void cycle() {
  cycleCount++;

  uint32_t ticksPerSeconde = 1000000UL / params.cycleTime;
  uint32_t halveperiode = ticksPerSeconde / 2;      // led knippert aan 1 Hz
  if (halveperiode == 0) halveperiode = 1;

  if (runRobot) {
    if (cycleCount % halveperiode == 0) ledState = !ledState;
    // >>> hier komen later: sensoren lezen, PID berekenen, motoren sturen
  } else {
    ledState = false;
  }

  if (PIN_LED >= 0) digitalWrite(PIN_LED, ledState);

  if (params.debug && cycleCount % ticksPerSeconde == 0) {
    Serial.printf("[debug] cyclus %lu, status %s, led %d\n",
                  (unsigned long)cycleCount, runRobot ? "RUN" : "STOP", ledState ? 1 : 0);
  }
}

// ============================
// setup en loop
// ============================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("Linefollower basisprogramma");

  pinMode(PIN_KNOP, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_KNOP), knopISR, FALLING);

  if (PIN_LED >= 0) pinMode(PIN_LED, OUTPUT);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);   // 100 kHz: betrouwbaar met de interne pull-ups
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
  server.begin();

  printParams();
  Serial.println("Typ 'help' voor de commando's");
  lastCycleStart = micros();
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
