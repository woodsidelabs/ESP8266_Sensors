// The default pins are defined in variants/nodemcu/pins_arduino.h as SDA=4 and SCL=5, but those are not pins number but GPIO number, 
// so since the pins are D1=5 and D2=4. - See more at: http://www.esp8266.com/viewtopic.php?f=13&t=10374#sthash.orDbUped.dpuf
#include <Arduino.h>
// Expose Espressif SDK functionality - wrapped in ifdef so that it still
// compiles on other platforms
#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

#include "Adafruit_Si7021.h"
#include "Adafruit_TCS34725.h"

/* Initialise with specific int time and gain values */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);
uint16_t lux = 0, colorTemp = 0;

// setup the Si7021 T/H sensor
Adafruit_Si7021 sensor = Adafruit_Si7021();
float tempC, humPct;

// setup the LED and PIR
String pirStatus = "off", colors = "";
int ledPin = BUILTIN_LED;
int pirPin = D5;
int ticks = 90, prevPir = 0;

// setup WiFi
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
ESP8266WiFiMulti WiFiMulti;
const char* ssid     = "your_ssid";
const char* password = "your_psk";

// setup device info
const char* host = "esp1";
const char* version = "0.010";
// this is the URL, user, password to send updated firmware
const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "admin";

// setup callback to HA over HTTPS
String haHost = "your.dns.name";
int haPort = 443; // default HTTPS port
String haEndpoint = "/api/states/your_sensor.your_name";
String httpsFingerprint = "YOUR:KEY:SEE:BELOW";
String haPassword = "changeme";
String haFriendlyName = "your_friendly_name";
String haServerStatus = "no connections yet";

// getting the fingerprint (thumbprint): (abstracted from thread at https://github.com/esp8266/Arduino/issues/2556)
// verify SSL
//curl -kvI https://192.168.1.194:8123
// get certificate
//openssl s_client -connect 192.168.1.194:8123
// write certificate to file
//nano cert.pem
// generate fingerprint
//openssl x509 -noout -in cert.pem -fingerprint -sha1

// root page: show a message with version info and some status
void handleRoot() {
  String msg = "hello from esp8266! Code v";
  msg.concat(version);
  msg.concat(", hostname = ");
  msg.concat(host);
  msg.concat(", last connection to HA server = ");
  msg.concat(haServerStatus);
  httpServer.send(200, "text/plain", msg);
}

void handleStatus() {
  String json = "{ \"status\" : \"alive\", \"temperature\": \"";
  json += tempC;
  json += "\", \"humidity\": \"";
  json += humPct;
  json += "\", \"lux\": \"";
  json += lux;
  json += "\", \"motion\": \"";
  json += pirStatus;
  json += "\", \"color\": \"";
  json += colors;
  json += "\" }";
  httpServer.send(200, "application/json", json);
}

void handleStatusT() {
  String json = "{ \"status\" : \"alive\", \"temperature\": \"";
  json += tempC;
  json += "\" }";
  httpServer.send(200, "application/json", json);
}

void handleStatusH() {
  String json = "{ \"status\" : \"alive\", \"humidity\": \"";
  json += humPct;
  json += "\" }";
  httpServer.send(200, "application/json", json);
}

void handleStatusL() {
  String json = "{ \"status\" : \"alive\", \"lux\": \"";
  json += lux;
  json += "\" }";
  httpServer.send(200, "application/json", json);
}

void handleStatusCT() {
  String json = "{ \"status\" : \"alive\", \"colorTemperature\": \"";
  json += colorTemp;
  json += "\" }";
  httpServer.send(200, "application/json", json);
}

void handleStatusC() {
  String json = "{ \"status\" : \"alive\", \"colors\": \"";
  json += colors;
  json += "\" }";
  httpServer.send(200, "application/json", json);
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += httpServer.uri();
  message += "\nMethod: ";
  message += (httpServer.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += httpServer.args();
  message += "\n";
  for (uint8_t i=0; i<httpServer.args(); i++){
    message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
  }
  httpServer.send(404, "text/plain", message);
}

void setup() {
  Serial.begin(9600);
  delay(5000);
  
  WiFi.hostname(host);
  WiFiMulti.addAP(ssid, password);

  if(WiFiMulti.run() == WL_CONNECTED) {
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
    
  if (MDNS.begin(host)) {
    //Serial.println("MDNS responder started");
  }

  httpServer.on("/", handleRoot);
  httpServer.on("/status", handleStatus);
  httpServer.on("/statusT", handleStatusT);
  httpServer.on("/statusH", handleStatusH);
  httpServer.on("/statusL", handleStatusL);
  httpServer.on("/statusC", handleStatusC);
  httpServer.on("/statusCT", handleStatusCT);
  httpServer.onNotFound(handleNotFound);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, 1);  
  pinMode(pirPin, INPUT);

  sensor.begin();

  getColors();
}

void getColors() {
  uint16_t r, g, b, c;
  
  tcs.getRawData(&r, &g, &b, &c);
  colorTemp = tcs.calculateColorTemperature(r, g, b);
  lux = tcs.calculateLux(r, g, b);
  
  String message = "Color Temp: "; 
  message += colorTemp; 
  message += " K, Lux: "; 
  message += lux; 
  message += ", R: "; 
  message += r; 
  message += ", G: "; 
  message += g; 
  message += ", B: "; 
  message += b; 
  message += ", C: "; 
  message += c; 
  colors = message;
}

void pirPost() {
  // Python version showing payload
  // response = requests.post(
  // 'http://hostname:8123/api/states/binary_sensor.back_door_pir',
  // headers={'x-ha-access': 'password', 'content-type': 'application/json'},
  // data=json.dumps({'state': value, 'attributes': {'friendly_name': 'Back Door PIR', 'device_class': 'motion'}}))

  if((WiFiMulti.run() == WL_CONNECTED)) {
    String data = String("");
    if (digitalRead(pirPin)) {
      prevPir = 1;
      data = String("{\"state\": \"on\", \"attributes\": {\"friendly_name\": \""+haFriendlyName+"\", \"device_class\": \"motion\"}}");
      pirStatus = "on";
      digitalWrite(ledPin, 0);  
    } else {
      prevPir = 0;
      data = String("{\"state\": \"off\", \"attributes\": {\"friendly_name\": \""+haFriendlyName+"\", \"device_class\": \"motion\"}}");
      pirStatus = "off";
      digitalWrite(ledPin, 1);  
    }
    HTTPClient http;
    http.begin(haHost, haPort, haEndpoint, httpsFingerprint);
    http.addHeader("x-ha-access", haPassword);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(data);
    if (httpCode > 0) {
      haServerStatus = "HTTP POST OK";
    } else {            
      haServerStatus = "HTTP POST Error (";
      haServerStatus.concat(httpCode);
      haServerStatus.concat("): ");
      haServerStatus.concat(http.errorToString(httpCode).c_str());
    }
    String payload = http.getString();
    http.end();
  }
}

void loop() {
  httpServer.handleClient();
  if (ticks > 60) {
    tempC = sensor.readTemperature();
    humPct = sensor.readHumidity();
    getColors(); 
    ticks = 0;
  } else {
    ticks++;
  }
  if (digitalRead(pirPin) != prevPir) {
    pirPost();
  }
  delay(1000);
}
