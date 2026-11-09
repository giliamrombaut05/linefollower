#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

const char* ssid = "LineFollower";
const char* password = "GiliamWouter";  

WebServer server(80);

#define EEPROM_ADDR 0x50

// ============================
// Instellingen
// ============================

struct param_t
{
  float kp;
  float ki;
  float kd;

  int maxSpeed;

  bool debug;

  unsigned long cycleTime;
};

param_t params;

bool runRobot = false;

bool showSaveMessage = false;

String statusMessage = "";

// ============================

void sendPage()
{
  String html = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

<meta name="viewport" content="width=device-width, initial-scale=1">

<style>

body{
font-family:Arial;
max-width:600px;
margin:auto;
padding:20px;
background:#f4f4f4;
}

.card{
background:white;
padding:15px;
border-radius:12px;
margin-bottom:15px;
box-shadow:0 0 8px rgba(0,0,0,0.12);
}

.running{
color:#28a745;
font-weight:bold;
animation:pulseGreen 0.8s infinite;
}

.stopped{
color:#dc3545;
font-weight:bold;
}

@keyframes pulseGreen
{
  0%
  {
    opacity:1;
  }

  50%
  {
    opacity:0.35;
  }

  100%
  {
    opacity:1;
  }
}

input{
width:100%;
padding:12px;
font-size:18px;
margin-top:5px;
margin-bottom:15px;
box-sizing:border-box;
}

button{
width:100%;
padding:15px;
font-size:18px;
margin-top:8px;
border:none;
border-radius:10px;
font-weight:bold;
cursor:pointer;
}

.save{
background:#2196F3;
color:white;
}

.start{
background:#28a745;
color:white;
}

.stop{
background:#dc3545;
color:white;
}

.debug{
background:#ff9800;
color:white;
}

h1{
text-align:center;
}

.statusText{
font-size:18px;
}

</style>

</head>

<body>

<h1>Line Follower</h1>

<div class="card">

<span style="color:green;font-weight:bold;">
%MESSAGE%
</span>

</div>

<div class="card">

<h2>PID Instellingen</h2>

<form action="/save">

Kp [-]

<input
type="number"
step="0.01"
min="0"
max="50"
name="kp"
value="%KP%">

Ki [-]

<input
type="number"
step="0.01"
min="0"
max="50"
name="ki"
value="%KI%">

Kd [-]

<input
type="number"
step="0.01"
min="0"
max="100"
name="kd"
value="%KD%">

Max Speed [0 - 255]

<input
type="number"
min="0"
max="255"
name="speed"
value="%SPEED%">

Cycle Time [&micro;s]

<input
type="number"
min="100"
max="100000"
name="cycle"
value="%CYCLE%">

<button
class="save"
type="submit"
%SAVESTATE%>
OPSLAAN
</button>

</form>

</div>

<div class="card">

<h2>Robot Bediening</h2>

<button class="start" onclick="startRobot()">
START
</button>

<button class="stop" onclick="stopRobot()">
STOP
</button>

<button class="debug" onclick="toggleDebug()">
DEBUG ON / OFF
</button>

</div>

<div class="card">

<h2>Status</h2>

<div class="statusText">

<p>
<b>Status:</b>
<span class="%STATUSCLASS%">
%STATUS%
</span>
</p>

<p><b>Debug:</b> %DEBUG%</p>

<p><b>Kp:</b> %KP%</p>

<p><b>Ki:</b> %KI%</p>

<p><b>Kd:</b> %KD%</p>

<p><b>Max Speed:</b> %SPEED%</p>

<p><b>Cycle Time:</b> %CYCLE% &micro;s</p>

</div>

</div>

<script>

function startRobot()
{
  fetch('/start')
  .then(() =>
  {
    setTimeout(function()
    {
      location.reload();
    },200);
  });
}

function stopRobot()
{
  fetch('/stop')
  .then(() => updateStatus());
}

function toggleDebug()
{
  fetch('/debug')
  .then(() => updateStatus());
}

function updateStatus()
{
  setTimeout(function()
  {
    location.reload();
  },200);
}

</script>

</body>

</html>

)rawliteral";

  html.replace("%KP%", String(params.kp,2));
  html.replace("%KI%", String(params.ki,2));
  html.replace("%KD%", String(params.kd,2));
  html.replace("%SPEED%", String(params.maxSpeed));
  html.replace("%CYCLE%", String(params.cycleTime));

  html.replace("%RUN%",
               runRobot ? "ACTIEF" : "GESTOPT");

  html.replace("%DEBUG%",
               params.debug ? "AAN" : "UIT");

  html.replace("%STATUS%",
               runRobot ?
               "ROBOT ACTIEF" :
               "ROBOT GESTOPT");

  html.replace("%STATUSCLASS%",
               runRobot ?
               "running" :
               "stopped");

  html.replace("%SAVESTATE%",
                runRobot ?
               "disabled" :
               "");


  if(showSaveMessage)
  {
    html.replace("%MESSAGE%", statusMessage);
  }
  else
  {
    html.replace("%MESSAGE%", "");
  }
 
  server.send(200, "text/html", html);
}

