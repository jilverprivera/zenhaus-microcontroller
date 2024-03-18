#include <Arduino.h>
#include <WiFi.h>
#include <ESP32_Supabase.h>
#include <Espalexa.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <esp_task_wdt.h>
#include <arduino-timer.h>

#define wifi_ssid "******" // replace with your wifi credentials.
#define wifi_password "******" // replace with your wifi credentials.
#define supabase_url "******" // replace with your supabase credentials.
#define supabase_anon_api_key "******" // replace with your supabase credentials.
#define supabase_user_email "******" // replace with your supabase credentials.
#define supabase_user_password "******" // replace with your supabase credentials.

#define dht1_type DHT22
#define dht1_pin 14

Supabase db;
Espalexa espalexa;
DHT dht1(dht1_pin, dht1_type);

auto timer = timer_create_default();
TaskHandle_t Task1;

const int sensors_number = 2;
String name[sensors_number] = { "Temperatura", "Humedad relativa" };
String key[sensors_number] = { "temperature", "relative_humidity" };
String unit[sensors_number] = { "°C", "%" };
float value[sensors_number];

int db_write_delay = 60000;
int db_read_delay = 5000;
boolean connect_wifi();
boolean wifi_connected = false;

void setup() {
  xTaskCreatePinnedToCore(
    alexa_loop,
    "Task_1",
    10000,
    NULL,
    2,
    &Task1,
    0);
  Serial.begin(115200);
  // Connecting to Wi-Fi
  wifi_connected = connect_wifi();
  if (wifi_connected) {
    Serial.println("[Wifi]: Connected!");
    Serial.println("[Supabase]: Initializing.");
    // Beginning Supabase connection
    db.begin(supabase_url, supabase_anon_api_key);
    Serial.println("[Supabase]: Succesfully connected!");
    // Logging in with your account you made in Supabase
    db.login_email(supabase_user_email, supabase_user_password);
    Serial.println("[Supabase]: Succesfully authenticated!");
    // Beginning Alexa connection
    espalexa.begin();
    Serial.println("[Alexa]: Succesfully connected!");
  } else {
    Serial.println("Cannot connect to Wifi, check credentials and reset ESP32.");
  }
  // Initializing DHT sensor
  dht1.begin();
  // write sensor data in supabase database every db_write_delay seconds, default 60 seconds.
  timer.every(db_write_delay, data_send);
}

void loop() {
  timer.tick();
}

bool data_send(void *argument) {
  value[0] = dht1.readTemperature();
  value[1] = dht1.readHumidity();
  String httpReqData = "";
  StaticJsonDocument<1024> doc;
  // doc["controller_uuid"] = ; // Replace with controller_uuid generated in the zenhaus app.
  JsonArray data = doc.createNestedArray("data");
  for (int i = 0; i < sensors_number; i++) {
    JsonObject sensor = data.createNestedObject();
    sensor["name"] = name[i];
    sensor["key"] = key[i];
    sensor["unit"] = unit[i];
    if (isnan(value[i])) {
      sensor["value"] = 0;
    } else {
      sensor["value"] = value[i];
    }
  }
  serializeJson(doc, httpReqData);
  int code = db.insert("data", httpReqData, false);
  Serial.println(code);
  if (code != 201) {
    db_write_delay = 2000;
  } else {
    db_write_delay = 60000;
  }
  return true;
}

void alexa_loop(void *parameter) {
  while (true) {
    espalexa.loop();
    delay(1);
  }
}

boolean connect_wifi() {
  boolean isConnected = true;
  int iteration = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  Serial.println("[Wifi]: Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    if (iteration > 30) {
      isConnected = false;
      break;
    }
    iteration++;
    delay(2000);
  }
  return isConnected;
}
