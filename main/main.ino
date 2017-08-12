/// GSM сигналка c установкой по кнопке
/// с датчиками движения и растяжкой (или с геркониевым датчиком)
/// ВНИМАНИЕ: для корретной работы sms необходимо установить размеры буферов вместо 64 на SERIAL_TX_BUFFER_SIZE 24 и SERIAL_RX_BUFFER_SIZE 170 в файле hardware\arduino\avr\cores\arduino\HardwareSerial.h

#include <EEPROM.h>
#include "DigitalSensor.h"
#include "MyGSM.h"
#include "PowerControl.h"
#include <avr/pgmspace.h>

#define debug Serial

//// НАСТРОЕЧНЫЕ КОНСТАНТЫ /////
const char sms_TensionCable[]    PROGMEM = {"ALARM: TensionCable sensor."};                                         // текст смс для растяжки
const char sms_PIR1[]            PROGMEM = {"ALARM: PIR1 sensor."};                                                 // текст смс для датчика движения 1
const char sms_PIR2[]            PROGMEM = {"ALARM: PIR2 sensor."};                                                 // текст смс для датчика движения 2
const char sms_Gas[]             PROGMEM = {"ALARM: Gas sensor."};                                                  // текст смс для датчика газа/дыма

const char sms_BattPower[]       PROGMEM = {"POWER: Backup Battery is used for powering system."};                  // текст смс для оповещения о том, что исчезло сетевое питание
const char sms_NetPower[]        PROGMEM = {"POWER: Network power has been restored."};                             // текст смс для оповещения о том, что сетевое питание возобновлено

const char sms_ErrorCommand[]    PROGMEM = {"SendSMS,\nBalance,\nTestOn(Off),\nControlOn(Off),\nRedirectOn(Off),\nSkimpy,\nStatus,\nReboot,\nSettings,\nSensors,\nOutOfContr,\nOnContr,\nSmsCommand."};  // смс команда не распознана
const char sms_TestModOn[]       PROGMEM = {"Command: Test mode has been turned on."};                              // выполнена команда для включения тестового режима для тестирования датчиков
const char sms_TestModOff[]      PROGMEM = {"Command: Test mode has been turned off."};                             // выполнена команда для выключения тестового режима для тестирования датчиков
const char sms_OnContrMod[]      PROGMEM = {"Command: Control mode has been turned on."};                           // выполнена команда для установку на охрану
const char sms_OutOfContrMod[]   PROGMEM = {"Command: Control mode has been turned off."};                          // выполнена команда для установку на охрану
const char sms_RedirectOn[]      PROGMEM = {"Command: SMS redirection has been turned on."};                        // выполнена команда для включения перенаправления всех смс от любого отправителя на номер SMSNUMBER
const char sms_RedirectOff[]     PROGMEM = {"Command: SMS redirection has been turned off."};                       // выполнена команда для выключения перенаправления всех смс от любого отправителя на номер SMSNUMBER
const char sms_SkimpySiren[]     PROGMEM = {"Command: Skimpy siren has been turned on."};                           // выполнена команда для коротковременного включения сирены
const char sms_WasRebooted[]     PROGMEM = {"Command: Device was rebooted."};                                       // выполнена команда для коротковременного включения сирены
const char sms_WrongUssd[]       PROGMEM = {"Command: Wrong USSD code."};                                           // сообщение о неправельной USSD коде
const char sms_ErrorSendSms[]    PROGMEM = {"Command: Format of command should be next:\nSendSMS 'number' 'text'"}; // выполнена команда для отправки смс другому абоненту
const char sms_SmsWasSent[]      PROGMEM = {"Command: Sms was sent."};                                              // выполнена команда для отправки смс другому абоненту

// Строки для смс команд
const char sendsms[]             PROGMEM = {"sendsms"};                                                             
const char balance[]             PROGMEM = {"balance"};                                                             
const char teston[]              PROGMEM = {"teston"};
const char testoff[]             PROGMEM = {"testoff"};
const char controlon[]           PROGMEM = {"controlon"};
const char controloff[]          PROGMEM = {"controloff"};
const char redirecton[]          PROGMEM = {"redirecton"};
const char redirectoff[]         PROGMEM = {"redirectoff"};
const char skimpy[]              PROGMEM = {"skimpy"};
const char reboot[]              PROGMEM = {"reboot"};
const char _status[]             PROGMEM = {"status"};
const char outofcontr1[]         PROGMEM = {"outofcontr1"};
const char oncontr1[]            PROGMEM = {"oncontr1"};
const char smscommand1[]         PROGMEM = {"smscommand1"};
const char outofcontr[]          PROGMEM = {"outofcontr"};
const char oncontr[]             PROGMEM = {"oncontr"};
const char smscommand[]          PROGMEM = {"smscommand"};
const char settings[]            PROGMEM = {"setting"};
const char sensor[]              PROGMEM = {"sensor"};
const char delaySiren[]          PROGMEM = {"delaysiren"};
const char _PIR1[]               PROGMEM = {"pir1"};

// Строки для формирования смс ответов на смс команды Status и Settings
const char control[]             PROGMEM = {"On controlling: "}; 
const char test[]                PROGMEM = {"Test mode: "}; 
const char redirSms[]            PROGMEM = {"Redirect SMS: "}; 
const char power[]               PROGMEM = {"Power: "}; 
const char delSiren[]            PROGMEM = {"DelaySiren: "}; 
const char PIR1[]                PROGMEM = {"PIR1: "}; 
const char PIR2[]                PROGMEM = {"PIR2: "};
const char Gas[]                 PROGMEM = {"Gas: "}; 
const char tension[]             PROGMEM = {"Tension: "};
const char siren[]               PROGMEM = {"Siren: "};
const char idle[]                PROGMEM = {"Idle"};
const char on[]                  PROGMEM = {"on"};
const char off[]                 PROGMEM = {"off"};
const char battery[]             PROGMEM = {"battery"};
const char network[]             PROGMEM = {"network"};
const char sec[]                 PROGMEM = {" sec."};
const char minut[]               PROGMEM = {" min."};
const char hour[]                PROGMEM = {" hours."};
const char delOnContr[]          PROGMEM = {"DelayOnContr: "};
const char intervalVcc[]         PROGMEM = {"IntervalVcc: "};
const char balanceUssd[]         PROGMEM = {"BalanceUssd: "};

// паузы
#define  delayOnContrTest     7                            // время паузы от нажатие кнопки до установки режима охраны в режиме тестирования
#define  timeAfterPressBtn    3000                         // время выполнения операций после единичного нажатия на кнопку
#define  timeSiren            20000                        // время работы сирены (милисекунды).
#define  timeSmsPIR1          120000                       // время паузы после последнего СМС датчика движения 1 (милисекунды)
#define  timeSmsPIR2          120000                       // время паузы после последнего СМС датчика движения 2 (милисекунды)
#define  timeSmsGas           120000                       // время паузы после последнего СМС датчика газа/дыма (милисекунды)
#define  timeSkimpySiren      300                          // время короткого срабатывания модуля сирены
#define  timeAllLeds          1200                         // время горение всех светодиодов во время включения устройства (тестирования светодиодов)
#define  timeTestBlinkLed     400                          // время мерцания светодиода при включеном режима тестирования
#define  timeRejectCall       3000                         // время пауза перед збросом звонка

