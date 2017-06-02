/// GSM сигналка c установкой по кнопке
/// с датчиками движения и растяжкой (или с геркониевым датчиком)
/// ВНИМАНИЕ: для корретной работы sms необходимо установить размеры буферов вместо 64 на SERIAL_TX_BUFFER_SIZE 24 и SERIAL_RX_BUFFER_SIZE 170 в файле hardware\arduino\avr\cores\arduino\HardwareSerial.h

#include <EEPROM.h>
#include "MyGSM.h"
#include "PowerControl.h"
#include <avr/pgmspace.h>

//#define debug Serial

//// НАСТРОЕЧНЫЕ КОНСТАНТЫ /////
const char sms_TensionCable[]    PROGMEM = {"ALARM: TensionCable sensor."};                                         // текст смс для растяжки
const char sms_PIR1[]            PROGMEM = {"ALARM: PIR1 sensor."};                                                 // текст смс для датчика движения 1
const char sms_PIR2[]            PROGMEM = {"ALARM: PIR2 sensor."};                                                 // текст смс для датчика движения 2

const char sms_BattPower[]       PROGMEM = {"POWER: Backup Battery is used for powering system."};                  // текст смс для оповещения о том, что исчезло сетевое питание
const char sms_NetPower[]        PROGMEM = {"POWER: Network power has been restored."};                             // текст смс для оповещения о том, что сетевое питание возобновлено


const char sms_ErrorCommand[]    PROGMEM = {"SendSMS,\nBalance,\nTest on/off,\nRedirect on/off,\nControl on/off,\nSkimpy,\nReboot,\nStatus,\nDelaySiren,\nBalanceUSSD,\nSensors,\nNotInContr,\nInContr,\nSmsCommand."};  // смс команда не распознана
const char sms_TestModOn[]       PROGMEM = {"Command: Test mode has been turned on."};                              // выполнена команда для включения тестового режима для тестирования датчиков
const char sms_TestModOff[]      PROGMEM = {"Command: Test mode has been turned off."};                             // выполнена команда для выключения тестового режима для тестирования датчиков
const char sms_InContrMod[]      PROGMEM = {"Command: Control mode has been turned on."};                           // выполнена команда для установку на охрану
const char sms_NotInContrMod[]   PROGMEM = {"Command: Control mode has been turned off."};                          // выполнена команда для установку на охрану
const char sms_RedirectOn[]      PROGMEM = {"Command: SMS redirection has been turned on."};                        // выполнена команда для включения перенаправления всех смс от любого отправителя на номер SMSNUMBER
const char sms_RedirectOff[]     PROGMEM = {"Command: SMS redirection has been turned off."};                       // выполнена команда для выключения перенаправления всех смс от любого отправителя на номер SMSNUMBER
const char sms_SkimpySiren[]     PROGMEM = {"Command: Skimpy siren has been turned on."};                           // выполнена команда для коротковременного включения сирены
const char sms_WasRebooted[]     PROGMEM = {"Command: Device was rebooted."};                                       // выполнена команда для коротковременного включения сирены
const char sms_WrongUssd[]       PROGMEM = {"Command: Wrong USSD code."};                                           // сообщение о неправельной USSD коде
const char sms_BalanceUssd[]     PROGMEM = {"Command: USSD code for getting balance was changed to "};              // выполнена команда для замены gsm команды для получения баланса
const char sms_ErrorSendSms[]    PROGMEM = {"Command: Format of command should be next:\nSendSMS 'number' 'text'"}; // выполнена команда для отправки смс другому абоненту
const char sms_SmsWasSent[]      PROGMEM = {"Command: Sms was sent."};                                              // выполнена команда для отправки смс другому абоненту
const char sms_DelaySiren[]      PROGMEM = {"Command: Delay of siren was changed to "};                             // выполнена команда для изменения паузы между срабатыванием датчиков и включение сирены

// паузы
#define  timeWaitInContr      25                           // время паузы от нажатие кнопки до установки режима охраны
#define  timeWaitInContrTest  7                            // время паузы от нажатие кнопки до установки режима охраны в режиме тестирования
#define  timeHoldingBtn       2                            // время удерживания кнопки для включения режима охраны  2 сек.
#define  timeAfterPressBtn    3000                         // время выполнения операций после единичного нажатия на кнопку
#define  timeSiren            20000                        // время работы сирены (милисекунды).
#define  timeSmsPIR1          120000                       // время паузы после последнего СМС датчика движения 1 (милисекунды)
#define  timeSmsPIR2          120000                       // время паузы после последнего СМС датчика движения 2 (милисекунды)
#define  timeSkimpySiren      300                          // время короткого срабатывания модуля сирены
#define  timeAllLeds          1200                         // время горение всех светодиодов во время включения устройства (тестирования светодиодов)
#define  timeTestBlinkLed     400                          // время мерцания светодиода при включеном режима тестирования
#define  timeRejectCall       3000                         // время пауза перед збросом звонка
#define  timeRefreshVcc       0                            // время паузы после последнего измерения питания (милисекунды)

// Количество нажатий на кнопку для включений режимова
#define countBtnInTestMod   2                              // количество нажатий на кнопку для включение/отключения режима тестирования 
#define countBtnBalance     3                              // количество нажатий на кнопку для запроса баланса счета
#define countBtnSkimpySiren 4                              // количество нажатий на кнопку для кратковременного включения сирены

//// КОНСТАНТЫ ПИТЯНИЯ ////
#define netVcc      10.0                        // значения питяния от сети (вольт)

