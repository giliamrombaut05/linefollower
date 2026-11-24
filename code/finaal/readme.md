# Finaal programma

`finaal.ino` is de uiteindelijke besturingssoftware van de linefollower. Het bouwt voort op het [basisprogramma](../basis/readme.md) (seriële commando's, cyclustijd, debug, start/stop, EEPROM, interrupt, wifi) en voegt toe:

* de 8 sensoren van de QTR-8A uitlezen, met kalibratie voor wit en zwart
* de positie van de lijn berekenen
* een PID-regeling die de twee motoren aanstuurt via de DRV8833

> **Status:** eerste versie. De code is geschreven volgens het schema maar nog niet op de baan getest. De PID-waarden moeten nog afgesteld worden.

## aansluitingen

| functie | ESP32-S3 |
|---|---|
| sensoren S1 … S8 | GPIO1, 2, 5, 6, 7, 8, 9, 10 |
| motor M1 (links): IN1, IN2 | GPIO13, GPIO12 |
| motor M2 (rechts): IN3, IN4 | GPIO4, GPIO3 |
| EEPROM SDA, SCL | GPIO43 (TX), GPIO44 (RX) |
| start/stop-knop | GPIO11 |

Instellingen Arduino IDE: **ESP32S3 Dev Module**, **USB CDC On Boot: Enabled**.

## werking

1. **Sensoren lezen.** Elke cyclus worden de 8 sensoren met `analogRead` uitgelezen (12 bit, 0 - 4095).
2. **Normalisatie.** Met de kalibratiewaarden per sensor wordt elke waarde omgezet naar 0 (wit) tot 1 (zwart). Dit vangt de verschillen tussen de sensoren op.
3. **Positie.** Het gewogen gemiddelde van de genormaliseerde waarden geeft een positie van -1 (lijn uiterst links) tot +1 (lijn uiterst rechts). Ziet geen enkele sensor de lijn, dan draait de robot verder naar de kant waar de lijn het laatst was.
4. **PID.** De fout is de positie. De correctie is `kp·fout + ki·integraal + kd·afgeleide`, begrensd tussen -1 en +1.
5. **Motoren.** Linkermotor = `power·(1 + diff·correctie)`, rechtermotor = `power·(1 - diff·correctie)`, begrensd tussen 0 en 255. Met `diff` = 0 rijdt de robot rechtdoor, met `diff` = 1 staat bij maximale correctie één motor stil.

## commando's

| commando | functie |
|---|---|
| `help`, `get`, `sensors` | commando's, parameters en status, ruwe en genormaliseerde sensorwaarden |
| `start`, `stop` | robot starten en stoppen |
| `debug on` / `debug off` | debug-uitvoer aan / uit |
| `set kp` / `set ki` / `set kd` | PID-waarden |
| `set power <0-255>` | maximale motorkracht |
| `set diff <0-1>` | invloed van de correctie op het snelheidsverschil |
| `set cycle <100-100000>` | cyclustijd in µs |
| `set invert <0 of 1>` | draait de stuurrichting om |
| `calibrate white` / `calibrate black` | sensoren kalibreren |
| `save`, `defaults` | parameters opslaan, standaardwaarden terugzetten |

Alle parameters en de kalibratie worden bewaard in de EEPROM. Dezelfde instellingen zijn beschikbaar op de webpagina (192.168.4.1).

## afstellen

1. `calibrate white` boven wit en `calibrate black` boven zwart. Controleer met `sensors` dat wit rond 0 en zwart rond 1 ligt.
2. Begin met een lage `power` (bv. 80), `ki` = 0 en `kd` = 0.
3. Verhoog `kp` tot de robot de lijn volgt. Stuurt de robot van de lijn weg, gebruik dan `set invert 1`. Draait een motor achteruit, wissel dan de twee motordraden van die motor om.
4. Verhoog `kd` om het slingeren te dempen.
5. Verhoog `power` stap voor stap en pas `kp`, `kd` en `diff` aan. `ki` is meestal niet nodig.
6. Noteer de werkende waarden in de [gebruiksaanwijzing](../../gebruiksaanwijzing/readme.md).