// Количество нажатий на кнопку для включений режимова
#define countBtnOnContr      1                              // количество нажатий на кнопку для установки на охрану
#define countBtnInTestMod    2                              // количество нажатий на кнопку для включение/отключения режима тестирования 
#define countBtnBalance      3                              // количество нажатий на кнопку для запроса баланса счета
#define countBtnSkimpySiren  4                              // количество нажатий на кнопку для кратковременного включения сирены

//// КОНСТАНТЫ ДЛЯ ПИНОВ /////
#define SpecerPin 8
#define gsmLED 13
#define OutOfContrLED 12
#define OnContrLED 11
#define SirenLED 10
#define BattPowerLED 9                          // LED для сигнализации о работе от резервного питания

#define pinBOOT 5                               // нога BOOT или K на модеме 
#define Button 2                                // нога на кнопку
#define SirenGenerator 7                        // нога на сирену

// Спикер
#define sysTone 98                              // системный тон спикера
#define clickTone 98                            // тон спикера при нажатии на кнопку
#define smsSpecDur 100                          // длительность сигнала при получении смс команд и отправки ответа 

//Power control 
#define pinMeasureVcc A0                        // нога чтения типа питания (БП или батарея)
#define pinMeasureVcc_stub A1                   // нога для заглушки чтения типа питания если резервное пинание не подключено (всегда network)
#define netVcc      10.0                        // значения питяния от сети (вольт)
#define battVcc     0.1                         // значения питяния от батареи (вольт)

//Sensores
#define pinSH1 A2                               // нога на растяжку
#define pinPIR1 4                               // нога датчика движения 1
#define pinPIR2 3                               // нога датчика движения 2
#define pinGas  6                               // нога датчика газа/дыма 6

//// КОНСТАНТЫ РЕЖИМОВ РАБОТЫ //// 
#define OutOfContrMod  1                        // снята с охраны
#define OnContrMod     3                        // установлена охрана

//// КОНСТАНТЫ EEPROM ////
#define E_mode           0                      // адресс для сохранения режимов работы 
#define E_inTestMod      1                      // адресс для сохранения режима тестирования
#define E_isRedirectSms  2                      // адресс для сохранения режима перенаправления всех смс
#define E_wasRebooted    3                      // адресс для сохранения факта перезагрузки устройства по смс команде

#define E_delaySiren     4                      // адресс для сохранения длины паузы между срабатыванием датяиков и включением сирены (в сикундах)
#define E_delayOnContr   6                      // время паузы от нажатия кнопки до установки режима охраны (в сикундах)
#define E_intervalVcc    8                      // интервал между измерениями питания (в сикундах)

#define E_SirenEnabled   10
#define E_IsPIR1Enabled  11                     
#define E_IsPIR2Enabled  12
#define E_IsGasEnabled   13                   
#define E_TensionEnabled 14

#define numSize            15                   // количество символов в строке телефонного номера

#define E_BalanceUssd      60                   // Ussd код для запроса баланца

#define E_NumberAnsUssd    80                   // для промежуточного хранения номера телефона, от которого получено gsm код и которому необходимо отправить ответ (баланс и т.д.)

#define E_NUM1_OutOfContr  100                  // 1-й номер для снятие с охраны
#define E_NUM2_OutOfContr  117                  // 2-й номер для снятие с охраны
#define E_NUM3_OutOfContr  134                  // 3-й номер для снятие с охраны
#define E_NUM4_OutOfContr  151                  // 4-й номер для снятие с охраны

#define E_NUM1_OnContr     168                  // 1-й номер для установки на охрану
#define E_NUM2_OnContr     185                  // 2-й номер для установки на охрану
#define E_NUM3_OnContr     202                  // 2-й номер для установки на охрану

#define E_NUM1_SmsCommand  219                  // 1-й номер для управления через sms
#define E_NUM2_SmsCommand  236                  // 2-й номер для управления через sms
#define E_NUM3_SmsCommand  253                  // 3-й номер для управления через sms
#define E_NUM4_SmsCommand  270                  // 3-й номер для управления через sms  


//// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ////

// константы режимов работы
byte mode = OutOfContrMod;                      // 1 - снята с охраны                                  
                                                // 2 - в ожидании перед установкой на охрану
                                                // 3 - установлено на охрану
                                                // при добавлении не забываем посмотреть рездел 
                                                
                                               
bool interrupt = false;                         // разрешить/запретить обработку прырывания (по умолчанию запретить, что б небыло ложного срабатывания при старте устройства)
bool inTestMod = false;                         // режим тестирования датчиков (не срабатывает сирена и не отправляются СМС)
bool isSiren = false;                           // режим сирены
bool reqSirena = false;                         // уст. в true когда сработал датчик и необходимо включить сирену
bool isRun = true;                              // флаг для управления выполнения блока кода в loop только при старте устройства

unsigned long prSiren = 0;                      // время включения сирены (милисекунды)
unsigned long prLastPressBtn = 0;               // время последнего нажатие на кнопку (милисекунды)
unsigned long prTestBlinkLed = 0;               // время мерцания светодиода при включеном режима тестирования (милисекунды)
unsigned long prRefreshVcc = 0;                 // время последнего измирения питания (милисекунды)
unsigned long prReqSirena = 0;                  // время последнего обнаружения, что необходимо включать сирену

byte countPressBtn = 0;                         // счетчик нажатий на кнопку
bool wasRebooted = false;                       // указываем была ли последний раз перезагрузка программным путем

MyGSM gsm(gsmLED, pinBOOT);                             // GSM модуль
PowerControl powCtr (netVcc, battVcc, pinMeasureVcc);   // контроль питания

// Датчики
DigitalSensor SenTension(pinSH1);
DigitalSensor SenPIR1(pinPIR1);
DigitalSensor SenPIR2(pinPIR2);
DigitalSensor SenGas(pinGas);

void(* RebootFunc) (void) = 0;                          // объявляем функцию Reboot

