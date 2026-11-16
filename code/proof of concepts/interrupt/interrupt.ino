const int knop = 7; // Aangesloten op GPIO 7 conform nieuw schema

volatile bool run = false;
volatile unsigned long laatsteInterruptTijd = 0;
const unsigned long debounceTijd = 50; // 50 ms ontdendering

void IRAM_ATTR knopISR() {
  unsigned long huidigeTijd = millis();
  if (huidigeTijd - laatsteInterruptTijd > debounceTijd) {
    run = !run;
    laatsteInterruptTijd = huidigeTijd;
  }
}

void setup() {
  pinMode(knop, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(knop),
    knopISR,
    FALLING
  );

  Serial.begin(115200);

  Serial.println("Robot ingeschakeld");
  Serial.println("Status = STOP");
}

void loop() {
  if (run) {
    Serial.println("RUN");
  } else {
    Serial.println("STOP");
  }

  delay(1000);
}