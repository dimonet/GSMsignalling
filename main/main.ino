/// GSM сигналка c установкой по кнопке
/// датчиком на прерывателях и движения
/// ВНИМАНИЕ: для корретной работы sms необходимо установить размеры буферов вместо 64 на SERIAL_TX_BUFFER_SIZE 24 и SERIAL_RX_BUFFER_SIZE 170 в файле hardware\arduino\avr\cores\arduino\HardwareSerial.h

#include <EEPROM.h>
#include "MyGSM.h"
#include "PowerControl.h"
#include <avr/pgmspace.h>

//#define debug Serial

//// НАСТРОЕЧНЫЕ КОНСТАНТЫ /////
// номера телефонов

//#define NUMBER1_NotInContr "380509151369"             // 1-й номер для снятие с охраны и на который будем звонить (Мой МТС)
//#define NUMBER2_NotInContr "380506228524"             // 2-й номер для снятие с охраны (Тони МТС)
//#define NUMBER3_NotInContr "***"                      // 3-й номер для снятие с охраны

//#define NUMBER1_InContr    "380969405835"             // 1-й номер для установки на охраны (Мой Киевстар)
//#define NUMBER2_InContr    "***"                      // 2-й номер для установки на охраны

//#define NUMBER1_SmsCommand    "+380509151369"         // 1-й номер для управления через sms и на который будем отправлять SMS (Мой МТС)
//#define NUMBER2_SmsCommand    "+380969405835"         // 2-й номер для управления через sms (Мой Киевстар)
//#define NUMBER3_SmsCommand    "+380506228524"         // 3-й номер для управления через sms 


//const char GSMCODE_BALANCE[]       PROGMEM = {"*101#"};                                               // GSM код для запроса баланца

// SMS
const char sms_TensionCable[]  PROGMEM = {"ALARM: TensionCable sensor."};                         // текст смс для растяжки
const char sms_PIR1[]          PROGMEM = {"ALARM: PIR1 sensor."};                                 // текст смс для датчика движения 1
const char sms_PIR2[]          PROGMEM = {"ALARM: PIR2 sensor."};                                 // текст смс для датчика движения 2

const char sms_BattPower[]     PROGMEM = {"POWER: Backup Battery is used for powering system."};  // текст смс для оповещения о том, что исчезло сетевое питание
const char sms_NetPower[]      PROGMEM = {"POWER: Network power has been restored."};             // текст смс для оповещения о том, что сетевое питание возобновлено


const char sms_ErrorCommand[]    PROGMEM = {"Command: Available commands:\nTest on/off,\nRedirect on/off,\nControl on/off,\nSkimpy,\nReboot,\nStatus,\nButtonGSMcode,\nNotInContr,\nInContr,\nSmsCommand."};  // смс команда не распознана
const char sms_TestModOn[]       PROGMEM = {"Command: Test mode has been turned on."};              // выполнена команда для включения тестового режима для тестирования датчиков
const char sms_TestModOff[]      PROGMEM = {"Command: Test mode has been turned off."};             // выполнена команда для выключения тестового режима для тестирования датчиков
const char sms_InContrMod[]      PROGMEM = {"Command: Control mode has been turned on."};           // выполнена команда для установку на охрану
const char sms_NotInContrMod[]   PROGMEM = {"Command: Control mode has been turned off."};          // выполнена команда для установку на охрану
const char sms_RedirectOn[]      PROGMEM = {"Command: SMS redirection has been turned on."};        // выполнена команда для включения перенаправления всех смс от любого отправителя на номер SMSNUMBER
const char sms_RedirectOff[]     PROGMEM = {"Command: SMS redirection has been turned off."};       // выполнена команда для выключения перенаправления всех смс от любого отправителя на номер SMSNUMBER
const char sms_SkimpySiren[]     PROGMEM = {"Command: Skimpy siren has been turned on."};           // выполнена команда для коротковременного включения сирены
const char sms_WasRebooted[]     PROGMEM = {"Command: Device was rebooted."};                       // выполнена команда для коротковременного включения сирены
const char sms_WrongGsmCommand[] PROGMEM = {"Command: Wrong GSM command."};                         // сообщение о неправельной gsm комманде
const char sms_ButtonGSMcode[]   PROGMEM = {"Command: GSM command for button was changed to "};        // сообщение о неправельной gsm комманде

