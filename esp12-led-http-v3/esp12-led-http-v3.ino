#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>

#define LED_PIN     3
#define NUM_LEDS    12
#define BRIGHTNESS  255
#define LED_TYPE    WS2812B
#define COLOR_ORDER RGB
#define UPDATES_PER_SECOND 500
CRGB leds[NUM_LEDS];

#ifndef STASSID
#define STASSID "my_ssid"
#define STAPSK  "my_pass"
#endif

// Configuration for fallback access point 
// if Wi-Fi connection fails.
const char * AP_ssid = "ESP12_fallback_AP";
const char * AP_password = "123456789";
IPAddress AP_IP = IPAddress(10,1,1,1);
IPAddress AP_subnet = IPAddress(255,255,255,0);

ESP8266WebServer server(80);
char ssid[20] = "your_ssid";  // your network SSID (name)
char pass[20] = "your_pass";  // your network password
const char* myssid = STASSID;    
const char* mypass = STAPSK; 
bool WiFiAP = false;      // Do yo want the ESP as AP?
int tryTime = 10 * UPDATES_PER_SECOND;
int brightness = BRIGHTNESS;
int r = 239;
int g = 235;
int b = 216;

uint addr = 0;
struct WifiConf {
  char savedssid[50];
  char savedpass[50];
  int R = 100;
  int G = 0;
  int B = 0;
  int bright = 100;
  // Make sure that there is a 0 
  // that terminatnes the c string
  // if memory is not initalized yet.
  char cstr_terminator = 0; // makse sure
};
WifiConf data;

void setup() {

  delay(1000); // 3 second delay for recovery

  Serial.begin(115200);

  EEPROM.begin(512);
  readWifiConf();

  //saveGateway();
  //saveColor();

  
  //EEPROM.get(addr, data);
  Serial.println("Old values are: " + String(data.savedssid) + ", " + String(data.savedpass) + ", " + String(data.bright)
                 + ", " + String(data.R) + ", " + String(data.G) + ", " + String(data.B));
  /*strcpy(ssid, data.savedssid);
  strcpy(pass, data.savedpass);
  r = data.R;
  g = data.G;
  b = data.B;
  brightness = data.bright;
 */
   if (!connectToWiFi()) {
    setUpAccessPoint();
  }

  setUpLed();

  // Start WiFi
  /*if (WiFiAP) {
    startWiFiAP();
  } else {
    startWiFiClient();
    if (WiFi.status() != WL_CONNECTED) {
      startWiFiAP();
    }
  }*/

  setUpServer();
  setUpOTA();

}

void setUpLed() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);//.setCorrection( TypicalLEDStrip );
  fill_solid(leds, NUM_LEDS, CRGB(data.R, data.G, data.B));
 
  FastLED.setBrightness( data.bright );
  FastLED.show();
}

bool connectToWiFi() {
  Serial.printf("Connecting to '%s'\n", data.savedssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(data.savedssid, data.savedpass);
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("Connection Failed!");
    return false;
  }
}

void setUpAccessPoint() {
    Serial.println("Setting up access point.");
    Serial.printf("SSID: %s\n", AP_ssid);
    Serial.printf("Password: %s\n", AP_password);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(AP_IP, AP_IP, AP_subnet);
    if (WiFi.softAP(AP_ssid, AP_password)) {
      Serial.print("Ready. Access point IP: ");
      Serial.println(WiFi.softAPIP());
    } else {
      Serial.println("Setting up access point failed!");
    }
}

void setUpServer() {

  server.on("/test", handleRoot);
  server.on("/", handleWebServerRequest);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

}

void handleRoot() {
  String brhtV = server.arg("brht"); 
  int brht = brhtV.toInt();
  String rV = server.arg("r"); 
  int red = rV.toInt();
  String gV = server.arg("g"); 
  int green = gV.toInt();
  String bV = server.arg("b"); 
  int blue = bV.toInt();

  Serial.println("Data " + brhtV + " - "+ rV+ " - " + gV+ " - " + bV);

  fill_solid(leds, NUM_LEDS, CRGB(red, green, blue));
  FastLED.setBrightness( brht );
  
  //digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from esp8266!");
  //digitalWrite(led, 0);
  FastLED.show();
}

void handleWebServerRequest() {
  bool save = false;

  if (server.hasArg("ssid") && server.hasArg("password")) {
    server.arg("ssid").toCharArray(
      data.savedssid,
      sizeof(data.savedssid));
    server.arg("password").toCharArray(
      data.savedpass,
      sizeof(data.savedpass));

    Serial.println(server.arg("ssid"));
    Serial.println(data.savedssid);

    writeWifiConf();
    save = true;
  }

  String message = "";
  message += "<!DOCTYPE html>";
  message += "<html>";
  message += "<head>";
  message += "<title>ESP8266 conf</title>";
  message += "</head>";
  message += "<body>";
  if (save) {
    message += "<div>Saved! Rebooting...</div>";
  } else {
    message += "<h1>Wi-Fi conf</h1>";
    message += "<form action='/' method='POST'>";
    message += "<div>SSID:</div>";
    message += "<div><input type='text' name='ssid' value='" + String(data.savedssid) + "'/></div>";
    message += "<div>Password:</div>";
    message += "<div><input type='password' name='password' value='" + String(data.savedpass) + "'/></div>";
    message += "<div><input type='submit' value='Save'/></div>";
    message += "</form>";
  }
  message += "</body>";
  message += "</html>";
  server.send(200, "text/html", message);

  if (save) {
    Serial.println("Wi-Fi conf saved. Rebooting...");
    delay(1000);
    ESP.restart();
  }
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

void readWifiConf() {
  // Read wifi conf from flash
  for (int i=0; i<sizeof(data); i++) {
    ((char *)(&data))[i] = char(EEPROM.read(i));
  }
  // Make sure that there is a 0 
  // that terminatnes the c string
  // if memory is not initalized yet.
  data.cstr_terminator = 0;
}

void writeWifiConf() {
  for (int i=0; i<sizeof(data); i++) {
    EEPROM.write(i, ((char *)(&data))[i]);
  }
  EEPROM.commit();
}

void setUpOTA() {

  // Change OTA port. 
  // Default: 8266
  // ArduinoOTA.setPort(8266);

  // Change the name of how it is going to 
  // show up in Arduino IDE.
  // Default: esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // Re-programming passowrd. 
  // No password by default.
  // ArduinoOTA.setPassword("123");

  ArduinoOTA.begin();
}

void loop() {

  // Give processing time for ArduinoOTA.
  // This must be called regularly
  // for the Over-The-Air upload to work.
  ArduinoOTA.handle();

  server.handleClient();
}

/*
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
*/
