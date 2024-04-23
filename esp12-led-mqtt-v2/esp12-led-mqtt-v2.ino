#include <FastLED.h>
#include <ESP8266WiFi.h>
#include "uMQTTBroker.h"
#include <EEPROM.h> //The EEPROM libray 

#define LED_PIN     3
#define NUM_LEDS    12
#define BRIGHTNESS  255
#define LED_TYPE    WS2812B
#define COLOR_ORDER RGB
#define UPDATES_PER_SECOND 500
CRGB leds[NUM_LEDS];
int pulse = 129;
int fadeAmount = 1;   
#ifndef STASSID
#define STASSID "my_ssid"
#define STAPSK  "my_pass"
#endif

char ssid[20] = "your_ssid";     // your network SSID (name)
char pass[20] = "your_pass"; // your network password
const char* myssid = STASSID;     
const char* mypass = STAPSK; 
bool WiFiAP = false;      // Do yo want the ESP as AP?
int tryTime = 10*UPDATES_PER_SECOND;
int brightness = BRIGHTNESS;
int r = 239;
int g = 235;
int b = 216;

uint addr = 0;
struct { 
    char savedssid[20] = "x";
    char savedpass[20] = "x";
    int R = 239;
    int G = 235;
    int B = 216;
    int bright = 210;
  } data;


void startWiFiClient(void);
void saveGateway(void);
void changeColor(int brht, int red, int green, int blue);

class myMQTTBroker: public uMQTTBroker
{
public:
    virtual bool onConnect(IPAddress addr, uint16_t client_count) {
      Serial.println(addr.toString()+" connected");
      return true;
    }
    
    virtual bool onAuth(String username, String password) {
      Serial.println("Username/Password: "+username+"/"+password);
      return true;
    }
    
    virtual void onData(String topic, const char *data, uint32_t length) {
      char data_str[length+1];
      os_memcpy(data_str, data, length);
      data_str[length] = '\0';
      
      Serial.println("received topic '"+topic+"' with data '"+(String)data_str+"'");
      
      int n = topic.length(); 
      char char_array[n + 1]; 
      strcpy(char_array, topic.c_str()); 

      
      
      if(strcmp(char_array,"change") == 0 ){
        if(length == 4 ){
          changeColor( ((int)data_str[0] + 127), ((int)data_str[1] + 127) , ((int)data_str[2] + 127), ((int)data_str[3] + 127));
        }
      }
      
      else if(strcmp(char_array,"connect") == 0 ){

          char newpass[20];
          char newssid[20];
          bool isSsid = true;
          int sharpidx = 0;
          for(int i = 0; i < length; i++){
            if(data_str[i] == '#'){
              isSsid = false;
              newssid[i] = '\0';
              sharpidx = i+1;
              if(i == length-1)
                newpass[0] = '\0';
            } else if(isSsid){
              newssid[i] = data_str[i];
            } else {
              newpass[i-sharpidx] = data_str[i];
              if(i == length-1)
                newpass[i-sharpidx+1] = '\0';
            }
          }
          Serial.println( "SSID: " + String(newpass)  );
          Serial.println( "PASS: " + String(newssid)  );

          if((strcmp(newssid, "") == 0)){
            Serial.println( "ssid invalid! ");
          } else 
            strncpy(ssid, newssid, 20);
          Serial.println( "SSID: " + String(ssid)  ); //printing each token
          
          
          if((strcmp(newpass, "") == 0)){
            Serial.println( "pass invalid! ");
          } else
            strncpy(pass, newpass, 20);
          Serial.println( "PASS: " + String(pass) ); //printing each token

          if((strcmp(ssid, "") == 0) || (strcmp(pass, "") == 0)){
            Serial.println( "ssid or pass invalid! ");
          } else {
            Serial.println( "ssid: " + String(ssid));
            Serial.println( "pass: " + String(pass));
            //startWiFiClient();
            saveGateway();
            ESP.restart();
          }       
         
      }
    }
};

myMQTTBroker myBroker;

void startWiFiClient()
{   
  delay(100);
  
  Serial.println("Connecting to "+(String)ssid + (String)pass);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  int tryCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000/UPDATES_PER_SECOND);
    //Serial.println(".");
    tryCount = tryCount + 1;
    if(tryCount == tryTime){
      Serial.println("WiFi not connected");
      break;  
    }
   
  }
  if(WiFi.status() == WL_CONNECTED){
    saveGateway();
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: " + WiFi.localIP().toString());
  }
  
}

void startWiFiAP()
{
  delay(100); 
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(myssid, mypass);
  delay(100); 
  Serial.println("AP started");
  Serial.println("IP address: " + WiFi.softAPIP().toString());
}

void saveGateway(){
  strncpy(data.savedssid, ssid,20);
  strncpy(data.savedpass, pass,20);
    //data.savedssid = ssid;
    //data.savedpass = pass;
    EEPROM.put(addr,data);
    EEPROM.commit(); 
}

void saveColor(){
  data.bright = brightness;
  data.R = r;
  data.G = g;
  data.B = b;
  EEPROM.put(addr,data);
  EEPROM.commit(); 
}

void changeColor(int brht, int red, int green, int blue){
  while(brht != brightness || red != r || green != g || b != blue){
    if(brightness > brht)
      brightness--;
    else if(brightness < brht)
      brightness++;

    if(r > red)
      r--;
    else if(r < red)
      r++;

    if(g > green)
      g--;
    else if(g < green)
      g++;

    if(b > blue)
      b--;
    else if(b < blue)
      b++;

    fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
    FastLED.setBrightness( brightness );  
    FastLED.show();
  }
  saveColor();
}

void setup()
{
  delay(1000); // 3 second delay for recovery
     
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  
  EEPROM.begin(512);

  saveGateway();
  saveColor();
 
  EEPROM.get(addr,data);
  Serial.println("Old values are: "+String(data.savedssid)+", "+String(data.savedpass)+", "+String(data.bright)
                  +", "+String(data.R)+", "+String(data.G)+", "+String(data.B));
  strcpy(ssid, data.savedssid);
  strcpy(pass, data.savedpass);
  r = data.R;
  g = data.G;
  b = data.B;
  brightness = data.bright;

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
  for(int i = 0; i < brightness+1; i++){
    FastLED.setBrightness( i );  
    FastLED.show();
    delay(10);//*(255/brightness)
  }
  
  // Start WiFi
  if (WiFiAP){
      startWiFiAP();
  } else {
    startWiFiClient();
    if(WiFi.status() != WL_CONNECTED){
      startWiFiAP();
    }
  }

  
  
  // Start the broker
  Serial.println("Starting MQTT broker");
  myBroker.init();

/*
 * Subscribe to anything
 */
  myBroker.subscribe("#");
}



void loop()
{
/*
 * Publish the counter value as String
 */
 /*myBroker.publish("broker/counter", (String)counter++);
   
  // wait a second
  delay(5000);*/
}