void setup() 
{
  delay(1000);                                // !! чтобы нечего не повисало при включении
  debug.begin(19200);
  pinMode(SpecerPin, OUTPUT);
  pinMode(gsmLED, OUTPUT);
  pinMode(OutOfContrLED, OUTPUT);
  pinMode(OnContrLED, OUTPUT);
  pinMode(SirenLED, OUTPUT);
  pinMode(BattPowerLED, OUTPUT);              // LED для сигнализации о работе от резервного питания
  pinMode(pinBOOT, OUTPUT);                   // нога BOOT на модеме
  pinMode(pinSH1, INPUT_PULLUP);              // нога на растяжку
  pinMode(pinPIR1, INPUT);                    // нога датчика движения 1
  pinMode(pinPIR2, INPUT);                    // нога датчика движения 2
  pinMode(pinGas, INPUT);                     // нога датчика газа/дыма 
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
        PlayTone(sysTone, 1000);               
        for (int i = 0 ; i < EEPROM.length() ; i++) 
          EEPROM.write(i, 0);                           // стираем все данные с EEPROM
        // установка дефолтных параметров
        EEPROM.write(E_mode, OutOfContrMod);            // устанавливаем по умолчанию режим не на охране
        EEPROM.write(E_inTestMod, false);               // режим тестирования по умолчанию выключен
        EEPROM.write(E_isRedirectSms, false);           // режим перенаправления всех смс по умолчанию выключен
        EEPROM.write(E_wasRebooted, false);             // факт перезагрузки устройства по умолчанию выключено (устройство не перезагружалось)
        EEPROM.write(E_delaySiren, 0);                  // пауза между сработкой датчиков и включением сирены отключена (0 секунд) 
        EEPROM.write(E_delayOnContr, 25);               // пауза от нажатия кнопки до установки режима охраны (25 сек)
        EEPROM.write(E_intervalVcc, 0);                 // интервал между измерениями питания (0 секунд)
        EEPROM.write(E_BalanceUssd, "***");             // Ussd код для запроса баланца
        EEPROM.write(E_SirenEnabled, true);             // сирена по умолчанию включена
        EEPROM.write(E_IsPIR1Enabled, true);            
        EEPROM.write(E_IsPIR2Enabled, true);
        EEPROM.write(E_IsGasEnabled, true);
        EEPROM.write(E_TensionEnabled, true);                    
        RebootFunc();                                   // перезагружаем устройство
    }
  }  
   
  // блок тестирования спикера и всех светодиодов
  PlayTone(sysTone, 100);                          
  delay(500);
  digitalWrite(gsmLED, HIGH);
  digitalWrite(OutOfContrLED, HIGH);
  digitalWrite(OnContrLED, HIGH);
  digitalWrite(SirenLED, HIGH);
  digitalWrite(BattPowerLED, HIGH);
  delay(timeAllLeds);
  digitalWrite(gsmLED, LOW);
  digitalWrite(OutOfContrLED, LOW);
  digitalWrite(OnContrLED, LOW);
  digitalWrite(SirenLED, LOW);
  digitalWrite(BattPowerLED, LOW);
  
  analogWrite(pinMeasureVcc_stub, 255);                 // запитываем ногу заглушку питания для заглушки определения типа питания если резервное пинание не подключено (всегда network)
  powCtr.Refresh();                                     // читаем тип питания (БП или батарея)
  digitalWrite(BattPowerLED, powCtr.IsBattPower);       // сигнализируем светодиодом режим питания (от батареи - светится, от сети - не светится)
  
  //gsm.Initialize();                                   // инициализация gsm модуля (включения, настройка) 
  
  attachInterrupt(0, ClickButton, FALLING);             // привязываем 0-е прерывание к функции ClickButton(). 
  interrupt = true;                                     // разрешаем обработку прырывания  
  
  // чтение конфигураций с EEPROM
  if (EEPROM.read(E_mode) == OnContrMod) Set_OnContrMod(true);  // читаем режим из еепром      
    else Set_OutOfContrMod();
   
  inTestMod = EEPROM.read(E_inTestMod);                         // читаем тестовый режим из еепром
  wasRebooted = EEPROM.read(E_wasRebooted);                     // читаем был ли последний раз перезагрузка программным путем 
}