//// КОНСТАНТЫ ДЛЯ ПИНОВ /////
#define SpecerPin 8
#define gsmLED 13
#define NotInContrLED 12
#define InContrLED 11
#define SirenLED 10
#define BattPowerLED 2                          // LED для сигнализации о работе от резервного питания

#define pinBOOT 5                               // нога BOOT или K на модеме 
#define Button 9                                // нога на кнопку
#define SirenGenerator 7                        // нога на сирену

// Спикер
#define specerTone 98                           // тон спикера

//Power control 
#define pinMeasureVcc A0                        // нога чтения типа питания (БП или батарея)
#define pinMeasureVcc_stub A1                   // нога для заглушки чтения типа питания если резервное пинание не подключено (всегда network)
 
//Sensores
#define SH1 A2                                  // нога на растяжку
#define pinPIR1 4                               // нога датчика движения 1
#define pinPIR2 3                               // нога датчика движения 2

//// КОНСТАНТЫ РЕЖИМОВ РАБОТЫ //// 
#define NotInContrMod  1                        // снята с охраны
#define InContrMod     3                        // установлена охрана

//// КОНСТАНТЫ EEPROM ////
#define E_mode           0                      // адресс для сохранения режимов работы 
#define E_inTestMod      1                      // адресс для сохранения режима тестирования
#define E_isRedirectSms  2                      // адресс для сохранения режима перенаправления всех смс
#define E_wasRebooted    3                      // адресс для сохранения факта перезагрузки устройства по смс команде
#define E_delaySiren     4                      // адресс для сохранения длины паузы между срабатыванием датяиков и включением сирены (в сикундах)

#define E_IsPIR1Enabled  10                     // адресс для сохранения включение/отключения режима 
#define E_IsPIR2Enabled  11                     // адресс для сохранения длины паузы между срабатыванием датяиков и включением сирены (в сикундах)
#define E_TensionCable   12

#define numSize            13                   // количество символов в строке телефонного номера

#define E_BalanceUssd      70                   // Ussd код для запроса баланца

#define E_NumberAnsUssd    85                   // для промежуточного хранения номера телефона, от которого получено gsm код и которому необходимо отправить ответ (баланс и т.д.)

#define E_NUM1_NotInContr  100                  // 1-й номер для снятие с охраны
#define E_NUM2_NotInContr  115                  // 2-й номер для снятие с охраны
#define E_NUM3_NotInContr  130                  // 3-й номер для снятие с охраны

#define E_NUM1_InContr     145                  // 1-й номер для установки на охрану
#define E_NUM2_InContr     160                  // 2-й номер для установки на охрану

#define E_NUM1_SmsCommand  175                  // 1-й номер для управления через sms
#define E_NUM2_SmsCommand  190                  // 2-й номер для управления через sms
#define E_NUM3_SmsCommand  205                  // 3-й номер для управления через sms
  


//// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ////
byte mode = NotInContrMod;                      // 1 - снята с охраны                                  
                                                // 3 - установлено на охрану
                                                // при добавлении не забываем посмотреть рездел //// КОНСТАНТЫ РЕЖИМОВ РАБОТЫ ////
                                               
bool btnIsHolding = false;
bool inTestMod = false;                         // режим тестирования датчиков (не срабатывает сирена и не отправляются СМС)
bool isSiren = false;                           // режим сирены
bool reqSirena = false;                         // уст. в true когда сработал датчик и необходимо включить сирену

unsigned long prSiren = 0;                      // время включения сирены (милисекунды)
unsigned long prAlarmPIR1 = 0;                  // время последнего СМС датчика движения 1 (милисекунды)
unsigned long prAlarmPIR2 = 0;                  // время последнего СМС датчика движения 2 (милисекунды)
unsigned long prLastPressBtn = 0;               // время последнего нажатие на кнопку (милисекунды)
unsigned long prTestBlinkLed = 0;               // время мерцания светодиода при включеном режима тестирования (милисекунды)
unsigned long prRefreshVcc = 0;                 // время последнего измирения питания (милисекунды)
unsigned long prReqSirena = 0;                  // время последнего обнаружения, что необходимо включать сирену

byte countPressBtn = 0;                         // счетчик нажатий на кнопку
bool controlTensionCable = true;                // включаем контроль растяжки
bool wasRebooted = false;                       // указываем была ли последний раз перезагрузка программным путем