// паузы
#define  timeWaitInContr      25                           // время паузы от нажатие кнопки до установки режима охраны
#define  timeWaitInContrTest  7                            // время паузы от нажатие кнопки до установки режима охраны в режиме тестирования
#define  timeHoldingBtn       2                            // время удерживания кнопки для включения режима охраны  2 сек.
#define  timeAfterPressBtn    3000                         // время выполнения операций после единичного нажатия на кнопку
#define  timeSiren            20000                        // время работы сирены (милисекунды).
#define  timeCall             120000                       // время паузы после последнего звонка тревоги (милисекунды)
#define  timeSmsPIR1          120000                       // время паузы после последнего СМС датчика движения 1 (милисекунды)
#define  timeSmsPIR2          120000                       // время паузы после последнего СМС датчика движения 2 (милисекунды)
#define  timeSkimpySiren      300                          // время короткого срабатывания модуля сирены
#define  timeAllLeds          1200                         // время горение всех светодиодов во время включения устройства (тестирования светодиодов)
#define  timeTestBlinkLed     400                          // время мерцания светодиода при включеном режима тестирования
#define  timeRejectCall       3000                         // время пауза перед збросом звонка

// Количество нажатий на кнопку для включений режимова
#define countBtnInTestMod   2                              // количество нажатий на кнопку для включение/отключения режима тестирования 
#define countBtnBalance     3                              // количество нажатий на кнопку для запроса баланса счета
#define countBtnSkimpySiren 4                              // количество нажатий на кнопку для кратковременного включения сирены

//// КОНСТАНТЫ ПИТЯНИЯ ////
#define netVcc      10.0                      // значения питяния от сети (вольт)

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
#define specerTone 98                         // тон спикера

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
#define E_mode           0                    // адресс для сохранения режимов работы 
#define E_inTestMod      1                    // адресс для сохранения режима тестирования
#define E_isRedirectSms  2                    // адресс для сохранения режима перенаправления всех смс
#define E_wasRebooted    3                    // адресс для сохранения режима перенаправления всех смс

#define numSize            13                   // количество символов в строке телефонного номера

#define E_ButtonGSMcode    70                   // GSM код для запроса баланца

#define E_NumberGsmCode    85                   // для промежуточного хранения номера телефона, от которого получено gsm код и которому необходимо отправить ответ (баланс и т.д.)

#define E_NUM1_NotInContr  100                  // 1-й номер для снятие с охраны
#define E_NUM2_NotInContr  115                  // 2-й номер для снятие с охраны
#define E_NUM3_NotInContr  130                  // 3-й номер для снятие с охраны

#define E_NUM1_InContr     145                  // 1-й номер для установки на охрану
#define E_NUM2_InContr     160                  // 2-й номер для установки на охрану

#define E_NUM1_SmsCommand  175                  // 1-й номер для управления через sms
#define E_NUM2_SmsCommand  190                  // 2-й номер для управления через sms
#define E_NUM3_SmsCommand  205                  // 3-й номер для управления через sms
  


//// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ////
byte mode = NotInContrMod;                    // 1 - снята с охраны                                  
                                              // 3 - установлено на охрану
                                              // при добавлении не забываем посмотреть рездел //// КОНСТАНТЫ РЕЖИМОВ РАБОТЫ ////
                                               
bool btnIsHolding = false;
bool inTestMod = false;                       // режим тестирования датчиков (не срабатывает сирена и не отправляются СМС)
bool isSiren = false;                         // режим сирены

