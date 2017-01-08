/// GSM сигналка c установкой по кнопке
/// датчиком на прерывателях и движения

// донат, https://money.yandex.ru/to/410012486954705

#include <EEPROM.h>
#include "MyGSM.h"

#define debug Serial

//// НАСТРОЕЧНЫЕ КОНСТАНТЫ /////
// номера телефонов
#define TELLNUMBER "380509151369"                // номен на который будем звонить
#define SMSNUMBER "+380509151369"                     // номер на который будем отправлять SMS
#define NUMBER1_NotInContr "380509151369"             // 1-й номер для снятие с охраны (Мой МТС)
#define NUMBER2_NotInContr "380506228524"             // 2-й номер для снятие с охраны (Тони МТС)
#define NUMBER3_NotInContr "***"                      // 3-й номер для снятие с охраны
#define NUMBER4_NotInContr "***"                      // 4-й номер для снятие с охраны

#define NUMBER1_InContr    "380969405835"             // 1-й номер для установки на охраны
#define NUMBER2_InContr    "***"                      // 2-й номер для установки на охраны
#define NUMBER3_InContr    "***"                      // 3-й номер для установки на охраны
#define NUMBER4_InContr    "***"                      // 4-й номер для установки на охраны

// SMS
const String smsText_TensionCable = "ALARM: TensionCable sensor";         //текст смс для растяжки
const String smsText_PIR1         = "ALARM: PIR1 sensor";                 //текст смс для датчика движения 1
const String smsText_PIR2         = "ALARM: PIR2 sensor";                 //текст смс для датчика движения 2

// паузы
const int timeWaitingInContr = 25;            // Время паузы от нажатие кнопки до установки режима охраны
const int timeHoldingBtn = 2;                 // время удерживания кнопки для включения режима охраны  2 сек.
const int timeSiren = 20;                     // время работы сирены (секунды).
const int timeCall = 300;                     // время паузы после последнего звонка тревоги (секунды)
const int timeSmsPIR1 = 300;                  // время паузы после последнего СМС датчика движения 1 (секунды)
const int timeSmsPIR2 = 300;                  // время паузы после последнего СМС датчика движения 2 (секунды)

//Спикер
const int specerTone = 98;                    //тон спикера


//// КОНСТАНТЫ ДЛЯ ПИНОВ /////
#define SpecerPin 8
#define gsmLED 13
#define NotInContrLED 12
#define InContrLED 11
#define SirenLED 10

#define power 3                               // нога чтения типа питания (БП или батарея)
#define pinBOOT 5                             // нога BOOT или K на модеме 
#define Button 9                              // нога на кнопку
#define SirenGenerator 7                      // нога на сирену


//Sensores
#define SH1 A2                                // нога на растяжку
#define pinPIR1 6                             // нога датчика движения 1
#define pinPIR2 4                             // нога датчика движения 2

//// КОНСТАНТЫ РЕЖИМОВ РАБОТЫ //// 
const byte NotInContrMod = 1;                 // снята с охраны
const byte InContrMod = 2;                    // установлена охрана


//// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ////
byte mode = NotInContrMod;          // 1 - снята с охраны                                  
                                    // 2 - установлена охрана
                                    // при добавлении не забываем посмотреть на 75 строку
bool btnIsHolding = false;

bool inTestMod = false;                 // режим тестирования датчиков (не срабатывает сирена и не отправляются СМС)

bool isSiren = false;                   // режим сирены

long prSiren = 0;                       // время включения сирены (милисекунды)
long prCall = 0;                        // время последнего звонка тревоги (милисекунды)
long prSmsPIR1 = 0;                     // время последнего СМС датчика движения 1 (милисекунды)
long prSmsPIR2 = 0;                     // время последнего СМС датчика движения 2 (милисекунды)

bool controlTensionCable = true;        // включаем контроль растяжки

MyGSM gsm(gsmLED, pinBOOT);

