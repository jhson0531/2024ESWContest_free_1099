/* Includes ------------------------------------------------------------------*/
#include <Arduino.h>
#include <Wire.h>
#include <vl53lx_class.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <SPI.h>
#include <SD.h>

#define DEV_I2C Wire   // i2c
#define led PA15      // debug led

//센서 accuracy 조정 (기본값 0)
#define VL53LX_TUNINGPARM_PHASECAL_PATCH_POWER_DEFAULT \       
((uint32_t) 4)

//latency 및 max ranging distance 개선 (기본값 15000)
#define VL53LX_TUNINGPARM_RESET_MERGE_THRESHOLD_DEFAULT \      
((uint32_t) 17000)

VL53LX sensor_vl53lx_sat(&DEV_I2C, A1);
File myFile;

unsigned long before_time = 0;
unsigned long lastMeasurementTime = 0;
int btn_state = 2;
int count = 0;
int print_state = 0;
int measurement_state = 0;

int speed = 200;     // 모터 pwm 설정 (속도)

int16_t distance_arr[40];

void runRC(int);
void resultPrint();
void rawdataPrint();
void calculatedDataPrint();
void sensorMeasurement(int);


/* Setup ---------------------------------------------------------------------*/
void setup()
{
  // Initialize serial for output.
  Serial.begin(9600);
  Serial.println("Starting...");

  digitalWrite(led, LOW);
  // SD card
  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    digitalWrite(led, LOW);
    while (1);
  }
  Serial.println("SD card initialization done.");

   // pin
   pinMode(4, OUTPUT);
   pinMode(5, OUTPUT);
   pinMode(7, OUTPUT);
   pinMode(8, OUTPUT);
   pinMode(6, OUTPUT);
   pinMode(9, OUTPUT);

   pinMode(2, OUTPUT);
   pinMode(led, OUTPUT);
   pinMode(PC13, INPUT);

   // Initialize I2C bus.
   DEV_I2C.begin();
   Serial.println("Starting I2C...");

   // Configure VL53LX satellite component.
   sensor_vl53lx_sat.begin();
   Serial.println("Starting sensor...");

   // Switch off VL53LX satellite component.
   sensor_vl53lx_sat.VL53LX_Off();
   Serial.println("Turning off sensor...");

   //Initialize VL53LX satellite component.
   digitalWrite(led, HIGH);
   sensor_vl53lx_sat.InitSensor(0x52);
   
   Serial.println("Initializing sensor...");

  //거리모드 설정
   sensor_vl53lx_sat.VL53LX_SetDistanceMode(VL53LX_DISTANCEMODE_MEDIUM);
   Serial.println("Setting distance mode...");

  //  Set timing budget
   sensor_vl53lx_sat. VL53LX_SetMeasurementTimingBudgetMicroSeconds(33000);
   Serial.println("Setting done.");
   digitalWrite(led, LOW);
}

void loop()
{
   unsigned long now_time = millis();

  if ( digitalRead(PC13) == LOW) { //PC13번은 뉴클레오 보드의 내장버튼에 연결되어 있음
    btn_state = 1;
  }

  if ( (digitalRead(PC13) == HIGH) && (btn_state == 1)){  //측정 전 초기화
    before_time = now_time;  
    lastMeasurementTime = now_time;
    count = 0;
    btn_state = 0;
    print_state=0;
    digitalWrite(led, LOW);
  }

  if ((digitalRead(PC13) == HIGH) && (btn_state == 0)){   //now_time - before_time : 버튼 누른 뒤 흐른 시간
    if((now_time - before_time) < 300 ){
      runRC((int)speed*0.75);
    }
    else if((now_time - before_time) < 2100 ){
      runRC(speed);
      
      if((now_time - before_time) > 1934){
        if(measurement_state == 0){
          // Start Measurements
          sensor_vl53lx_sat.VL53LX_StartMeasurement();
          Serial.println("start measurement...");
        }
        measurement_state = 1;
      }
    }
    else if((now_time - before_time) < 3500){
      digitalWrite(2, LOW);
    }
    else{
      measurement_state = 0;
      digitalWrite(2, LOW);
      resultPrint();
    }
    sensorMeasurement(now_time);
  }
}

void runRC(int speed){
    digitalWrite(2, HIGH);
    analogWrite(6, speed);
    analogWrite(9, speed);
    digitalWrite(4, HIGH);
    digitalWrite(5, LOW);
    digitalWrite(7, LOW);
    digitalWrite(8, HIGH);
}

void resultPrint(){
    if(print_state == 0){
        rawdataPrint();
        calculatedDataPrint();
    }
}

void rawdataPrint(){
  String s = "";
  for(int i = 0; i<40; i++){
    s += distance_arr[i];
    s += ", ";
  }
  Serial.println(s);
  myFile = SD.open("rawdata.txt", FILE_WRITE);
  if (myFile) {
    digitalWrite(led, LOW);
    myFile.print(s);
    myFile.println("");
    myFile.close(); // close the file:
  } else {
    // if the file didn't open, print an error:
    digitalWrite(led, HIGH);
    Serial.println("error opening rawdata.txt");
  }
}

void calculatedDataPrint(){
    String st = "";
    st += distance_arr[5]; // 브레이크 밟은 순간의 거리
    st += ", ";

    float velocity = 0;
    for(int i = 1; i<8; i++){
        velocity += distance_arr[i] - distance_arr[i+1] ; // 브레이크 밟은 직전 직후 속도 7개
    }
    st += (velocity/7); //속도 평균
    st += ", ";

    float accel = 0;  // 가속도
    float velocity_arr[3] = {0,0,0};
    for(int i = 6; i<9; i++){
        velocity_arr[i-6] = distance_arr[i] - distance_arr[i+1]; 
    }
    accel = (velocity_arr[1] + velocity_arr[2])/2 - (velocity_arr[0] + velocity_arr[1])/2;
    st += accel;
    st += ", ";

    myFile = SD.open("C_data.txt", FILE_WRITE);
    if(myFile){
        digitalWrite(led, LOW);
        myFile.print(st);
        myFile.println("");
        myFile.close();
    } else{
        digitalWrite(led, HIGH);
        Serial.println("error opening C_data.txt");
    }
    print_state = 1;
}

void sensorMeasurement(int now_time){
  VL53LX_MultiRangingData_t MultiRangingData;
  VL53LX_MultiRangingData_t *pMultiRangingData = &MultiRangingData;
  uint8_t NewDataReady = 0;
  int status;
  if(count<40 && measurement_state == 1){
    do{
      status = sensor_vl53lx_sat.VL53LX_GetMeasurementDataReady(&NewDataReady);
    } while (!NewDataReady);
    if((!status)&&(NewDataReady!=0)){
      Serial.print(now_time - lastMeasurementTime);
      lastMeasurementTime = now_time;
      status = sensor_vl53lx_sat.VL53LX_GetMultiRangingData(pMultiRangingData);
      //no_of_object_found=pMultiRangingData->NumberOfObjectsFound; //감지된 물체 개수를 8bit 정수로 반환
      distance_arr[count] = pMultiRangingData->RangeData[0].RangeMilliMeter ;
      Serial.print(" / D :");
      Serial.println(pMultiRangingData->RangeData[0].RangeMilliMeter);
      count++ ;
    }
    if (status==0){
      status = sensor_vl53lx_sat.VL53LX_ClearInterruptAndStartMeasurement();
    }
  }
  else{
    sensor_vl53lx_sat.VL53LX_StopMeasurement();
  }
}