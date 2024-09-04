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
#include "data.h"
#include "action.h"

#define DEV_I2C Wire

#define LED_B PA15  //air bag 작동을 의미(충돌 예측)
#define LED_G PA13  //비충돌을 의미 (비충돌 예측)

#define VL53LX_TUNINGPARM_PHASECAL_PATCH_POWER_DEFAULT \
((uint32_t) 4)

#define VL53LX_TUNINGPARM_RESET_MERGE_THRESHOLD_DEFAULT \
((uint32_t) 17000)

#define friction 0.515    // 부직포 : 0.539, 필름 : 0.515

VL53LX sensor_vl53lx_sat(&DEV_I2C, A1);

unsigned long before_time = 0;
unsigned long lastMeasurementTime = 0;
int btn_state = 2;
int count = 0;
int measurement_state = 0;

int speed = 200;

int16_t distance_arr[10];

void runRC(int);
void sensorMeasurement(int);
void prediction();

/* Setup ---------------------------------------------------------------------*/
void setup()
{
  // Initialize serial for output.
  Serial.begin(9600);
  Serial.println("Starting...");

  digitalWrite(LED_B, LOW);
  digitalWrite(LED_G, LOW);

   // pin
   pinMode(4, OUTPUT);
   pinMode(5, OUTPUT);
   pinMode(7, OUTPUT);
   pinMode(8, OUTPUT);
   pinMode(6, OUTPUT);
   pinMode(9, OUTPUT);

   pinMode(2, OUTPUT);
   pinMode(LED_B, OUTPUT); // airbag
   pinMode(LED_G, OUTPUT); // safe
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
   digitalWrite(LED_B, HIGH);
   sensor_vl53lx_sat.InitSensor(0x52);
   
   Serial.println("Initializing sensor...");

  //거리모드 설정
   sensor_vl53lx_sat.VL53LX_SetDistanceMode(VL53LX_DISTANCEMODE_MEDIUM);
   Serial.println("Setting distance mode...");
   digitalWrite(LED_B, LOW);

  //  Set timing budget
   sensor_vl53lx_sat. VL53LX_SetMeasurementTimingBudgetMicroSeconds(33000);
   Serial.println("Setting done."); 
}

void loop()
{
   unsigned long now_time = millis();

  if ( digitalRead(PC13) == LOW) {
    btn_state = 1;
  }

  if ( (digitalRead(PC13) == HIGH) && (btn_state == 1)){
    before_time = now_time;
    lastMeasurementTime = now_time;
    count = 0;
    btn_state = 0;

    digitalWrite(LED_G, LOW);
    digitalWrite(LED_B, LOW);
  }

  if ((digitalRead(PC13) == HIGH) && (btn_state == 0)){
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
    }
    
    if(count<10 && measurement_state == 1){
      sensorMeasurement(now_time);
    }
    else if(count == 10 && measurement_state ==1){
      prediction();
    }
    else{
      sensor_vl53lx_sat.VL53LX_StopMeasurement();
    }
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

void sensorMeasurement(int now_time){
  VL53LX_MultiRangingData_t MultiRangingData;
  VL53LX_MultiRangingData_t *pMultiRangingData = &MultiRangingData;
  uint8_t NewDataReady = 0;
  int status;

  int temp=0;
  do{
    status = sensor_vl53lx_sat.VL53LX_GetMeasurementDataReady(&NewDataReady);
    temp++;
    if(temp >=5000){
      Serial.println("Time out");
      while(1){
        digitalWrite(LED_B, HIGH);
        delay(300);
      }
    }
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

void prediction(){
  sensor_vl53lx_sat.VL53LX_StopMeasurement();
  double input_arr[4]; // {distance, velocity, accel, friction}
  input_arr[0] = distance_arr[5];
  input_arr[1] = (distance_arr[1] - distance_arr[9])/7;
  double velocity_arr[3];
  for(int i = 6; i<9; i++){
    velocity_arr[i-6] = distance_arr[i] - distance_arr[i+1];
  }
  input_arr[2] = (velocity_arr[1] + velocity_arr[2])/2 - (velocity_arr[0] + velocity_arr[1])/2;
  input_arr[3] = friction;

  if(isAirbag(input_arr) == 1) {
    digitalWrite(LED_B, HIGH);
    digitalWrite(LED_G, LOW);
  }
  else{
    digitalWrite(LED_B, LOW);
    digitalWrite(LED_G, HIGH);
  }
  sensor_vl53lx_sat.InitSensor(0x52);
  measurement_state = 0;
}