unsigned long prSiren = 0;                       // время включения сирены (милисекунды)
unsigned long prCall = 0;                        // время последнего звонка тревоги (милисекунды)
unsigned long prSmsPIR1 = 0;                     // время последнего СМС датчика движения 1 (милисекунды)
unsigned long prSmsPIR2 = 0;                     // время последнего СМС датчика движения 2 (милисекунды)
unsigned long prLastPressBtn = 0;                // время последнего нажатие на кнопку (милисекунды)
unsigned long prTestBlinkLed = 0;                // время мерцания светодиода при включеном режима тестирования (милисекунды)

byte countPressBtn = 0;                          // счетчик нажатий на кнопку
bool controlTensionCable = true;                 // включаем контроль растяжки
bool wasRebooted = false;                        // указываем была ли последний раз перезагрузка программным путем

MyGSM gsm(gsmLED, pinBOOT);                             // GSM модуль
PowerControl powCtr (netVcc, 0.1, pinMeasureVcc);       // контроль питания

void(* RebootFunc) (void) = 0;                          // объявляем функцию Reboot

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

  // блок тестирования спикера и всех светодиодов
  PlayTone(specerTone, 100);                          
  delay(500);
  digitalWrite(gsmLED, HIGH);
  digitalWrite(NotInContrLED, HIGH);
  digitalWrite(InContrLED, HIGH);
  digitalWrite(SirenLED, HIGH);
  digitalWrite(BattPowerLED, HIGH);
  delay(timeAllLeds);
  digitalWrite(gsmLED, LOW);
  digitalWrite(NotInContrLED, LOW);
  digitalWrite(InContrLED, LOW);
  digitalWrite(SirenLED, LOW);
  digitalWrite(BattPowerLED, LOW);
  
  powCtr.Refresh();                                     // читаем тип питания (БП или батарея)
  digitalWrite(BattPowerLED, powCtr.IsBattPower);       // сигнализируем светодиодом режим питания (от батареи - светится, от сети - не светится)
  
  gsm.Initialize();                                     // инициализация gsm модуля (включения, настройка) 
    
  // чтение конфигураций с EEPROM
  mode = EEPROM.read(E_mode);                           // читаем режим из еепром
  if (mode == InContrMod) Set_InContrMod(true);          
  else Set_NotInContrMod();

  inTestMod = EEPROM.read(E_inTestMod);                 // читаем тестовый режим из еепром
  wasRebooted = EEPROM.read(E_wasRebooted);             // читаем был ли последний раз перезагрузка программным путем  
}

bool newClick = true;

