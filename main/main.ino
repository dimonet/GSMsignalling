/// GSM сигналка c установкой по кнопке
/// датчиком на прерывателях и движения

// донат, https://money.yandex.ru/to/410012486954705

#include <EEPROM.h>
#include "MyGSM.h"
#include "PowerControl.h"

#define debug Serial

//// НАСТРОЕЧНЫЕ КОНСТАНТЫ /////
// номера телефонов
#define TELLNUMBER "380509151369"                     // номен на который будем звонить
#define SMSNUMBER  "+380509151369"                     // номер на который будем отправлять SMS

#define NUMBER1_NotInContr "380509151369"             // 1-й номер для снятие с охраны (Мой МТС)
#define NUMBER2_NotInContr "380506228524"             // 2-й номер для снятие с охраны (Тони МТС)
#define NUMBER3_NotInContr "***"                      // 3-й номер для снятие с охраны
#define NUMBER4_NotInContr "***"                      // 4-й номер для снятие с охраны

#define NUMBER1_InContr    "380969405835"             // 1-й номер для установки на охраны (Мой Киевстар)
#define NUMBER2_InContr    "***"                      // 2-й номер для установки на охраны
#define NUMBER3_InContr    "***"                      // 3-й номер для установки на охраны
#define NUMBER4_InContr    "***"                      // 4-й номер для установки на охраны

#define NUMBER1_SmsCommand    "+380509151369"             // 1-й номер для управления через sms (Мой МТС)
#define NUMBER2_SmsCommand    "+380969405835"             // 2-й номер для управления через sms (Мой Киевстар)
#define NUMBER3_SmsCommand    "***"                       // 3-й номер для управления через sms 
#define NUMBER4_SmsCommand    "***"                       // 4-й номер для управления через sms


// SMS
#define smsText_TensionCable    "ALARM: TensionCable sensor."                         // текст смс для растяжки
#define smsText_PIR1            "ALARM: PIR1 sensor."                                 // текст смс для датчика движения 1
#define smsText_PIR2            "ALARM: PIR2 sensor."                                 // текст смс для датчика движения 2

#define smsText_BattPower       "POWER: Backup Battery is used for powering system."  // текст смс для оповещения о том, что исчезло сетевое питание
#define smsText_NetPower        "POWER: Network power has been restored."             // текст смс для оповещения о том, что сетевое питание возобновлено

#define GSMCODE_BALANCE         "*101#"                                               // GSM код для запроса баланца

#define smsText_ErrorCommand    "Command: ERROR. Available only commands: Balance, Test on/off, Redirect on/off, Control on/off, Skimpy, gsm code."  // смс команда не распознана
#define smsText_TestModOn       "Command: Test mode has been turned on."              // выполнена команда для включения тестового режима для тестирования датчиков
#define smsText_TestModOff      "Command: Test mode has been turned off."             // выполнена команда для выключения тестового режима для тестирования датчиков
#define smsText_InContrMod      "Command: Control mode has been turned on."           // выполнена команда для установку на охрану
#define smsText_NotInContrMod   "Command: Control mode has been turned off."          // выполнена команда для установку на охрану
#define smsText_RedirectOn      "Command: SMS redirection has been turned on."        // выполнена команда для включения перенаправления всех смс от любого отправителя на номер SMSNUMBER
#define smsText_RedirectOff     "Command: SMS redirection has been turned off."       // выполнена команда для выключения перенаправления всех смс от любого отправителя на номер SMSNUMBER
#define smsText_SkimpySiren     "Command: Skimpy siren has been turned on."           // выполнена команда для коротковременного включения сирены

// паузы
#define  timeWaitInContr      25                           // Время паузы от нажатие кнопки до установки режима охраны
#define  timeWaitInContrTest  7                            // Время паузы от нажатие кнопки до установки режима охраны в режиме тестирования
#define  timeHoldingBtn       2                            // время удерживания кнопки для включения режима охраны  2 сек.
#define  timeAfterPressBtn    3000                         // время выполнения операций после единичного нажатия на кнопку
#define  timeSiren            20000                        // время работы сирены (милисекунды).
#define  timeCall             120000                       // время паузы после последнего звонка тревоги (милисекунды)
#define  timeSmsPIR1          120000                       // время паузы после последнего СМС датчика движения 1 (милисекунды)
#define  timeSmsPIR2          120000                       // время паузы после последнего СМС датчика движения 2 (милисекунды)
#define  timeSkimpySiren      300                          // время короткого срабатывания модуля сирены 