void setup() 
{
  delay(1000);                                //// !! чтобы нечего не повисало при включении
  debug.begin(9600);
  pinMode(SpecerPin, OUTPUT);
  pinMode(gsmLED, OUTPUT);
  pinMode(NotInContrLED, OUTPUT);
  pinMode(InContrLED, OUTPUT);
  pinMode(SirenLED, OUTPUT);
  pinMode(pinBOOT, OUTPUT);                   /// нога BOOT на модеме
  pinMode(SH1, INPUT_PULLUP);                 /// нога на растяжку
  pinMode(pinPIR1, INPUT);                    /// нога датчика движения 1
  pinMode(pinPIR2, INPUT);                    /// нога датчика движения 2
  pinMode(Button, INPUT_PULLUP);              /// кнопка для установки режима охраны
  pinMode(SirenGenerator, OUTPUT);            /// нога на сирену
  pinMode(power, INPUT);                      /// нога чтения типа питания (БП или батарея)    

  digitalWrite(SirenGenerator, HIGH);         /// выключаем сирену через релье
                             
  gsm.Initialize();                           // Инициализация gsm модуля (включения, настройка) 
  mode = EEPROM.read(0);                      // читаем режим из еепром  
  if (mode == InContrMod) Set_InContrMod(1);          
  else Set_NotInContrMod();

  inTestMod = EEPROM.read(1);                 // читаем тестовый режим из еепром    
}

void loop() 
{  
  if (isSiren == 1)                           // если включена сирена проверяем время ее работы
  {
    if (GetElapsed(prSiren) > (timeSiren * 1000))          // если сирена работает больше установленного времени то выключаем ее
    {
      StopSiren();
    }
  }
  
  if (inTestMod == 1 && isSiren == 0)         // если включен режим тестирования и не сирена то мигаем светодиодом
  {
     digitalWrite(SirenLED, !digitalRead(SirenLED));     
     delay(200);
  }
  
  if (mode != InContrMod && ButtonIsHold(timeHoldingBtn))             // если режим не на охране и если кнопка удерживается заданое время то ставим на охрану
  {
    Set_InContrMod(1);                                                // ставим на охрану 
  }

  if (mode == InContrMod)                                             // если в режиме охраны
  {
    bool sTensionCable = SensorTriggered_TensionCable();              // проверяем датчики
    bool sPIR1 = SensorTriggered_PIR1();
    bool sPIR2 = SensorTriggered_PIR2();
                                   
    if ((sTensionCable && controlTensionCable) || sPIR1 || sPIR2)                  // если обрыв
    {                                                                 
      if (isSiren == 0) StartSiren();                                              // включаем сирену
      
      if ((GetElapsed(prCall) > (timeCall * 1000)) or prCall == 0)                 // проверяем сколько прошло времени после последнего звонка (выдерживаем паузц между звонками)
      {
        gsm.Call(String(TELLNUMBER));                                              // отзваниваемся
        prCall = millis();
      }
      
      if (sTensionCable && !inTestMod)                                             // отправляем СМС если сработал обрыв растяжки и не включен режим тестирование
        gsm.SendSMS(String(smsText_TensionCable), String(SMSNUMBER));    
      
      if (sPIR1 && !inTestMod                                                      // отправляем СМС если сработал датчик движения и не включен режим тестирование 
        && ((GetElapsed(prSmsPIR1) > (timeSmsPIR1 * 1000)) or prSmsPIR1 == 0))     // и выдержена пауза после последнего смс
      {
        gsm.SendSMS(String(smsText_PIR1), String(SMSNUMBER));
        prSmsPIR1 = millis();
      }
      
      if (sPIR2 && !inTestMod                                                      // отправляем СМС если сработал датчик движения и не включен режим тестирование  
        && ((GetElapsed(prSmsPIR2) > (timeSmsPIR2 * 1000)) or prSmsPIR2 == 0))     // и выдержена пауза после последнего смс
      {  
        gsm.SendSMS(String(smsText_PIR2), String(SMSNUMBER));
        prSmsPIR2 = millis();
      }
      
      if (sTensionCable) controlTensionCable = false;                 // отключаем контроль растяжки что б сирена не работала постоянно после разрыва растяжки
    }
  }  
 
  // ищим RING
  // если нашли, опрашиваем кто это и снимаем с охраны
  
   if (gsm.Available())                                     // если GSM модуль что-то послал нам, то
   {    
    String val = "";
    val = gsm.Read();                                       // читаем посланую gsm строку
    if (val.indexOf("RING") > -1)                           // если звонок обнаружен, то проверяем номер
    {
      if (mode == InContrMod                                // если включен режим охраны и найден зарегистрированный звонок то снимаем с охраны
          && (val.indexOf(NUMBER1_NotInContr) > -1 
              || val.indexOf(NUMBER2_NotInContr) > -1 
              || val.indexOf(NUMBER3_NotInContr) > -1 
              || val.indexOf(NUMBER4_NotInContr) > -1
             )
         )       
      {  
        //Serial.println("--- MASTER RING DETECTED ---");
        BlinkLED(gsmLED, 0, 250, 0);                        // сигнализируем об этом      
        delay(2500);                                        // небольшая пауза перед збросом звонка
        Set_NotInContrMod();                                // снимаем с охраны         
        gsm.RejectCall();                                   // сбрасываем вызов
      }
      else
      if (mode == NotInContrMod                                // если включен режим снята с охраны и найден зарегистрированный звонок то ставим на охрану
          && (val.indexOf(NUMBER1_InContr) > -1 
              || val.indexOf(NUMBER2_InContr) > -1 
              || val.indexOf(NUMBER3_InContr) > -1 
              || val.indexOf(NUMBER4_InContr) > -1
             )
         )       
      {  
        // DOTO
        //Serial.println("--- MASTER RING DETECTED ---");
        BlinkLED(gsmLED, 0, 250, 0);                      // сигнализируем об этом
        delay(7000);                                        // большая пауза перед збросом звонка
        gsm.RejectCall();                                   // сбрасываем вызов        
        Set_InContrMod(0);                                  // устанавливаем на охрану без паузы      
      }
      else gsm.RejectCall();                                // если не найден зарегистрированный звонок то сбрасываем вызов (без паузы)
      val = "";
    }
    //else Serial.println(val);                             // печатаем в монитор порта пришедшую строку         
  }      
}



