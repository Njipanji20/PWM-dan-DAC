#include <Arduino.h>

// ==================== KONFIGURASI PIN ====================
#define LED_PIN   18   // GPIO18 - PWM untuk LED
#define POT_PIN   34   // GPIO34 - Input analog potensiometer (ADC1_CH6)

// ==================== KONFIGURASI PWM LED ====================
// Untuk LED lebih nyaman pakai frekuensi lebih tinggi daripada servo.
#define LEDC_CHANNEL 0
#define PWM_FREQ  5000
#define PWM_RESOLUTION 12  // 0..4095 (lebih halus)

static constexpr int PWM_MAX = (1 << PWM_RESOLUTION) - 1;

// ==================== VARIABEL GLOBAL ====================
int lastPercent = -1;

// Simple Moving Average Filter (3 sample)
#define FILTER_SIZE 3
int potSamples[FILTER_SIZE] = {0, 0, 0};
int sampleIndex = 0;

// ==================== FUNGSI HELPER ====================
int readPotFiltered() {
    potSamples[sampleIndex] = analogRead(POT_PIN);
    sampleIndex = (sampleIndex + 1) % FILTER_SIZE;

    int sum = 0;
    for (int i = 0; i < FILTER_SIZE; i++) {
        sum += potSamples[i];
    }
    return sum / FILTER_SIZE;
}

void printStatus(int potValue, int duty, int percent) {
    Serial.print("Potensiometer: ");
    Serial.print(potValue);
    Serial.print(" | Duty: ");
    Serial.print(duty);
    Serial.print("/");
    Serial.print(PWM_MAX);
    Serial.print(" | Kecerahan: ");
    Serial.print(percent);
    Serial.println("%");
}

void setup() {
    // Inisialisasi Serial Monitor
    Serial.begin(115200);
    delay(1000);

    // Konfigurasi ADC agar pembacaan potensiometer lebih konsisten
    // (range 0..4095 untuk 0..3.3V)
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    
    // Cetak header informasi
    Serial.println("\n========================================");
    Serial.println("Program: Kontrol Kecerahan LED");
    Serial.println("Kontrol: Potensiometer");
    Serial.println("========================================\n");

    // Konfigurasi PWM untuk LED
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        // Versi Arduino ESP32 >= 3.0.0 (New API)
        ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);
    #else
        // Versi Arduino ESP32 < 3.0.0 (Old API)
        ledcSetup(LEDC_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
        ledcAttachPin(LED_PIN, LEDC_CHANNEL);
    #endif
    
    delay(500);

    // Mulai dari LED mati
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWrite(LED_PIN, 0);
    #else
        ledcWrite(LEDC_CHANNEL, 0);
    #endif

    Serial.println("Putar potensiometer untuk mengubah kecerahan LED\n");
    Serial.println("Potensiometer | Duty | Kecerahan");
    Serial.println("-------------|------|----------");
}

void loop() {
    // Baca nilai potensiometer dengan filter (untuk mengurangi noise)
    int potValue = readPotFiltered();

    // Guard kalau ada pembacaan aneh (mis. floating / wiring lepas)
    if (potValue < 0) potValue = 0;
    if (potValue > 4095) potValue = 4095;

    // Mapping nilai potensiometer ke duty PWM LED
    int duty = map(potValue, 0, 4095, 0, PWM_MAX);
    int percent = map(duty, 0, PWM_MAX, 0, 100);

    // Kirim PWM ke LED
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWrite(LED_PIN, duty);
    #else
        ledcWrite(LEDC_CHANNEL, duty);
    #endif
    
    // Tampilkan status hanya jika kecerahan berubah (threshold 1%)
    // atau setiap 500ms
    static unsigned long lastPrintTime = 0;
    unsigned long currentTime = millis();

    if (abs(lastPercent - percent) >= 1 || (currentTime - lastPrintTime) > 500) {
        printStatus(potValue, duty, percent);
        lastPercent = percent;
        lastPrintTime = currentTime;
    }
    
    delay(20);  // Update setiap 20ms (lebih sering untuk pergerakan smooth)
}