// Количество нажатий на кнопку для включений режимова
#define countBtnInTestMod   2                              // количество нажатий на кнопку для включение/отключения режима тестирования 
#define countBtnBalance     3                              // количество нажатий на кнопку для запроса баланса счета
#define countBtnSkimpySiren 4                              // количество нажатий на кнопку для кратковременного включения сирены

//// КОНСТАНТЫ ПИТЯНИЯ ////
#define netVcc      10.0                      // значения питяния от сети (вольт)
#define battVcc     0.1                       // значения питяния от сети (вольт)
#define battVccMin  2.75                      // минимальное напряжение батареи (для сигнализации о том, что батарея разряжена)

//// КОНСТАНТЫ ДЛЯ ПИНОВ /////
#define SpecerPin 8
#define gsmLED 13
#define NotInContrLED 12
#define InContrLED 11
#define SirenLED 10
#define BattPowerLED 2                        // LED для сигнализации о работе от резервного питания

#define pinBOOT 5                             // нога BOOT или K на модеме 
#define Button 9                              // нога на кнопку
#define SirenGenerator 7                      // нога на сирену

// Спикер
#define specerTone 98                                      // тон спикера

//Power control 
#define pinMeasureVcc A0                      // нога чтения типа питания (БП или батарея)

//Sensores
#define SH1 A2                                // нога на растяжку
#define pinPIR1 4                             // нога датчика движения 1
#define pinPIR2 3                             // нога датчика движения 2

//// КОНСТАНТЫ РЕЖИМОВ РАБОТЫ //// 
#define NotInContrMod  1                      // снята с охраны
#define InContrMod     3                      // установлена охрана

//// КОНСТАНТЫ EEPROM ////
#define E_mode  0                             // адресс для сохранения режимов работы 
#define E_inTestMod  1                        // адресс для сохранения режима тестирования
#define E_isRedirectSms  2                    // адресс для сохранения режима перенаправления всех смс


//// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ////
byte mode = NotInContrMod;                    // 1 - снята с охраны                                  
                                              // 3 - установлено на охрану
                                              // при добавлении не забываем посмотреть рездел //// КОНСТАНТЫ РЕЖИМОВ РАБОТЫ ////
                                               
bool btnIsHolding = false;
byte countPressBtn = 0;                       // счетчик нажатий на кнопку
bool inTestMod = false;                       // режим тестирования датчиков (не срабатывает сирена и не отправляются СМС)
bool isSiren = false;                         // режим сирены
bool isRedirectSms = false;                   // режим перенаправления всех смс от любого отправителя на номер SMSNUMBER

unsigned long prSiren = 0;                       // время включения сирены (милисекунды)
unsigned long prCall = 0;                        // время последнего звонка тревоги (милисекунды)
unsigned long prSmsPIR1 = 0;                     // время последнего СМС датчика движения 1 (милисекунды)
unsigned long prSmsPIR2 = 0;                     // время последнего СМС датчика движения 2 (милисекунды)
unsigned long prLastPressBtn = 0;                // время последнего нажатие на кнопку (милисекунды)

bool controlTensionCable = true;                 // включаем контроль растяжки
String str = "";

MyGSM gsm(gsmLED, pinBOOT);                             // GSM модуль
PowerControl powCtr (netVcc, battVcc, pinMeasureVcc);   // контроль питания

