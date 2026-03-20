#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "driver/ledc.h"

#define PWM_PIN_A 2
#define PWM_PIN_B 3

#define RGB_LED_PIN 10
#define NUM_PIXELS 1

Adafruit_NeoPixel pixels(NUM_PIXELS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

unsigned long flashDuration = 500;

const ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE;
const ledc_timer_t timer_num = LEDC_TIMER_0;

uint32_t current_freq = 1000;

unsigned long lastTick = 0;
// bool ledState = false;
float filling = 0.5;  // filling PWM
ledc_timer_bit_t resolution;
uint32_t max_duty = (1 << resolution);
// uint32_t duty_val = max_duty / 2;  // 50% заполнение
uint32_t duty_val = filling * ((1 << resolution) - 1);

void update_ledc_config(uint32_t freq) {
  // Выбираем разрешение в зависимости от целевой частоты
  if (freq < 500) {
    resolution = LEDC_TIMER_13_BIT;  // Для низких частот
  } else if (freq < 50000) {
    resolution = LEDC_TIMER_10_BIT;  // Средний диапазон
  } else {
    resolution = LEDC_TIMER_7_BIT;  // Для высоких частот (до 500кГц+)
  }

  ledc_timer_config_t ledc_timer = {
      .speed_mode = speed_mode,
      .duty_resolution = resolution,
      .timer_num = timer_num,
      .freq_hz = freq,
      .clk_cfg = LEDC_AUTO_CLK};

  if (ledc_timer_config(&ledc_timer) != ESP_OK) {
    Serial.println("!!! Ошибка: Частота недоступна для железа !!!");
    return;
  }

  // Канал A
  ledc_channel_config_t ch_a = {
      .gpio_num = PWM_PIN_A,
      .speed_mode = speed_mode,
      .channel = LEDC_CHANNEL_0,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = timer_num,
      .duty = duty_val,
      .hpoint = 0};
  ledc_channel_config(&ch_a);

  // Канал B (инверсия фазы через hpoint)
  ledc_channel_config_t ch_b = {
      .gpio_num = PWM_PIN_B,
      .speed_mode = speed_mode,
      .channel = LEDC_CHANNEL_1,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = timer_num,
      .duty = duty_val,
      .hpoint = (int)duty_val};
  ledc_channel_config(&ch_b);

  Serial.printf("Mode: %d bit | Duty: %u %% | Freq: %u Hz\n", (int)resolution, (uint32_t)(filling * 100), freq);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  unsigned long start = millis();
  while (!Serial && (millis() - start < 5000)) {
    delay(100);
  }
  Serial.println("System started and connected to Serial!");
  delay(5000);

  pixels.begin();
  pixels.setBrightness(5);
  pixels.setPixelColor(0, pixels.Color(0, 0, 255));
  // pixels.clear();
  pixels.show();

  Serial.println("\n========================");
  Serial.println("--- Generator for ESP32-C3 ---");
  Serial.printf("Default Frequency: %u Hz\n", current_freq);
  Serial.printf("GPIO 2,3 pin 6,7 \n");
  Serial.println("Frequency range: 10 - 500 000 Hz ");
  Serial.println("========================");
  Serial.flush();

  delay(1000);
  update_ledc_config(current_freq);
}

void loop() {
  // Системное мигание
  // if (millis() - lastTick > 15000) {
  //   lastTick = millis();
  //   ledState = true;
  //   pixels.setPixelColor(0, pixels.Color(0, 0, 255));
  //   pixels.show();
  // }

  // if (ledState && (millis() - lastTick > flashDuration)) {
  //   ledState = false;
  //   pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  //   pixels.show();
  // }

  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() < 1) return;

    // 1. Команда перезагрузки
    if (input == "r") {
      Serial.println("Soft reboot...");
      ESP.restart();
    } else if (input == "h") {
      Serial.println("Available commands:");
      Serial.println("f:XXX - Set frequency (10-500000 Hz)");
      Serial.println("d:X.XX - Set duty cycle (0.0-1.0)");
      Serial.println("r - Soft reboot");
      Serial.printf("GPIO 2,3 pin 6,7 out 2 channels freq. \n");
    }

    // 2. Установка частоты (формат f:5000)
    else if (input.startsWith("f:")) {
      String valStr = input.substring(2);  // Отрезаем "f:"
      long new_freq = valStr.toInt();

      if (new_freq >= 10 && new_freq <= 500000) {
        current_freq = (uint32_t)new_freq;
        update_ledc_config(current_freq);
      } else {
        Serial.println("Error: Frequency out of range (10-500000)");
      }
    }

    // 3. Установка заполнения (формат d:0.15)
    else if (input.startsWith("d:")) {
      String valStr = input.substring(2);  // Отрезаем "d:"
      filling = valStr.toFloat();          // Преобразуем в 0.15

      if (filling >= 0.0 && filling <= 1.0) {
        max_duty = (1 << resolution) - 1;
        duty_val = filling * max_duty;
        update_ledc_config(current_freq);
        // Serial.printf("Duty set to: %.2f (Value: %u)\n", filling, duty_val);
      } else {
        Serial.println("Error: Duty must be between 0.0 and 1.0");
      }
    }

    else {
      Serial.println("Unknown command. Use h for help.");
    }
  }
}

