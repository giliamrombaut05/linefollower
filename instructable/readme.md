# Instructable

Instructable: linefollower nabouwen
Stappenplan om de robot na te bouwen, vertrekkend van de bill of materials en de technische tekeningen.

Voorbereiding
Stap 1. Bestel alle componenten uit de bill of materials.

Stap 2. Verzamel gereedschap: soldeerbout, tin, draad, kniptang, schroevendraaier, multimeter, USB-C-datakabel, lader voor 18650-cellen.

Stap 3. Laad de twee 18650-cellen volledig op.

Stap 4. Download of clone de GitHub-repository linefollower op je computer.
Mechanica

Stap 5. Maak de mechanische onderdelen volgens de mechanische tekeningen.

Stap 6. Monteer de twee N20-motoren op het chassis.

Stap 7. Monteer de wielen op de motoras.

Stap 8. Monteer de batterijhouder op het chassis.

Stap 9. Monteer de QTR-8A-sensorbalk vooraan, volgens de tekeningen.

Elektronica
Stap 10. Soldeer de pinheaders op de ESP32-S3 SuperMini, de DRV8833-module en de QTR-8A.

Stap 11. Verbind de voeding volgens het elektrisch schema:

• Batterijhouder (+) naar de schakelaar, schakelaar naar IN+ van de buckconverter.
• Batterijhouder (−) naar IN− van de buckconverter.
• Plaats nog geen cellen.

Stap 12. Plaats de cellen, zet de schakelaar aan en meet met de multimeter dat OUT+ van de buckconverter 3,3 V is. Zet de schakelaar uit.

Stap 13. Verbind de ESP32-S3 SuperMini:

• OUT+ van de buckconverter naar 3V3.
• OUT− van de buckconverter naar GND.

Stap 14. Verbind de DRV8833 volgens het schema:

• VCC en GND volgens het schema.
• EEP (nSLEEP) naar 3V3.
• OUT1 en OUT2 naar motor M1.
• OUT3 en OUT4 naar motor M2.

Stap 15. Verbind de stuursignalen van de DRV8833:

DRV8833
ESP32-S3
IN1
GPIO13
IN2
GPIO12
IN3
GPIO4
IN4
GPIO3

Stap 16. Verbind de QTR-8A:

• VCC naar 3V3.
• GND naar GND.
• IR naar VCC, zoals in het schema.

Stap 17. Verbind de sensoruitgangen:

QTR-8A
Netlabel
ESP32-S3
D1
S1
GPIO1
D2
S2
GPIO2
D3
S3
GPIO5
D4
S4
GPIO6
D5
S5
GPIO7
D6
S6
GPIO8
D7
S7
GPIO9
D8
S8
GPIO10
Stap 18. Verbind de EEPROM (FM24C256):

• VCC naar 3V3.
• GND, WP, A0, A1 en A2 naar GND.
• SDA naar GPIO44 (RX).
• SCL naar GPIO43 (TX).

Stap 19. Verbind de start/stop-knop tussen GPIO11 en GND.

Stap 20. Verbind de power-led: 3V3, dan weerstand van 220 Ω, dan led, dan GND.

Stap 21. Controleer alle verbindingen met het elektrisch schema.

Stap 22. Meet met de multimeter dat er geen kortsluiting is tussen 3V3 en GND.
Software installeren

Stap 23. Installeer Arduino IDE 2.

Stap 24. Open Arduino IDE, ga naar Bestand, Voorkeuren en voeg bij Additional boards manager URLs toe: https://espressif.github.io/arduino-esp32/package_esp32_index.json

Stap 25. Open de Boards Manager en installeer esp32 van Espressif Systems.
Compileren en uploaden

Stap 26. Open basisprogramma.ino in Arduino IDE.

Stap 27. Kies bij Tools:

• Board: ESP32S3 Dev Module
• USB CDC On Boot: Enabled
• Port: de poort van de ESP32

Stap 28. Zet de schakelaar van de robot uit en verbind de ESP32 met de computer via USB-C.

Stap 29. Verschijnt er geen poort: houd BOOT ingedrukt, druk op RESET en laat beide los.

Stap 30. Klik op Verify om te compileren.

Stap 31. Klik op Upload.
Testen

Stap 32. Open de Serial Monitor op 115200 baud en typ get.

Stap 33. Druk op de start/stop-knop en controleer dat de status wisselt tussen RUN en STOP.

Stap 34. Verbind je telefoon of computer met het wifi-netwerk van de robot en open 192.168.4.1.

Stap 35. Stel een waarde in via de webpagina, klik op Opslaan, herstart de robot en controleer dat de waarde blijft.

Stap 36. Upload codeH-brug.ino en controleer dat beide motoren draaien.

Stap 37. Upload Sensoren_Uitlezen_POC.ino en controleer in de Serial Monitor dat alle acht sensoren waarden geven.
Rijden

Stap 38. Upload de finale code
.
Stap 39. Stel Kp, Ki, Kd, snelheid en cyclustijd in via de webpagina of met seriële commando's.

Stap 40. Plaats de robot op de lijn, zet de schakelaar aan en druk op de start/stop-knop.
