#define ECHO 2
#define TRIG 3
#define sensVCC 4

#define dispGND 9
#define dispVCC 10

#define buttPin 11
#define buttGND 12

#include <LCDI2C_Multilingual.h>

// Настройки фильтра
#define FILTER_SIZE 5  // размер фильтра (рекомендуется нечетное: 3, 5, 7)
float filterBuffer[FILTER_SIZE];
byte filterIndex = 0;

float getDist(uint8_t trig, uint8_t echo) {
  // импульс 10 мкс
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  // измеряем время ответного импульса
  uint32_t us = pulseIn(echo, HIGH, 30000); // таймаут 30 мс (≈5 метров)

  // если нет эха, возвращаем 0
  if (us == 0) return 0.0;

  // считаем расстояние
  float distance = us / 58.2;
  
  // Ограничиваем максимальное расстояние (например, 400 см)
  if (distance > 400.0) distance = 400.0;
  
  return distance;
}

// Функция медианного фильтра
float medianFilter(float newValue) {
  // Добавляем новое значение в буфер
  filterBuffer[filterIndex] = newValue;
  filterIndex = (filterIndex + 1) % FILTER_SIZE;
  
  // Копируем буфер для сортировки
  float tempBuffer[FILTER_SIZE];
  for (byte i = 0; i < FILTER_SIZE; i++) {
    tempBuffer[i] = filterBuffer[i];
  }
  
  // Сортируем пузырьком (для небольших массивов)
  for (byte i = 0; i < FILTER_SIZE - 1; i++) {
    for (byte j = i + 1; j < FILTER_SIZE; j++) {
      if (tempBuffer[i] > tempBuffer[j]) {
        float temp = tempBuffer[i];
        tempBuffer[i] = tempBuffer[j];
        tempBuffer[j] = temp;
      }
    }
  }
  
  // Возвращаем медиану (средний элемент)
  return tempBuffer[FILTER_SIZE / 2];
}

// Функция скользящего среднего (альтернатива медианному)
float movingAverage(float newValue) {
  static float avgBuffer[FILTER_SIZE];
  static byte avgIndex = 0;
  static float sum = 0;
  
  // Вычитаем самое старое значение
  sum -= avgBuffer[avgIndex];
  
  // Добавляем новое значение
  avgBuffer[avgIndex] = newValue;
  sum += newValue;
  
  // Обновляем индекс
  avgIndex = (avgIndex + 1) % FILTER_SIZE;
  
  // Возвращаем среднее
  return sum / FILTER_SIZE;
}

LCDI2C_Russian lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(sensVCC, OUTPUT);
  pinMode(dispGND, OUTPUT);
  pinMode(dispVCC, OUTPUT);

  pinMode(buttPin, INPUT_PULLUP);
  pinMode(buttGND, OUTPUT);

  digitalWrite(sensVCC, HIGH);
  digitalWrite(dispGND, LOW);
  digitalWrite(dispVCC, HIGH);
  digitalWrite(buttGND, LOW);
  
  // Инициализируем буфер фильтра нулями
  for (byte i = 0; i < FILTER_SIZE; i++) {
    filterBuffer[i] = 0.0;
  }
  
  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print("Init");
  delay(1000);
} 

void loop() {
  // Получаем сырое значение
  float rawDist = getDist(TRIG, ECHO);
  
  // Фильтруем значение
  float filteredDist = medianFilter(rawDist);
  // Или используйте скользящее среднее:
  // float filteredDist = movingAverage(rawDist);
  
  // Корректируем по кнопке
  if (!digitalRead(buttPin)) {
    filteredDist += 12.5;
  }
  
  // Округляем до 2 знаков после запятой
  filteredDist = round(filteredDist * 100) / 100.0;
  
  // Очищаем экран
  lcd.clear();
  
  // Выводим заголовок
  lcd.setCursor(0, 0);
  lcd.print("Distance:");
  
  // Выводим отфильтрованное значение
  lcd.setCursor(0, 1);
  
  // Если расстояние 0 или очень маленькое
  if (filteredDist < 0.1) {
    lcd.print("No object ");
  } else {
    // Форматированный вывод с 1 знаком после запятой
    lcd.print(filteredDist, 1);
    lcd.print(" cm");
  }
  
  // Опционально: вывод в Serial для отладки
  Serial.print("Raw: ");
  Serial.print(rawDist, 1);
  Serial.print(" cm, Filtered: ");
  Serial.print(filteredDist, 1);
  Serial.println(" cm");
  
  delay(750); // задержка между измерениями
}
