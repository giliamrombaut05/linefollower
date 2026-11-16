# Sensoren proof of concept
Doel:
Aantonen dat minimaal 6 sensoren onafhankelijk van elkaar kunnen uitgelezen worden.

Hardware:
ESP32-S3 SuperMini
QTR-8A

Resultaat:
De sensorarray werd uitgelezen via de ADC van de ESP32-S3.
Sensoren boven een zwarte lijn geven de status ZWART.
Sensoren boven de witte achtergrond geven de status WIT.

Voorbeeld:
D1: WIT D2: WIT D3: WIT D4: WIT D5: ZWART D6: ZWART D7: ZWART D8: ZWART

Besluit:
Alle acht sensoren kunnen onafhankelijk van elkaar uitgelezen worden. 
