#define SENSORPIN PA0
#define BUTTONPIN PB7

int sampleNo = 20;

void setup() {
  Serial.begin(9600);
  pinMode(BUTTONPIN, INPUT_PULLUP);
}

void loop() {
  int value[sampleNo];
  int sum = 0;
  int average;
  if(digitalRead(BUTTONPIN) == LOW) {
    for(int i = 0; i < sampleNo; i++) {
      int readValue = analogRead(SENSORPIN);
      value[i] = readValue;
    }

    int maxIndex = 0;
    int minIndex = 0;

    for(int i = 1; i < sampleNo; i++) {
      if(value[i] > value[maxIndex]) {maxIndex = i;}
      if(value[i] < value[minIndex]) {minIndex = i;}
    }

    for(int i = 0; i < sampleNo; i++) {
      if(i != maxIndex && i != minIndex) {sum += value[i];}
    }

    if(maxIndex != minIndex) {
      average = sum / (sampleNo - 2);
    } else {
      average = sum / (sampleNo - 1);
    }

    for(int i = 0; i < sampleNo; i++) {
      Serial.print(value[i]);
      Serial.print(',');
    }

    Serial.print(';');
    Serial.println(average);

    float voltage = (float)average * 3.3 / 1024 + 0.15;

    Serial.print(voltage);
    Serial.print(';');

    float length = 1 / ((voltage * 1000 - 1125) / 137500);

    Serial.println(length);

    delay(1000);
  }
}