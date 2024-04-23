#include "WiFi.h"
#include <WiFiClient.h>
#include <WiFiAP.h>
 
const char* ssid = "my_ssid";
const char* password = "my_pass";

// Motor A
int motor1Pin1 = 27; 
int motor1Pin2 = 26; 
int enable1Pin = 14; 
// Motor B
int motor1Pin3 = 5; 
int motor1Pin4 = 17; 
int enable1Pin2 = 16;

// Setting PWM properties
const int freq = 30000;
const int pwmChannel = 0;
const int pwmChannel2 = 1;
const int resolution = 8;
int dutyCycle = 200;

WiFiServer wifiServer(80);
WiFiClient client;

void setup() {
 
  // sets the pins as outputs:
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  // sets the pins as outputs:
  pinMode(motor1Pin3, OUTPUT);
  pinMode(motor1Pin4, OUTPUT);
  pinMode(enable1Pin2, OUTPUT);
  
  // configure LED PWM functionalitites
  ledcSetup(pwmChannel, freq, resolution);
  // configure LED PWM functionalitites
  ledcSetup(pwmChannel2, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(enable1Pin, pwmChannel);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(enable1Pin2, pwmChannel2);

  Serial.begin(115200);

  // testing
  //Serial.print("Testing DC Motor...");
 
  delay(1000);

 /*
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());
  */
  
  Serial.println();
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  
  wifiServer.begin();

  Serial.println("Server started");
  //startConn();
  
}

void runLeft(int i){
  if(i == 5){
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, LOW);
  } else if (i > 5){
    Serial.println("value: " + i);
    digitalWrite(motor1Pin1, HIGH);
    digitalWrite(motor1Pin2, LOW);
    ledcWrite(pwmChannel, (i-5)*13 + 200);
  } else {
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, HIGH);
    ledcWrite(pwmChannel, 55/i + 200);
  }
}

void runRight(int i){
  if(i == 5){
    digitalWrite(motor1Pin3, LOW);
    digitalWrite(motor1Pin4, LOW);
  } else if (i > 5){
    digitalWrite(motor1Pin3, HIGH);
    digitalWrite(motor1Pin4, LOW);
    ledcWrite(pwmChannel2, (i-5)*13 + 200);
  } else {
    digitalWrite(motor1Pin3, LOW);
    digitalWrite(motor1Pin4, HIGH);
    ledcWrite(pwmChannel2, 55/i + 200);
  }
}

void runEngine(int left, int right){
  Serial.println(left + right);
  runLeft(left);
  runRight(right);
}
 
void loop() {
 
  client = wifiServer.available();

  if (client) {
 
    while (client.connected()) {
      int arr[2];
      int i = 0;
      while (client.available()>0) {
        char c = client.read();
        client.write(c);
        Serial.println(c);
        if(i < 2)
          arr[i] = c - '0';
        if(i == 1)
          runEngine(arr[0], arr[1]);
        i++;
      }
      
      delay(10);
    }
 
    client.stop();
    Serial.println("Client disconnected");
 
  }
}
