const int knop = 11;

volatile bool knopFlag = false;
volatile unsigned long flankTijd = 0;

bool run = false;
bool vergrendeld = false;
unsigned long losSinds = 0;
unsigned long laatstePrint = 0;

const unsigned long debounceTijd = 30;   // ms
const unsigned long losTijd = 50;        // ms stabiel los

void IRAM_ATTR knopISR() {
  if (!knopFlag) {
    knopFlag = true;
    flankTijd = millis();
  }
}

void setup() {
  pinMode(knop, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(knop), knopISR, FALLING);
  Serial.begin(115200);
  Serial.println("Status = STOP");
}

void loop() {
  bool laag = (digitalRead(knop) == LOW);

  if (vergrendeld) {
    // wacht tot de knop stabiel losgelaten is
    if (laag) {
      losSinds = 0;
    } else if (losSinds == 0) {
      losSinds = millis();
    } else if (millis() - losSinds >= losTijd) {
      vergrendeld = false;
      knopFlag = false;
      losSinds = 0;
    }
  } else if (knopFlag && millis() - flankTijd >= debounceTijd) {
    knopFlag = false;
    if (laag) {                       // echt ingedrukt, geen storing
      run = !run;
      vergrendeld = true;
      losSinds = 0;
      Serial.println(run ? "RUN" : "STOP");
    }
  }

  if (millis() - laatstePrint >= 1000) {
    laatstePrint = millis();
    Serial.println(run ? "status: RUN" : "status: STOP");
  }
}