# draadloze communicatie proof of concept
Deze proof of concept toont aan dat er draadloos duplex gecommuniceerd kan worden tussen een ESP32-C3 en een smartphone of laptop via een webinterface. Via deze webinterface kunnen instellingen naar de microcontroller gestuurd worden en kunnen de actuele instellingen en status van de robot terug uitgelezen worden.
<br />
### configuratie
De ESP32-C3 maakt een eigen WiFi-netwerk aan met de naam "LineFollower". Een smartphone of laptop verbindt met dit netwerk en opent de webinterface via het IP-adres 192.168.4.1.

Via de webinterface kunnen volgende instellingen aangepast worden:

Kp
Ki
Kd
Maximum snelheid
Cycle time
Debug status
Daarnaast kan de robot gestart en gestopt worden via de START- en STOP-knoppen.
### opmerkingen
De instellingen worden opgeslagen in een externe EEPROM (24C256). Hierdoor blijven de laatst ingestelde waarden behouden na het uitschakelen van de robot.

De communicatie verloopt volledig draadloos via WiFi.
### gebruiksaanwijzing
Schakel de robot in.
Verbind de smartphone met het WiFi-netwerk "LineFollower".
Vul het wachtwoord in.
Open een webbrowser.
Surf naar 192.168.4.1.
Pas indien gewenst de PID-instellingen, maximale snelheid of cycle time aan.
Klik op "OPSLAAN" om de instellingen in de EEPROM te bewaren.
Gebruik de START- en STOP-knoppen om de robot draadloos te bedienen.
Controleer de huidige status en instellingen onderaan de webinterface.
