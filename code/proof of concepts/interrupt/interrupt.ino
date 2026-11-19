// start/stop interrupt proof of concept
// drukknop KEY1 tussen GPIO7 en GND (zie schema), interne pull-up

const int knop = 7;                       // GPIO7 volgens elektrisch schema
const unsigned long debounceTijd = 200;   // ms: drukken binnen deze tijd worden genegeerd (contactdender)

volatile bool run = false;                // false = STOP, true = RUN
volatile unsigned long laatsteDruk = 0;   // tijdstip (ms) van de laatst aanvaarde druk
volatile unsigned int aantalInterrupts = 0;  // alle dalende flanken, ook dender
volatile unsigned int aantalDrukken = 0;     // enkel de aanvaarde drukken

void IRAM_ATTR knopISR()
{
  unsigned long nu = millis();
  aantalInterrupts++;
  if (nu - laatsteDruk >= debounceTijd)
  {
    laatsteDruk = nu;
    aantalDrukken++;
    run = !run;
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(knop, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(knop), knopISR, FALLING);
  Serial.println("Robot ingeschakeld");
  Serial.println("Status = STOP");
}

void loop()
{
  // de delay simuleert code die bezig is: de interrupt wordt toch onmiddellijk verwerkt
  Serial.print(run ? "RUN " : "STOP");
  Serial.print("  drukken: ");
  Serial.print(aantalDrukken);
  Serial.print("  interrupts: ");
  Serial.println(aantalInterrupts);
  delay(1000);
}