// Logic TL494

// Adafruit_NeoPixel pixels(NUM_PIXELS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

// const ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE;
// const ledc_timer_t timer_num = LEDC_TIMER_0;

// uint32_t current_freq = 1000;
// uint32_t current_duty_pct = 45; // По умолчанию 45% (безопасно для старта)

// void update_ledc_config() {
//     ledc_timer_bit_t resolution;
//     if (current_freq < 500) resolution = LEDC_TIMER_13_BIT;
//     else if (current_freq < 50000) resolution = LEDC_TIMER_10_BIT;
//     else resolution = LEDC_TIMER_7_BIT;

//     uint32_t max_duty = (1 << resolution);
//     // Duty для каждого плеча (как в TL494)
//     uint32_t duty_val = (max_duty * current_duty_pct) / 100;
// float period_us = 1000000.0 / current_freq;

//     // 2. Считаем длительность импульса одного плеча (Duty Cycle)
//     float pulse_width_us = (period_us * current_duty_pct) / 100.0;

//     // 3. Считаем мертвое время (пауза между импульсами плеч A и B)
//     // Формула для 180-градусного сдвига: (Период / 2) - Длительность импульса
//     float dead_time_us = (period_us / 2.0) - pulse_width_us;

//     ledc_timer_config_t ledc_timer = {
//         .speed_mode = speed_mode,
//         .duty_resolution = resolution,
//         .timer_num = timer_num,
//         .freq_hz = current_freq,
//         .clk_cfg = LEDC_AUTO_CLK
//     };

//     if (ledc_timer_config(&ledc_timer) != ESP_OK) return;

//     // Плечо 1 (Выход 1 TL494)
//     ledc_channel_config_t ch_a = {
//         .gpio_num = PWM_PIN_A,
//         .speed_mode = speed_mode,
//         .channel = LEDC_CHANNEL_0,
//         .timer_sel = timer_num,
//         .duty = duty_val,
//         .hpoint = 0
//     };
//     ledc_channel_config(&ch_a);

//     // Плечо 2 (Выход 2 TL494) - сдвиг на 180 градусов
//     ledc_channel_config_t ch_b = {
//         .gpio_num = PWM_PIN_B,
//         .speed_mode = speed_mode,
//         .channel = LEDC_CHANNEL_1,
//         .timer_sel = timer_num,
//         .duty = duty_val,
//         .hpoint = (int)(max_duty / 2)
//     };
//     ledc_channel_config(&ch_b);

//     // Расчет мертвого времени в процентах
//     float dead_time = 50.0 - (float)current_duty_pct;
//     Serial.printf("[TL494 Mode] F: %u Hz | Duty: %u%% | DeadTime: %.1f%% | Res: %d bit\n",
//                   current_freq, current_duty_pct, dead_time, (int)resolution);
//   Serial.println("\n----------------------------------------");
//     Serial.printf("Частота:    %u Гц\n", current_freq);
//     Serial.printf("Скважность: %u%%\n", current_duty_pct);
//     Serial.printf("Импульс:    %.2f мкс\n", pulse_width_us);
//     Serial.printf("Dead Time:  %.2f мкс\n", dead_time_us);
//     Serial.printf("Разрешение: %d бит\n", (int)resolution);
//     Serial.println("----------------------------------------");
// }

// void setup() {
//     Serial.begin(115200);
//     delay(1000);
//     pixels.begin();
//     pixels.setBrightness(10);
//     update_ledc_config();
//     Serial.println("--- Эмулятор TL494 (Push-Pull) ---");
//     Serial.println("f[число] - частота (10-500000)");
//     Serial.println("d[число] - заполнение (1-49%) ВАЖНО: >50% вызовет КЗ!");
// }

// void loop() {
//     if (Serial.available() > 0) {
//         String input = Serial.readStringUntil('\n');
//         input.trim();
//         if (input.length() < 2) return;

//         char type = input[0];
//         long val = input.substring(1).toInt();

