# Basisprogramma

Template-project waarop de finale linefollower verder bouwt (`basisprogramma.ino`). Volgende zaken werken samen:

* instelbare parameters met seriële commando's
* cyclustijd
* debug aan/uit
* start/stop
* parameters worden opgeslagen in het externe EEPROM-geheugen
* start/stop kan ook geactiveerd worden met een externe interrupt (drukknop)
* draadloze communicatie (eigen wifi-netwerk met webpagina)

Met behulp van de cyclus kan je bv. een knipperende led programmeren: zet `PIN_LED` in de code op een vrije GPIO.

## hardware

* ESP32-S3 SuperMini
* EEPROM 24C256 (I²C)
* drukknop

De schakeling komt overeen met het elektrisch schema van de linefollower (zie [technische tekeningen/elektronisch](../../technische%20tekeningen/elektronisch/readme.md)). Om enkel dit programma te testen zijn de ESP32, de EEPROM en de drukknop voldoende.

## aansluitingen

| functie | ESP32-S3 | opmerking |
|---|---|---|
| EEPROM SDA | GPIO43 (TX) | I²C data |
| EEPROM SCL | GPIO44 (RX) | I²C klok |
| EEPROM WP, A0, A1, A2 | GND | legt het I²C-adres vast op 0x50 |
| EEPROM VCC / GND | 3V3 / GND | |
| drukknop | GPIO11 en GND | interne pull-up, interrupt op dalende flank |

GPIO43 en GPIO44 zijn de seriële pinnen van de chip. Ze zijn hier enkel vrij omdat `Serial` via de USB-poort loopt (*USB CDC On Boot: Enabled*).

## instellingen Arduino IDE

* Board: **ESP32S3 Dev Module**
* USB CDC On Boot: **Enabled**
* Serial Monitor: **115200 baud**, regeleinde *Newline*
* na het uploaden eenmaal op **RESET** drukken

## seriële commando's

| commando | functie |
|---|---|
| `help` | toont alle commando's |
| `get` | toont alle parameters en de status |
| `start` / `stop` | robot starten / stoppen |
| `debug on` / `debug off` | debug-uitvoer aan / uit |
| `set kp <waarde>` | Kp (0 - 50) |
| `set ki <waarde>` | Ki (0 - 50) |
| `set kd <waarde>` | Kd (0 - 100) |
| `set power <0-255>` | maximale motorkracht (ook `set speed`) |
| `set cycle <100-100000>` | cyclustijd in µs |
| `save` | parameters nu opslaan in de EEPROM |
| `defaults` | standaardwaarden terugzetten |

Wijzigingen worden automatisch in de EEPROM opgeslagen. Gebeurt dat terwijl de robot rijdt, dan wacht het programma tot de robot stopt, zodat de cyclus niet vertraagt.

## draadloze communicatie

De ESP32-S3 maakt een eigen wifi-netwerk **LineFollower** aan (wachtwoord: zie `WIFI_PASS` in de code). Surf na het verbinden naar **192.168.4.1**. De webpagina toont de status en laat toe de parameters aan te passen, op te slaan en de robot te starten en te stoppen.

## test

| test | werkwijze | verwacht resultaat |
|---|---|---|
| seriële commando's | `get`, dan `set kp 0.5` | Kp wordt 0,50 |
| EEPROM | `save`, RESET drukken, `get` | Kp blijft 0,50 |
| interrupt | druk op de knop | "ROBOT START", bij een tweede druk "ROBOT STOP" |
| draadloos | verbind met **LineFollower**, open 192.168.4.1 | status verandert mee met de knop, opslaan geeft "Instellingen opgeslagen." |
| cyclus | `debug on` en `start` | elke seconde een regel "[debug] cyclus ..." |

Meldt de Serial Monitor "EEPROM niet gevonden", controleer dan SDA, SCL en de adrespinnen A0 - A2. Het programma zoekt zelf het adres tussen 0x50 en 0x57.

Screenshot van een geslaagde test: ![seriële monitor](Schermafbeelding%202026-11-20%20122216.png)