void loop() 
{   
  if (GetElapsed(prRefreshVcc) > EEPROM.read(E_intervalVcc) * 1000 || prRefreshVcc == 0) // проверяем сколько прошло времени после последнего измерения питания (секунды) (выдерживаем паузц между измерениями что б не загружать контроллер)
  {   
    PowerControl();                                                   // мониторим питание системы
    prRefreshVcc = millis();
  }   
  
  gsm.Refresh();                                                      // читаем сообщения от GSM модема   

  if(wasRebooted)
  {    
    SendSms(&GetStrFromFlash(sms_WasRebooted), &NumberRead(E_NUM1_OutOfContr));
    wasRebooted = false;
    EEPROM.write(E_wasRebooted, false);
  }
  
  if (inTestMod && !isSiren)                                          // если включен режим тестирования и не сирена
  {
    if (GetElapsed(prTestBlinkLed) > timeTestBlinkLed)   
    {
      digitalWrite(SirenLED, digitalRead(SirenLED) == LOW);           // то мигаем светодиодом
      prTestBlinkLed = millis();
    }
  }
      
  if (countPressBtn != 0)
  {     
    if (GetElapsed(prLastPressBtn) > timeAfterPressBtn && mode == OutOfContrMod)
    {       
      // установка на охрану countBtnOnContrMod
      if (countPressBtn == countBtnOnContr)                                         // если кнопку нажали заданное количество для включение/отключения режима тестирования
      {
        countPressBtn = 0;  
        Set_OnContrMod(true);       
      }
      else
      // включение/отключения режима тестирования
      if (countPressBtn == countBtnInTestMod)                                       // если кнопку нажали заданное количество для включение/отключения режима тестирования
      {
        countPressBtn = 0;  
        PlayTone(sysTone, 250);                                                     // сигнализируем об этом спикером  
        inTestMod = !inTestMod;                                                     // включаем/выключаем режим тестирование датчиков        
        debug.print("Test = ");
        debug.print(inTestMod);
        digitalWrite(SirenLED, LOW);                                                // выключаем светодиод
        EEPROM.write(E_inTestMod, inTestMod);                                       // пишим режим тестирование датчиков в еепром
        debug.println(countPressBtn);
      }
      else
      // запрос баланса счета
      if (countPressBtn == countBtnBalance)                                         // если кнопку нажали заданное количество для запроса баланса счета
      {
        countPressBtn = 0;  
        PlayTone(sysTone, 250);                                                     // сигнализируем об этом спикером                        
        if(gsm.RequestUssd(&ReadFromEEPROM(E_BalanceUssd)))
          WriteToEEPROM(E_NumberAnsUssd, &NumberRead(E_NUM1_SmsCommand));           // сохраняем номер на который необходимо будет отправить ответ                   
        else
          SendSms(&GetStrFromFlash(sms_WrongUssd), &NumberRead(E_NUM1_OutOfContr)); // если ответ пустой то отправляем сообщение об ошибке команды         
      }                                                                                
      else
      // кратковременное включение сирены (для тестирования модуля сирены)
      if (countPressBtn == countBtnSkimpySiren)                      
      {
        countPressBtn = 0;  
        PlayTone(sysTone, 250);                                    
        SkimpySiren();
      }
      else
      {
        PlayTone(sysTone, 250);   
        countPressBtn = 0;  
      }       
    }
    else
    // снятие кнопкой с охраны (работает только в тестовом режиме, когда не блокируются прерывания)
    if (mode == OnContrMod)                                                         // в тестовом режиме можно сниамть кнопкой с охраны
    {
      delay(200);                                                                   // пайза, что б не сливались звуковые сигналы нажатия кнопки и установки режима
      countPressBtn = 0;  
      gsm.RejectCall();                                                             // сбрасываем вызов      
      Set_OutOfContrMod();
      debug.println(countPressBtn);        
    }                  
  }

   ////// NOT IN CONTROL MODE ///////  
  if (mode == OutOfContrMod)                                                        // если режим не на охране
  {

    if (gsm.NewRing)                                                                // если обнаружен входящий звонок
    {
      if (NumberRead(E_NUM1_OnContr).indexOf(gsm.RingNumber) > -1 ||                // если найден зарегистрированный звонок то ставим на охрану
          NumberRead(E_NUM2_OnContr).indexOf(gsm.RingNumber) > -1 ||
          NumberRead(E_NUM3_OnContr).indexOf(gsm.RingNumber) > -1           
         )      
      {               
        digitalWrite(SirenLED, LOW);                        // на время выключаем мигание светодиода сирены если включен режим тестирования
        delay(timeRejectCall);                              // пауза перед збросом звонка        
        gsm.RejectCall();                                   // сбрасываем вызов               
        Set_OnContrMod(false);                              // устанавливаем на охрану без паузы              
      }
      else gsm.RejectCall();                                // если не найден зарегистрированный звонок то сбрасываем вызов (без паузы)      
    gsm.ClearRing();                                        // очищаем обнаруженный входящий звонок    
    }
    
  }                                                         // end OutOfContrMod 
  else
  
  ////// IN CONTROL MODE ///////  
  if (mode == OnContrMod)                                             // если в режиме охраны
  {
    if (isSiren)
    {
      int cSiren;
      if (!inTestMod) cSiren = timeSiren;                             // если выключен режим тестирования то сохраняем установленное время работы сирены
        else cSiren = timeSiren / 10;                                  // если включен режим тестирования то время работы сирены сокращаем в десять раза для удобства проверки датчиков
      if (GetElapsed(prSiren) > cSiren)                               // если включена сирена и сирена работает больше установленного времени то выключаем ее
        StopSiren();
    }   

    if (EEPROM.read(E_TensionEnabled) && !SenTension.isTrig && SenTension.CheckSensor())// проверяем растяжку только если она не срабатывала ранее (что б смс и звонки совершались единоразово)
    {
      digitalWrite(SirenLED, HIGH);                                                          // сигнализируем светодиодом о тревоге
      reqSirena = true;                                                                      // запоминаем когда сработал датчик для отображения статуса датчика
      SenTension.prTrigTime = millis();
      SenTension.isTrig = true;       
      if (prReqSirena == 1) prReqSirena = millis();
      SenTension.isAlarm = true;     
    }
    
    if (EEPROM.read(E_IsPIR1Enabled) && SenPIR1.CheckSensor())
    {       
      digitalWrite(SirenLED, HIGH);                                                          // сигнализируем светодиодом о тревоге
      reqSirena = true;
      SenPIR1.prTrigTime = millis();                                                         // запоминаем когда сработал датчик для отображения статуса датчика
      if (prReqSirena == 1) prReqSirena = millis();
      if (GetElapsed(SenPIR1.prAlarmTime) > timeSmsPIR1 || SenPIR1.prAlarmTime == 0)         // если выдержена пауза после последнего звонка и отправки смс 
        SenPIR1.isAlarm = true;
    }

    if (EEPROM.read(E_IsPIR2Enabled) && SenPIR2.CheckSensor())
    { 
      digitalWrite(SirenLED, HIGH);                                                          // сигнализируем светодиодом о тревоге
      reqSirena = true;
      SenPIR2.prTrigTime = millis();                                                            // запоминаем когда сработал датчик для отображения статуса датчика
      if (prReqSirena == 1) prReqSirena = millis();
      if (GetElapsed(SenPIR2.prAlarmTime) > timeSmsPIR2 || SenPIR2.prAlarmTime == 0)               // если выдержена пауза после последнего звонка и отправки смс
        SenPIR2.isAlarm = true;
    }   
    
    if (reqSirena 
      && (inTestMod || GetElapsed(prReqSirena)/1000 >= EEPROM.read(E_delaySiren) || prReqSirena == 0))      
    {     
      reqSirena = false;            
      if (!isSiren)
      {
        StartSiren();
        prReqSirena = 0;                                                                      // устанавливаем в 0 для отключения паузы между следующим срабатыванием датчиков и включением сирены
      }
      else
        prSiren = millis();      
    }      
    
    if (SenTension.isAlarm)                                                                       // проверяем состояние растяжки и если это первое обнаружение обрыва (TensionTriggered = false) то выполняем аналогичные действие
    {                                                  
      if (gsm.IsAvailable())
      {
        if (!inTestMod)    
          gsm.SendSms(&GetStrFromFlash(sms_TensionCable), &NumberRead(E_NUM1_OutOfContr)); 
        gsm.Call(&NumberRead(E_NUM1_OutOfContr));      
        SenTension.isAlarm = false;
      }                                                    
    }
    
    if (SenPIR1.isAlarm)                                                                          // проверяем состояние 1-го датчика движения
    {                                                                 
      if (gsm.IsAvailable())              
      {  
        if (!inTestMod)  
          gsm.SendSms(&GetStrFromFlash(sms_PIR1), &NumberRead(E_NUM1_OutOfContr));            // если не включен режим тестирование отправляем смс
        gsm.Call(&NumberRead(E_NUM1_OutOfContr));                                             // сигнализируем звонком о сработке датчика движения
        SenPIR1.prAlarmTime = millis();
        SenPIR1.isAlarm = false;
      }
    }
    
    if (SenPIR2.isAlarm)                                                                          // проверяем состояние 2-го датчика движения
    {      
      if (gsm.IsAvailable())
      {  
        if (!inTestMod)
          gsm.SendSms(&GetStrFromFlash(sms_PIR2), &NumberRead(E_NUM1_OutOfContr));
        gsm.Call(&NumberRead(E_NUM1_OutOfContr));
        SenPIR2.prAlarmTime = millis();
        SenPIR2.isAlarm = false;
      }
    }
    
    if (gsm.NewRing)                                                                  // если обнаружен входящий звонок
    {      
      if (NumberRead(E_NUM1_OutOfContr).indexOf(gsm.RingNumber) > -1 ||               // если найден зарегистрированный звонок то снимаем с охраны
          NumberRead(E_NUM2_OutOfContr).indexOf(gsm.RingNumber) > -1 || 
          NumberRead(E_NUM3_OutOfContr).indexOf(gsm.RingNumber) > -1 ||
          NumberRead(E_NUM4_OutOfContr).indexOf(gsm.RingNumber) > -1         
         )               
      {                    
        delay(timeRejectCall);                                                        // пауза перед збросом звонка
        Set_OutOfContrMod();                                                          // снимаем с охраны         
        gsm.RejectCall();                                                             // сбрасываем вызов        
      }
      else
      if ((NumberRead(E_NUM1_OnContr).indexOf(gsm.RingNumber) > -1 ||
           NumberRead(E_NUM2_OnContr).indexOf(gsm.RingNumber) > -1 ||
           NumberRead(E_NUM3_OnContr).indexOf(gsm.RingNumber) > -1) && 
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
  }                                                                                   // end OnContrMod

  // обработка датчика газа/дыма
  if (SenGas.prTrigTime != 0 && (GetElapsed(SenGas.prTrigTime)/1000) > 43200)         // если время последнего срабатывания больше чем 12 часов то обнуляем его 
    SenGas.prTrigTime = 0;
    
  if (EEPROM.read(E_IsGasEnabled))                                                    // если датчик газа/дыма включен
  {
    if (SenGas.CheckSensor())                                                         // то опрашиваем его                  
    {       
      digitalWrite(SirenLED, HIGH);                                                   // сигнализируем светодиодом о тревоге
      if (!SenGas.isTrig && inTestMod) PlayTone(sysTone, 100);                     // если включен режим тестирование и это первое срабатывание то сигнализируем спикером  
      SenGas.isTrig = true;
      //reqSirena = true;
      SenGas.prTrigTime = millis();                                                   // запоминаем когда сработал датчик для отображения статуса датчика
      //if (prReqSirena == 1) prReqSirena = millis();
      if (GetElapsed(SenGas.prAlarmTime) > timeSmsGas || SenGas.prAlarmTime == 0)     // если выдержена пауза после последнего звонка и отправки смс 
        SenGas.isAlarm = true;
    }
    else if (SenGas.isTrig && !isSiren)
    {
      digitalWrite(SirenLED, LOW);
      SenGas.isTrig = false;
    }
  
    if (SenGas.isAlarm)                                                                      
    {                                                                 
      if (gsm.IsAvailable())              
      {  
        if (!inTestMod)  
          gsm.SendSms(&GetStrFromFlash(sms_Gas), &NumberRead(E_NUM1_OutOfContr));     // если не включен режим тестирование отправляем смс
        gsm.Call(&NumberRead(E_NUM1_OutOfContr));                                     // сигнализируем звонком о сработке датчика
        SenGas.prAlarmTime = millis();
        SenGas.isAlarm = false;
      }
    }
  }
  ///----------------
  
  if (gsm.NewUssd)                                                                    // если доступный новый ответ на Ussd запрос
  {
    SendSms(&gsm.UssdText, &NumberRead(E_NumberAnsUssd));                             // отправляем ответ на Ussd запрос
    gsm.ClearUssd();                                                                  // сбрасываем ответ на gsm команду 
  }
  if(!SenTension.isAlarm && !SenPIR1.isAlarm && !SenPIR2.isAlarm && !SenGas.isAlarm)
    ExecSmsCommand();                                                                 // если нет необработаных датчиков то проверяем доступна ли новая команда по смс и если да то выполняем ее
}



//// ------------------------------- Functions --------------------------------- ////
void ClickButton()
{ 
  if (interrupt)
  {
    static unsigned long millis_prev;
    if(millis()-300 > millis_prev) 
    {
      debug.println("ButtonPressed");
      BlinkLEDlow(OutOfContrLED,  0, 100, 0);      
      PlayTone(clickTone, 40);    
      countPressBtn++;
      prLastPressBtn = millis();         
    }       
    millis_prev = millis();           
  }      
}

bool Set_OutOfContrMod()                                // метод для снятие с охраны
{ 
  debug.println("OutOfContrMod");
  interrupt = true;                                     // разрешаем обрабатывать прерывания
  digitalWrite(OutOfContrLED, HIGH);
  digitalWrite(OnContrLED, LOW);
  digitalWrite(SirenLED, LOW);
  PlayTone(sysTone, 500);
  mode = OutOfContrMod;                                 // снимаем с охраны
  StopSiren();                                          // выключаем сирену                         
  reqSirena = false; 
    
  SenPIR1.ResetSensor();
  SenPIR2.ResetSensor();
  SenTension.ResetSensor();
  
  EEPROM.write(E_mode, mode);                           // пишим режим в еепром, что б при следующем включении устройства, оно оставалось в данном режиме
  return true;
}

bool Set_OnContrMod(bool IsWaiting)                     // метод для установки на охрану
{ 
  if (IsWaiting == true)                                // если включен режим ожидание перед установкой охраны, выдерживаем заданную паузу, что б успеть покинуть помещение
  {       
    debug.println("Set_OnContrMod");
    digitalWrite(OutOfContrLED, LOW);   
    byte timeWait = 0;
    if (inTestMod) timeWait = delayOnContrTest;         // если включен режим тестирования, устанавливаем для удобства тестирования меньшую паузу
    else timeWait = EEPROM.read(E_delayOnContr);        // если режим тестирования выклюяен, используем обычную паузу
    for(byte i = 0; i < timeWait; i++)   
    {               
      if (countPressBtn > 0)                            // если пользователь нажал на кнопку то установка на охрану прерывается
      {
        countPressBtn = 0;
        delay(200);                                     // пайза, что б не сливались звуковые сигналы нажатия кнопки и установки режима
        Set_OutOfContrMod();
        return false;
      }        
      if (i < (timeWait * 0.7))                         // первых 70% паузы моргаем медленным темпом
        BlinkLEDSpecer(OnContrLED, 0, 500, 500);              
      else                                              // последних 30% паузы ускоряем темп
      {
        BlinkLEDSpecer(OnContrLED, 0, 250, 250); 
        BlinkLEDSpecer(OnContrLED, 0, 250, 250);              
      }         
    }
  }
  
  if (!inTestMod) interrupt = false;                    // если не включен режим тестирования то запрещаем обрабатывать прерывания
  
  // установка переменных в дефолтное состояние  
  SenPIR1.ResetSensor();
  SenPIR2.ResetSensor();
  SenTension.ResetSensor();
  
  //установка на охрану     
  mode = OnContrMod;                                    // ставим на охрану                                                    
  digitalWrite(OutOfContrLED, LOW);
  digitalWrite(OnContrLED, HIGH);
  digitalWrite(SirenLED, LOW);  
  PlayTone(sysTone, 500);  
  EEPROM.write(E_mode, mode);                           // пишим режим в еепром, что б при следующем включении устройства, оно оставалось в данном режиме
  delay (2500);                                         // дополнительная пауза так как датчик держит лог. единицу 2,5
  prReqSirena = 1;                                      // устанавливаем в 1 для активации паузы между срабатыванием датчиков и включением сирены
  return true;
}

void  StartSiren()
{  
  digitalWrite(SirenLED, HIGH);                         // сигнализируем светодиодом о тревоге
  if (!inTestMod)                                       // если не включен тестовый режим
  {
    if (EEPROM.read(E_SirenEnabled))                    // и сирена не отключена в конфигурации
      digitalWrite(SirenGenerator, LOW);                // включаем сирену через релье
  }
  else
    PlayTone(sysTone, 100);                          // если включен режим тестирование то сигнализируем только спикером
  isSiren = true; 
  prSiren = millis();  
}


void  StopSiren()
{
  if(!SenGas.isTrig) digitalWrite(SirenLED, LOW);        // Если не надо сигнализировать о газе то выключаем светодиод о индикации тревоги 
  digitalWrite(SirenGenerator, HIGH);                    // выключаем сирену через релье
  isSiren = false;   
}

void PowerControl()                                                                       // метод для обработки событий питания системы (переключение на батарею или на сетевое)
{
  powCtr.Refresh();    
  digitalWrite(BattPowerLED, powCtr.IsBattPower);
        
  if (!inTestMod && !powCtr.IsBattPowerPrevious && powCtr.IsBattPower)                    // если предыдущий раз было от сети а сейчас от батареи (пропало сетевое питание 220v) и если не включен режим тестирования
    SendSms(&GetStrFromFlash(sms_BattPower), &NumberRead(E_NUM1_OutOfContr));             // отправляем смс о переходе на резервное питание         
   
  if (!inTestMod && powCtr.IsBattPowerPrevious && !powCtr.IsBattPower)                    // если предыдущий раз было от батареи a сейчас от сети (сетевое питание 220v возобновлено) и если не включен режим тестирования  
    SendSms(&GetStrFromFlash(sms_NetPower), &NumberRead(E_NUM1_OutOfContr));              // отправляем смс о возобновлении  сетевое питание 220v 
}

void SkimpySiren()                                                                        // метод для кратковременного включения сирены (для теститования сирены)
{
  digitalWrite(SirenLED, HIGH);
  digitalWrite(SirenGenerator, LOW);                                                      // включаем сирену через релье
  delay(timeSkimpySiren);                                                                 // кратковременный период на который включается сирена
  digitalWrite(SirenLED, LOW);
  digitalWrite(SirenGenerator, HIGH);                                                     // выключаем сирену через релье  
}

// читаем смс и если доступна новая команда по смс то выполняем ее
void ExecSmsCommand()
{ 
  if (gsm.NewSms)
  {
    if ((gsm.SmsNumber.indexOf(NumberRead(E_NUM1_SmsCommand)) > -1 ||                    // если обнаружено зарегистрированый номер
         gsm.SmsNumber.indexOf(NumberRead(E_NUM2_SmsCommand)) > -1 ||
         gsm.SmsNumber.indexOf(NumberRead(E_NUM3_SmsCommand)) > -1 ||
         gsm.SmsNumber.indexOf(NumberRead(E_NUM4_SmsCommand)) > -1
        ) 
        ||
        (NumberRead(E_NUM1_SmsCommand).startsWith("***")  &&                             // если нет зарегистрированных номеров (при первом включении необходимо зарегистрировать номера)
         NumberRead(E_NUM2_SmsCommand).startsWith("***")  &&
         NumberRead(E_NUM3_SmsCommand).startsWith("***")  &&
         NumberRead(E_NUM4_SmsCommand).startsWith("***")
         )
       )
    { 
      gsm.SmsText.toLowerCase();                                                         // приводим весь текст команды к нижнему регистру что б было проще идентифицировать команду
      gsm.SmsText.trim();                                                                // удаляем пробелы в начале и в конце комманды
      
      if (gsm.SmsText.startsWith("*") || gsm.SmsText.startsWith("#"))                    // если сообщение начинается на * или # то это Ussd запрос
      {
        PlayTone(sysTone, smsSpecDur); 
        if (gsm.RequestUssd(&gsm.SmsText))                                               // отправляем Ussd запрос и если он валидный (запрос заканчиваться на #)             
          WriteToEEPROM(E_NumberAnsUssd, &gsm.SmsNumber);                                // то сохраняем номер на который необходимо будет отправить ответ от Ussd запроса                                                    
        else
          SendSms(&GetStrFromFlash(sms_WrongUssd), &gsm.SmsNumber);                      // иначе отправляем сообщение об инвалидном Ussd запросе 
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(sendsms)))                              // если обнаружена смс команда для отправки смс другому абоненту
      {
        PlayTone(sysTone, smsSpecDur); 
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
            SendSms(&GetStrFromFlash(sms_SmsWasSent), &gsm.SmsNumber);                   // то отправляем отчет об успешном выполнении комманды          
        }
        else
          SendSms(&GetStrFromFlash(sms_ErrorSendSms), &gsm.SmsNumber);                   // если номер получателя не обнаружен (пустой) то отправляем сообщение с правильным форматом комманды 
      }
      else      
      if (gsm.SmsText == GetStrFromFlash(balance))                                       // если обнаружена смс команда для запроса баланса
      {        
        digitalWrite(SirenLED, LOW);                                                     // выключаем светодиод, который может моргать если включен тестовый режим
        PlayTone(sysTone, smsSpecDur); 
        if(gsm.RequestUssd(&ReadFromEEPROM(E_BalanceUssd)))                              // отправляем Ussd запрос для получения баланса и если он валидный (запрос заканчиваться на #)    
          WriteToEEPROM(E_NumberAnsUssd, &gsm.SmsNumber);                                // то сохраняем номер на который необходимо будет отправить баланс                 
        else
          SendSms(&GetStrFromFlash(sms_WrongUssd), &gsm.SmsNumber);                      // иначе отправляем сообщение об инвалидном Ussd запросе 
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(teston)))                              // если обнаружена смс команда для включения тестового режима для тестирования датчиков
      {        
        digitalWrite(SirenLED, LOW);                                                     // выключаем светодиод, который может моргать если включен тестовый режим
        PlayTone(sysTone, smsSpecDur); 
        inTestMod = true;
        EEPROM.write(E_inTestMod, true);                                                 // пишим режим тестирование датчиков в еепром 
        SendSms(&GetStrFromFlash(sms_TestModOn), &gsm.SmsNumber);                        // отправляем смс о завершении выполнения даной смс команды                                                         
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(testoff)))                             // если обнаружена смс команда для выключения тестового режима для тестирования датчиков
      {
        digitalWrite(SirenLED, LOW);                                                     // выключаем светодиод, который может моргать если включен тестовый режим                                                 
        PlayTone(sysTone, smsSpecDur); 
        inTestMod = false;
        EEPROM.write(E_inTestMod, false);                                                // пишим режим тестирование датчиков в еепром 
        SendSms(&GetStrFromFlash(sms_TestModOff), &gsm.SmsNumber);                       // отправляем смс о завершении выполнения даной смс команды                 
      }            
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(controlon)))                            // если обнаружена смс команда для установки на охрану
      {        
        digitalWrite(SirenLED, LOW);                                                     // выключаем светодиод, который может моргать если включен тестовый режим
        Set_OnContrMod(false);                                                           // устанавливаем на охрану без паузы                                                
        SendSms(&GetStrFromFlash(sms_OnContrMod), &gsm.SmsNumber);                       // отправляем смс о завершении выполнения даной смс команды                           
      }
      else 
      if (gsm.SmsText.startsWith(GetStrFromFlash(controloff)))                           // если обнаружена смс команда для снятие с охраны
      {
        digitalWrite(SirenLED, LOW);                                                     // выключаем светодиод, который может моргать если включен тестовый режим
        Set_OutOfContrMod();                                                             // снимаем с охраны
        SendSms(&GetStrFromFlash(sms_OutOfContrMod), &gsm.SmsNumber);                    // отправляем смс о завершении выполнения даной смс команды
      }      
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(redirecton)))                           // если обнаружена смс команда для включения режима "перенапралять входящие смс от незарегистрированных номеров на номер SmsCommand1" 
      {
        PlayTone(sysTone, smsSpecDur);
        EEPROM.write(E_isRedirectSms, true);         
        SendSms(&GetStrFromFlash(sms_RedirectOn), &gsm.SmsNumber);                                          
      }
      else 
      if (gsm.SmsText.startsWith(GetStrFromFlash(redirectoff)))                          // если обнаружена смс команда для выключения режима "перенапралять входящие смс от незарегистрированных номеров на номер SmsCommand1" 
      {
        PlayTone(sysTone, smsSpecDur);
        EEPROM.write(E_isRedirectSms, false);          
        SendSms(&GetStrFromFlash(sms_RedirectOff), &gsm.SmsNumber);       
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(skimpy)))                                // если обнаружена смс команда для кратковременного включения сирены (для теститования сирены)
      {
        SkimpySiren();
        SendSms(&GetStrFromFlash(sms_SkimpySiren), &gsm.SmsNumber);   
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(reboot)))                                // если обнаружена смс команда для перезагрузки устройства
      {
        PlayTone(sysTone, smsSpecDur);
        EEPROM.write(E_wasRebooted, true);                                                // записываем статус, что устройство перезагружается        
        gsm.Shutdown();                                                                   // выключаем gsm модуль
        RebootFunc();                                                                     // вызываем Reboot arduino платы
      }      
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(_status)))                               // если обнаружена смс команда для запроса статуса режимов и настроек устройства  
      {
        PlayTone(sysTone, smsSpecDur);        
        String msg = GetStrFromFlash(control)          + String((mode == OnContrMod) ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "\n"
                   + GetStrFromFlash(test)             + String((inTestMod) ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "\n" 
                   + GetStrFromFlash(redirSms)         + String((EEPROM.read(E_isRedirectSms)) ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "\n"
                   + GetStrFromFlash(power)            + String((powCtr.IsBattPower) ? GetStrFromFlash(battery) : GetStrFromFlash(network)) + "\n"
                   + GetStrFromFlash(delSiren)         + String(EEPROM.read(E_delaySiren)) + GetStrFromFlash(sec);
        
        if (!EEPROM.read(E_SirenEnabled))
          msg = msg + "\n" + GetStrFromFlash(siren) + GetStrFromFlash(off);
          
        unsigned long ltime;
        String sStatus = "";                  
        if (EEPROM.read(E_IsGasEnabled))
        {
          if (SenGas.prTrigTime == 0) sStatus = GetStrFromFlash(idle);
          else
          {
            ltime = GetElapsed(SenGas.prTrigTime)/1000;
            if (ltime <= 180) sStatus = String(ltime) + GetStrFromFlash(sec);             // < 180 сек. 
            else 
            if (ltime <= 7200) sStatus = String(ltime / 60) + GetStrFromFlash(minut);     // < 120 мин.
            else 
            sStatus = String(ltime / 3600) + GetStrFromFlash(hour);                       
          }            
          msg = msg + "\n" + GetStrFromFlash(Gas) + sStatus;
        }         
        if (mode == OnContrMod)
        {
          if (EEPROM.read(E_IsPIR1Enabled))
          {             
            if (SenPIR1.prTrigTime == 0) sStatus = GetStrFromFlash(idle);
            else
            {
              ltime = GetElapsed(SenPIR1.prTrigTime)/1000;
              if (ltime <= 180) sStatus = String(ltime) + GetStrFromFlash(sec);             // < 180 сек. 
              else 
              if (ltime <= 7200) sStatus = String(ltime / 60) + GetStrFromFlash(minut);     // < 120 мин.
              else 
              sStatus = String(ltime / 3600) + GetStrFromFlash(hour);                       
            }            
            msg = msg + "\n" + GetStrFromFlash(PIR1) + sStatus;
          }
          if (EEPROM.read(E_IsPIR2Enabled))
          {
            if (SenPIR2.prTrigTime == 0) sStatus = GetStrFromFlash(idle);
            else
            {
              ltime = GetElapsed(SenPIR2.prTrigTime)/1000;
              if (ltime <= 180) sStatus = String(ltime) + GetStrFromFlash(sec);             // < 180 сек. 
              else 
              if (ltime <= 7200) sStatus = String(ltime / 60) + GetStrFromFlash(minut);     // < 120 мин.
              else 
              sStatus = String(ltime / 3600) + GetStrFromFlash(hour);                       
            }            
            msg = msg + "\n" + GetStrFromFlash(PIR2) + sStatus;
          }                    
          if (EEPROM.read(E_TensionEnabled))
          {
            if (SenTension.prTrigTime == 0) sStatus = GetStrFromFlash(idle);
            else
            {
              ltime = GetElapsed(SenTension.prTrigTime)/1000;
              if (ltime <= 180) sStatus = String(ltime) + GetStrFromFlash(sec);             // < 180 сек. 
              else 
              if (ltime <= 7200) sStatus = String(ltime / 60) + GetStrFromFlash(minut);     // < 120 мин.
              else 
              sStatus = String(ltime / 3600) + GetStrFromFlash(hour);                       
            }            
            msg = msg + "\n" + GetStrFromFlash(tension) + sStatus;            
          }
        }
        SendSms(&msg, &gsm.SmsNumber);          
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(outofcontr1)))                          // если обнаружена смс команда для регистрации группы телефонов для снятие с охраны
      {
        PlayTone(sysTone, smsSpecDur);                      
        String nums[4];
        String str = gsm.SmsText;;
        for(byte i = 0; i < 4; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');  
          nums[i] = str.substring(0, duration);      
          str = str.substring(duration +1);            
        }        
        WriteToEEPROM(E_NUM1_OutOfContr, &nums[0]);        
        WriteToEEPROM(E_NUM2_OutOfContr, &nums[1]);  
        WriteToEEPROM(E_NUM3_OutOfContr, &nums[2]);
        WriteToEEPROM(E_NUM4_OutOfContr, &nums[3]);        
        String msg = "OutOfContr1:\n'" + NumberRead(E_NUM1_OutOfContr) + "'" + "\n"
                   + "OutOfContr2:\n'" + NumberRead(E_NUM2_OutOfContr) + "'" + "\n"
                   + "OutOfContr3:\n'" + NumberRead(E_NUM3_OutOfContr) + "'" + "\n"
                   + "OutOfContr4:\n'" + NumberRead(E_NUM4_OutOfContr) + "'";
        SendSms(&msg, &gsm.SmsNumber);    
      }
      else     
      if (gsm.SmsText.startsWith(GetStrFromFlash(oncontr1)))                           // если обнаружена смс команда для регистрации группы телефонов для установки на охрану
      {
        PlayTone(sysTone, smsSpecDur);                     
        String nums[3];
        String str = gsm.SmsText;         
        for(byte i = 0; i < 3; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');  
          nums[i] = str.substring(0, duration);      
          str = str.substring(duration +1);             
        }              
        WriteToEEPROM(E_NUM1_OnContr, &nums[0]);  
        WriteToEEPROM(E_NUM2_OnContr, &nums[1]);
        WriteToEEPROM(E_NUM3_OnContr, &nums[2]);        
        String msg = "OnContr1:\n'" + NumberRead(E_NUM1_OnContr) + "'" + "\n"
                   + "OnContr2:\n'" + NumberRead(E_NUM2_OnContr) + "'" + "\n"
                   + "OnContr3:\n'" + NumberRead(E_NUM3_OnContr) + "'";
        SendSms(&msg, &gsm.SmsNumber);               
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(smscommand1)))                         // если обнаружена смс команда для регистрации группы телефонов для управления через смс команды
      {
        PlayTone(sysTone, smsSpecDur);                     
        String nums[4];
        String str = gsm.SmsText;        
        for(byte i = 0; i < 4; i++)
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
        WriteToEEPROM(E_NUM4_SmsCommand, &nums[3]);         
        String msg = "SmsCommand1:\n'" + NumberRead(E_NUM1_SmsCommand) + "'" + "\n"
                   + "SmsCommand2:\n'" + NumberRead(E_NUM2_SmsCommand) + "'" + "\n"
                   + "SmsCommand3:\n'" + NumberRead(E_NUM3_SmsCommand) + "'" + "\n"
                   + "SmsCommand4:\n'" + NumberRead(E_NUM4_SmsCommand) + "'";
        SendSms(&msg, &gsm.SmsNumber);     
      }      
      else            
      if (gsm.SmsText.startsWith(GetStrFromFlash(outofcontr)))                       // если обнаружена смс команда для запроса списка зарегистрированных телефонов для снятие с охраны
      {
        PlayTone(sysTone, smsSpecDur);        
        String msg = "OutOfContr1:\n'" + NumberRead(E_NUM1_OutOfContr) + "'" + "\n"
                   + "OutOfContr2:\n'" + NumberRead(E_NUM2_OutOfContr) + "'" + "\n"
                   + "OutOfContr3:\n'" + NumberRead(E_NUM3_OutOfContr) + "'" + "\n"
                   + "OutOfContr4:\n'" + NumberRead(E_NUM4_OutOfContr) + "'";
        SendSms(&msg, &gsm.SmsNumber);                    
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(oncontr)))                           // если обнаружена смс команда для запроса списка зарегистрированных телефонов для установки на охрану
      {
        PlayTone(sysTone, smsSpecDur);       
        String msg = "OnContr1:\n'" + NumberRead(E_NUM1_OnContr) + "'" + "\n"
                   + "OnContr2:\n'" + NumberRead(E_NUM2_OnContr) + "'" + "\n"                    
                   + "OnContr3:\n'" + NumberRead(E_NUM3_OnContr) + "'";   
        SendSms(&msg, &gsm.SmsNumber);
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(smscommand)))                         // если обнаружена смс команда для запроса списка зарегистрированных телефонов для управления через смс команды
      {
        PlayTone(sysTone, smsSpecDur);       
        String msg = "SmsCommand1:\n'" + NumberRead(E_NUM1_SmsCommand) + "'" + "\n"
                   + "SmsCommand2:\n'" + NumberRead(E_NUM2_SmsCommand) + "'" + "\n" 
                   + "SmsCommand3:\n'" + NumberRead(E_NUM3_SmsCommand) + "'" + "\n" 
                   + "SmsCommand4:\n'" + NumberRead(E_NUM4_SmsCommand) + "'";
        SendSms(&msg, &gsm.SmsNumber);
      }     
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(settings)))
      {
        PlayTone(sysTone, smsSpecDur);                                
        String msg = GetStrFromFlash(delSiren)     + "'" + String(EEPROM.read(E_delaySiren)) + "'" + GetStrFromFlash(sec) + "\n"
           + GetStrFromFlash(delOnContr)           + "'" + String(EEPROM.read(E_delayOnContr)) + "'" + GetStrFromFlash(sec) + "\n"
           + GetStrFromFlash(intervalVcc)          + "'" + String(EEPROM.read(E_intervalVcc)) + "'" + GetStrFromFlash(sec) + "\n"
           + GetStrFromFlash(balanceUssd)          + "'" + ReadFromEEPROM(E_BalanceUssd) + "'" + "\n" 
           + GetStrFromFlash(siren)                + "'" + String((EEPROM.read(E_SirenEnabled)) ? "on" : "off") + "'" + "\n";
        SendSms(&msg, &gsm.SmsNumber);   
      }
      else
        if (gsm.SmsText.startsWith(GetStrFromFlash(sensor)))               // если обнаружена команда для возврата сетингов датчиков
        {
         String msg = GetStrFromFlash(PIR1)        + "'" + String((EEPROM.read(E_IsPIR1Enabled))  ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "'" + "\n"
           + GetStrFromFlash(PIR2)                 + "'" + String((EEPROM.read(E_IsPIR2Enabled))  ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "'" + "\n"
           + GetStrFromFlash(Gas)                  + "'" + String((EEPROM.read(E_IsGasEnabled))   ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "'" + "\n"
           + GetStrFromFlash(tension)              + "'" + String((EEPROM.read(E_TensionEnabled)) ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "'";
        SendSms(&msg, &gsm.SmsNumber);
      }
      else  
      if (gsm.SmsText.startsWith(GetStrFromFlash(delaySiren)))              // если обнаружена команда с основными настройками устройства (сетинги)
      {
        PlayTone(sysTone, smsSpecDur);                        
        String sConf[4];
        String str = gsm.SmsText;        
        bool bSirEnab;
        for(byte i = 0; i < 5; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');  
          if (i < 4)
            sConf[i] = str.substring(0, duration);
          else if (i == 4)
          {
            if (str.substring(0, duration) == "off")
              bSirEnab = false;      
            else if (str.substring(0, duration) == "on")
              bSirEnab = true; 
          }               
          str = str.substring(duration + 1);         
        }        
        EEPROM.write(E_delaySiren, sConf[0].toInt());
        EEPROM.write(E_delayOnContr, sConf[1].toInt());        
        EEPROM.write(E_intervalVcc, sConf[2].toInt());
        WriteToEEPROM(E_BalanceUssd, &sConf[3]);       
        EEPROM.write(E_SirenEnabled, bSirEnab);

        String msg = GetStrFromFlash(delSiren)     + "'" + String(EEPROM.read(E_delaySiren)) + "'" + GetStrFromFlash(sec) + "\n"
           + GetStrFromFlash(delOnContr)           + "'" + String(EEPROM.read(E_delayOnContr)) + "'" + GetStrFromFlash(sec) + "\n"
           + GetStrFromFlash(intervalVcc)          + "'" + String(EEPROM.read(E_intervalVcc)) + "'" + GetStrFromFlash(sec) + "\n"
           + GetStrFromFlash(balanceUssd)          + "'" + ReadFromEEPROM(E_BalanceUssd) + "'" + "\n" 
           + GetStrFromFlash(siren)                + "'" + String((EEPROM.read(E_SirenEnabled)) ? "on" : "off") + "'" + "\n";
        SendSms(&msg, &gsm.SmsNumber);  
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(_PIR1)))                          // если обнаружена команда с настройками датчиков
      {
        PlayTone(sysTone, smsSpecDur);                        
        String str = gsm.SmsText;        
        bool bConf[4];                                                             // сохраняем настройки по датчикам
        for(byte i = 0; i < 4; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');  
          if (str.substring(0, duration) == "off")
            bConf[i] = false;      
          else if (str.substring(0, duration) == "on")
            bConf[i] = true; 
          str = str.substring(duration +1);         
        }
        EEPROM.write(E_IsPIR1Enabled, bConf[0]);
        EEPROM.write(E_IsPIR2Enabled, bConf[1]);
        EEPROM.write(E_IsGasEnabled,  bConf[2]);
        EEPROM.write(E_TensionEnabled, bConf[3]);
        String msg = GetStrFromFlash(PIR1)         + "'" + String((EEPROM.read(E_IsPIR1Enabled))  ? "on" : "off") + "'" + "\n"
           + GetStrFromFlash(PIR2)                 + "'" + String((EEPROM.read(E_IsPIR2Enabled))  ? "on" : "off") + "'" + "\n"
           + GetStrFromFlash(Gas)                  + "'" + String((EEPROM.read(E_IsGasEnabled))   ? "on" : "off") + "'" + "\n"
           + GetStrFromFlash(tension)              + "'" + String((EEPROM.read(E_TensionEnabled)) ? "on" : "off") + "'";
        SendSms(&msg, &gsm.SmsNumber);  
      }
      else                                                                              // если смс команда не распознана
      {
        PlayTone(sysTone, smsSpecDur);              
        SendSms(&GetStrFromFlash(sms_ErrorCommand), &gsm.SmsNumber);                    // то отправляем смс со списком всех доступных смс команд
      }                                                                                 
    }    
    else if (EEPROM.read(E_isRedirectSms))                                              // если смс пришла не с зарегистрированых номеров и включен режим перенаправления всех смс
    {
      SendSms(&String(gsm.SmsText), &NumberRead(E_NUM1_OutOfContr));                    // перенаправляем смс на зарегистрированный номер под именем SmsCommand1
    }    
  gsm.ClearSms(); 
  }  
}