void setup() 
{
  delay(1000);                                // !! чтобы нечего не повисало при включении
  //debug.begin(9600);
  pinMode(SpecerPin, OUTPUT);
  pinMode(gsmLED, OUTPUT);
  pinMode(NotInContrLED, OUTPUT);
  pinMode(InContrLED, OUTPUT);
  pinMode(SirenLED, OUTPUT);
  pinMode(BattPowerLED, OUTPUT);              // LED для сигнализации о работе от резервного питания
  pinMode(pinBOOT, OUTPUT);                   // нога BOOT на модеме
  pinMode(SH1, INPUT_PULLUP);                 // нога на растяжку
  pinMode(pinPIR1, INPUT);                    // нога датчика движения 1
  pinMode(pinPIR2, INPUT);                    // нога датчика движения 2
  pinMode(Button, INPUT_PULLUP);              // кнопка для установки режима охраны
  pinMode(SirenGenerator, OUTPUT);            // нога на сирену
  pinMode(pinMeasureVcc, INPUT);              // нога чтения типа питания (БП или батарея)    

  digitalWrite(SirenGenerator, HIGH);         // выключаем сирену через релье
  
  digitalWrite(BattPowerLED, powCtr.IsBattPower());     // если питаимся от батареи включам соотвествующий LED
  
  // блок тестирования спикера и всех светодиодов
  PlayTone(specerTone, 100);                          
  delay(500);
  digitalWrite(gsmLED, HIGH);
  digitalWrite(NotInContrLED, HIGH);
  digitalWrite(InContrLED, HIGH);
  digitalWrite(SirenLED, HIGH);
  digitalWrite(BattPowerLED, HIGH);
  delay(1500);
  digitalWrite(gsmLED, LOW);
  digitalWrite(NotInContrLED, LOW);
  digitalWrite(InContrLED, LOW);
  digitalWrite(SirenLED, LOW);
  digitalWrite(BattPowerLED, LOW);
  
  powCtr.Refresh();                                     // читаем тип питания (БП или батарея)
  digitalWrite(BattPowerLED, powCtr.IsBattPower());     // сигнализируем светодиодом режим питания (от батареи - светится, от сети - не светится)
  
  gsm.Initialize();                                     // инициализация gsm модуля (включения, настройка) 
    
  // чтение конфигураций с EEPROM
  mode = EEPROM.read(E_mode);                           // читаем режим из еепром
  if (mode == InContrMod) Set_InContrMod(1);          
  else Set_NotInContrMod();

  inTestMod = EEPROM.read(E_inTestMod);                 // читаем тестовый режим из еепром
  isRedirectSms = EEPROM.read(E_isRedirectSms);         // читаем режима перенаправления всех смс    
}

bool newClick = true;