// ============================

void handleRoot()
{
  sendPage();
}

// ============================

void handleSave()
{
 
  if(server.hasArg("cycle"))
  {
    params.cycleTime =
      server.arg("cycle").toInt();

    params.cycleTime =
      constrain(params.cycleTime,
                100,
                100000);
  }
    
  if(server.hasArg("kp"))
  {
    params.kp = server.arg("kp").toFloat();
    params.kp = constrain(params.kp,0.0f,50.0f);

  }

  if(server.hasArg("ki"))
  {
    params.ki = server.arg("ki").toFloat();
    params.ki = constrain(params.ki,0.0f,50.0f);
  }

  if(server.hasArg("kd"))
  {
    params.kd = server.arg("kd").toFloat();
    params.kd = constrain(params.kd,0.0f,100.0f);
  }

  if(server.hasArg("speed"))
  {
    params.maxSpeed = server.arg("speed").toInt();
    params.maxSpeed = constrain(params.maxSpeed,0,255);
  }

  Serial.println("===== INSTELLINGEN =====");

  Serial.print("Kp = ");
  Serial.println(params.kp);

  Serial.print("Ki = ");
  Serial.println(params.ki);

  Serial.print("Kd = ");
  Serial.println(params.kd);

  Serial.print("MaxSpeed = ");
  Serial.println(params.maxSpeed);

  Serial.print("CycleTime = ");
  Serial.println(params.cycleTime);

  saveSettings();

  statusMessage =
    "Instellingen opgeslagen.";

  showSaveMessage = true;

  Serial.println("MELDING GEZET");

  sendPage();
}

// ============================

void handleStart()
{
  runRobot = true;

  showSaveMessage = false;
  statusMessage = "";

  Serial.println("MELDING GEWIST");

  Serial.println("ROBOT START");

  server.send(200,"text/plain","OK");
}

// ============================

void handleStop()
{
  runRobot = false;

  showSaveMessage = false;
  statusMessage = "";

  Serial.println("ROBOT STOP");

  server.send(200,"text/plain","OK");
}

// ============================

void handleDebug()
{
  params.debug = !params.debug;

  saveSettings();

  Serial.print("Debug = ");
  Serial.println(params.debug);

  server.send(200,"text/plain","OK");
}

// ============================
//// EEPROM Functie ////

void writeByte(uint16_t address, byte data)
{
  Wire.beginTransmission(EEPROM_ADDR);

  Wire.write(highByte(address));
  Wire.write(lowByte(address));

  Wire.write(data);

  Wire.endTransmission();

  delay(10);
}

byte readByte(uint16_t address)
{
  Wire.beginTransmission(EEPROM_ADDR);

  Wire.write(highByte(address));
  Wire.write(lowByte(address));

  Wire.endTransmission();

  Wire.requestFrom(EEPROM_ADDR,1);

  if(Wire.available())
  {
    return Wire.read();
  }

  return 0;
}

void saveSettings()
{
  byte* p = (byte*)&params;

  for(unsigned int i=0;i<sizeof(params);i++)
  {
    writeByte(i,p[i]);
  }
}

void loadSettings()
{
  byte* p = (byte*)&params;

  for(unsigned int i=0;i<sizeof(params);i++)
  {
    p[i] = readByte(i);
  }
}

// ============================


void setup()
{
  Serial.begin(115200);
  
  /// EEPROM ///
  Wire.begin(7,6);

  loadSettings();

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Serial.println("EEPROM geladen");

  Serial.print("Kp = ");
  Serial.println(params.kp);

  Serial.print("Ki = ");
  Serial.println(params.ki);

  Serial.print("Kd = ");
  Serial.println(params.kd);

  Serial.print("MaxSpeed = ");
  Serial.println(params.maxSpeed);

  Serial.print("CycleTime = ");
  Serial.println(params.cycleTime);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  if(params.maxSpeed <= 0 || params.maxSpeed > 255)
  {
    params.kp = 0.30;
    params.ki = 0.00;
    params.kd = 2.00;

    params.maxSpeed = 180;

    params.debug = false;

    params.cycleTime = 1000;

    saveSettings();
  }

  /// Web interface ///
  WiFi.softAP(ssid,password);

  Serial.println();
  Serial.print("IP adres: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/debug", handleDebug);

  server.begin();

  Serial.println("Webserver gestart");

}

// ============================

void loop()
{
  server.handleClient();
}