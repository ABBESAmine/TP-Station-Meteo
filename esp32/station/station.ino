#include <WiFi.h>
#include <PubSubClient.h>

// ---------------- CONFIG ----------------
#define BUTTON 18
#define LED_C  26
#define LED_F  27

const char* WIFI_SSID     = "Wifiamine";
const char* WIFI_PASSWORD = "Amine2004";

const char* MQTT_SERVER   = "captain.dev0.pandor.cloud";
const int   MQTT_PORT     = 1884;
const char* MQTT_TOPIC    = "goat/data";

const unsigned long PUBLISH_INTERVAL_MS = 5000;
const unsigned long DEBOUNCE_MS        = 50;
const unsigned long MQTT_RETRY_MS      = 5000;

// ---------------- VARIABLES ----------------
WiFiClient espClient;
PubSubClient mqttClient(espClient);

bool modeCelsius = true;

int lastButtonReading = HIGH;
int stableButtonState = HIGH;
unsigned long lastButtonChange = 0;

unsigned long lastPublish = 0;
unsigned long lastMqttTry  = 0;

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED_C, OUTPUT);
  pinMode(LED_F, OUTPUT);

  modeCelsius = true;
  updateLEDs();

  Serial.println("=== Station Meteo (simulation) ===");
  randomSeed(esp_random());

  connectWiFi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setBufferSize(512);

  tryMqttConnect();
  if (mqttClient.connected()) publishData();
}

// ---------------- LOOP ----------------
void loop() {
  handleButton();
  updateLEDs();

  if (!mqttClient.connected()) tryMqttConnect();
  mqttClient.loop();

  if (millis() - lastPublish >= PUBLISH_INTERVAL_MS) {
    lastPublish = millis();
    publishData();
  }
}

// ---------------- WIFI ----------------
void connectWiFi() {
  Serial.print("WiFi ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int n = 0;
  while (WiFi.status() != WL_CONNECTED && n < 24) {
    delay(500);
    Serial.print(".");
    n++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" OK");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" timeout");
  }
}

// ---------------- MQTT (non bloquant) ----------------
void tryMqttConnect() {
  if (mqttClient.connected()) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (lastMqttTry != 0 && (millis() - lastMqttTry < MQTT_RETRY_MS)) return;

  lastMqttTry = millis();
  Serial.print("MQTT ");
  String id = "esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  if (mqttClient.connect(id.c_str())) {
    Serial.println(" OK");
  } else {
    Serial.print(" fail ");
    Serial.println(mqttClient.state());
  }
}

// ---------------- BOUTON : un seul toggle par appui ----------------
void handleButton() {
  int r = digitalRead(BUTTON);

  if (r != lastButtonReading) {
    lastButtonChange = millis();
    lastButtonReading = r;
  }

  if (millis() - lastButtonChange < DEBOUNCE_MS) return;

  if (stableButtonState == r) return;
  stableButtonState = r;

  if (stableButtonState == LOW) {
    modeCelsius = !modeCelsius;
    Serial.println(modeCelsius ? "Mode: C" : "Mode: F");
    if (mqttClient.connected()) publishData();
  }
}

void updateLEDs() {
  digitalWrite(LED_C, modeCelsius ? HIGH : LOW);
  digitalWrite(LED_F, modeCelsius ? LOW : HIGH);
}

// ---------------- PUBLISH (format doc/example.json, simulation) ----------------
void publishData() {
  if (!mqttClient.connected()) return;

  float tempC = random(180, 301) / 10.0f;
  float hum   = random(300, 701) / 10.0f;
  float tempF = tempC * 9.0f / 5.0f + 32.0f;
  unsigned long ts = millis() / 1000;

  String json = "{";
  json += "\"temperature\":{\"celsius\":" + String(tempC, 1) + ",\"fahrenheit\":" + String(tempF, 1) + "},";
  json += "\"humidity\":" + String(hum, 1) + ",";
  json += "\"unit\":\"" + String(modeCelsius ? "C" : "F") + "\",";
  json += "\"simulation\":true,";
  json += "\"timestamp\":" + String(ts) + "}";

  mqttClient.publish(MQTT_TOPIC, json.c_str());
  Serial.print("MQTT ");
  Serial.println(json);
}
