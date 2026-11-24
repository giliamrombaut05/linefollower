# elektronisch schema

Elektronisch schema van de plan B opstelling, getekend in EasyEDA.

* [elektrisch schema line follower.pdf](elektrisch%20schema%20line%20follower.pdf)
* originele EasyEDA-bestanden: [Elektrisch schema line follower.epro2](Elektrisch%20schema%20line%20follower.epro2) en [easyeda_linefollower_planB.json](easyeda_linefollower_planB.json)

### pintoewijzing ESP32-S3 SuperMini

| ESP32-pin | aangesloten op | functie |
|---|---|---|
| GPIO1, 2, 5, 6, 7, 8, 9, 10 | QTR-8A D1 … D8 | sensoren (allemaal ADC1, werkt ook met wifi aan) |
| TX (GPIO43) | EEPROM SDA | I2C data |
| RX (GPIO44) | EEPROM SCL | I2C klok |
| GPIO13 | DRV8833 IN1 | motor M1 |
| GPIO12 | DRV8833 IN2 | motor M1 |
| GPIO4 | DRV8833 IN3 | motor M2 |
| GPIO3 | DRV8833 IN4 | motor M2 |
| GPIO11 | drukknop (andere kant naar GND, interne pull-up) | start/stop |
| 3V3 | buck converter OUT+ | voeding ESP32, QTR-8A, EEPROM |

* elke sensoruitgang D1 … D8 heeft een weerstand van 100 kΩ naar GND (R2 t/m R9)
* de DRV8833 wordt rechtstreeks gevoed door de batterijen (na de schakelaar), de rest via de buck converter op 3,3 V
* power-led met 220 Ω tussen 3,3 V en GND

Aandachtspunten (nog aan te passen in het schema):
* EEPROM-symbool heet FM24C256, gebruikte component is AT24C256
* er staat één batterijsymbool, in werkelijkheid zijn het 2x 18650 in serie (8,4V)