// переменные для определения когда необходимо сигнализировать о срабатывании датчиков
bool isAlarmTension = false;                    // true если сработал датчик растяжки 
bool isAlarmPIR1 = false;                       // true если сработал 1-й датчик движения 
bool isAlarmPIR2 = false;                       // true если сработал 2-й датчик движения 


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
  pinMode(pinMeasureVcc_stub, OUTPUT);        // нога для заглушки определения типа питания если резервное пинание не подключено (всегда network)

  digitalWrite(SirenGenerator, HIGH);         // выключаем сирену через релье
   
  // блок сброса очистки EEPROM (сброс всех настроек)
  if (digitalRead(Button) == LOW)
  { 
    byte count = 0;
    while (count < 100)
    {
      if (digitalRead(Button) == HIGH) break;
      count++;
      delay(100);
    }
    if (count == 100)
    {
        PlayTone(specerTone, 1000);               
        for (int i = 0 ; i < EEPROM.length() ; i++) 
          EEPROM.write(i, 0);                           // стираем все данные с EEPROM
        // установка дефолтных параметров
        EEPROM.write(E_mode, NotInContrMod);            // устанавливаем по умолчанию режим не на охране
        EEPROM.write(E_inTestMod, false);               // режим тестирования по умолчанию выключен
        EEPROM.write(E_isRedirectSms, false);           // режим перенаправления всех смс по умолчанию выключен
        EEPROM.write(E_wasRebooted, false);             // факт перезагрузки устройства по умолчанию выключено (устройство не перезагружалось)
        EEPROM.write(E_delaySiren, 0);                  // пауза между сработкой датчиков и включением сирены отключена (0 секунд) 
        EEPROM.write(E_IsPIR1Enabled, true);            
        EEPROM.write(E_IsPIR2Enabled, true);            
        EEPROM.write(E_TensionCable, true);            
        RebootFunc();                                   // перезагружаем устройство
    }
  }  
 
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
  
  analogWrite(pinMeasureVcc_stub, 255);                 // запитываем ногу заглушку питания для заглушки определения типа питания если резервное пинание не подключено (всегда network)
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
  if (GetElapsed(prRefreshVcc) > timeRefreshVcc)                      // проверяем сколько прошло времени после последнего измерения питания (секунды) (выдерживаем паузц между измерениями что б не загружать контроллер)
  {   
    PowerControl();                                                   // мониторим питание системы
    prRefreshVcc = millis();
  }   
  
  gsm.Refresh();                                                      // читаем сообщения от GSM модема   

  if(wasRebooted)
  {    
    SendSms(&GetStringFromFlash(sms_WasRebooted), &NumberRead(E_NUM1_SmsCommand));
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
      if (countPressBtn == countBtnInTestMod)                                          // если кнопку нажали заданное количество для включение/отключения режима тестирования
      {
        PlayTone(specerTone, 250);                                                     // сигнализируем об этом спикером  
        inTestMod = !inTestMod;                                                        // включаем/выключаем режим тестирование датчиков        
        digitalWrite(SirenLED, LOW);                                                   // выключаем светодиод
        EEPROM.write(E_inTestMod, inTestMod);                                          // пишим режим тестирование датчиков в еепром
      }
      else
      // запрос баланса счета
      if (countPressBtn == countBtnBalance)                                            // если кнопку нажали заданное количество для запроса баланса счета
      {
        PlayTone(specerTone, 250);                                                     // сигнализируем об этом спикером                        
        if(gsm.RequestUssd(&ReadFromEEPROM(E_BalanceUssd)))
          WriteToEEPROM(E_NumberAnsUssd, &NumberRead(E_NUM1_SmsCommand));              // сохраняем номер на который необходимо будет отправить ответ                   
        else
          SendSms(&GetStringFromFlash(sms_WrongUssd), &NumberRead(E_NUM1_SmsCommand)); // если ответ пустой то отправляем сообщение об ошибке команды 
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

    if (EEPROM.read(E_TensionCable) && controlTensionCable && SensorTriggered_TensionCable())
    {
      reqSirena = true;
      if (prReqSirena == 1) prReqSirena = millis();
      isAlarmTension = true;      
      controlTensionCable = false;                                                            // выключаем контроль растяжки до следующей установки на охрану (что б смс и звонки совершались единоразово)
    }
    
    if (EEPROM.read(E_IsPIR1Enabled) && SensorTriggered_PIR1())
    {       
      reqSirena = true;
      if (prReqSirena == 1) prReqSirena = millis();
      if (GetElapsed(prAlarmPIR1) > timeSmsPIR1 || prAlarmPIR1 == 0)                         // если выдержена пауза после последнего звонка и отправки смс 
        isAlarmPIR1 = true;
    }

    if (EEPROM.read(E_IsPIR2Enabled) && SensorTriggered_PIR2())
    {      
      reqSirena = true;
      if (prReqSirena == 1) prReqSirena = millis();
      if (GetElapsed(prAlarmPIR2) > timeSmsPIR2 || prAlarmPIR2 == 0)                         // если выдержена пауза после последнего звонка и отправки смс
        isAlarmPIR2 = true;
    }   
    
    if (reqSirena 
      && (inTestMod || GetElapsed(prReqSirena)/1000 >= EEPROM.read(E_delaySiren) || prReqSirena == 0))      
    {     
      reqSirena = false;            
      if (!isSiren)
      {
        StartSiren();
        prReqSirena = 0;                                                                        // устанавливаем в 0 для отключения паузы между следующим срабатыванием датчиков и включением сирены
      }
      else
        prSiren = millis();      
    }      
    
    if (isAlarmTension)                                                                       // проверяем состояние растяжки и если это первое обнаружение обрыва (controlTensionCable = true) то выполняем аналогичные действие
    {                                                  
      if (gsm.IsAvailable())
      {
        if (!inTestMod)    
          gsm.SendSms(&GetStringFromFlash(sms_TensionCable), &NumberRead(E_NUM1_SmsCommand)); 
        gsm.Call(&NumberRead(E_NUM1_NotInContr));      
        isAlarmTension = false;
      }                                                    
    }
    
    if (isAlarmPIR1)                                                                          // проверяем состояние 1-го датчика движения
    {                                                                 
      if (gsm.IsAvailable())              
      {  
        if (!inTestMod)  
          gsm.SendSms(&GetStringFromFlash(sms_PIR1), &NumberRead(E_NUM1_SmsCommand));         // если не включен режим тестирование отправляем смс
        gsm.Call(&NumberRead(E_NUM1_NotInContr));                                             // сигнализируем звонком о сработке датчика движения
        prAlarmPIR1 = millis();
        isAlarmPIR1 = false;
      }
    }
    
    if (isAlarmPIR2)                                                                          // проверяем состояние 2-го датчика движения
    {      
      if (gsm.IsAvailable())
      {  
        if (!inTestMod)
          gsm.SendSms(&GetStringFromFlash(sms_PIR2), &NumberRead(E_NUM1_SmsCommand));
        gsm.Call(&NumberRead(E_NUM1_NotInContr));
        prAlarmPIR2 = millis();
        isAlarmPIR2 = false;
      }
    }
    
    if (gsm.NewRing)                                                                  // если обнаружен входящий звонок
    {      
      if (NumberRead(E_NUM1_NotInContr).indexOf(gsm.RingNumber) > -1 ||               // если найден зарегистрированный звонок то снимаем с охраны
          NumberRead(E_NUM2_NotInContr).indexOf(gsm.RingNumber) > -1 || 
          NumberRead(E_NUM3_NotInContr).indexOf(gsm.RingNumber) > -1          
         )               
      {                    
        delay(timeRejectCall);                                                        // пауза перед збросом звонка
        Set_NotInContrMod();                                                          // снимаем с охраны         
        gsm.RejectCall();                                                             // сбрасываем вызов        
      }
      else
      if ((NumberRead(E_NUM1_InContr).indexOf(gsm.RingNumber) > -1 ||
           NumberRead(E_NUM2_InContr).indexOf(gsm.RingNumber) > -1) && 
          reqSirena &&
          !isSiren
          )                // если найден зарегистрированный звонок то снимаем с охраны
      {
        reqSirena = false;        
        StartSiren();     
        prReqSirena = 0;                                                              // устанавливаем в 0 для отключения паузы между следующим срабатыванием датчиков и включением сирены   
      }
      else gsm.RejectCall();                                                          // если не найден зарегистрированный звонок то сбрасываем вызов (без паузы)
      gsm.ClearRing();                                                                // очищаем обнаруженный входящий звонок 
    }         
  }                                                                                   // end InContrMod   
  
  if (gsm.NewUssd)                                                                    // если доступный новый ответ на Ussd запрос
  {
    SendSms(&gsm.UssdText, &NumberRead(E_NumberAnsUssd));                             // отправляем ответ на Ussd запрос
    gsm.ClearUssd();                                                                  // сбрасываем ответ на gsm команду 
  }
  if (!isSiren) ExecSmsCommand();                                                     // если не сирена проверяем доступна ли новая команда по смс и если да то выполняем ее
}