void loop() 
{       
  PowerControl();                                                   // мониторим питание системы
  gsm.Refresh();                                                    // читаем сообщения от GSM модема   
  
  if (inTestMod && !isSiren)                                        // если включен режим тестирования и не сирена то мигаем светодиодом
  {
    digitalWrite(SirenLED, digitalRead(SirenLED) == LOW);     
    delay(200);
  }

  ////// NOT IN CONTROL MODE ///////  
  if (mode == NotInContrMod)                                           // если режим не на охране
  {
    if (digitalRead(Button) == HIGH) newClick = true;
    if (digitalRead(Button) == LOW && newClick)
    {
      BlinkLEDlow(NotInContrLED,  0, 100, 0);      
      PlayTone(specerTone, 40);
      newClick = false;
      countPressBtn++;
      prLastPressBtn = millis();              
    }       
    if (countPressBtn != 0 && (GetElapsed(prLastPressBtn) > timeAfterPressBtn))
    {       
      // запрос баланса счета
      if (countPressBtn == countBtnBalance)                           // если кнопку нажали заданное количество для запроса баланса счета
      {
        PlayTone(specerTone, 250);                                    // сигнализируем об этом спикером         
        RequestGsmCode(SMSNUMBER, GSMCODE_BALANCE);        
      }                                                               // отправляем смс с балансом            
      else 
      // включение/отключения режима тестирования
      if (countPressBtn == countBtnInTestMod)                         // если кнопку нажали заданное количество для включение/отключения режима тестирования
      {
        PlayTone(specerTone, 250);                                    // сигнализируем об этом спикером  
        inTestMod = !inTestMod;                                       // включаем/выключаем режим тестирование датчиков        
        digitalWrite(SirenLED, LOW);                                  // выключаем светодиод
        EEPROM.write(E_inTestMod, inTestMod);                         // пишим режим тестирование датчиков в еепром
      }
      else
      // кратковременное включение сирены (для тестирования модуля сирены)
      if (countPressBtn == countBtnSkimpySiren)                      
      {
        PlayTone(specerTone, 250);                                    
        SkimpySiren();
      }  
      countPressBtn = 0;      
    }

    if (ButtonIsHold(timeHoldingBtn))                                 // если режим не на охране и если кнопка удерживается заданое время
    {  
      countPressBtn = 0;                                              // сбрасываем счетчик нажатий на кнопку 
      Set_InContrMod(1);                                              // то ставим на охрану
      return;     
    }

    if (gsm.NewRing)                                                  // если обнаружен входящий звонок
    {
      if (gsm.RingNumber.indexOf(NUMBER1_InContr) > -1 ||             // если включен режим снята с охраны и найден зарегистрированный звонок то ставим на охрану
          gsm.RingNumber.indexOf(NUMBER2_InContr) > -1 ||
          gsm.RingNumber.indexOf(NUMBER3_InContr) > -1 ||
          gsm.RingNumber.indexOf(NUMBER4_InContr) > -1 
         )      
      {               
        digitalWrite(SirenLED, LOW);                        // на время выключаем мигание светодиода сирены если включен режим тестирования
        delay(5000);                                        // большая пауза перед збросом звонка        
        gsm.RejectCall();                                   // сбрасываем вызов        
        Set_InContrMod(0);                                  // устанавливаем на охрану без паузы 
        return;     
      }
      else gsm.RejectCall();                                // если не найден зарегистрированный звонок то сбрасываем вызов (без паузы)      
    }    
  }                                                         // end NotInContrMod 
  else
  
  ////// IN CONTROL MODE ///////  
  if (mode == InContrMod)                                             // если в режиме охраны
  {
    if (isSiren && GetElapsed(prSiren) > timeSiren)                   // если включена сирена и сирена работает больше установленного времени то выключаем ее
    StopSiren();
    
    if (ButtonIsHold(timeHoldingBtn) && inTestMod)                    // снимаем с охраны если кнопка удерживается заданое время и включен режим тестирования
    {
      gsm.RejectCall();                                               // сбрасываем вызов
      Set_NotInContrMod();
      return;                         
    }
    
    bool sTensionCable = SensorTriggered_TensionCable();              // проверяем датчики
    bool sPIR1 = SensorTriggered_PIR1();
    bool sPIR2 = SensorTriggered_PIR2();
                                   
    if ((sTensionCable && controlTensionCable) || sPIR1 || sPIR2)                  // если обрыв
    {                                                                 
      if (isSiren == false) StartSiren();                                          // включаем сирену
            
      if (sPIR1 && !inTestMod                                                      // отправляем СМС если сработал датчик движения и не включен режим тестирование 
        && ((GetElapsed(prSmsPIR1) > timeSmsPIR1) or prSmsPIR1 == 0))              // и выдержена пауза после последнего смс
      {
        if(gsm.SendSMS(&String(smsText_PIR1), String(SMSNUMBER)))
          prSmsPIR1 = millis();               
      }
      
      if (sPIR2 && !inTestMod                                                      // отправляем СМС если сработал датчик движения и не включен режим тестирование  
        && ((GetElapsed(prSmsPIR2) > timeSmsPIR2) or prSmsPIR2 == 0))              // и выдержена пауза после последнего смс
      {  
        if(gsm.SendSMS(&String(smsText_PIR2), String(SMSNUMBER)))
          prSmsPIR2 = millis();               
      }

      if (sTensionCable && !inTestMod)                                             // отправляем СМС если сработал обрыв растяжки и не включен режим тестирование
      {  
        gsm.SendSMS(&String(smsText_TensionCable), String(SMSNUMBER));               
      }
      
      if ((GetElapsed(prCall) > timeCall) or prCall == 0)                          // проверяем сколько прошло времени после последнего звонка (выдерживаем паузц между звонками)
      {
        if(gsm.Call(String(TELLNUMBER)))                                           // отзваниваемся
          prCall = millis();                                                       // если отзвон осуществлен то запоминаем время последнего отзвона
      }
            
      if (sTensionCable) controlTensionCable = false;                              // отключаем контроль растяжки что б сирена не работала постоянно после разрыва растяжки
    }

     if (gsm.NewRing)                                                              // если обнаружен входящий звонок
     {
       if (gsm.RingNumber.indexOf(NUMBER1_NotInContr) > -1 ||                      // если включен режим охраны и найден зарегистрированный звонок то снимаем с охраны
           gsm.RingNumber.indexOf(NUMBER2_NotInContr) > -1 || 
           gsm.RingNumber.indexOf(NUMBER3_NotInContr) > -1 ||
           gsm.RingNumber.indexOf(NUMBER4_NotInContr) > -1 
          )               
       {                    
         delay(2500);                                        // небольшая пауза перед збросом звонка
         Set_NotInContrMod();                                // снимаем с охраны         
         gsm.RejectCall();                                   // сбрасываем вызов
         return;
       }
       else gsm.RejectCall();                                // если не найден зарегистрированный звонок то сбрасываем вызов (без паузы)
     }         
  }                                                          // end InContrMod     
  if (ExecSmsCommand()) return;                              // читаем смс и если доступна новая команда по смс то выполняем ее
}



