#define Photo_Sen 3

int state = 0; // 0: 없음, 1:감지
float Pass_Count = 0;
float velocity_arr[56];
int count = 0;
unsigned long before_time = 0;
int val = 0; 

void ISR_count();

void setup() 
{
  Serial.begin(9600);
  pinMode(Photo_Sen, INPUT);
}

void loop() 
{
  unsigned long now_time = millis();
  int state = digitalRead(Photo_Sen);

  attachInterrupt(digitalPinToInterrupt(Photo_Sen), ISR_count, CHANGE);
  
  if( (now_time - before_time) >= 500)
  {
    velocity_arr[count] = Pass_Count / 60 * 20.42 / (now_time - before_time) * 36;                 // km/h
    Serial.print("횟수: ");
    Serial.println(Pass_Count);

    Serial.print("속력: ");
    Serial.println(velocity_arr[count]);

    before_time = now_time;
    Pass_Count = 0;
  }
}

void ISR_count()
{
  Pass_Count += 1;
}
