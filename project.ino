#include <Wire.h> // библиотека для управления устройствами по I2C 
#include <LiquidCrystal_I2C.h>


#define MOTOR_EN 11	// Enable пин для мотора
#define MOTOR_IN1 10 // Input 1 для мотора
#define MOTOR_IN2 9 // Input 2 для мотора

#define WATER_LEVEL_LED 12 // светодиод, уведомляющий о низком уровне воды 
#define WATER_LEVEL_SENSOR A1 // датчик уровня воды 

#define LOW_SALT_LED 6 // светодиод, уведомляющий о низкой концентрации солей 
#define HIGH_SALT_LED 7 // светодиод, уведомляющий о высокой концентрации солей
#define SALT_SENSOR A2 // датчик концентрации солей

#define LAMP 13 // лампа
#define LAMP_BUTTON 4 // кнопка перключения режимов

#define T_SENSOR A0 // датчик температуры
#define V_EN 3 // вентилятор
#define V_IN4 2 // вентилятор
#define V_IN3 1 // вентилятор
#define WARMLAMP 8 // нагреватель



LiquidCrystal_I2C LCD(0x27,16,2);  // дисплей
const unsigned int onTimeMode1 = 2000; // время в мс когда лампа включена (режим 1)
const unsigned int offTimeMode1 = 2000; // время в мс когда лампа выключена (режим 1)
const unsigned int onTimeMode2 = 5000; // время в мс когда лампа включена (режим 2)
const unsigned int offTimeMode2 = 1000; // время в мс когда лампа выключена (режим 2)
unsigned long lampStartTime = 0; // точка осчета в мс
int isLampOn = 0; // 0 - лампа выключена, 1 - лампа включена 


void setup() 
{
  Serial.begin(9600);
  
  // МОТОР
  pinMode(MOTOR_EN, OUTPUT);
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  
  // ДИСПЛЕЙ
  LCD.init();                     
  LCD.backlight();// включение подсветку дисплея
  
  // МОДУЛЬ ОТСЛЕЖИВАНИЯ УРОВНЯ ВОДЫ
  pinMode(WATER_LEVEL_LED, OUTPUT);
  
  // МОДУЛЬ ОТСЛЕЖИВАНИЯ УРОВНЯ СОЛЕЙ
  pinMode(LOW_SALT_LED, OUTPUT);
  pinMode(HIGH_SALT_LED, OUTPUT);
  
  // МОДУЛЬ ОСВЕЩЕНИЯ
  pinMode(LAMP, OUTPUT);
  
  //Serial.println("Mode: 1");
  //Serial.print("Lamp on duration (const): ");
  //Serial.println(onTimeMode1);
  //Serial.print("Lamp off duration (const): ");
  //Serial.println(offTimeMode1);
  //Serial.println("Mode: 2");
  //Serial.print("Lamp on duration (const): ");
  //Serial.println(onTimeMode2);
  //Serial.print("Lamp off duration (const): ");
  //Serial.println(offTimeMode2);
  
  // ВЕНТИЛЯТОР
  pinMode(V_EN, OUTPUT);
  pinMode(V_IN3, OUTPUT);
  pinMode(V_IN4, OUTPUT);
  // НАГРЕВАТЕЛЬ
  pinMode(WARMLAMP, OUTPUT);

}


void loop() 
{ 
  motorManage();
  waterLevelTracking();
  saltMonitoring();
  lampManage();
  termControl();
  //delay(100);
}


// УПРАВЛЕНИЕ МОТОРОМ
void motorManage() {
  int motorSpeed = 1; // скорость вращения мотора [0; 255]
  // установка направления вращения по часовой стрелке
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  // установка выбранной скорости вращения
  analogWrite(MOTOR_EN, motorSpeed);
}

// МОДУЛЬ ОТСЛЕЖИВАНИЯ УРОВНЯ ВОДЫ
void waterLevelTracking() {  
  // считывание напряжения датчика в переменную water_level
  // микроконтроллер преобразовывает значение напряжения в число от 0 до 1023
  int waterLevel = analogRead(WATER_LEVEL_SENSOR);
  if (waterLevel < 500) {
    analogWrite(WATER_LEVEL_LED, 255);
  } else {
    analogWrite(WATER_LEVEL_LED, 0);
  }
  // вывод данных в консоль
  Serial.println(waterLevel);
}

