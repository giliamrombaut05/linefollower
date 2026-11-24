# Gebruiksaanwijzing

## opladen / vervangen batterijen

De robot werkt op twee 18650 Li-ion cellen in serie.

1. Zet de aan/uit-schakelaar van de robot uit.
2. Neem de cellen uit de batterijhouder.
3. Laad de cellen op met een lader voor Li-ion 18650-cellen tot de lader aangeeft dat ze vol zijn.
4. Plaats beide cellen terug volgens de + en − in de batterijhouder.
5. Zet de schakelaar aan. De power-led brandt.

Let op:

* vervang altijd beide cellen tegelijk, door cellen van hetzelfde type en met dezelfde laadtoestand
* gebruik geen beschadigde cellen en sluit ze nooit kort
* laad de cellen niet op in de robot
* zet de schakelaar uit als je de robot met USB-C verbindt

## draadloze communicatie

### verbinding maken

1. Zet de robot aan.
2. Open op je laptop of smartphone de wifi-instellingen en kies het netwerk **LineFollower**.
3. Geef het wachtwoord in (zie `WIFI_PASS` in de code).
4. Blijf verbonden als er "geen internet" gemeld wordt.
5. Open een browser en surf naar **192.168.4.1**.

De webpagina toont de status en positie van de lijn, laat toe de parameters aan te passen en op te slaan, de sensoren te kalibreren en de robot te starten en te stoppen. Opslaan en kalibreren kan enkel als de robot stilstaat.

### commando's

Verbind de robot met USB-C, open een seriële monitor op 115200 baud en typ een commando gevolgd door Enter.

| commando | waarde | functie |
|---|---|---|
| `debug on` / `debug off` | | debug-uitvoer aan of uit |
| `start` | | robot starten |
| `stop` | | robot stoppen |
| `set cycle [µs]` | 100 - 100000 | cyclustijd |
| `set power [0..255]` | 0 - 255 | maximale motorkracht |
| `set diff [0..1]` | 0 - 1 | invloed van de correctie op het snelheidsverschil tussen de motoren |
| `set kp [0..]` | 0 - 50 | proportionele versterking |
| `set ki [0..]` | 0 - 50 | integrerende versterking |
| `set kd [0..]` | 0 - 100 | afgeleide versterking |
| `calibrate black` | | sensoren kalibreren op zwart |
| `calibrate white` | | sensoren kalibreren op wit |

Extra commando's: `help` (alle commando's), `get` (parameters en status), `sensors` (sensorwaarden en positie), `set invert 0|1` (stuurrichting omdraaien), `save` (opslaan) en `defaults` (standaardwaarden). Alle instellingen worden bewaard in de EEPROM en blijven na het uitschakelen behouden.

## kalibratie

Kalibreer de sensoren bij een nieuwe ondergrond of ander licht.

1. Zet de robot aan en verbind via USB-C of wifi. De robot staat stil.
2. Houd de sensorbalk op ongeveer 3 mm boven het **witte** deel van de baan en typ `calibrate white` (of druk op KALIBREER WIT op de webpagina).
3. Houd de sensorbalk boven het **zwarte** deel van de lijn en typ `calibrate black` (of druk op KALIBREER ZWART).
4. Typ `sensors`: de genormaliseerde waarden zijn dan ongeveer 0 boven wit en ongeveer 1 boven zwart.

De kalibratie wordt per sensor bewaard, zodat de verschillen tussen de sensoren opgevangen worden.

## settings

De robot rijdt stabiel met volgende parameters:

| parameter | waarde |
|---|---|
| cycle | *in te vullen na het afstellen* µs |
| power | *in te vullen na het afstellen* |
| diff | *in te vullen na het afstellen* |
| kp | *in te vullen na het afstellen* |
| ki | *in te vullen na het afstellen* |
| kd | *in te vullen na het afstellen* |

Startwaarden in de code: cycle 2000 µs, power 120, diff 0,80, kp 1,00, ki 0,00, kd 0,02. Zie [code/finaal](../code/finaal/readme.md) voor de afstelprocedure.

## start/stop button

De start/stop-knop is aangesloten tussen GPIO11 en GND. *(locatie op de robot: in te vullen)*

1. Zet de robot aan met de aan/uit-schakelaar.
2. Plaats de robot op de lijn.
3. Druk één keer op de knop: de robot start.
4. Druk opnieuw op de knop: de robot stopt.

De knop werkt hetzelfde als de commando's `start` en `stop` en de knoppen op de webpagina.