//         if (type == 'f') {
//             if (val >= 10 && val <= 500000) {
//                 current_freq = val;
//                 update_ledc_config();
//             }
//         }
//         else if (type == 'd') {
//             // Защита: в двухтактном режиме заполнение одного плеча
//             // не может быть больше 50%, иначе будет перекрытие (Cross-conduction)
//             if (val >= 1 && val <= 49) {
//                 current_duty_pct = val;
//                 update_ledc_config();
//             } else {
//                 Serial.println("ОШИБКА: Для TL494 заполнение плеча должно быть < 50%!");
//             }
//         }
//     }
// }
// logic TL494 with dead time calculation and user input for both frequency and duty cycle.
// #include <Adafruit_NeoPixel.h>
// #include <Arduino.h>
// #include "driver/ledc.h"

// #define PWM_PIN_A 2
// #define PWM_PIN_B 3
// #define RGB_LED_PIN 10
// #define NUM_PIXELS 1
// #define MIN_DEAD_TIME_US 0.5  // Порог опасного мертвого времени в микросекундах

// Adafruit_NeoPixel pixels(NUM_PIXELS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

// const ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE;
// const ledc_timer_t timer_num = LEDC_TIMER_0;

// uint32_t current_freq = 1000;
// uint32_t current_duty_pct = 45;
// unsigned long lastTick = 0;

// void update_ledc_config() {
//     ledc_timer_bit_t resolution;
//     if (current_freq < 500) resolution = LEDC_TIMER_13_BIT;
//     else if (current_freq < 50000) resolution = LEDC_TIMER_10_BIT;
//     else resolution = LEDC_TIMER_7_BIT;

//     uint32_t max_duty = (1 << resolution);
//     uint32_t duty_val = (max_duty * current_duty_pct) / 100;

//     ledc_timer_config_t ledc_timer = {
//         .speed_mode = speed_mode,
//         .duty_resolution = resolution,
//         .timer_num = timer_num,
//         .freq_hz = current_freq,
//         .clk_cfg = LEDC_AUTO_CLK
//     };

//     if (ledc_timer_config(&ledc_timer) != ESP_OK) return;

//     // Каналы A и B (сдвиг 180 градусов как в TL494)
//     ledc_set_duty_and_update(speed_mode, LEDC_CHANNEL_0, duty_val, 0);
//     ledc_set_duty_and_update(speed_mode, LEDC_CHANNEL_1, duty_val, max_duty / 2);

//     // --- РАСЧЕТЫ ДЛЯ ДИАГНОСТИКИ ---
//     float period_us = 1000000.0 / current_freq;
//     float pulse_width_us = (period_us * current_duty_pct) / 100.0;
//     float dead_time_us = (period_us / 2.0) - pulse_width_us;

//     // --- ПРОВЕРКА БЕЗОПАСНОСТИ (LED ИНДИКАЦИЯ) ---
//     if (dead_time_us < MIN_DEAD_TIME_US) {
//         pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // КРАСНЫЙ - ОПАСНО!
//         Serial.println("!!! ВНИМАНИЕ: DEAD TIME СЛИШКОМ МАЛО !!!");
//     } else {
//         pixels.setPixelColor(0, pixels.Color(0, 255, 0)); // ЗЕЛЕНЫЙ - ОК
//     }
//     pixels.show();

//     // Вывод в Serial с использованием спецификаторов
//     Serial.println("\n---------- DIAGNOSTICS ----------");
//     Serial.printf("Frequency:  %u Hz\n", current_freq);
//     Serial.printf("Duty Cycle: %u %%\n", current_duty_pct);
//     Serial.printf("Pulse T:    %.2f us\n", pulse_width_us);
//     Serial.printf("Dead Time:  %.2f us\n", dead_time_us);
//     Serial.printf("Timer Res:  %d bit\n", (int)resolution);
//     Serial.println("---------------------------------");
// }

// void setup() {
//     Serial.begin(115200);
//     delay(1000);
//     pixels.begin();
//     pixels.setBrightness(20);

//     // Инициализация каналов перед первым запуском
//     ledc_channel_config_t ch = {.speed_mode = speed_mode, .intr_type = LEDC_INTR_DISABLE, .timer_sel = timer_num};
//     ch.channel = LEDC_CHANNEL_0; ch.gpio_num = PWM_PIN_A; ledc_channel_config(&ch);
//     ch.channel = LEDC_CHANNEL_1; ch.gpio_num = PWM_PIN_B; ledc_channel_config(&ch);

//     update_ledc_config();
// }

// void loop() {
//     if (Serial.available() > 0) {
//         String input = Serial.readStringUntil('\n');
//         input.trim();
//         if (input.length() < 2) return;

//         char type = input[0];
//         long val = input.substring(1).toInt();

//         if (type == 'f' && val >= 10 && val <= 500000) {
//             current_freq = val;
//             update_ledc_config();
//         }
//         else if (type == 'd' && val >= 1 && val <= 49) {
//             current_duty_pct = val;
//             update_ledc_config();
//         }
//     }
// }