void loop() 
{       
  PowerControl();                                                     // мониторим питание системы
  gsm.Refresh();                                                      // читаем сообщения от GSM модема   

  if(wasRebooted)
  {    
    gsm.SendSms(&GetStringFromFlash(sms_WasRebooted), &NumberRead(E_NUM1_SmsCommand));
    wasRebooted = false;
    EEPROM.write(E_wasRebooted, false);
  }
  
  if (inTestMod && !isSiren)                                          // если включен режим тестирования и не сирена
    if (GetElapsed(prTestBlinkLed) > timeTestBlinkLed)   
    {
      digitalWrite(SirenLED, digitalRead(SirenLED) == LOW);           // то мигаем светодиодом
      prTestBlinkLed = millis();
    }

  ////// NOT IN CONTROL MODE ///////  
  if (mode == NotInContrMod)                                          // если режим не на охране
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
      // включение/отключения режима тестирования
      if (countPressBtn == countBtnInTestMod)                            // если кнопку нажали заданное количество для включение/отключения режима тестирования
      {
        PlayTone(specerTone, 250);                                       // сигнализируем об этом спикером  
        inTestMod = !inTestMod;                                          // включаем/выключаем режим тестирование датчиков        
        digitalWrite(SirenLED, LOW);                                     // выключаем светодиод
        EEPROM.write(E_inTestMod, inTestMod);                            // пишим режим тестирование датчиков в еепром
      }
      else
      // запрос баланса счета
      if (countPressBtn == countBtnBalance)                              // если кнопку нажали заданное количество для запроса баланса счета
      {
        PlayTone(specerTone, 250);                                       // сигнализируем об этом спикером                        
        if(gsm.RequestGsmCode(&ReadFromEEPROM(E_ButtonGSMcode)))
          NumberWrite(E_NumberGsmCode, &NumberRead(E_NUM1_SmsCommand));  // сохраняем номер на который необходимо будет отправить ответ           
        else
        {
          gsm.SendSms(&GetStringFromFlash(sms_WrongGsmCommand), &NumberRead(E_NUM1_SmsCommand));                                
        }
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
      Set_InContrMod(true);                                           // то ставим на охрану
      return;     
    }

    if (gsm.NewRing)                                                  // если обнаружен входящий звонок
    {
      if (NumberRead(E_NUM1_InContr).indexOf(gsm.RingNumber) > -1 ||  // если найден зарегистрированный звонок то ставим на охрану
          NumberRead(E_NUM2_InContr).indexOf(gsm.RingNumber) > -1          
         )      
      {               
        digitalWrite(SirenLED, LOW);                        // на время выключаем мигание светодиода сирены если включен режим тестирования
        delay(timeRejectCall);                              // пауза перед збросом звонка        
        gsm.RejectCall();                                   // сбрасываем вызов        
        Set_InContrMod(false);                              // устанавливаем на охрану без паузы 
        return;     
      }
      else gsm.RejectCall();                                // если не найден зарегистрированный звонок то сбрасываем вызов (без паузы)      
    gsm.ClearRing();                                        // очищаем обнаруженный входящий звонок    
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
            
      if (sPIR1 && !inTestMod)                                                     // отправляем СМС если сработал датчик движения и не включен режим тестирование 
        if ((GetElapsed(prSmsPIR1) > timeSmsPIR1) or prSmsPIR1 == 0)               // и выдержена пауза после последнего смс
        {
          if(gsm.SendSms(&GetStringFromFlash(sms_PIR1), &NumberRead(E_NUM1_SmsCommand)))
            prSmsPIR1 = millis();               
        }
      
      if (sPIR2 && !inTestMod)                                                     // отправляем СМС если сработал датчик движения и не включен режим тестирование  
        if ((GetElapsed(prSmsPIR2) > timeSmsPIR2) or prSmsPIR2 == 0)               // и выдержена пауза после последнего смс
        {  
          if(gsm.SendSms(&GetStringFromFlash(sms_PIR2), &NumberRead(E_NUM1_SmsCommand)))
            prSmsPIR2 = millis();               
        }

      if (sTensionCable && !inTestMod)                                             // отправляем СМС если сработал обрыв растяжки и не включен режим тестирование
      {         
        gsm.SendSms(&GetStringFromFlash(sms_TensionCable), &NumberRead(E_NUM1_SmsCommand));                    
      }
      
      if ((GetElapsed(prCall) > timeCall) or prCall == 0)                          // проверяем сколько прошло времени после последнего звонка (выдерживаем паузц между звонками)
      {
        if(gsm.Call(&NumberRead(E_NUM1_InContr)))                                   // отзваниваемся
          prCall = millis();                                                       // если отзвон осуществлен то запоминаем время последнего отзвона
      }
            
      if (sTensionCable) controlTensionCable = false;                              // отключаем контроль растяжки что б сирена не работала постоянно после разрыва растяжки
    }

     if (gsm.NewRing)                                                              // если обнаружен входящий звонок
     {      
       if (NumberRead(E_NUM1_NotInContr).indexOf(gsm.RingNumber) > -1 ||           // если найден зарегистрированный звонок то снимаем с охраны
           NumberRead(E_NUM2_NotInContr).indexOf(gsm.RingNumber) > -1 || 
           NumberRead(E_NUM3_NotInContr).indexOf(gsm.RingNumber) > -1          
          )               
       {                    
         delay(timeRejectCall);                              // пауза перед збросом звонка
         Set_NotInContrMod();                                // снимаем с охраны         
         gsm.RejectCall();                                   // сбрасываем вызов        
       }
       else gsm.RejectCall();                                // если не найден зарегистрированный звонок то сбрасываем вызов (без паузы)
       gsm.ClearRing();                                      // очищаем обнаруженный входящий звонок 
     }         
  }                                                          // end InContrMod   
  if (gsm.NewUssd)                                           // если доступный новый ответ на gsm команду
  {    
    gsm.SendSms(&gsm.UssdText, &NumberRead(E_NumberGsmCode));// отправляем ответ на gsm команду
    gsm.ClearUssd();                                         // сбрасываем ответ на gsm команду 
  }
  if (!isSiren) ExecSmsCommand();                            // если не сирена проверяем доступна ли новая команда по смс и если да то выполняем ее
}



