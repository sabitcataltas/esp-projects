#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h> //The EEPROM libray 
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#define LED_PIN     2
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

ESP8266WebServer server(80);
char ssid[20] = "your_ssid";     // your network SSID (name)
char pass[20] = "your_pass"; // your network password
const char* myssid = STASSID;     
const char* mypass = STAPSK; 
bool WiFiAP = false;      // Do yo want the ESP as AP?
int tryTime = 10 * UPDATES_PER_SECOND;
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

void handleRoot() {
  String brhtV = server.arg("brht"); 
  int brht = brhtV.toInt();
  String rV = server.arg("r"); 
  int red = rV.toInt();
  String gV = server.arg("g"); 
  int green = gV.toInt();
  String bV = server.arg("b"); 
  int blue = bV.toInt();

  Serial.println("Data " + brhtV + rV + gV + bV);

  fill_solid(leds, NUM_LEDS, CRGB(0, 0, 200));
  FastLED.setBrightness( 255 );
  FastLED.show();
  //digitalWrite(led, 1);
  //server.send(200, "text/plain", "hello from esp8266!");
  //digitalWrite(led, 0);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void startWiFiClient() {
  delay(100);

  Serial.println("Connecting to " + (String)ssid + (String)pass);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  int tryCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000 / UPDATES_PER_SECOND);
    //Serial.println(".");
    tryCount = tryCount + 1;
    if (tryCount == tryTime) {
      Serial.println("WiFi not connected");
      break;
    }

  }
  if (WiFi.status() == WL_CONNECTED) {
    saveGateway();
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: " + WiFi.localIP().toString());
  }

}

void startWiFiAP() {
  delay(100);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(myssid, mypass);
  delay(100);
  Serial.println("AP started");
  Serial.println("IP address: " + WiFi.softAPIP().toString());
}

void saveGateway() {
  strncpy(data.savedssid, ssid, 20);
  strncpy(data.savedpass, pass, 20);
  EEPROM.put(addr, data);
  EEPROM.commit();
}

void saveColor() {
  data.bright = brightness;
  data.R = r;
  data.G = g;
  data.B = b;
  EEPROM.put(addr, data);
  EEPROM.commit();
}

void changeColor(int brht, int red, int green, int blue) {


  fill_solid(leds, NUM_LEDS, CRGB(red, green, blue));
  FastLED.setBrightness( brht );
  FastLED.show();

  //saveColor();
}

void setup() {

  delay(1000); // 3 second delay for recovery

  Serial.begin(115200);
  Serial.println();
  Serial.println();

  EEPROM.begin(512);

  saveGateway();
  saveColor();

  EEPROM.get(addr, data);
  Serial.println("Old values are: " + String(data.savedssid) + ", " + String(data.savedpass) + ", " + String(data.bright)
                 + ", " + String(data.R) + ", " + String(data.G) + ", " + String(data.B));
  strcpy(ssid, data.savedssid);
  strcpy(pass, data.savedpass);
  r = data.R;
  g = data.G;
  b = data.B;
  brightness = data.bright;

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);//.setCorrection( TypicalLEDStrip );
  fill_solid(leds, NUM_LEDS, CRGB(200, 0, 0));
 
  FastLED.setBrightness( 255 );
  FastLED.show();

  // Start WiFi
  if (WiFiAP) {
    startWiFiAP();
  } else {
    startWiFiClient();
    if (WiFi.status() != WL_CONNECTED) {
      startWiFiAP();
    }
  }

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/test", handleRoot);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}



void loop() {
  server.handleClient();
  MDNS.update();
}