// МОДУЛЬ ОТСЛЕЖИВАНИЯ КОНЦЕНТРАЦИИ СОЛЕЙ
void saltMonitoring() {
  
  // считывание данных с датчика концентрации солей (число от 0 до 1023)
  int valueSensor = analogRead(SALT_SENSOR);
  // перевод данных с датчика в напряжение
  float voltageSensor = valueSensor * 5 / 1024.0;
  // перевод напряжения в концентрацию
  int tdsSensor = (133.42 * pow(voltageSensor, 3) - 255.86 * pow(voltageSensor, 2) + 857.39 * voltageSensor) * 0.5;
  
  // вывод данных на дисплей
  LCD.setCursor(0, 0);
  LCD.print("TDS=");
  LCD.print(tdsSensor);
  LCD.print("ppm ");
  
  // уведомление о значении, выходящем за границы нормы
  if (tdsSensor < 350) {
    analogWrite(LOW_SALT_LED, 255);
    analogWrite(HIGH_SALT_LED, 0);
    LCD.setCursor(0, 1);
    LCD.print("below normal!");
  } else if (tdsSensor > 900) {
    analogWrite(HIGH_SALT_LED, 255);
    analogWrite(LOW_SALT_LED, 0);
    LCD.setCursor(0, 1);
    LCD.print("above normal!");
  } else {
    analogWrite(LOW_SALT_LED, 0);
    analogWrite(HIGH_SALT_LED, 0);
    LCD.setCursor(0, 1);
    LCD.print("                ");    
  }
  
  // вывод данных в консоль
  //Serial.println(tdsSensor);
  //delay(1000);
}

// МОДУЛЬ ОСВЕЩЕНИЯ
// считывание состояния переключателя и передача данных, 
// соответствующих выбранному режиму
void lampManage() {
  int buttonState = digitalRead(LAMP_BUTTON);
  if (buttonState == 0) {
    lampStart(onTimeMode1, offTimeMode1);
  } else if (buttonState == 1) {
    lampStart(onTimeMode2, offTimeMode2);
  }
}

// включение и выключение лампы по заданным периодам
void lampStart(int onTime, int offTime) {
  if (lampStartTime == 0) {
    digitalWrite(LAMP, HIGH);
    isLampOn = 1;
    // сохранение времени в мс, когда лампа включилась
    lampStartTime = millis();
    //Serial.println("Lamp on");
  }
  // текущее время в мс
  unsigned long currentTime = millis();
  // время в мс прошедшее с включения/выключения лампы
  unsigned long pastTime = (currentTime - lampStartTime);
  
  if (isLampOn == 1 && pastTime >= onTime) {
    //Serial.print("Lamp on duration (actual) = ");
    //Serial.println(pastTime);
    digitalWrite(LAMP, LOW);
    isLampOn = 0;
    // сохранение времени в мс, когда лампа выключилась
    lampStartTime = millis();
    //Serial.println("Lamp off");
  } else if (isLampOn == 0 && pastTime >= offTime) {
    //Serial.print("Lamp off duration (actual)= ");
    //Serial.println(pastTime);
    lampStartTime = 0;
  }
}

void termControl() {
  int readValue = analogRead(T_SENSOR);
  float voltage = readValue * 5 / 1024.0;
  int t = (voltage - 0.5) * 100 ;
  
  LCD.setCursor(11, 0);
  LCD.print("t=");
  LCD.print(t);
  LCD.print("C");

  if(t > 25) {
    // включение вентилятора
    int vSpeed = 15; 
    digitalWrite(V_IN3, HIGH);
    digitalWrite(V_IN4, LOW);
    analogWrite(V_EN, vSpeed);
    // выключение нагревателя
    digitalWrite(WARMLAMP, LOW);
  } else if(t < 16) {
    // включение нагревателя
    digitalWrite(WARMLAMP, HIGH);
    // выключение вентилятора
    analogWrite(V_EN, 0);
  } else {
    // выключение вентилятора
    analogWrite(V_EN, 0);
    // выключение нагревателя
    digitalWrite(WARMLAMP, LOW);
  }
}















