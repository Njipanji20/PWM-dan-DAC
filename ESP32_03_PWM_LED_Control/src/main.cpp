#include <Arduino.h>
#include <DHT.h>

#define DHTPIN 10           // DHT22 data pin (sudah benar!)
#define DHTTYPE DHT22
#define LED_PIN 8           // LED untuk warning
#define POTENTIOMETER_PIN 9 // Potensio untuk kontrol threshold

DHT dht(DHTPIN, DHTTYPE);

float tempThreshold = 30.0;
unsigned long lastBlinkTime = 0;
bool ledState = false;
float currentTemp = 0;
int readFails = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n\n=== DHT22 Temperature Monitor dengan LED Warning ===\n");
    
    pinMode(LED_PIN, OUTPUT);
    pinMode(POTENTIOMETER_PIN, INPUT);
    pinMode(DHTPIN, INPUT_PULLUP);  // Pull-up internal pada data pin
    
    Serial.println("Initializingn DHT22 pada GPIO 10...");
    dht.begin();
    delay(2000);
    
    // Test pembacaan pertama
    float testTemp = dht.readTemperature();
    if(isnan(testTemp)) {
        Serial.println("⚠️ DHT belum terbaca, tunggu sebentar...");
    } else {
        Serial.printf("✓ DHT siap! Suhu awal: %.1f°C\n", testTemp);
    }
    Serial.println("\nThreshold bisa diatur dengan potensio\n");
}

void loop() {
    // Baca suhu dari DHT22
    currentTemp = dht.readTemperature();
    
    // Cek apakah pembacaan valid
    if(isnan(currentTemp)) {
        readFails++;
        Serial.printf("ERROR: DHT22 fail (Count: %d) | ", readFails);
    } else {
        if(readFails > 0) {
            Serial.printf("✓ DHT OK setelah gagal %d kali | ", readFails);
        }
        readFails = 0;
    }
    
    // Baca potensiometer untuk kontrol threshold (25-35°C)
    int potValue = analogRead(POTENTIOMETER_PIN);
    tempThreshold = map(potValue, 0, 4095, 25, 35) / 10.0;
    
    if(!isnan(currentTemp)) {
        Serial.printf("Suhu: %.1f°C | Threshold: %.1f°C | ADC: %4d | ", 
                      currentTemp, tempThreshold, potValue);
        
        // LED warning jika suhu > threshold
        if(currentTemp > tempThreshold) {
            // Blink LED
            if(millis() - lastBlinkTime >= 500) {
                ledState = !ledState;
                digitalWrite(LED_PIN, ledState);
                lastBlinkTime = millis();
                Serial.print("⚠️ WARNING ALERT!");
            }
        } else {
            // Matikan LED
            digitalWrite(LED_PIN, LOW);
            Serial.print("✓ Aman");
        }
    }
    
    Serial.println();
    delay(1000);  // DHT butuh minimal 2 detik antar pembacaan
}