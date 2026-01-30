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
// Génère une température réaliste
float fakeTemperature() {
  return random(180, 300) / 10.0; // 18.0 à 30.0
}

// Génère une humidité réaliste
float fakeHumidity() {
  return random(300, 700) / 10.0; // 30% à 70%
}

/* ========== LED ========== */
// Met à jour les LEDs selon l’unité
void updateLEDs() {
  digitalWrite(LED_C, isCelsius ? HIGH : LOW);
  digitalWrite(LED_F, isCelsius ? LOW : HIGH);
}

/* ========== SETUP ========== */
void setup() {
  // Initialisation Serial pour voir les valeurs
  Serial.begin(115200);
  Serial.println("=== MODE SIMULATION DEMARRE ===");

  // Configuration des pins
  pinMode(PINBUTTON, INPUT_PULLUP);
  pinMode(LED_C, OUTPUT);
  pinMode(LED_F, OUTPUT);

  updateLEDs(); // LED initiale
}

/* ========== LOOP ========== */
void loop() {

  /* ---- BOUTON AVEC ANTI-REBOUND ---- */
  if (digitalRead(PINBUTTON) == LOW) {
    if (millis() - lastButtonTime > debounceDelay) {
      isCelsius = !isCelsius;   // bascule unité
      updateLEDs();
      lastButtonTime = millis();
    }
  }

  /* ---- AFFICHAGE DES DONNEES SIMULEES ---- */
  if (millis() - lastPrint > 3000) {

    float temperature = fakeTemperature();
    float humidity = fakeHumidity();

    // Conversion en Fahrenheit si besoin
    if (!isCelsius) {
      temperature = temperature * 9 / 5 + 32;
    }

    // Affichage local
    Serial.print("Température : ");
    Serial.print(temperature);
    Serial.print(isCelsius ? " °C" : " °F");
    Serial.print(" | Humidité : ");
    Serial.print(humidity);
    Serial.println(" %");

    lastPrint = millis();
  }
}
