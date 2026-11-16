/*
  QTR-8A
  ESP32-S3 SuperMini

  D1 -> GPIO8
  D2 -> GPIO9
  D3 -> GPIO10
  D4 -> GPIO11
  D5 -> GPIO12
  D6 -> GPIO13
  D7 -> GPIO1
  D8 -> GPIO2
*/

const int sensor[8] =
{
  8,
  9,
  10,
  11,
  12,
  13,
  1,
  2
};

const int threshold = 3700;

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  for(int i = 0; i < 8; i++)
  {
    int value = analogRead(sensor[i]);

    Serial.print("D");
    Serial.print(i + 1);
    Serial.print(": ");

    if(value > threshold)
    {
      Serial.print("ZWART");
    }
    else
    {
      Serial.print("WIT");
    }

    Serial.print(" (");
    Serial.print(value);
    Serial.print(")");

    Serial.print("\t");
  }

  Serial.println();

  delay(200);
}