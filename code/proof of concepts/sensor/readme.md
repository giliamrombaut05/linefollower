# Sensoren proof of concept

minimale hard- en software die aantoont dat minimaal 6 sensoren onafhankelijk van elkaar kunnen uitgelezen worden (geen calibratie, normalisatie of interpolatie). Hierbij moet een zo groot mogelijk bereik van de AD converter benut worden (indien van toepassing)

### hardware
* ESP32-S3 SuperMini
* QTR-8A (8 analoge IR-reflectiesensoren)

### schema
[schema sensor.pdf](schema%20sensor.pdf)

### aansluitingen
| QTR-8A | ESP32-S3 | ADC |
|--------|----------|-----|
| D1 | GPIO1 | ADC1 |
| D2 | GPIO2 | ADC1 |
| D3 | GPIO5 | ADC1 |
| D4 | GPIO6 | ADC1 |
| D5 | GPIO7 | ADC1 |
| D6 | GPIO8 | ADC1 |
| D7 | GPIO9 | ADC1 |
| D8 | GPIO10 | ADC1 |
| VCC | 3V3 | |
| IR | 3V3 (leds aan) | |
| GND | GND | |

Elke uitgang D1…D8 heeft daarnaast een weerstand van 100 kΩ naar GND.

Alle sensoren zitten op ADC1 (GPIO1–10). ADC2 (GPIO11–20) wordt gedeeld met wifi: in een eerste versie zaten D4–D6 op GPIO11–13, en met wifi aan gaven die op wit en zwart dezelfde waarde (±400).

De sensor is een kloon van de QTR-8A (rode print "8 sensor lines follow", pinnen VCC, D8…D1, IR, GND), zonder 3.3V BYPASS. Gemeten weerstand tussen VCC en een uitgang: 10 kΩ.

### code
[Sensoren_Uitlezen_POC.ino](Sensoren_Uitlezen_POC/Sensoren_Uitlezen_POC.ino)

De 8 sensoren worden om de 200 ms uitgelezen met `analogRead` (12-bit ADC van de ESP32-S3, bereik 0 - 4095, demping 11 dB = grootste meetbereik). De ruwe waarde per sensor gaat naar de seriële monitor (115200 baud). Per sensor wordt ook de minimum- en maximumwaarde bijgehouden.

Commando's via de seriële monitor:

| commando | functie |
|---|---|
| `w` | wifi access point aan/uit (om te testen of de metingen met wifi aan blijven kloppen) |
| `r` | min/max van alle sensoren wissen |
| `s` | tabel met min/max per sensor tonen (markeert waarden 0 of 4095) |

### werking QTR-8A
Elke sensor is een spanningsdeler van een fototransistor en een vaste weerstand van 47 kΩ naar Vcc. Wit reflecteert veel IR-licht → uitgang naar 0 V (lage waarde). Zwart reflecteert weinig → uitgang naar Vcc (hoge waarde). Optimale afstand tot de ondergrond is 3 mm, maximaal 6 mm. Bij Vcc = 3,3 V moeten de pinnen *3.3V BYPASS* doorverbonden worden, anders branden de IR-leds te zwak (bron: QTR-8x datasheet).

Een gemeten waarde van 0 of 4095 is geen bruikbare informatie: de spanning ligt dan op of buiten de grens van het meetbereik.

De ADC van de ESP32-S3 meet tot ongeveer 3,0–3,1 V; boven zwart staat er bijna Vcc = 3,3 V → 4095. Een extra weerstand Rg van de uitgang naar GND vormt met de 10 kΩ op het bord een spanningsdeler:

U_zwart = Vcc · Rg / (10 kΩ + Rg) = 3,3 V · 100 / 110 = 3,0 V

Getest: met 100 kΩ op D4 werd zwart 3820 in plaats van 4095.

### resultaat

#### meting 1 (probleem)
![resultaat seriële monitor](resultaat%20seriele%20monitor.png)

* wit: 2841 - 3451 → gebruikt maar een klein deel van het bereik
* zwart: 4095 → ADC verzadigd, geen bruikbare waarde

#### meting 2 (na aanpassing)
Aanpassingen:
* sensoren op ADC1-pinnen (GPIO 1, 2, 5–10) i.p.v. deels ADC2
* per uitgang 100 kΩ naar GND (spanningsdeler met de 10 kΩ op het bord)

Hoogte sensor boven de ondergrond: ± 3 mm (optimale afstand volgens de QTR-8x datasheet)

Werkwijze: sensor stil boven wit → `r` → enkele seconden wachten → `s`. Daarna idem boven zwart. Dan `w` (wifi aan) en alles herhalen. Waarden = min – max over die periode. Volledige uitvoer van de seriële monitor: [meting 2 seriele monitor.txt](meting%202%20seriele%20monitor.txt)

| sensor | wit, wifi uit | zwart, wifi uit | wit, wifi aan | zwart, wifi aan |
|---|---|---|---|---|
| D1 | 2814 – 2969 | 3413 – 3449 | 2355 – 2862 | 3285 – 3343 |
| D2 | 2207 – 2698 | 3370 – 3449 | 1196 – 2247 | 3213 – 3347 |
| D3 | 2283 – 2694 | 3369 – 3462 | 1419 – 2387 | 3243 – 3341 |
| D4 | 2194 – 2676 | 3371 – 3467 | 1205 – 2407 | 3239 – 3346 |
| D5 | 2193 – 2652 | 3335 – 3467 | 1263 – 2453 | 3226 – 3343 |
| D6 | 2203 – 2666 | 3351 – 3449 | 1258 – 2543 | 3211 – 3342 |
| D7 | 2024 – 2620 | 3359 – 3497 | 1128 – 2569 | 3225 – 3355 |
| D8 | 2611 – 2891 | 3420 – 3492 | 2301 – 3006 | 3285 – 3376 |

### besluit
* Alle acht sensoren worden onafhankelijk van elkaar uitgelezen, ook met wifi aan (alle sensoren op ADC1).
* Geen enkele meting is 0 of 4095: zwart ligt tussen 3211 en 3497 dankzij de 100 kΩ naar GND.
* Wit en zwart overlappen bij geen enkele sensor, ook niet met wifi aan. Kleinste marge: D8 met wifi aan (wit max 3006, zwart min 3285).
* Het benutte bereik is beperkt (wit ± 1100 – 3000, zwart ± 3200 – 3500): de IR-leds van deze kloon branden zwak op 3,3 V en het bord heeft geen 3.3V BYPASS. De buitenste sensoren D1 en D8 hebben het kleinste verschil tussen wit en zwart.
* Met wifi aan liggen de waarden iets lager. Het grote verschil tussen min en max bij wit (bv. D2: 1196 – 2247) verloopt geleidelijk over enkele seconden, terwijl opeenvolgende metingen maar ± 10 – 20 verschillen: dit komt door beweging/hoogte van de sensor, niet door ruis van wifi.
* Kalibratie per sensor (wit/zwart) en normalisatie vangen de verschillen tussen de sensoren op.