//// ------------------------------- Functions --------------------------------- ////

// подсчет сколько прошло милисикунд после последнего срабатывания события (сирена, звонок и т.д.)
unsigned long GetElapsed(unsigned long prEventMillis)
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
  StopSiren();                          //останавливаем сирену
  prSmsPIR1 = 0;
  prSmsPIR2 = 0;
  prCall = 0;
  EEPROM.write(E_mode, mode);           // пишим режим в еепром 
  return true;
}

bool Set_InContrMod(bool IsWaiting)
{ 
  if (IsWaiting == true)                                // если включен режим ожидание перед установкой охраны выдерживаем заданную паузу что б успеть покинуть помещение
  {    
    digitalWrite(NotInContrLED, LOW);   
    
    byte timeWait = 0;
    if (inTestMod) timeWait = timeWaitInContrTest;      // если включен режим тестирования то устанавливаем для удобства тестирования меньшую паузу
    else timeWait = timeWaitInContr;                    // если режим тестирования выклюяен то используем обычную паузу
    
    for(int i = 0; i < timeWait; i++)   
    {
      if (ButtonIsHold(timeHoldingBtn))                 // проверяем не нажата ли кнопка и если кнопка удерживается заданое время, функция вернет true и установка на охрану прерывается     
      {
        Set_NotInContrMod();
        return false;      
      }    
      
      if (i < (timeWait * 0.7))                         // первых 70% паузы моргаем медленным темпом
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
  }
  
  // установка переменных в дефолтное состояние
  controlTensionCable = true;                           // включаем контроль растяжки
  prCall = 0;                                           // сбрвсываем переменные пауз для gsm
  prSmsPIR1 = 0;
  prSmsPIR2 = 0;
  
  //установка на охрану                                                       
  digitalWrite(NotInContrLED, LOW);
  digitalWrite(InContrLED, HIGH);
  digitalWrite(SirenLED, LOW);  
  PlayTone(specerTone, 500);
  mode = InContrMod;                                    // ставим на охрану  
  EEPROM.write(E_mode, mode);                           // пишим режим в еепром
  delay (2500);                                         // дополнительная пауза так как датчик держит лог. единицу 2,5
  return true;
}


void  StartSiren()
{
  digitalWrite(SirenLED, HIGH);
  if (!inTestMod)                                        // если не включен тестовый режим
    digitalWrite(SirenGenerator, LOW);                   // включаем сирену через релье
  isSiren = 1;
  prSiren = millis();
}


void  StopSiren()
{
  digitalWrite(SirenLED, LOW);
  digitalWrite(SirenGenerator, HIGH);                    // выключаем сирену через релье
  isSiren = 0; 
}


bool ButtonIsHold(byte timeHold)
{  
  if (digitalRead(Button) == HIGH) btnIsHolding = false;               // если кнопка не нажата сбрасываем показатеь удерживания кнопки
  if (digitalRead(Button) == LOW && btnIsHolding == false)             // проверяем нажата ли кнопка и отпускалась ли после предыдущего нажатия (для избежание ложного считывание кнопки)
  { 
    btnIsHolding = true;
    if (timeHold == 0) return true;                                    // если нужно реагировать немедленно после нажатия на кнопку (без паузы на удерживания)
    if (inTestMod == 1 && mode != InContrMod)                          // если включен режим тестирование на время удерживание кнопки выключаем мигание светодиода
      digitalWrite(SirenLED, LOW); 
       
    byte i = 1;
    while(1) 
    {
      delay(500);
      if (i == timeHold) return true;  
      if (digitalRead(Button) == HIGH) return false;                 
      i++;
      delay(500);      
    }
  }   
  return false;  
}


void PlayTone(byte tone, unsigned int duration) 
{
  for (unsigned long i = 0; i < duration * 1000L; i += tone * 2) 
  {
    digitalWrite(SpecerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(SpecerPin, LOW);
    delayMicroseconds(tone);
  }
} 


////// Методы датчиков ////// 
bool SensorTriggered_TensionCable()                                     // растяжка
{
  if (digitalRead(SH1) == HIGH) return true;
  else return false;
}

bool SensorTriggered_PIR1()                                             // датчик движения 1
{
  if (digitalRead(pinPIR1) == HIGH) return true;
  else return false;
}

bool SensorTriggered_PIR2()                                             // датчик движения 2
{
  if (digitalRead(pinPIR2) == HIGH) return true;
  else return false;
}

// Блымание светодиодом
void BlinkLEDhigh(byte pinLED,  unsigned int millisBefore,  unsigned int millisHIGH,  unsigned int millisAfter)
{ 
  digitalWrite(pinLED, LOW);                          
  delay(millisBefore);  
  digitalWrite(pinLED, HIGH);                                           // блымаем светодиодом
  delay(millisHIGH); 
  digitalWrite(pinLED, LOW);
  delay(millisAfter);
}

void BlinkLEDlow(byte pinLED,  unsigned int millisBefore,  unsigned int millisLOW,  unsigned int millisAfter)
{ 
  digitalWrite(pinLED, HIGH);                          
  delay(millisBefore);  
  digitalWrite(pinLED, LOW);                                            // выключаем светодиод
  delay(millisLOW); 
  digitalWrite(pinLED, HIGH);
  delay(millisAfter);
}

// Блымание светодиодом со спикером
void BlinkLEDSpecer(byte pinLED,  unsigned int millisBefore,  unsigned int millisHIGH,  unsigned int millisAfter)
{ 
  digitalWrite(pinLED, LOW);                          
  delay(millisBefore);  
  digitalWrite(pinLED, HIGH);                                           // блымаем светодиодом
  PlayTone(specerTone, millisHIGH);
  digitalWrite(pinLED, LOW);
  delay(millisAfter);
}
//читаем тип питания системы (БП или батарея)
void PowerControl()
{
  powCtr.Refresh();    
  digitalWrite(BattPowerLED, powCtr.IsBattPower());
        
  if (!inTestMod && !powCtr.IsBattPowerPrevious() && powCtr.IsBattPower())   // если предыдущий раз было от сети а сейчас от батареи (пропало сетевое питание 220v) и если не включен режим тестирования
    gsm.SendSMS(&String(smsText_BattPower), String(SMSNUMBER));              // отправляем смс о переходе на резервное питание         
    
  if (!inTestMod && powCtr.IsBattPowerPrevious() && !powCtr.IsBattPower())   // если предыдущий раз было от батареи a сейчас от сети (сетевое питание 220v возобновлено) и если не включен режим тестирования
    gsm.SendSMS(&String(smsText_NetPower), String(SMSNUMBER));               // отправляем смс о возобновлении  сетевое питание 220v        
}

// запрос gsm кода (*#) и отсылаем результат через смс
void RequestGsmCode(String smsNumber, String code)
{
  digitalWrite(SirenLED, LOW);                                  // выключаем светодиод  
  gsm.RequestGsmCode(code);                                    // запрашиваем баланс                      
  str = "";
  while (!gsm.Available())
  {
    BlinkLEDlow(NotInContrLED, 0, 500, 500);                    // мигаем светодиодом  
  }
  BlinkLEDlow(NotInContrLED, 0, 500, 500);                 

  str = gsm.Read();
  int beginStr = str.indexOf('\"');
  str = str.substring(beginStr + 1); 
  str = str.substring(0, str.indexOf("\","));
  gsm.SendSMS(&str, smsNumber); 
}

// короткое включение сирены (для тестирования модуля сирены)
void SkimpySiren()
{
  digitalWrite(SirenLED, HIGH);
  digitalWrite(SirenGenerator, LOW);                   // включаем сирену через релье
  delay(timeSkimpySiren);                              // период короткой работы сирены
  digitalWrite(SirenLED, LOW);
  digitalWrite(SirenGenerator, HIGH);                  // выключаем сирену через релье
  
}

// читаем смс и если доступна новая команда по смс то выполняем ее
bool ExecSmsCommand()
{
  if (gsm.NewSms)
  {
    if ( gsm.SmsNumber.indexOf(NUMBER1_SmsCommand) > -1 ||              
         gsm.SmsNumber.indexOf(NUMBER2_SmsCommand) > -1 ||
         gsm.SmsNumber.indexOf(NUMBER3_SmsCommand) > -1 ||
         gsm.SmsNumber.indexOf(NUMBER4_SmsCommand) > -1                                                                                      
       )
    {       
      if (gsm.SmsText == "Balance" || gsm.SmsText == "balance")                               // запрос баланса
      {
        PlayTone(specerTone, 250);                                             
        RequestGsmCode(gsm.SmsNumber, GSMCODE_BALANCE);
        return true;
      }
      else
      if (gsm.SmsText == "Test on" || gsm.SmsText == "test on")                               // включения тестового режима для тестирования датчиков
      {
        PlayTone(specerTone, 250); 
        inTestMod = true;
        EEPROM.write(E_inTestMod, true);                                                 // пишим режим тестирование датчиков в еепром                                          
        gsm.SendSMS(&String(smsText_TestModOn), gsm.SmsNumber);      
        return true;
      }
      else
      if (gsm.SmsText == "Test off" || gsm.SmsText == "test off")                        // выключения тестового режима для тестирования датчиков
      {
        digitalWrite(SirenLED, LOW);                                                     // выключаем светодиод
        PlayTone(specerTone, 250);                                            
        inTestMod = false;        
        EEPROM.write(E_inTestMod, false);                                                // пишим режим тестирование датчиков в еепром
        gsm.SendSMS(&String(smsText_TestModOff), gsm.SmsNumber);
        return true;
      }
      else
      if (gsm.SmsText.startsWith("*"))                                                        // Если сообщение начинается на * то это gsm код
      {
        unsigned int endCommand = gsm.SmsText.indexOf('#');                                   // если команда не заканчивается на # то информируем по смс об ошибке
        if (endCommand == 65535)                                                                
        {
          gsm.SendSMS(&String(smsText_ErrorCommand), gsm.SmsNumber);
          return false;    
        }            
        PlayTone(specerTone, 250);                                                  
        RequestGsmCode(gsm.SmsNumber, gsm.SmsText);
        return true;      
      }
      else
      if (gsm.SmsText.startsWith("Control on") || gsm.SmsText.startsWith("control on"))       // если сообщение начинается на * то это gsm код
      {
        digitalWrite(SirenLED, LOW);                                                          // выключаем светодиод
        Set_InContrMod(0);                                                                    // устанавливаем на охрану без паузы                                                
        gsm.SendSMS(&String(smsText_InContrMod), gsm.SmsNumber);
        return true;      
      }
      else
      if (gsm.SmsText.startsWith("Control off") || gsm.SmsText.startsWith("control off"))     // если сообщение начинается на * то это gsm код
      {
        digitalWrite(SirenLED, LOW);                                                          // выключаем светодиод
        Set_NotInContrMod();                                                                 // устанавливаем на охрану без паузы                                                
        gsm.SendSMS(&String(smsText_NotInContrMod), gsm.SmsNumber);
        return true;      
      }
      else
      if (gsm.SmsText.startsWith("Redirect on") || gsm.SmsText.startsWith("redirect on"))     // если сообщение начинается на * то это gsm код       
      {
        isRedirectSms = true;
        EEPROM.write(E_isRedirectSms, true);
        gsm.SendSMS(&String(smsText_RedirectOn), gsm.SmsNumber);
      }
      else
      if (gsm.SmsText.startsWith("Redirect off") || gsm.SmsText.startsWith("redirect off"))   // если сообщение начинается на * то это gsm код       
      {
        isRedirectSms = false;
        EEPROM.write(E_isRedirectSms, false);
        gsm.SendSMS(&String(smsText_RedirectOff), gsm.SmsNumber);
      }
      else
      if (gsm.SmsText.startsWith("Skimpy") || gsm.SmsText.startsWith("skimpy"))   // если сообщение начинается на * то это gsm код       
      {
        SkimpySiren();
        gsm.SendSMS(&String(smsText_SkimpySiren), gsm.SmsNumber);
      }
      else
      {
        PlayTone(specerTone, 250);      
        gsm.SendSMS(&String(smsText_ErrorCommand), gsm.SmsNumber);
        return false;
      }        
    }
    else if (isRedirectSms)                                                                    // если смс пришла не с зарегистрированых номеров и включен режим перенаправления всех смс
      gsm.SendSMS(&String("N: " + gsm.SmsNumber + '\n' + gsm.SmsText), String(SMSNUMBER));     
  }         
  return false;
}

