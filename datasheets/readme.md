# Datasheets & Pinouts

Overzicht van de datasheets en pinout-diagrammen van alle gebruikte componenten in de linefollower.

| Onderdeel | Datasheet / Pinout | Belangrijke specificaties & aandachtspunten |
|------------|------------|------------|
| **ESP32-S3 SuperMini** | [ESP32-S3 SuperMini Datasheet](./datasheet%20ESP32-S3%20Supermini%202%20.pdf)<br>[ESP32-S3 Pinout](./datasheet%20ESP32-S3%20supermini.avif) | Logica op 3,3 V. GPIO 1 t/m 10 ondersteunen ADC voor analoge sensoren. |
| **DRV8833 (IC & Module)** | [DRV8833 Chip Datasheet](./drv8833-datasheet%20(3).pdf)<br>[DRV8833 Module Pinout](./Pinout%20DRV8833.pdf) | Dubbele H-brug voor 2 DC-motoren (2,7 V - 10,8 V). EEP (nSLEEP) moet hoog zijn om de driver te activeren. ULT is de fault-uitgang. |
| **N20 DC-motor** | [N20 Motor Datasheet](./N20%20DC-motor%20datasheet.pdf) | 5 V micro-reductiemotor (500 RPM). |
| **QTR-8A Lijnsensor** | [QTR-8A Datasheet](./QTR-8x%20(2).pdf) | 8 reflectiesensoren met analoge uitgangen. |
| **AT24C256 EEPROM** | [AT24C256 Datasheet](./AT24C256%20(2).pdf) | 256 Kbit (32 KB) extern I²C-geheugen. |
| **Buck Converter (3,3 V)** | [Buck Converter Datasheet](./DC-DC%20Step-down%20Buck%20Converter%205A%20-%203.3V%20output-datasheet%20(1).pdf) | Zet de batterijspanning om naar een stabiele 3,3 V. |
| **18650 Li-ion cel** | [18650 Batterij Datasheet](./18650%20Li-ion%20batterij.pdf) | Nominale spanning 3,7 V, maximaal 4,2 V volledig geladen. |
| **Schuifschakelaar** | [Schakelaar Datasheet](./schakelaar.pdf) | Hoofdschakelaar van de robot. |
| **Drukknop (SMD)** | [Drukknop Datasheet](./Button%20(1).pdf) | Start/stop-knop gebruikt voor hardware interrupts. |
