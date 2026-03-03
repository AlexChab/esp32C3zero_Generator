#include <Adafruit_NeoPixel.h>  // Добавляем библиотеку
#include <Arduino.h>

#include "driver/ledc.h"

#define PWM_PIN_A 2
#define PWM_PIN_B 3
#define RGB_LED_PIN 10  // Пин адресного светодиода на C3 Zero
#define NUM_PIXELS 1

// Создаем объект светодиода
Adafruit_NeoPixel pixels(NUM_PIXELS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

const ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE;
const ledc_timer_t timer_num = LEDC_TIMER_0;
uint32_t current_freq = 1000;

unsigned long lastTick = 0;
bool ledState = false;

void setup_pwm(uint32_t freq) {
    ledc_timer_config_t ledc_timer = {.speed_mode = speed_mode,
                                      .duty_resolution = LEDC_TIMER_10_BIT,
                                      .timer_num = timer_num,
                                      .freq_hz = freq,
                                      .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ch_a = {.gpio_num = PWM_PIN_A,
                                  .speed_mode = speed_mode,
                                  .channel = LEDC_CHANNEL_0,
                                  .intr_type = LEDC_INTR_DISABLE,
                                  .timer_sel = timer_num,
                                  .duty = 512,
                                  .hpoint = 0};
    ledc_channel_config(&ch_a);

    ledc_channel_config_t ch_b = {.gpio_num = PWM_PIN_B,
                                  .speed_mode = speed_mode,
                                  .channel = LEDC_CHANNEL_1,
                                  .intr_type = LEDC_INTR_DISABLE,
                                  .timer_sel = timer_num,
                                  .duty = 512,
                                  .hpoint = 512};
    ledc_channel_config(&ch_b);
}

void setup() {
    Serial.begin(115200);
    
    uint32_t start_time = millis();
    while (!Serial && (millis() - start_time < 5000)) {
        delay(10);
    }

    delay(3000);

    pixels.begin();
    pixels.setBrightness(20);
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));  // Красный
    pixels.show();

    setup_pwm(current_freq);
    Serial.println("\n========================");
    Serial.println("--- Система готова! ---");
    Serial.printf("Частота: %u Гц\n", current_freq);
    Serial.printf("GPIO 2,3 pin 6,7 \n");
    Serial.println("========================");
    // Serial.println("--- Система готова! ---");
     Serial.flush(); 
}

void loop() {
    // 1. Ввод частоты
    if (Serial.available() > 0) {
        long new_freq = Serial.parseInt();
        if (new_freq >= 10 && new_freq <= 80000) {
            current_freq = (uint32_t)new_freq;
            ledc_set_freq(speed_mode, timer_num, current_freq);
            Serial.printf("\nЧастота: %u Гц\n", current_freq);
        }
        while (Serial.available() > 0) Serial.read();
    }

    // 2. Мигание адресным светодиодом (Синим цветом)
    if (millis() - lastTick > 15000) {
        lastTick = millis();
        ledState = !ledState;
        if (ledState) {
            pixels.setPixelColor(0, pixels.Color(0, 0, 255));  // Синий
        } else {
            pixels.setPixelColor(0, pixels.Color(0, 0, 0));  // Выключен
        }
        pixels.show();  // Обновить состояние светодиода
    }
}
