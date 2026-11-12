# start/stop interrupt proof of concept
Deze proof of concept toont aan hoe een start/stop functie gerealiseerd kan worden met een drukknop en een hardware interrupt op een ESP32-C3.

Bij elke druk op de knop wordt een interrupt uitgevoerd, waardoor de variabele run wisselt tussen RUN en STOP. De huidige toestand wordt weergegeven in de Serial Monitor.

Deze test toont aan dat de start/stop functionaliteit correct werkt zonder de drukknop voortdurend softwarematig uit te lezen.
