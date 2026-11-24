# Instructable

Stappenplan om de linefollower na te bouwen, vertrekkend van de [bill of materials](../bill%20of%20materials/readme.md) en de [technische tekeningen](../technische%20tekeningen/elektronisch/readme.md).

## Voorbereiding

1. Bestel alle componenten uit de bill of materials.
2. Verzamel gereedschap: soldeerbout, tin, draad, kniptang, schroevendraaier, multimeter, USB-C-datakabel, lader voor 18650-cellen.
3. Laad de twee 18650-cellen volledig op.
4. Download of clone de GitHub-repository `linefollower` op je computer.

## Mechanica

5. Maak de mechanische onderdelen volgens de [mechanische tekeningen](../technische%20tekeningen/mechanisch/readme.md).
6. Monteer de twee N20-motoren op het chassis.
7. Monteer de wielen op de motoras en de ball caster achteraan of vooraan, volgens de tekeningen.
8. Monteer de batterijhouder op het chassis.
9. Monteer de QTR-8A-sensorbalk vooraan, ongeveer 3 mm boven de ondergrond.

## Elektronica

Volg bij het verbinden het [elektrisch schema](../technische%20tekeningen/elektronisch/readme.md).

10. Soldeer de pinheaders op de ESP32-S3 SuperMini, de DRV8833-module en de QTR-8A.
11. Verbind de voeding:
    * batterijhouder (+) naar de schakelaar, schakelaar naar IN+ van de buckconverter
    * batterijhouder (−) naar IN− van de buckconverter
    * plaats nog geen cellen
12. Plaats de twee cellen in serie, zet de schakelaar aan en meet met de multimeter dat OUT+ van de buckconverter 3,3 V is. Zet de schakelaar uit.
13. Verbind de ESP32-S3 SuperMini:
    * OUT+ van de buckconverter naar 3V3
    * OUT− van de buckconverter naar GND
    * condensator C1 van 100 nF tussen 3V3 en GND, zo dicht mogelijk bij de ESP32
14. Verbind de DRV8833:
    * VCC naar de plus na de schakelaar (batterijspanning, dus niet de 3,3 V)
    * GND naar de massa
    * EEP (nSLEEP) naar 3V3
    * ULT blijft onaangesloten
    * condensator C2 van 100 nF tussen VCC en GND van de module
    * OUT1 en OUT2 naar motor M1
    * OUT3 en OUT4 naar motor M2
15. Verbind de stuursignalen van de DRV8833:

    | DRV8833 | ESP32-S3 |
    |---|---|
    | IN1 | GPIO13 |
    | IN2 | GPIO12 |
    | IN3 | GPIO4 |
    | IN4 | GPIO3 |

16. Verbind de QTR-8A:
    * VCC naar 3V3
    * GND naar GND
    * IR naar 3V3
17. Verbind de sensoruitgangen en plaats tussen elke uitgang D1 … D8 en GND een weerstand van 100 kΩ (weerstandsnetwerk):

    | QTR-8A | netlabel | ESP32-S3 |
    |---|---|---|
    | D1 | S1 | GPIO1 |
    | D2 | S2 | GPIO2 |
    | D3 | S3 | GPIO5 |
    | D4 | S4 | GPIO6 |
    | D5 | S5 | GPIO7 |
    | D6 | S6 | GPIO8 |
    | D7 | S7 | GPIO9 |
    | D8 | S8 | GPIO10 |

18. Verbind de EEPROM (24C256):
    * VCC naar 3V3
    * GND, WP, A0, A1 en A2 naar GND
    * SDA naar GPIO43 (TX)
    * SCL naar GPIO44 (RX)
19. Verbind de start/stop-knop tussen GPIO11 en GND.
20. Verbind de power-led: 3V3, dan een weerstand van 220 Ω, dan de led, dan GND.
21. Controleer alle verbindingen met het elektrisch schema.
22. Meet met de multimeter dat er geen kortsluiting is tussen 3V3 en GND.

## Software installeren

23. Installeer Arduino IDE 2.
24. Ga in Arduino IDE naar Bestand, Voorkeuren en voeg bij *Additional boards manager URLs* toe: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
25. Open de Boards Manager en installeer `esp32` van Espressif Systems.

## Compileren en uploaden

26. Open `code/basis/basisprogramma.ino` in Arduino IDE. Vraagt Arduino IDE om het bestand in een map met dezelfde naam te plaatsen, bevestig dat dan.
27. Kies bij Tools:
    * Board: ESP32S3 Dev Module
    * USB CDC On Boot: Enabled
    * Port: de poort van de ESP32
28. Zet de schakelaar van de robot uit en verbind de ESP32 met de computer via USB-C.
29. Verschijnt er geen poort: houd BOOT ingedrukt, druk op RESET en laat beide los.
30. Klik op Verify om te compileren.
31. Klik op Upload.
32. Druk na het uploaden op RESET.

## Testen

33. Open de Serial Monitor op 115200 baud en typ `get`.
34. Druk op de start/stop-knop en controleer dat de status wisselt tussen RUN en STOP.
35. Verbind je telefoon of computer met het wifi-netwerk `LineFollower` (wachtwoord in de code) en open `192.168.4.1`.
36. Stel een waarde in via de webpagina, klik op Opslaan, herstart de robot en controleer dat de waarde blijft.
37. Upload `code/proof of concepts/hbridge/codeH-brug.ino`, trek de USB-kabel uit, zet de schakelaar aan en controleer dat beide motoren draaien.
38. Upload de sensor-proof-of-concept uit `code/proof of concepts/sensor` en controleer in de Serial Monitor dat alle acht sensoren een waarde geven.

## Rijden

39. Upload `code/finaal/finaal.ino` met dezelfde instellingen als in stap 27.
40. Zet de schakelaar aan, houd de sensorbalk boven wit en typ `calibrate white`. Houd de sensorbalk daarna boven zwart en typ `calibrate black`.
41. Stel Kp, Ki, Kd, power, diff en cyclustijd in via de webpagina of met seriële commando's (zie de [gebruiksaanwijzing](../gebruiksaanwijzing/readme.md)).
42. Plaats de robot op de lijn en druk op de start/stop-knop.
