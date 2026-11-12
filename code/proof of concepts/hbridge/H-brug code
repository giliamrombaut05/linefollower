const int IN1 = 4;
const int IN2 = 3;
const int IN3 = 2;
const int IN4 = 1;

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  Serial.begin(115200);
}

void stelMotorSnelheidIn(int pin1, int pin2, int snelheid) {

  if (snelheid > 0) {
    analogWrite(pin1, snelheid);
    analogWrite(pin2, 0);
  }
  else if (snelheid < 0) {
    analogWrite(pin1, 0);
    analogWrite(pin2, -snelheid);
  }
  else {
    analogWrite(pin1, 0);
    analogWrite(pin2, 0);
  }
}

void loop() {

  Serial.println("Test 1: Motor A 25%, Motor B 100%");

  stelMotorSnelheidIn(IN1, IN2, 64);
  stelMotorSnelheidIn(IN3, IN4, 255);

  delay(4000);

  Serial.println("Test 2: Motor A achteruit 50%, Motor B vooruit 100%");

  stelMotorSnelheidIn(IN1, IN2, -128);
  stelMotorSnelheidIn(IN3, IN4, 255);

  delay(4000);

  Serial.println("Test 3: Motor A stop, Motor B 75%");

  stelMotorSnelheidIn(IN1, IN2, 0);
  stelMotorSnelheidIn(IN3, IN4, 192);

  delay(4000);

  Serial.println("Test 4: Beide motoren achteruit");

  stelMotorSnelheidIn(IN1, IN2, -255);
  stelMotorSnelheidIn(IN3, IN4, -255);

  delay(4000);

  Serial.println("Test 5: Stop");

  stelMotorSnelheidIn(IN1, IN2, 0);
  stelMotorSnelheidIn(IN3, IN4, 0);

  delay(4000);
}
