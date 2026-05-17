#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#include <Preferences.h>

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
// LOCATION
// =====================================================
const char* Location = "Kollam";

// =====================================================
// SERVER SETTINGS
// =====================================================
const char* SERVER_URL  = "https://YOUR-SERVER-NAME/process";
const char* ESP_API_KEY = "XXXXXXXXXXXXXXXXXXXXXXXX";

const char* Lang = "English";

// =====================================================
// SEND INTERVAL (12 HOURS)
// =====================================================
const unsigned long SEND_INTERVAL =
1000UL * 60UL * 60UL * 12UL;

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
// PREFERENCES STORAGE
// =====================================================
Preferences preferences;

// =====================================================
// GLOBALS
// =====================================================
unsigned long lastSent = 0;

uint16_t r, g, b, c;
uint8_t proximity = 0;

// =====================================================
// STORE OFFLINE EVENT
// =====================================================
void storeOfflineEvent(String eventData) {

  int count = preferences.getInt("count", 0);

  String key = "event" + String(count);

  preferences.putString(key.c_str(), eventData);

  preferences.putInt("count", count + 1);

  Serial.println("OFFLINE EVENT STORED");

  // OLED
  display.clearDisplay();

  display.setCursor(0, 0);

  display.println("NO INTERNET");

  display.println("");

  display.println("EVENT STORED");

  display.display();

  delay(2000);
}

// =====================================================
// WIFI CONNECT
// =====================================================
void connectWiFi() {

  Serial.println("WIFI CONNECTING...");

  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(WHITE);

  display.setCursor(0, 0);

  display.println("WIFI");
  display.println("CONNECTING...");

  display.display();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

#ifdef ESP32
  WiFi.setSleep(false);
#endif

  int retry = 0;

  while (WiFi.status() != WL_CONNECTED && retry < 20) {

    delay(500);

    Serial.print(".");

    display.print(".");

    display.display();

    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("\nWiFi Connected");

    Serial.println(WiFi.localIP());

    display.clearDisplay();

    display.setCursor(0, 0);

    display.println("WIFI CONNECTED");

    display.println("");

    display.print("IP:");

    display.println(WiFi.localIP());

    display.display();

    delay(2000);

  } else {

    Serial.println("\nWIFI FAILED");

    display.clearDisplay();

    display.setCursor(0, 0);

    display.println("WIFI FAILED");

    display.display();

    delay(2000);
  }
}

// =====================================================
// SEND STORED EVENTS
// =====================================================
void sendStoredEvents() {

  int count = preferences.getInt("count", 0);

  if (count == 0) {
    return;
  }

  Serial.println("SENDING STORED EVENTS");

  display.clearDisplay();

  display.setCursor(0, 0);

  display.println("UPLOADING");
  display.println("OFFLINE DATA");

  display.display();

  for (int i = 0; i < count; i++) {

    String key = "event" + String(i);

    String data = preferences.getString(
      key.c_str(),
      ""
    );

    if (data.length() > 0) {

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

      https.addHeader(
        "Content-Type",
        "text/plain"
      );

      https.addHeader(
        "X-ESP-Key",
        ESP_API_KEY
      );

      https.POST(data);

      https.end();

      preferences.remove(key.c_str());

      delay(1000);
    }
  }

  preferences.putInt("count", 0);

  Serial.println("ALL STORED EVENTS SENT");

  display.clearDisplay();

  display.setCursor(0, 0);

  display.println("OFFLINE DATA");

  display.println("SYNCED");

  display.display();

  delay(2000);
}

