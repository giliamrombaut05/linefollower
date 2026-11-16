# Datasheets & Pinouts

Overzicht van de datasheets en pinout-diagrammen van alle gebruikte componenten in de linefollower.

| Onderdeel | Datasheet / Pinout | Belangrijke specificaties & aandachtspunten |
| :--- | :--- | :--- |
| **ESP32-S3 SuperMini** | [ESP32-S3 SuperMini Datasheet](<datasheet ESP32-S3 Supermini 2 .pdf>)<br>[ESP32-S3 Pinout](<datasheet ESP32-S3 supermini.avif>) | Logica op $3,3\text{ V}$. GPIO 1 t/m 10 ondersteunen ADC1 voor analoge sensoren. Voeding via `3V3`-pin (gereguleerd) of `5V`-pin. |
| **DRV8833 (IC & Module)** | [DRV8833 Chip Datasheet](<drv8833 datasheet.pdf>)<br>[DRV8833 Module Pinout](<DRV8833 module pinout.png>) | Dubbele H-brug voor 2 DC-motoren ($2,7\text{ V} - 10,8\text{ V}$, tot $1,5\text{ A}$ RMS per kanaal). `EEP` (nSLEEP) moet hoog ($3,3\text{ V}$) zijn om de driver te activeren. |
| **N20 DC-motor** | [N20 Motor Datasheet](<N20 motor datasheet.pdf>) | 5 V micro-reductiemotor (500 RPM). Rechtstreeks aangestuurd via `OUT1/OUT2` en `OUT3/OUT4` van de DRV8833. |
| **QTR-8A Lijnsensor** | [QTR-8x Datasheet](<QTR-8x datasheet.pdf>) | 8 reflectiesensoren met analoge uitgangen (`D1`–`D8`). Werkt op $3,3\text{ V}$; optimale leesafstand is circa $3\text{ mm}$ boven de baan. |
| **AT24C256 EEPROM** | [24C256 Datasheet](<AT24C256 (2).pdf>) | 256 Kbit (32 KB) extern I2C-geheugen ($3,3\text{ V}$). Adrespinnen `A0`–`A2` en `WP` aan `GND` geeft I2C-adres `0x50` met schrijftoegang. |
| **Buck Converter (3,3 V)** | [Step-Down Converter Datasheet](<DC-DC Step-down Buck Converter 5A - 3.3V output-datasheet (1).pdf>) | Verlaagt de batterijspanning naar een stabiele $3,3\text{ V}$ voor de ESP32-S3, QTR-8A en 24C256. |
| **18650 Li-ion cel** | [18650 Batterij Datasheet](<18650 Li-ion batterij.pdf>) | Nominale spanning $3,7\text{ V}$ (volgeladen $4,2\text{ V}$), capaciteit $3500\text{ mAh}$. Voedt de DRV8833 en de buckconverter. |
| **Schuifschakelaar** | [Schakelaar Datasheet](<schakelaar.pdf>) | Hoofdschakelaar in serie met de batterij om de volledige voeding van de robot aan en uit te zetten. |
| **Drukknop (SMD)** | [Drukknop Datasheet](<Button (1).pdf>) | Start/stop-knop verbonden tussen een GPIO-pin (met interne pull-up) en `GND` voor hardware-interrupts. |
