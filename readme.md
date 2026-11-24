# LineFollower

![Foto van de linefollower (nog te vervangen door een echte foto)](images/empty.png)

Repository voor ons linefollower-robotproject (HOGENT). Deze repository bevat de elektrische schema's, datasheets, proof-of-concept-testen, de bill of materials, de instructable, de gebruiksaanwijzing en de besturingssoftware met PID-regeling en draadloze webinterface.

## Specifications

* **Microcontroller:** ESP32-S3 SuperMini (3,3 V logica, dual-core Xtensa LX7, ingebouwde WiFi)
* **Motors:** 2x N20 DC-motor met reductie (6 V, 500 RPM)
* **H-bridge:** DRV8833 H-brug motordriver-module
* **Sensors:** QTR-8A (8-kanaals analoge IR-reflectiesensor array)
* **Batteries:** 2x 18650 Li-ion cel in serie, met 3,3 V DC-DC step-down (buck) converter
* **Memory:** 24C256 I2C-EEPROM voor het bewaren van parameters en kalibratie
* **Wireless communication:** WiFi (access point "LineFollower", webpagina op 192.168.4.1)
* **Distance sensor - motors:** *nog te meten en in te vullen*
* **Weight:** *nog te wegen en in te vullen*
* **Speed:** *nog te meten op de baan en in te vullen*

## Inhoud van de repository

| map | inhoud |
|---|---|
| [bill of materials](bill%20of%20materials/readme.md) | onderdelen met prijzen |
| [datasheets](datasheets/readme.md) | datasheets van de gebruikte componenten |
| [technische tekeningen](technische%20tekeningen/elektronisch/readme.md) | elektrische schema's en pinoverzicht |
| [code/proof of concepts](code/proof%20of%20concepts) | testcode per onderdeel: H-brug, sensor, interrupt, wireless |
| [code/basis](code/basis/readme.md) | basisprogramma: communicatie, EEPROM en interrupt |
| [code/finaal](code/finaal/readme.md) | finaal programma met sensoren, kalibratie en PID |
| [instructable](instructable/readme.md) | stap-voor-stap bouwhandleiding |
| [gebruiksaanwijzing](gebruiksaanwijzing/readme.md) | gebruik, commando's en kalibratie |