// =====================================================
// SEND TO SERVER
// =====================================================
void sendToServer(String payload) {

  if (WiFi.status() != WL_CONNECTED) {

    storeOfflineEvent(payload);

    return;
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

  https.addHeader(
    "Content-Type",
    "text/plain"
  );

  https.addHeader(
    "X-ESP-Key",
    ESP_API_KEY
  );

  Serial.println("\n[POST]");
  Serial.println(payload);

  int httpCode = https.POST(payload);

  if (httpCode > 0) {

    Serial.printf(
      "[HTTP] %d\n",
      httpCode
    );

    String response = https.getString();

    Serial.println("[SERVER]");

    Serial.println(response);

  } else {

    Serial.printf(
      "[HTTP ERROR] %s\n",
      https.errorToString(httpCode).c_str()
    );
  }

  https.end();
}

// =====================================================
// BUILD AI MESSAGE
// =====================================================
String buildLogicString(
  float temperature,
  float pressure,
  uint16_t light,
  uint8_t proximity
) {

  String msg = "";

  msg += "Farm report from ";

  msg += String(Location);

  msg += ". ";

  msg += "Temperature: ";

  msg += String(temperature);

  msg += " C. ";

  msg += "Pressure: ";

  msg += String(pressure / 100.0);

  msg += " hPa. ";

  msg += "Light level: ";

  msg += String(light);

  msg += ". ";

  msg += "Proximity: ";

  msg += String(proximity);

  msg += ". ";

  if (proximity > 50 || light < 20) {

    msg += "Possible animal intrusion detected. ";
  }

  msg += "Fetch local weather also. ";

  msg += "Reply in ";

  msg += String(Lang);

  msg += " under 500 words.";

  return msg;
}

// =====================================================
// SETUP
// =====================================================
void setup() {

  Serial.begin(115200);

  Wire.begin();

  Wire.setClock(100000);

  preferences.begin("khetgpt", false);

  // =================================================
  // OLED START
  // =================================================
  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C
      )) {

    Serial.println("OLED FAIL");

    while (1);
  }

  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(WHITE);

  display.setCursor(0, 0);

  display.println("KhetGPT");

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
  // SEND OFFLINE EVENTS
  // =================================================
  if (WiFi.status() == WL_CONNECTED) {

    sendStoredEvents();
  }

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
  float temperature =
    bmp.readTemperature();

  float pressure =
    bmp.readPressure();

  uint16_t light = 0;

  if (apds.colorDataReady()) {

    apds.getColorData(
      &r,
      &g,
      &b,
      &c
    );

    light = c;
  }

  apds.readProximity(proximity);

  // =================================================
  // SERIAL MONITOR
  // =================================================
  Serial.println(
    "========== SENSOR DATA =========="
  );

  Serial.print("Temperature: ");

  Serial.print(temperature);

  Serial.println(" C");

  Serial.print("Pressure: ");

  Serial.print(pressure / 100.0);

  Serial.println(" hPa");

  Serial.print("Light: ");

  Serial.println(light);

  Serial.print("Proximity: ");

  Serial.println(proximity);

  Serial.println();

  // =================================================
  // OLED DISPLAY
  // =================================================
  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.println("KhetGPT");

  display.println("----------------");

  display.print("Temp:");

  display.print(temperature);

  display.println(" C");

  display.print("Pres:");

  display.print(pressure / 100.0);

  display.println("hPa");

  display.print("Light:");

  display.println(light);

  display.print("Prox :");

  display.println(proximity);

  if (WiFi.status() == WL_CONNECTED) {

    display.println("WiFi: OK");

  } else {

    display.println("WiFi: OFF");
  }

  display.display();

  // =================================================
  // SEND EVERY 12 HOURS
  // =================================================
  if (now - lastSent >= SEND_INTERVAL) {

    lastSent = now;

    String logicString =
      buildLogicString(
        temperature,
        pressure,
        light,
        proximity
      );

    sendToServer(logicString);
  }

  // =================================================
  // TRY WIFI RECONNECT
  // =================================================
  if (WiFi.status() != WL_CONNECTED) {

    connectWiFi();

    if (WiFi.status() == WL_CONNECTED) {

      sendStoredEvents();
    }
  }

  delay(1000);
}