//// ------------------------------- Functions --------------------------------- ////

// подсчет сколько прошло милисикунд после последнего срабатывания события (сирена, звонок и т.д.)
unsigned long GetElapsed(long prEventMillis)
{
  unsigned long tm = millis();
  return (tm >= prEventMillis) ? tm - prEventMillis : 0xFFFFFFFF - prEventMillis + tm + 1;  //возвращаем милисикунды после последнего события
}

////// Function for setting of mods ////// 
bool Set_NotInContrMod()
{
  digitalWrite(NotInContrLED, HIGH);
  digitalWrite(InContrLED, LOW);
  digitalWrite(SirenLED, LOW);
  PlayTone(specerTone, 500);
  mode = NotInContrMod;                 // снимаем охранку
  EEPROM.write(0, mode);                // пишим режим в еепром 
  return true;
}

bool Set_InContrMod(bool IsWaiting)
{
  int btnHold = 0;
  if (IsWaiting == true)                                // если включен режим ожидание перед установкой охраны выдерживаем заданную паузу что б успеть покинуть помещение
  {
    digitalWrite(NotInContrLED, LOW);   
    for(int i = 0; i < timeWaitingInContr; i++)   
    {
      if (!digitalRead(Button))
        btnHold++;
      else btnHold = 0;

      if (btnHold >= (timeWaitingInContr * 0.7) && mode == NotInContrMod )      //проверка на включение/выключение режима тестирование датчиков
      {
         inTestMod = !inTestMod;                        // включаем/выключаем режим тестирование датчиков 
         EEPROM.write(1, inTestMod);                    // пишим режим тестирование датчиков в еепром
         PlayTone(specerTone, 500);
         Set_NotInContrMod();
         return false;
      }
      
      if (i < (timeWaitingInContr * 0.7))               // первых 70% паузы моргаем медленным темпом
        BlinkLEDSpecer(InContrLED, 0, 500, 500);              
      else                                              // последних 30% паузы ускоряем темп
      {
        BlinkLEDSpecer(InContrLED, 0, 250, 250); 
        BlinkLEDSpecer(InContrLED, 0, 250, 250);              
      }
      if (ButtonIsHold(timeHoldingBtn))                 // проверяем не нажата ли кнопка и если кнопка удерживается заданое время, функция вернет true и установка на охрану прерывается     
      {
        Set_NotInContrMod();
        return false;      
      }            
    }
    
    // установка переменных в дефолтное состояние
    controlTensionCable = true;           // включаем контроль растяжки
    prCall = 0;                           // сбрвсываем переменные пауз для gsm
    prSmsPIR1 = 0;
    prSmsPIR2 = 0;
    
    delay (300);  
  }
  
  //установка на охрану                                                       
  digitalWrite(NotInContrLED, LOW);
  digitalWrite(InContrLED, HIGH);
  digitalWrite(SirenLED, LOW);  
  PlayTone(specerTone, 500);
  mode = InContrMod;                                    // ставим на охрану  
  EEPROM.write(0, mode);                                // пишим режим в еепром
  return true;
}