//// ------------------------------- Functions --------------------------------- ////

// подсчет сколько прошло милисикунд после последнего срабатывания события (сирена, звонок и т.д.)
unsigned long GetElapsed(unsigned long &prEventMillis)
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
    
    for(byte i = 0; i < timeWait; i++)   
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
  else
    PlayTone(specerTone, 100);                           // если включен режим тестирование то сигнализируем только спикером
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
  digitalWrite(BattPowerLED, powCtr.IsBattPower);
        
  if (!inTestMod && !powCtr.IsBattPowerPrevious && powCtr.IsBattPower)                    // если предыдущий раз было от сети а сейчас от батареи (пропало сетевое питание 220v) и если не включен режим тестирования
    gsm.SendSms(&GetStringFromFlash(sms_BattPower), &NumberRead(E_NUM1_SmsCommand));      // отправляем смс о переходе на резервное питание         
   
  if (!inTestMod && powCtr.IsBattPowerPrevious && !powCtr.IsBattPower)                    // если предыдущий раз было от батареи a сейчас от сети (сетевое питание 220v возобновлено) и если не включен режим тестирования  
    gsm.SendSms(&GetStringFromFlash(sms_NetPower), &NumberRead(E_NUM1_SmsCommand));       // отправляем смс о возобновлении  сетевое питание 220v 
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

String GetStringFromFlash(char* addr)
{
  String buffstr = "";
  int len = strlen_P(addr);
  char currSymb;
  for (byte i = 0; i < len; i++)
  {
    currSymb = pgm_read_byte_near(addr + i);
    buffstr += String(currSymb);
  }
  return buffstr;
}

void NumberWrite(byte e_addr, String *number)
{
  char charStr[numSize+1];
  number->toCharArray(charStr, numSize+1);
  EEPROM.put(e_addr, charStr);
}

String NumberRead(byte e_add)
{
 char charread[numSize+1];
 EEPROM.get(e_add, charread);
 String str(charread);
 if (str.startsWith("+")) return str;
 else return "***";
}

String ReadFromEEPROM(byte e_add)
{
 char charread[numSize+1];
 EEPROM.get(e_add, charread);
 String str(charread);
 return str;
}

