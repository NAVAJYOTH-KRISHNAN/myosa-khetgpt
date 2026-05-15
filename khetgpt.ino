#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#include <Adafruit_BMP085.h>      // BMP180
#include <Adafruit_APDS9960.h>    // APDS9960
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiClientSecureBearSSL.h>
#else
  #include <WiFi.h>
  #include <HTTPClient.h>
  #include <WiFiClientSecure.h>
#endif

// =====================================================
// WIFI SETTINGS
// =====================================================
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// =====================================================
// SERVER SETTINGS
// =====================================================
const char* SERVER_URL  = "https://YOUR-SERVER-NAME/process"; // replace with your hosted server link
const char* ESP_API_KEY = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"; // replace with your desired entry key

const char* Lang = "English"; // Change the language if you need

// =====================================================
// INTERVAL
// =====================================================
const unsigned long SEND_INTERVAL = 600000UL; // 10 minutes

// =====================================================
// OLED SETTINGS
// =====================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// =====================================================
// SENSOR OBJECTS
// =====================================================
Adafruit_BMP085 bmp;
Adafruit_APDS9960 apds;

// =====================================================
// GLOBALS
// =====================================================
unsigned long lastSent = 0;

uint16_t r, g, b, c;
uint8_t proximity = 0;

// =====================================================
// WIFI CONNECT
// =====================================================
void connectWiFi() {

  Serial.println("WIFI CONNECTING...");

  // OLED DISPLAY
  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(WHITE);

  display.setCursor(0, 0);

  display.println("WIFI");
  display.println("CONNECTING...");

  display.display();

  // START WIFI
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

#ifdef ESP32
  WiFi.setSleep(false);
#endif

  // WAIT
  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");

    display.print(".");

    display.display();
  }

  Serial.println("\nWiFi Connected");

  Serial.println(WiFi.localIP());

  // OLED SUCCESS
  display.clearDisplay();

  display.setCursor(0, 0);

  display.println("WIFI CONNECTED");

  display.println("");

  display.print("IP:");

  display.println(WiFi.localIP());

  display.display();

  delay(2000);
}

// =====================================================
// SEND TO SERVER
// =====================================================
void sendToServer(String payload) {

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

#ifdef ESP8266

  BearSSL::WiFiClientSecure client;

  client.setInsecure();

  HTTPClient https;

  https.begin(client, SERVER_URL);

#else

  WiFiClientSecure client;

  client.setInsecure();

  HTTPClient https;

  https.begin(client, SERVER_URL);

#endif

  https.addHeader("Content-Type", "text/plain");

  https.addHeader("X-ESP-Key", ESP_API_KEY);

  Serial.println("\n[POST]");
  Serial.println(payload);

  int httpCode = https.POST(payload);

  if (httpCode > 0) {

    Serial.printf("[HTTP] %d\n", httpCode);

    String response = https.getString();

    Serial.println("[SERVER]");
    Serial.println(response);

  } else {

    Serial.printf("[HTTP ERROR] %s\n",
      https.errorToString(httpCode).c_str());
  }

  https.end();
}

// =====================================================
// BUILD AI MESSAGE
// =====================================================
String buildLogicString(float temperature,
                        float pressure,
                        uint16_t light,
                        uint8_t proximity) {

  String tempLabel;

  if      (temperature < 10) tempLabel = "very cold";
  else if (temperature < 20) tempLabel = "cold";
  else if (temperature < 30) tempLabel = "normal";
  else if (temperature < 38) tempLabel = "warm";
  else                       tempLabel = "dangerously hot";

  float pressHpa = pressure / 100.0;

  String pressLabel;

  if      (pressHpa < 990)  pressLabel = "low pressure";
  else if (pressHpa < 1013) pressLabel = "slightly low";
  else if (pressHpa < 1025) pressLabel = "normal";
  else                      pressLabel = "high pressure";

  String lightLabel;

  if      (light < 50)   lightLabel = "very dark";
  else if (light < 200)  lightLabel = "dim";
  else if (light < 1000) lightLabel = "moderate";
  else                   lightLabel = "bright daylight";

  String proximityLabel;

  if      (proximity < 10) proximityLabel = "nothing nearby";
  else if (proximity < 50) proximityLabel = "something nearby";
  else                     proximityLabel = "object very close";

  String msg = "Farm sensor report: ";

  msg += "Temperature is ";
  msg += String(temperature, 1);
  msg += " C (";
  msg += tempLabel;
  msg += "). ";

  msg += "Pressure is ";
  msg += String(pressHpa, 1);
  msg += " hPa (";
  msg += pressLabel;
  msg += "). ";

  msg += "Ambient light level is ";
  msg += String(light);
  msg += " (";
  msg += lightLabel;
  msg += "). ";

  msg += "Proximity level is ";
  msg += String(proximity);
  msg += " (";
  msg += proximityLabel;
  msg += "). ";

  if (proximity > 50) {

    msg += "Possible nearby movement or animal detected. ";
    msg += "What should the farmer do immediately?";

  } else {

    msg += "No nearby activity detected. ";
    msg += "What should the farmer monitor next?";
  }

  msg += "\nThe report should be in ";
  msg += String(Lang);
  msg += " language under 500 words.";

  return msg;
}