//// ------------------------------- Functions --------------------------------- ////

// подсчет сколько прошло милисикунд после последнего срабатывания события (сирена, звонок и т.д.)
unsigned long GetElapsed(unsigned long &prEventMillis)
{
  unsigned long tm = millis();
  return (tm >= prEventMillis) ? tm - prEventMillis : 0xFFFFFFFF - prEventMillis + tm + 1;  //возвращаем милисикунды после последнего события
}

bool Set_NotInContrMod()                                // метод для снятие с охраны
{
  digitalWrite(NotInContrLED, HIGH);
  digitalWrite(InContrLED, LOW);
  digitalWrite(SirenLED, LOW);
  PlayTone(specerTone, 500);
  mode = NotInContrMod;                                 // снимаем с охраны
  StopSiren();                                          // выключаем сирену
  prAlarmPIR1 = 0;                                      // сбрасываем счетчики временных пауз в ноль
  prAlarmPIR2 = 0;
  isAlarmTension = false;
  isAlarmPIR1 = false;
  isAlarmPIR2 = false;
  reqSirena = false; 
  EEPROM.write(E_mode, mode);                           // пишим режим в еепром, что б при следующем включении устройства, оно оставалось в данном режиме
  return true;
}

bool Set_InContrMod(bool IsWaiting)                     // метод для установки на охрану
{ 
  if (IsWaiting == true)                                // если включен режим ожидание перед установкой охраны, выдерживаем заданную паузу, что б успеть покинуть помещение
  {    
    digitalWrite(NotInContrLED, LOW);   
    
    byte timeWait = 0;
    if (inTestMod) timeWait = timeWaitInContrTest;      // если включен режим тестирования, устанавливаем для удобства тестирования меньшую паузу
    else timeWait = timeWaitInContr;                    // если режим тестирования выклюяен, используем обычную паузу
    
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
  prAlarmPIR1 = 0;
  prAlarmPIR2 = 0;
  isAlarmTension = false;
  isAlarmPIR1 = false;
  isAlarmPIR2 = false; 
  
  //установка на охрану                                                       
  digitalWrite(NotInContrLED, LOW);
  digitalWrite(InContrLED, HIGH);
  digitalWrite(SirenLED, LOW);  
  PlayTone(specerTone, 500);
  mode = InContrMod;                                    // ставим на охрану  
  EEPROM.write(E_mode, mode);                           // пишим режим в еепром, что б при следующем включении устройства, оно оставалось в данном режиме
  delay (2500);                                         // дополнительная пауза так как датчик держит лог. единицу 2,5
  prReqSirena = 1;                                      // устанавливаем в 1 для активации паузы между срабатыванием датчиков и включением сирены
  return true;
}


void  StartSiren()
{
  digitalWrite(SirenLED, HIGH);
  if (!inTestMod)                                        // если не включен тестовый режим
    digitalWrite(SirenGenerator, LOW);                   // включаем сирену через релье
  else
    PlayTone(specerTone, 100);                           // если включен режим тестирование то сигнализируем только спикером
  isSiren = true; 
  prSiren = millis(); 
}


void  StopSiren()
{
  digitalWrite(SirenLED, LOW);
  digitalWrite(SirenGenerator, HIGH);                    // выключаем сирену через релье
  isSiren = false;   
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
bool SensorTriggered_TensionCable()                                     // метод проверки состояния растяжки
{
  if (digitalRead(SH1) == HIGH) return true;
  else return false;
}

bool SensorTriggered_PIR1()                                             // метод проверки состояния датчик движения 1
{
  if (digitalRead(pinPIR1) == HIGH) return true;
  else return false;
}

bool SensorTriggered_PIR2()                                             // метод проверки состояния датчик движения 2
{
  if (digitalRead(pinPIR2) == HIGH) return true;
  else return false;
}

void BlinkLEDhigh(byte pinLED,  unsigned int millisBefore,  unsigned int millisHIGH,  unsigned int millisAfter)        // метод для включения заданного светодиода на заданное время
{ 
  digitalWrite(pinLED, LOW);                          
  delay(millisBefore);  
  digitalWrite(pinLED, HIGH);                                           
  delay(millisHIGH); 
  digitalWrite(pinLED, LOW);
  delay(millisAfter);
}

void BlinkLEDlow(byte pinLED,  unsigned int millisBefore,  unsigned int millisLOW,  unsigned int millisAfter)         // метод для выключения заданного светодиода на заданное время        
{ 
  digitalWrite(pinLED, HIGH);                          
  delay(millisBefore);  
  digitalWrite(pinLED, LOW);                                           
  delay(millisLOW); 
  digitalWrite(pinLED, HIGH);
  delay(millisAfter);
}

void BlinkLEDSpecer(byte pinLED,  unsigned int millisBefore,  unsigned int millisHIGH,  unsigned int millisAfter)     // метод для включения спикера и заданного светодиода на заданное время
{ 
  digitalWrite(pinLED, LOW);                          
  delay(millisBefore);  
  digitalWrite(pinLED, HIGH);                                          
  PlayTone(specerTone, millisHIGH);
  digitalWrite(pinLED, LOW);
  delay(millisAfter);
}

void PowerControl()                                                                                                   // метод для обработки событий питания системы (переключение на батарею или на сетевое)
{
  powCtr.Refresh();    
  digitalWrite(BattPowerLED, powCtr.IsBattPower);
        
  if (!inTestMod && !powCtr.IsBattPowerPrevious && powCtr.IsBattPower)                    // если предыдущий раз было от сети а сейчас от батареи (пропало сетевое питание 220v) и если не включен режим тестирования
    SendSms(&GetStringFromFlash(sms_BattPower), &NumberRead(E_NUM1_SmsCommand));          // отправляем смс о переходе на резервное питание         
   
  if (!inTestMod && powCtr.IsBattPowerPrevious && !powCtr.IsBattPower)                    // если предыдущий раз было от батареи a сейчас от сети (сетевое питание 220v возобновлено) и если не включен режим тестирования  
    SendSms(&GetStringFromFlash(sms_NetPower), &NumberRead(E_NUM1_SmsCommand));           // отправляем смс о возобновлении  сетевое питание 220v 
}

void SkimpySiren()                                                                        // метод для кратковременного включения сирены (для теститования сирены)
{
  digitalWrite(SirenLED, HIGH);
  digitalWrite(SirenGenerator, LOW);                                                      // включаем сирену через релье
  delay(timeSkimpySiren);                                                                 // кратковременный период на который включается сирена
  digitalWrite(SirenLED, LOW);
  digitalWrite(SirenGenerator, HIGH);                                                     // выключаем сирену через релье  
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

void WriteToEEPROM(byte e_addr, String *number)
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

bool SendSms(String *text, String *phone)      // собственный метод отправки СМС (возвращает true если смс отправлена успешно) создан для инкапсуляции сигнализации об отправки смс
{
  if(gsm.SendSms(text, phone))                 // если смс отправлено успешно 
  {  
    PlayTone(specerTone, 250);                 // сигнализируем об этом
    return true;
  }
  else return false;
}

// читаем смс и если доступна новая команда по смс то выполняем ее
void ExecSmsCommand()
{ 
  if (gsm.NewSms)
  {
    if ((gsm.SmsNumber.indexOf(NumberRead(E_NUM1_SmsCommand)) > -1 ||                    // если обнаружено зарегистрированый номер
         gsm.SmsNumber.indexOf(NumberRead(E_NUM2_SmsCommand)) > -1 ||
         gsm.SmsNumber.indexOf(NumberRead(E_NUM3_SmsCommand)) > -1
        ) 
        ||
        (NumberRead(E_NUM1_SmsCommand).startsWith("***")  &&                             // если нет зарегистрированных номеров (при первом включении необходимо зарегистрировать номера)
         NumberRead(E_NUM2_SmsCommand).startsWith("***")  &&
         NumberRead(E_NUM3_SmsCommand).startsWith("***")
         )
       )
    { 
      gsm.SmsText.toLowerCase();                                                         // приводим весь текст команды к нижнему регистру что б было проще идентифицировать команду
      gsm.SmsText.trim();                                                                // удаляем пробелы в начале и в конце комманды
      
      if (gsm.SmsText.startsWith("*") || gsm.SmsText.startsWith("#"))                    // если сообщение начинается на * или # то это Ussd запрос
      {
        PlayTone(specerTone, 250); 
        if (gsm.RequestUssd(&gsm.SmsText))                                               // отправляем Ussd запрос и если он валидный (запрос заканчиваться на #)             
          WriteToEEPROM(E_NumberAnsUssd, &gsm.SmsNumber);                                // то сохраняем номер на который необходимо будет отправить ответ от Ussd запроса                                                    
        else
          SendSms(&GetStringFromFlash(sms_WrongUssd), &gsm.SmsNumber);                   // иначе отправляем сообщение об инвалидном Ussd запросе 
      }
      else
      if (gsm.SmsText.startsWith("sendsms"))                                             // если обнаружена смс команда для отправки смс другому абоненту
      {
        PlayTone(specerTone, 250); 
        String number = "";                                                              // переменная для хранения номера получателя
        String text = "";                                                                // переменная для хранения текста смс
        String str = gsm.SmsText;
        
        // достаем номер телефона кому отправляем смс
        int beginStr = str.indexOf('\'');                                                                                            
        if (beginStr > 0)                                                                // проверяем, что прискутсвуют параметры SendSMS  команды (номер и текст перенаправляемой смс)
        {
          str = str.substring(beginStr + 1);                                             // достаем номер получателя
          int duration = str.indexOf('\'');  
          number = str.substring(0, duration);      
          str = str.substring(duration +1);
          
          beginStr = 0;                                                                  // достаем текст смс, который необходимо отправить получателю
          duration = 0;
          beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          duration = str.indexOf('\'');  
          text = str.substring(0, duration);
         }
        number.trim();
        if (number.length() > 0)                                                         // проверяем что номер получателя не пустой (смс текст не проверяем так как перенаправление пустого смс возможно)
        {
          if(SendSms(&text, &number))                                                    // перенаправляем смс указанному получателю и если сообщение перенаправлено успешно
            SendSms(&GetStringFromFlash(sms_SmsWasSent), &gsm.SmsNumber);                // то отправляем отчет об успешном выполнении комманды          
        }
        else
          SendSms(&GetStringFromFlash(sms_ErrorSendSms), &gsm.SmsNumber);                // если номер получателя не обнаружен (пустой) то отправляем сообщение с правильным форматом комманды 
      }
      else      
      if (gsm.SmsText == "balance")                                                      // если обнаружена смс команда для запроса баланса
      {        
        digitalWrite(SirenLED, LOW);                                                     // выключаем светодиод, который может моргать если включен тестовый режим
        PlayTone(specerTone, 250); 
        if(gsm.RequestUssd(&ReadFromEEPROM(E_BalanceUssd)))                              // отправляем Ussd запрос для получения баланса и если он валидный (запрос заканчиваться на #)    
          WriteToEEPROM(E_NumberAnsUssd, &gsm.SmsNumber);                                // то сохраняем номер на который необходимо будет отправить баланс                 
        else
          SendSms(&GetStringFromFlash(sms_WrongUssd), &gsm.SmsNumber);                   // иначе отправляем сообщение об инвалидном Ussd запросе 
      }
      else
      if (gsm.SmsText.startsWith("test on"))                                             // если обнаружена смс команда для включения тестового режима для тестирования датчиков
      {        
        digitalWrite(SirenLED, LOW);                                                     // выключаем светодиод, который может моргать если включен тестовый режим
        PlayTone(specerTone, 250); 
        inTestMod = true;
        EEPROM.write(E_inTestMod, true);                                                 // пишим режим тестирование датчиков в еепром 
        SendSms(&GetStringFromFlash(sms_TestModOn), &gsm.SmsNumber);                     // отправляем смс о завершении выполнения даной смс команды                                                         
      }
      else
      if (gsm.SmsText.startsWith("test off"))                                            // если обнаружена смс команда для выключения тестового режима для тестирования датчиков
      {
        digitalWrite(SirenLED, LOW);                                                     // выключаем светодиод, который может моргать если включен тестовый режим                                                 
        PlayTone(specerTone, 250); 
        inTestMod = false;
        EEPROM.write(E_inTestMod, false);                                                // пишим режим тестирование датчиков в еепром 
        SendSms(&GetStringFromFlash(sms_TestModOff), &gsm.SmsNumber);                    // отправляем смс о завершении выполнения даной смс команды                 
      }            
      else
      if (gsm.SmsText.startsWith("control on"))                                          // если обнаружена смс команда для установки на охрану
      {        
        digitalWrite(SirenLED, LOW);                                                     // выключаем светодиод, который может моргать если включен тестовый режим
        Set_InContrMod(false);                                                           // устанавливаем на охрану без паузы                                                
        SendSms(&GetStringFromFlash(sms_InContrMod), &gsm.SmsNumber);                    // отправляем смс о завершении выполнения даной смс команды                           
      }
      else 
      if (gsm.SmsText.startsWith("control off"))                                         // если обнаружена смс команда для снятие с охраны
      {
        digitalWrite(SirenLED, LOW);                                                     // выключаем светодиод, который может моргать если включен тестовый режим
        Set_NotInContrMod();                                                             // снимаем с охраны
        SendSms(&GetStringFromFlash(sms_NotInContrMod), &gsm.SmsNumber);                 // отправляем смс о завершении выполнения даной смс команды
      }      
      else
      if (gsm.SmsText.startsWith("redirect on"))                                         // если обнаружена смс команда для включения режима "перенапралять входящие смс от незарегистрированных номеров на номер SmsCommand1" 
      {
        PlayTone(specerTone, 250);
        EEPROM.write(E_isRedirectSms, true);         
        SendSms(&GetStringFromFlash(sms_RedirectOn), &gsm.SmsNumber);                                          
      }
      else 
      if (gsm.SmsText.startsWith("redirect off"))                                        // если обнаружена смс команда для выключения режима "перенапралять входящие смс от незарегистрированных номеров на номер SmsCommand1" 
      {
        PlayTone(specerTone, 250);
        EEPROM.write(E_isRedirectSms, false);          
        SendSms(&GetStringFromFlash(sms_RedirectOff), &gsm.SmsNumber);       
      }
      else
      if (gsm.SmsText.startsWith("skimpy"))                                              // если обнаружена смс команда для кратковременного включения сирены (для теститования сирены)
      {
        SkimpySiren();
        SendSms(&GetStringFromFlash(sms_SkimpySiren), &gsm.SmsNumber);   
      }
      else
      if (gsm.SmsText.startsWith("reboot"))                                              // если обнаружена смс команда для перезагрузки устройства
      {
        PlayTone(specerTone, 250);
        EEPROM.write(E_wasRebooted, true);                                               // записываем статус, что устройство перезагружается        
        gsm.Shutdown();                                                                  // выключаем gsm модуль
        RebootFunc();                                                                    // вызываем Reboot arduino платы
      }
      else
      if (gsm.SmsText.startsWith("status"))                                              // если обнаружена смс команда для запроса статуса режимов и настроек устройства  
      {
        PlayTone(specerTone, 250);        
        String msg = "On controlling: "   + String((mode == InContrMod) ? "on" : "off") + "\n"
                   + "Test mode: "        + String((inTestMod) ? "on" : "off") + "\n" 
                   + "Redirect SMS: "     + String((EEPROM.read(E_isRedirectSms)) ? "on" : "off") + "\n"
                   + "Power: "            + String((powCtr.IsBattPower) ? "battery" : "network") + "\n"
                   + "DelaySiren: "       + String(EEPROM.read(E_delaySiren)) + " sec";
        SendSms(&msg, &gsm.SmsNumber);          
      }           
      else
      if (gsm.SmsText.startsWith("delaysiren"))                                          // если обнаружена смс команда для установки длины паузы между срабатыванием датчиков и включение сирены  
      {
        PlayTone(specerTone, 250);
        String str = gsm.SmsText;
        int beginStr = str.indexOf('\'');
        str = str.substring(beginStr + 1);
        int duration = str.indexOf('\'');  
        str = str.substring(0, duration);             
        if (beginStr <= 0 || duration <= 0 || str.length() == 0)
          str = '0'; 
        EEPROM.write(E_delaySiren, str.toInt());
        String msg = GetStringFromFlash(sms_DelaySiren) + String(EEPROM.read(E_delaySiren)) + " sec.";
        SendSms(&msg, &gsm.SmsNumber);                                                   // отправляем смс о завершении выполнения даной смс команды (какой Ussd запрос был установлен для получения баланса)      
      }
      else 
      if(gsm.SmsText.startsWith("balanceussd"))                                          // если обнаружена смс команда для установки тескта Ussd запроса для получение баланса по команде balance или используя кнопку 
      {
        PlayTone(specerTone, 250);
        String str = gsm.SmsText;
        int beginStr = str.indexOf('\'');
        str = str.substring(beginStr + 1);
        int duration = str.indexOf('\'');  
        str = str.substring(0, duration);             
        if (beginStr <= 0 || duration <= 0 || str.length() == 0)
          str = ""; 
        WriteToEEPROM(E_BalanceUssd, &str);
        String msg = GetStringFromFlash(sms_BalanceUssd) + "'" + ReadFromEEPROM(E_BalanceUssd) + "'";
        SendSms(&msg, &gsm.SmsNumber);                                                   // отправляем смс о завершении выполнения даной смс команды (какой Ussd запрос был установлен для получения баланса)         
      }     
      else
      if (gsm.SmsText.startsWith("notincontr1"))                                         // если обнаружена смс команда для регистрации группы телефонов для снятие с охраны
      {
        PlayTone(specerTone, 250);                      
        String nums[3];
        String str = gsm.SmsText;;
        for(int i = 0; i < 3; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');  
          nums[i] = str.substring(0, duration);      
          str = str.substring(duration +1);            
        }        
        WriteToEEPROM(E_NUM1_NotInContr , &nums[0]);        
        WriteToEEPROM(E_NUM2_NotInContr, &nums[1]);  
        WriteToEEPROM(E_NUM3_NotInContr, &nums[2]);          
        String msg = "NotInContr1:\n'" + NumberRead(E_NUM1_NotInContr) + "'" + "\n"
                   + "NotInContr2:\n'" + NumberRead(E_NUM2_NotInContr) + "'" + "\n"
                   + "NotInContr3:\n'" + NumberRead(E_NUM3_NotInContr) + "'";
        SendSms(&msg, &gsm.SmsNumber);    
      }
      else     
      if (gsm.SmsText.startsWith("incontr1"))                                           // если обнаружена смс команда для регистрации группы телефонов для установки на охрану
      {
        PlayTone(specerTone, 250);                     
        String nums[2];
        String str = gsm.SmsText;         
        for(int i = 0; i < 2; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');  
          nums[i] = str.substring(0, duration);      
          str = str.substring(duration +1);             
        }              
        WriteToEEPROM(E_NUM1_InContr, &nums[0]);  
        WriteToEEPROM(E_NUM2_InContr, &nums[1]);
        String msg = "InContr1:\n'" + NumberRead(E_NUM1_InContr) + "'" + "\n"
                   + "InContr2:\n'" + NumberRead(E_NUM2_InContr) + "'";
        SendSms(&msg, &gsm.SmsNumber);               
      }
      else
      if (gsm.SmsText.startsWith("smscommand1"))                                        // если обнаружена смс команда для регистрации группы телефонов для управления через смс команды
      {
        PlayTone(specerTone, 250);                     
        String nums[3];
        String str = gsm.SmsText;        
        for(int i = 0; i < 3; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');  
          nums[i] = str.substring(0, duration);      
          str = str.substring(duration +1);         
        }        
        WriteToEEPROM(E_NUM1_SmsCommand, &nums[0]);  
        WriteToEEPROM(E_NUM2_SmsCommand, &nums[1]);
        WriteToEEPROM(E_NUM3_SmsCommand, &nums[2]);        
        String msg = "SmsCommand1:\n'" + NumberRead(E_NUM1_SmsCommand) + "'" + "\n"
                   + "SmsCommand2:\n'" + NumberRead(E_NUM2_SmsCommand) + "'" + "\n"
                   + "SmsCommand3:\n'" + NumberRead(E_NUM3_SmsCommand) + "'";
        SendSms(&msg, &gsm.SmsNumber);     
      }
      else
      if (gsm.SmsText.startsWith("pir1"))
      {
        PlayTone(specerTone, 250);                     
        bool nums[3];
        String str = gsm.SmsText;        
        for(int i = 0; i < 3; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');  
          if (str.substring(0, duration) == "off")
            nums[i] = false;      
          else if (str.substring(0, duration) == "on")
            nums[i] = true; 
          str = str.substring(duration +1);         
        }        
        EEPROM.write(E_IsPIR1Enabled, &nums[0]);
        EEPROM.write(E_IsPIR2Enabled, &nums[1]);
        EEPROM.write(E_TensionCable,  &nums[2]);               
       /* String msg = "Sensors:"
                   +  "PIR1: '"         + String((EEPROM.read(E_IsPIR1Enabled)) ? "on" : "off") + "'" + "\n"
                   +  "PIR2: '"         + String((EEPROM.read(E_IsPIR2Enabled)) ? "on" : "off") + "'" + "\n"
                   +  "TensionCable: '" + String((EEPROM.read(E_TensionCable)) ? "on" : "off")  + "'";
        SendSms(&msg, &gsm.SmsNumber);  */   
      }
      else            
      if (gsm.SmsText.startsWith("notincontr"))                                         // если обнаружена смс команда для запроса списка зарегистрированных телефонов для снятие с охраны
      {
        PlayTone(specerTone, 250);        
        String msg = "NotInContr1:\n'" + NumberRead(E_NUM1_NotInContr) + "'" + "\n"
                   + "NotInContr2:\n'" + NumberRead(E_NUM2_NotInContr) + "'" + "\n"
                   + "NotInContr3:\n'" + NumberRead(E_NUM3_NotInContr) + "'";
        SendSms(&msg, &gsm.SmsNumber);                    
      }
      else
      if (gsm.SmsText.startsWith("incontr"))                                            // если обнаружена смс команда для запроса списка зарегистрированных телефонов для установки на охрану
      {
        PlayTone(specerTone, 250);       
        String msg = "InContr1:\n'" + NumberRead(E_NUM1_InContr) + "'" + "\n"
                   + "InContr2:\n'" + NumberRead(E_NUM2_InContr) + "'";    
        SendSms(&msg, &gsm.SmsNumber);
      }
      else
      if (gsm.SmsText.startsWith("smscommand"))                                         // если обнаружена смс команда для запроса списка зарегистрированных телефонов для управления через смс команды
      {
        PlayTone(specerTone, 250);       
        String msg = "SmsCommand1:\n'" + NumberRead(E_NUM1_SmsCommand) + "'" + "\n"
                   + "SmsCommand2:\n'" + NumberRead(E_NUM2_SmsCommand) + "'" + "\n" 
                   + "SmsCommand3:\n'" + NumberRead(E_NUM3_SmsCommand) + "'";
        SendSms(&msg, &gsm.SmsNumber);
      }
      else
      if (gsm.SmsText.startsWith("sensor"))
      {
        PlayTone(specerTone, 250);
        String msg = "Sensors:"
                     +  "PIR1: \'"         + String((EEPROM.read(E_IsPIR1Enabled)) ? "on" : "off") + "'" + "\n"
                     +  "PIR2: \'"         + String((EEPROM.read(E_IsPIR2Enabled)) ? "on" : "off") + "'" + "\n"
                     +  "TensionCable: \'" + String((EEPROM.read(E_TensionCable)) ? "on" : "off")  + "'" ;
        SendSms(&msg, &gsm.SmsNumber);  

        
        SendSms(&msg, &gsm.SmsNumber);  
      }
      else                                                                              // если смс команда не распознана
      {
        PlayTone(specerTone, 250);              
        SendSms(&GetStringFromFlash(sms_ErrorCommand), &gsm.SmsNumber);                 // то отправляем смс со списком всех доступных смс команд
      }                                                                                 
    }    
    else if (EEPROM.read(E_isRedirectSms))                                              // если смс пришла не с зарегистрированых номеров и включен режим перенаправления всех смс
    {
      SendSms(&String(gsm.SmsText), &NumberRead(E_NUM1_SmsCommand));                    // перенаправляем смс на зарегистрированный номер под именем SmsCommand1
    }    
  gsm.ClearSms(); 
  }  
}