void  StartSiren()
{
  digitalWrite(SirenLED, HIGH);
  if (!inTestMod)                                        // если не включен тестовый режим
    digitalWrite(SirenGenerator, LOW);                  // включаем сирену через релье
  isSiren = 1;
  prSiren = millis();
}


void  StopSiren()
{
  digitalWrite(SirenLED, LOW);
  digitalWrite(SirenGenerator, HIGH);                    // выключаем сирену через релье
  isSiren = 0; 
}


bool ButtonIsHold(int timeHold)
{  
  if (digitalRead(Button)) btnIsHolding = false;                       // если кнопка не нажата сбрасываем показатеь удерживания кнопки
  if (!digitalRead(Button) && btnIsHolding == false)                   // проверяем нажата ли кнопка и отпускалась ли после предыдущего нажатия (для избежание ложного считывание кнопки)
  { 
    if (inTestMod == 1 && mode != InContrMod)                          // если включен режим тестирование на время удерживание кнопки выключаем мигание светодиода
      digitalWrite(SirenLED, LOW); 
      
    btnIsHolding = true;
    int i = 1;
    while(1) 
    {
      if (digitalRead(Button)) return false; 
      if (i == timeHold) return true;
      i++;
      delay(1000);      
    }
  }   
  return false;  
}


void PlayTone(int tone, int duration) 
{
  for (long i = 0; i < duration * 1000L; i += tone * 2) 
  {
    digitalWrite(SpecerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(SpecerPin, LOW);
    delayMicroseconds(tone);
  }
} 


////// Методы датчиков ////// 
bool SensorTriggered_TensionCable()          // растяжка
{
  if (digitalRead(SH1) == HIGH) return true;
  else return false;
}

bool SensorTriggered_PIR1()                  // датчик движения 1
{
  if (digitalRead(pinPIR1) == HIGH) return true;
  else return false;
}

bool SensorTriggered_PIR2()                  // датчик движения 2
{
  if (digitalRead(pinPIR2) == HIGH) return true;
  else return false;
}

// Блымание светодиодом
void BlinkLED(int pinLED, int millisBefore, int millisHIGH, int millisAfter)
{ 
  digitalWrite(pinLED, LOW);                          
  delay(millisBefore);  
  digitalWrite(pinLED, HIGH);                 // блымаем светодиодом
  delay(millisHIGH); 
  digitalWrite(pinLED, LOW);
  delay(millisAfter);
}

// Блымание светодиодом со спикером
void BlinkLEDSpecer(int pinLED, int millisBefore, int millisHIGH, int millisAfter)
{ 
  digitalWrite(pinLED, LOW);                          
  delay(millisBefore);  
  digitalWrite(pinLED, HIGH);                 // блымаем светодиодом
  PlayTone(specerTone, millisHIGH);
  digitalWrite(pinLED, LOW);
  delay(millisAfter);
}
