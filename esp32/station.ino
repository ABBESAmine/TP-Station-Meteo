/***************************************************
 * TP STATION METEO CONNECTEE - ESP32
 * STEP 2 : MODE SIMULATION UNIQUEMENT
 *
 * Ce code NE FAIT QUE :
 * - Simuler température + humidité
 * - Gérer un bouton avec anti-rebond
 * - Allumer une LED selon l’unité
 * - Afficher les valeurs en local (Serial Monitor)
 *
 * PAS de WiFi
 * PAS de MQTT
 * PAS de DHT22
 ***************************************************/

#include <Arduino.h>

/* ========== GPIO ========== */
#define PINBUTTON 18   // Bouton
#define LED_C 26       // LED Celsius
#define LED_F 27       // LED Fahrenheit

/* ========== ETAT ========== */
bool isCelsius = true;        // unité actuelle
unsigned long lastButtonTime = 0;
const unsigned long debounceDelay = 300;
unsigned long lastPrint = 0;

/* ========== DONNEES SIMULEES ========== */
float fakeTemperature() {
  return random(180, 300) / 10.0; // 18.0 à 30.0
}

float fakeHumidity() {
  return random(300, 700) / 10.0; // 30% à 70%
}

/* ========== LED ========== */
void updateLEDs() {
  // Une seule LED allumée selon l’unité
  digitalWrite(LED_C, isCelsius ? HIGH : LOW);
  digitalWrite(LED_F, isCelsius ? LOW : HIGH);
}

/* ========== SETUP ========== */
void setup() {
  Serial.begin(115200);
  Serial.println("=== MODE SIMULATION DEMARRE ===");

  // Configuration des pins
  pinMode(PINBUTTON, INPUT_PULLUP); // bouton vers GND
  pinMode(LED_C, OUTPUT);
  pinMode(LED_F, OUTPUT);

  // LED initiale
  isCelsius = true;
  updateLEDs();
}

/* ========== LOOP PRINCIPALE ========== */
void loop() {

  /* ---- BOUTON AVEC ANTI-REBOUND ---- */
  if (digitalRead(PINBUTTON) == LOW) {
    if (millis() - lastButtonTime > debounceDelay) {
      isCelsius = !isCelsius;   // bascule unité
      updateLEDs();             // met à jour LEDs
      lastButtonTime = millis();

      // Debug
      Serial.print("Button pressed! LED_C=");
      Serial.print(isCelsius ? "ON" : "OFF");
      Serial.print(", LED_F=");
      Serial.println(!isCelsius ? "ON" : "OFF");
    }
  }

  /* ---- AFFICHAGE DES DONNEES SIMULEES ---- */
  if (millis() - lastPrint > 3000) {
    float temperature = fakeTemperature();
    float humidity = fakeHumidity();

    if (!isCelsius) temperature = temperature * 9 / 5 + 32;

    Serial.print("Température : ");
    Serial.print(temperature);
    Serial.print(isCelsius ? " °C" : " °F");
    Serial.print(" | Humidité : ");
    Serial.print(humidity);
    Serial.println(" %");

    lastPrint = millis();
  }
}
