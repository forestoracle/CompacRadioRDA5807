/*

 MAX7219 для этого проекта считается штатным средством отображения 
  установленной частоты радио.

  CLK - 10  
  CS  - 11
  DIO - 12

  Важное замечание относительно EEPROM.
  Для компиляции используется библиотека gt8fx, которая по умолчанию выделяет для EEPROM 1 кб из Flash памяти.
  См. https://htrd.su/blog/2022/12/04/lgt8fx-lgt8328p/#lgt8fx

*/
#include <RDA5807.h>
#include "max7219.h"
#include <EEPROM.h>

//#define DEBUG

// Подключение MAX7219. На самом деле используются определения из max7219.cpp
#define MAX7219_CLK 10
#define MAX7219_CS 11
#define MAX7219_DIO 12


#define FIX_STATION 10090  // Частота по умолчанию
#define FIX_VOLUME 10      // Громкость по умолчанию

#define EEPROM_WRITE_DELAY 10000  // Отсрочка записи в EEPROM в миллисекундах

#define LOOP_DELAY 200
#define LIGTH_SENSOR_DELAY 5000

MAX7219 max7219;
RDA5807 rx;

bool eeprom_written = true;
unsigned long eeprom_written_time = 0;
unsigned long light_sensor_time = 0;
uint8_t current_brightness = 0;
/*
*  Инициализация
*/
void setup() {
#ifdef DEBUG
  Serial.begin(115200);
  Serial.println("\nInit start");
#endif
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  // Инициализация MAX7219
  max7219.Begin();
  max7219.Clear();
  //readLightSensor();
  max7219.MAX7219_SetBrightness(current_brightness);

  // Инициализация радио
  rx.setup(); 
  rx.setSeekThreshold(4); // от 0 до 15. Чем меньше, тем на более слабых станциях останавливаетс поиск
  delay(1000); // Эта задержка нужна чтобы RDA5807 могла прийти в себя после включения и быть готовой к настройке
  readEEPROM();
  //rx.setVolume(FIX_VOLUME);
  //rx.setFrequency(FIX_STATION);
#ifdef DEBUG
  Serial.println("Init finish");
#endif
  digitalWrite(LED_BUILTIN, LOW);
}  // init
/*
  Главный цикл
*/
void loop() {
  // Чтение напряжения от кнопок
  delay(LOOP_DELAY);
  uint16_t buttonValue = analogRead(A0);  // Чтение кнопок
  //Serial.println(analogRead(A1));
#ifdef DEBUG
  //Serial.println(buttonValue);
#endif

  // Анализ значения напряжения
  if (buttonValue < 20) {
    // Уменьшение громкости
    rx.setVolumeDown();
    eeprom_flag_down();
  } else if (between(buttonValue, 500, 560)) {
    // Увеличение громкости
    rx.setVolumeUp();
    eeprom_flag_down();
  } else if (between(buttonValue, 650, 700)) {
    // Увеличение частоты
    //rx.setFrequencyUp();
    rx.seek(RDA_SEEK_WRAP, RDA_SEEK_UP, showFrequency);
    delay(100);
    showFrequency();
    eeprom_flag_down();
  } else if (between(buttonValue, 750, 800)) {
    // Уменьшение частоты
    //rx.setFrequencyDown();
    rx.seek(RDA_SEEK_WRAP, RDA_SEEK_DOWN, showFrequency);
    delay(100);
    showFrequency();
    eeprom_flag_down();
  }

  if (!eeprom_written && millis() - eeprom_written_time > EEPROM_WRITE_DELAY) {
    eeprom_written = true;
    writeEEPROM();
  }
  //if (millis() - light_sensor_time > LIGTH_SENSOR_DELAY) {
  //  readLightSensor();
  //}
}

/**/
void showFrequency(void) {
  unsigned char f[5];
  uint16_t freq = rx.getFrequency() / 10;
  uint8_t start = 0;
  itoa(freq, f, 10);  // преобразуем целое в строку
  max7219.Clear();
  if (freq < 1000) { start = 1; }
  for (int i = 0; i < 4; i++) {
    max7219.DisplayChar(start, f[i], start == 2 || (start == 3 && rx.isStereo()));
    start++;
  }
}

/*
  Проверяет, что число находится в заданном интервале
*/
bool between(uint16_t val, uint16_t min, uint16_t max) {
  return (val >= min && val <= max);
}
/*
  Опускает флаг - eeprom записан
*/
void eeprom_flag_down() {
  eeprom_written = false;
  eeprom_written_time = millis();
}

/*
  Запись в EEPROM
*/
void writeEEPROM(void) {
  digitalWrite(LED_BUILTIN, HIGH);
  uint16_t frequency = rx.getFrequency();
  uint8_t volume = rx.getVolume();
  EEPROM.put(0, frequency);
  EEPROM.put(4, volume);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
}
/*
  Чтение из EEPROM
*/
void readEEPROM(void) {
  digitalWrite(LED_BUILTIN, HIGH);
  uint16_t frequency;
  uint8_t volume;
  EEPROM.get(0, frequency);
  EEPROM.get(4, volume);
  rx.setFrequency(frequency);
  rx.setVolume(volume);
  showFrequency();
  digitalWrite(LED_BUILTIN, LOW);
}
/*
void readLightSensor(vo
id) {
  light_sensor_time = millis();
  uint16_t ls = analogRead(A1)/100-1;
  if (current_brightness != ls) {
    current_brightness = constrain(ls, INTENSITY_MIN, INTENSITY_MAX-12);
    max7219.MAX7219_SetBrightness(current_brightness);
  }
#ifdef DEBUG
  Serial.println(current_brightness);
#endif
}
*/