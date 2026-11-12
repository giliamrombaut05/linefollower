# H-Bridge proof of concept

Voor het aansturen van de twee DC-motoren wordt gebruik gemaakt van een DRV8833 bipolaire stappenmotor- en DC-motoraansturing. Deze H-Bridge maakt het mogelijk om twee DC-motoren onafhankelijk van elkaar te besturen. De snelheid van de motoren kan traploos geregeld worden met PWM-signalen afkomstig van de ESP32-C3 Super Mini, terwijl de draairichting bepaald wordt door de combinatie van de stuursignalen.
De DRV8833 beschikt over vier ingangen (IN1, IN2, IN3 en IN4), vier uitgangen (OUT1, OUT2, OUT3 en OUT4), een voedingsingang (VCC) en een massa-aansluiting (GND).
Motor A wordt bestuurd via IN1 en IN2 en is aangesloten op OUT1 en OUT2. Motor B wordt bestuurd via IN3 en IN4 en is aangesloten op OUT3 en OUT4.
Door PWM-signalen op de ingangen aan te bieden kan zowel de snelheid als de draairichting softwarematig geregeld worden.
