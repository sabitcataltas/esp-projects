#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include "ClosedCube_HDC1080.h"
#include "EasyBuzzer.h"
#include <stdlib.h>

unsigned int frequency = 1000;
unsigned int beeps = 5;
unsigned int duration = 1000;
int buzzerPin = 33;
bool isWarned = false;

char *SUUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
char *CUUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
BLEServer* ps = NULL;
BLECharacteristic* pc = NULL;

float sensor_volt_mq2; 
float RS_air_mq2; //  Get the value of RS via in a clear air
float R0_mq2;  // Get the value of R0 via in Alcohol
float sensorValue_mq2;
int mq2pin = 34;
int mq2ppm;

float sensor_volt_mq7; 
float RS_air_mq7; //  Get the value of RS via in a clear air
float R0_mq7;  // Get the value of R0 via in Alcohol
float sensorValue_mq7;
int mq7pin = 35;
int mq7ppm;

int sensorValue_mq135;

float LPGCurve[3]  =  {2.3,0.21,-0.47};
float COCurve[3]  =  {1.7,0.20,-0.66};

int temperature;
int humidity;

Adafruit_SH1106 display(21, 22);
ClosedCube_HDC1080 hdc1080;

void showAlarm(void);
void readSensors(void);
long ratioToPpm(float rs_ro_ratio, float *pcurve, bool isMQ2);
void calibrateSensors(void);
void heatUpSensors(void);

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* ps) {
      BLEDevice::startAdvertising();
    };
    void onDisconnect(BLEServer* ps) {
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pc) {
      std::string value = pc->getValue();
      
      if (value.length() > 0) {
        Serial.print("New value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);

        Serial.println("");
      }
    }

    void onRead(BLECharacteristic *pc) {
      std::string value = pc->getValue();
      
      if (value.length() > 0) {
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);
      }
    }
};

/* create a hardware timer */
hw_timer_t * timer = NULL;

void IRAM_ATTR onTimer(){
  Serial.println("Done!");
  digitalWrite(33, LOW);
}

void done() {
  Serial.println("Done!");
  digitalWrite(33, LOW);
}