// читаем смс и если доступна новая команда по смс то выполняем ее
void ExecSmsCommand()
{ 
  if (gsm.NewSms)
  {
    if ((gsm.SmsNumber.indexOf(NumberRead(E_NUM1_SmsCommand)) > -1 ||                                  // если обнаружено зарегистрированый номер
         gsm.SmsNumber.indexOf(NumberRead(E_NUM2_SmsCommand)) > -1 ||
         gsm.SmsNumber.indexOf(NumberRead(E_NUM3_SmsCommand)) > -1
        ) 
        ||
        (NumberRead(E_NUM1_SmsCommand).startsWith("***")  &&                                            // если нет зарегистрированных номеров (при первом включении необходимо зарегистрировать номера)
         NumberRead(E_NUM2_SmsCommand).startsWith("***")  &&
         NumberRead(E_NUM3_SmsCommand).startsWith("***")
         )
       )
    {                   
      if (gsm.SmsText.startsWith("*"))                                                   // Если сообщение начинается на * то это gsm код
      {
        PlayTone(specerTone, 250); 
        if (gsm.RequestGsmCode(&gsm.SmsText))                                                                
          NumberWrite(E_NumberGsmCode, &gsm.SmsNumber);                                  // сохраняем номер на который необходимо будет отправить ответ                                            
        else
          gsm.SendSms(&GetStringFromFlash(sms_WrongGsmCommand), &gsm.SmsNumber);         
      }
      else
      if (gsm.SmsText.startsWith("Test") || gsm.SmsText.startsWith("test"))              // включения тестового режима для тестирования датчиков
      {        
        digitalWrite(SirenLED, LOW);                                                     // выключаем светодиод
        PlayTone(specerTone, 250); 
        if (gsm.SmsText.indexOf(" on") > -1)
        {
          inTestMod = true;
          gsm.SendSms(&GetStringFromFlash(sms_TestModOn), &gsm.SmsNumber);               
        }
        else if (gsm.SmsText.indexOf(" off") > -1)
        {
          inTestMod = false;
          gsm.SendSms(&GetStringFromFlash(sms_TestModOff), &gsm.SmsNumber);                                                      
        }
        else gsm.SendSms(&GetStringFromFlash(sms_ErrorCommand), &gsm.SmsNumber);   
        EEPROM.write(E_inTestMod, inTestMod);                                            // пишим режим тестирование датчиков в еепром                                                  
      }     
      else
      if (gsm.SmsText.startsWith("Control") || gsm.SmsText.startsWith("control"))        // если сообщение начинается на * то это gsm код
      {        
        digitalWrite(SirenLED, LOW);                                                     // выключаем светодиод
        if (gsm.SmsText.indexOf(" on") > -1)
        {
          Set_InContrMod(false);                                                         // устанавливаем на охрану без паузы                                                
          gsm.SendSms(&GetStringFromFlash(sms_InContrMod), &gsm.SmsNumber);                                            
        }
        else if (gsm.SmsText.indexOf(" off") > -1)
        {
          Set_NotInContrMod();
          gsm.SendSms(&GetStringFromFlash(sms_NotInContrMod), &gsm.SmsNumber);      
        }
        else gsm.SendSms(&GetStringFromFlash(sms_ErrorCommand), &gsm.SmsNumber);
      }     
      else
      if (gsm.SmsText.startsWith("Redirect") || gsm.SmsText.startsWith("redirect"))        
      {
        PlayTone(specerTone, 250);
        if (gsm.SmsText.indexOf(" on") > -1)
        {
          EEPROM.write(E_isRedirectSms, true);         
          gsm.SendSms(&GetStringFromFlash(sms_RedirectOn), &gsm.SmsNumber);                                          
        }
        else if (gsm.SmsText.indexOf(" off") > -1) 
        {
          EEPROM.write(E_isRedirectSms, false);          
          gsm.SendSms(&GetStringFromFlash(sms_RedirectOff), &gsm.SmsNumber);       
        }
        else gsm.SendSms(&GetStringFromFlash(sms_ErrorCommand), &gsm.SmsNumber);            
      }
      else
      if (gsm.SmsText.startsWith("Skimpy") || gsm.SmsText.startsWith("skimpy"))          
      {
        SkimpySiren();
        gsm.SendSms(&GetStringFromFlash(sms_SkimpySiren), &gsm.SmsNumber);   
      }
      else
      if (gsm.SmsText.startsWith("Reboot") || gsm.SmsText.startsWith("reboot"))          
      {
        PlayTone(specerTone, 250);
        EEPROM.write(E_wasRebooted, true);                                                       // записываем статус что устройство перезагружается        
        gsm.Shutdown();                                                                          // выключаем gsm модуль
        RebootFunc();                                                                            // вызываем Reboot
      }
      else
      if (gsm.SmsText.startsWith("Status") || gsm.SmsText.startsWith("status"))          
      {
        PlayTone(specerTone, 250);        
        String msg = "On controlling: "   +           String((mode == InContrMod) ? "on" : "off") + "\n"
            + "Test mode: "        +                    String((inTestMod) ? "on" : "off") + "\n" 
            + "Redirect SMS: "     + String((EEPROM.read(E_isRedirectSms)) ? "on" : "off") + "\n"
            + "Power: "            + String((powCtr.IsBattPower) ? "battery" : "network");
        gsm.SendSms(&msg, &gsm.SmsNumber);          
      }           
      else 
      if(gsm.SmsText.startsWith("ButtonGSMcode")|| gsm.SmsText.startsWith("Buttongsmcode") || gsm.SmsText.startsWith("buttongsmcode"))
      {
        PlayTone(specerTone, 250);
        int beginStr = gsm.SmsText.indexOf('\'');
        gsm.SmsText = gsm.SmsText.substring(beginStr + 1);
        int duration = gsm.SmsText.indexOf('\'');  
        gsm.SmsText = gsm.SmsText.substring(0, duration);             
        NumberWrite(E_ButtonGSMcode, &gsm.SmsText);
        String msg = GetStringFromFlash(sms_ButtonGSMcode) + "'" + ReadFromEEPROM(E_ButtonGSMcode) + "'";
        gsm.SendSms(&msg, &gsm.SmsNumber);           
      }      
      else
      if (gsm.SmsText.startsWith("NotInContr1") || gsm.SmsText.startsWith("Notincontr1") || gsm.SmsText.startsWith("notincontr1"))
      {
        PlayTone(specerTone, 250);                      
        String nums[3];
        
        for(int i = 0; i < 3; i++)
        {
          int beginStr = gsm.SmsText.indexOf('\'');
          gsm.SmsText = gsm.SmsText.substring(beginStr + 1);
          int duration = gsm.SmsText.indexOf('\'');  
          nums[i] = gsm.SmsText.substring(0, duration);      
          gsm.SmsText = gsm.SmsText.substring(duration +1);            
        }        
        NumberWrite(E_NUM1_NotInContr , &nums[0]);        
        NumberWrite(E_NUM2_NotInContr, &nums[1]);  
        NumberWrite(E_NUM3_NotInContr, &nums[2]);          
        String msg = "NotInContr1:\n'" + NumberRead(E_NUM1_NotInContr) + "'" + "\n"
            + "NotInContr2:\n'" + NumberRead(E_NUM2_NotInContr) + "'" + "\n"
            + "NotInContr3:\n'" + NumberRead(E_NUM3_NotInContr) + "'";
        gsm.SendSms(&msg, &gsm.SmsNumber);    
      }
      else     
      if (gsm.SmsText.startsWith("InContr1") || gsm.SmsText.startsWith("Incontr1") || gsm.SmsText.startsWith("incontr1"))
      {
        PlayTone(specerTone, 250);                     
        String nums[2];
        
        for(int i = 0; i < 2; i++)
        {
          int beginStr = gsm.SmsText.indexOf('\'');
          gsm.SmsText = gsm.SmsText.substring(beginStr + 1);
          int duration = gsm.SmsText.indexOf('\'');  
          nums[i] = gsm.SmsText.substring(0, duration);      
          gsm.SmsText = gsm.SmsText.substring(duration +1);             
        }              
        NumberWrite(E_NUM1_InContr, &nums[0]);  
        NumberWrite(E_NUM2_InContr, &nums[1]);
        String msg = "InContr1:\n'" + NumberRead(E_NUM1_InContr) + "'" + "\n"
            + "InContr2:\n'" + NumberRead(E_NUM2_InContr) + "'";
        gsm.SendSms(&msg, &gsm.SmsNumber);               
      }
      else
      if (gsm.SmsText.startsWith("SmsCommand1") || gsm.SmsText.startsWith("Smscommand1") || gsm.SmsText.startsWith("smscommand1"))
      {
        PlayTone(specerTone, 250);                     
        String nums[3];
        
        for(int i = 0; i < 3; i++)
        {
          int beginStr = gsm.SmsText.indexOf('\'');
          gsm.SmsText = gsm.SmsText.substring(beginStr + 1);
          int duration = gsm.SmsText.indexOf('\'');  
          nums[i] = gsm.SmsText.substring(0, duration);      
          gsm.SmsText = gsm.SmsText.substring(duration +1);         
        }        
        NumberWrite(E_NUM1_SmsCommand, &nums[0]);  
        NumberWrite(E_NUM2_SmsCommand, &nums[1]);
        NumberWrite(E_NUM3_SmsCommand, &nums[2]);        
        String msg = "SmsCommand1:\n'" + NumberRead(E_NUM1_SmsCommand) + "'" + "\n"
            + "SmsCommand2:\n'" + NumberRead(E_NUM2_SmsCommand) + "'" + "\n"
            + "SmsCommand3:\n'" + NumberRead(E_NUM3_SmsCommand) + "'";
        gsm.SendSms(&msg, &gsm.SmsNumber);     
      }
      else      
      if (gsm.SmsText.startsWith("NotInContr") || gsm.SmsText.startsWith("Notincontr") || gsm.SmsText.startsWith("notincontr"))
      {
        PlayTone(specerTone, 250);        
        String msg = "NotInContr1:\n'" + NumberRead(E_NUM1_NotInContr) + "'" + "\n"
            + "NotInContr2:\n'" + NumberRead(E_NUM2_NotInContr) + "'" + "\n"
            + "NotInContr3:\n'" + NumberRead(E_NUM3_NotInContr) + "'";
        gsm.SendSms(&msg, &gsm.SmsNumber);                    
      }
      else
      if (gsm.SmsText.startsWith("InContr") || gsm.SmsText.startsWith("Incontr") || gsm.SmsText.startsWith("incontr"))
      {
        PlayTone(specerTone, 250);       
        String msg = "InContr1:\n'" + NumberRead(E_NUM1_InContr) + "'" + "\n"
            + "InContr2:\n'" + NumberRead(E_NUM2_InContr) + "'";    
        gsm.SendSms(&msg, &gsm.SmsNumber);
      }
      else
      if (gsm.SmsText.startsWith("SmsCommand") || gsm.SmsText.startsWith("Smscommand") || gsm.SmsText.startsWith("smscommand"))
      {
        PlayTone(specerTone, 250);       
        String msg = "SmsCommand1:\n'" + NumberRead(E_NUM1_SmsCommand) + "'" + "\n"
            + "SmsCommand2:\n'" + NumberRead(E_NUM2_SmsCommand) + "'" + "\n" 
            + "SmsCommand3:\n'" + NumberRead(E_NUM3_SmsCommand) + "'";
        gsm.SendSms(&msg, &gsm.SmsNumber);
      }   
      //смс команда не распознана
      else
      {
        PlayTone(specerTone, 250);              
        gsm.SendSms(&GetStringFromFlash(sms_ErrorCommand), &gsm.SmsNumber);        
      }                                                                                       // очищаем обнаруженное входящие Смс
    }    
    else if (EEPROM.read(E_isRedirectSms))                                                     // если смс пришла не с зарегистрированых номеров и включен режим перенаправления всех смс
    {
      gsm.SendSms(&String(/*"N: " + gsm.SmsNumber + '\n' + */gsm.SmsText), &NumberRead(E_NUM1_SmsCommand));     
    }    
  gsm.ClearSms(); 
  }  
}