// =====================================================
// SETUP
// =====================================================
void setup() {

  Serial.begin(115200);

  Wire.begin();

  Wire.setClock(100000);

  // =================================================
  // OLED START
  // =================================================
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

    Serial.println("OLED FAIL");

    while (1);
  }

  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(WHITE);

  display.setCursor(0, 0);

  display.println("INNO FARM SYSTEM");
  display.println("BOOTING...");

  display.display();

  delay(2000);

  // =================================================
  // BMP180
  // =================================================
  display.clearDisplay();

  display.setCursor(0, 0);

  display.println("CHECKING BMP180");

  display.display();

  if (!bmp.begin()) {

    Serial.println("BMP180 FAIL");

    display.println("BMP FAIL");

    display.display();

    while (1);
  }

  Serial.println("BMP180 OK");

  // =================================================
  // APDS9960
  // =================================================
  display.clearDisplay();

  display.setCursor(0, 0);

  display.println("CHECKING APDS");

  display.display();

  if (!apds.begin()) {

    Serial.println("APDS9960 FAIL");

    display.println("APDS FAIL");

    display.display();

    while (1);
  }

  apds.enableColor(true);

  apds.enableProximity(true);

  Serial.println("APDS9960 OK");

  // =================================================
  // WIFI
  // =================================================
  connectWiFi();

  // =================================================
  // READY SCREEN
  // =================================================
  display.clearDisplay();

  display.setCursor(0, 0);

  display.println("SYSTEM READY");

  display.println("");

  display.println("BMP180 OK");

  display.println("APDS9960 OK");

  display.display();

  delay(2000);
}

// =====================================================
// LOOP
// =====================================================
void loop() {

  unsigned long now = millis();

  // =================================================
  // SENSOR VALUES
  // =================================================
  float temperature = bmp.readTemperature();

  float pressure = bmp.readPressure() / 100.0;

  uint16_t light = 0;

  if (apds.colorDataReady()) {

    apds.getColorData(&r, &g, &b, &c);

    light = c;
  }

  apds.readProximity(proximity);

  // =================================================
  // SERIAL MONITOR
  // =================================================
  Serial.println("========== SENSOR DATA ==========");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.println(" hPa");

  Serial.print("Light: ");
  Serial.println(light);

  Serial.print("Red: ");
  Serial.println(r);

  Serial.print("Green: ");
  Serial.println(g);

  Serial.print("Blue: ");
  Serial.println(b);

  Serial.print("Proximity: ");
  Serial.println(proximity);

  Serial.println();

  // =================================================
  // OLED DISPLAY
  // =================================================
  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.println("INNO FARM");

  display.println("----------------");

  display.print("Temp:");
  display.print(temperature);
  display.println(" C");

  display.print("Pres:");
  display.print(pressure);
  display.println("hPa");

  display.print("Light:");
  display.println(light);

  display.print("Prox :");
  display.println(proximity);

  display.display();

  // =================================================
  // SEND EVERY 10 MINUTES
  // =================================================
  if (now - lastSent >= SEND_INTERVAL) {

    lastSent = now;

    String logicString = buildLogicString(
      temperature,
      pressure,
      light,
      proximity
    );

    sendToServer(logicString);
  }

  delay(1000);
}