void setup(){  
  Serial.begin(115200);
  
  EasyBuzzer.setPin(buzzerPin);
  
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  
  hdc1080.begin(0x40);

  BLEDevice::init("AirQua");
  ps = BLEDevice::createServer();
  ps->setCallbacks(new MyServerCallbacks());
  BLEService *pService = ps->createService(SUUID);
  pc = pService->createCharacteristic(
                      CUUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  pc->setCallbacks(new MyCallbacks());
  pc->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SUUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

  delay(20);
 
  display.clearDisplay();
  display.setTextSize(1.50);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Initializing...");
  display.display();
  
  heatUpSensors();
  calibrateSensors();

  /*timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);*/
}
char buffer[8];
char buffer1[8];
char buffer2[8];
char buffer3[8];
char buffer4[8];
char buff[44];
int idx;
int mytimer = 0;

void alarmSet(){
  if(isWarned){
    mytimer++;
  }
  if(mytimer >= 5){
    mytimer = 0;
    isWarned = false;    
  }
  //Serial.println("Done!");
  digitalWrite(33, LOW);
}

char *voltageToQua(){
  if(sensorValue_mq135 < 120){
    return "Good";
  } else if(sensorValue_mq135 < 800){
    return "Normal";
  } else {
    return "Bad";
  }
}

void loop(){

  alarmSet();
  EasyBuzzer.update();
  readSensors();
  testscrolltext();
  
  buff[0]= '\0';
  
  itoa (temperature,buffer,10);
  itoa (humidity,buffer1,10);
  itoa (mq2ppm,buffer2,10);
  itoa (mq7ppm,buffer3,10);

  strcat(buff, buffer);
  strcat(buff, "#");
  strcat(buff, buffer1);
  strcat(buff, "#");
  strcat(buff, buffer2);
  strcat(buff, "#");
  strcat(buff, buffer3);
  strcat(buff, "#");
  strcat(buff, voltageToQua());
  
  Serial.println(buff);
  pc->setValue((buff));
  pc->notify();

  showAlarm();
  
  delay(1000);
}

void heatUpSensors(){
  Serial.println("Sensors Heating Up!");
  hdc1080.heatUp(20);
}

void calibrateSensors(){
  for(int x = 0 ; x < 100 ; x++){
    sensorValue_mq2 = sensorValue_mq2 + analogRead(mq2pin);
    sensorValue_mq7 = sensorValue_mq7 + analogRead(mq7pin);
    delay(100);
  }
  sensorValue_mq2 = sensorValue_mq2/100.0;
  sensor_volt_mq2 = 0.01;//sensorValue_mq2/4095*5.0; //esp32 has 12 bit ADC
  RS_air_mq2 = (5.0-sensor_volt_mq2)/sensor_volt_mq2; // omit *RL
  sensorValue_mq7 = sensorValue_mq7/100.0;
  sensor_volt_mq7 = 0.03;//sensorValue_mq7/4095*5.0; //esp32 has 12 bit ADC
  RS_air_mq7 = (5.0-sensor_volt_mq7)/sensor_volt_mq7; // omit *RL
  
  R0_mq2 = RS_air_mq2/9.8; // The ratio of RS/R0 is 10 in a clear air (MQ2)
  R0_mq7 = RS_air_mq7/28.0; // The ratio of RS/R0 is 28 in a clear air (MQ7)
  
  Serial.print("sensor_volt_mq2 = ");
  Serial.print(sensor_volt_mq2);
  Serial.println("V_mq2");
  Serial.print("sensor_volt_mq7 = ");
  Serial.print(sensor_volt_mq7);
  Serial.println("V_mq7");
 
  Serial.print("R0_mq2 = ");
  Serial.println(R0_mq2);
  Serial.print("R0_mq7 = ");
  Serial.println(R0_mq7);
  //delay(1000);
}

void readSensors(){

  sensorValue_mq135 = analogRead(32);
  Serial.print("sensorValue_mq135 = ");
  Serial.println(sensorValue_mq135);
  
  temperature = hdc1080.readTemperature();
  Serial.println("temp :" + (String)temperature + " C");
  humidity = hdc1080.readHumidity();
  Serial.println("humd: " + (String)humidity + " %");
  
  float sensor_volt_mq2, sensor_volt_mq7 ;
  float RS_gas_mq2, RS_gas_mq7 ; // Get value of RS in a GAS
  float mq2ratio, mq7ratio; // Get ratio RS_GAS/RS_air
  int mq2sensorValue = analogRead(mq2pin);
  int mq7sensorValue = analogRead(mq7pin);
  
  sensor_volt_mq2=(float)mq2sensorValue/4095*5.0;
  RS_gas_mq2 = (5.0-sensor_volt_mq2)/sensor_volt_mq2; // omit *RL
  sensor_volt_mq7=(float)mq7sensorValue/4095*5.0;
  RS_gas_mq7 = (5.0-sensor_volt_mq7)/sensor_volt_mq7; // omit *RL
 
  mq2ratio = RS_gas_mq2/R0_mq2;  // ratio = RS/R0 
  mq7ratio = RS_gas_mq7/R0_mq7;  // ratio = RS/R0

  mq2ppm = ratioToPpm(mq2ratio , LPGCurve, true);
  mq7ppm = ratioToPpm(mq7ratio , COCurve, false);
  
  Serial.print("sensor_volt_mq2 = ");
  Serial.println(sensor_volt_mq2);
  Serial.print("RS_ratio_mq2 = ");
  Serial.println(RS_gas_mq2);
  Serial.print("Rs/R0_mq2 = ");
  Serial.println(mq2ratio);
  Serial.print("ppmmq2 = ");
  Serial.println(mq2ppm);

  Serial.print("sensor_volt_mq7 = ");
  Serial.println(sensor_volt_mq7);
  Serial.print("RS_ratio_mq7 = ");
  Serial.println(RS_gas_mq7);
  Serial.print("Rs/R0_mq7 = ");
  Serial.println(mq7ratio);
  Serial.print("ppmmq7 = ");
  Serial.println(mq7ppm);
  
  //Serial.print("\n");
  
}

void testscrolltext() {
  Serial.println("Screen printing...");
  display.clearDisplay();
  display.setTextSize(2);
  
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Temp:" + (String)temperature +"C");  //°
  //display.println(temperature); 
  display.setCursor(0,16);
  display.println("Air:" + (String)voltageToQua() );
  //display.println(humidity);
  display.setCursor(0,32);
  display.println("Humd:" + (String)humidity + "%");
  //
  display.setTextSize(1);
  display.setCursor(0,48);
  display.println("CO:" + (String)mq7ppm + " ppm");
  display.setCursor(0,56);
  display.println("LPG:" + (String)mq2ppm + " ppm");
  display.display();
 
}

long ratioToPpm(float rs_ro_ratio, float *pcurve, bool isMQ2){
  int ppm = (pow(10,( ((log(rs_ro_ratio)-pcurve[1])/pcurve[2]) + pcurve[0])));

  if(isMQ2){
    if(ppm > 10000)
      ppm = 10000;
  } else {
    if(ppm > 4000)
      ppm = 4000;
  }
  return ppm;
}


void showAlarm(){
  if(isWarned)
    return;
  if(sensorValue_mq135 > 800){
   // EasyBuzzer.beep(frequency, beeps);
   EasyBuzzer.singleBeep(
      frequency,  // Frequency in hertz(HZ).  
      duration   // Duration of the beep in milliseconds(ms). 
    );
    isWarned = true;
  }/* else if(mq7ppm>300){
    //EasyBuzzer.beep(frequency, beeps);
    EasyBuzzer.singleBeep(
    frequency,  // Frequency in hertz(HZ).  
    duration   // Duration of the beep in milliseconds(ms).
    //done
  );
    isWarned = true;
  }*/
}
