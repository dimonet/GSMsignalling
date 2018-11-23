// Версия: 5.0
/// GSM сигналка c установкой по кнопке
/// с датчиками движения и растяжкой (или с геркониевым датчиком)
/// ВНИМАНИЕ: для корретной работы sms необходимо установить размеры буферов вместо 64 на SERIAL_TX_BUFFER_SIZE 24 и SERIAL_RX_BUFFER_SIZE 170 в файле hardware\arduino\avr\cores\arduino\HardwareSerial.h

#include <EEPROM.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include "DigitalSensor.h"
#include "GasSensor.h"
#include "MyGSM.h"
#include "PowerControl.h"
#include "Utilities.h"

//#define debug Serial

//// НАСТРОЕЧНЫЕ КОНСТАНТЫ /////
const char sms_TensionCable[]    PROGMEM = {"ALARM: TensionCable sensor."};                                         // текст смс для растяжки
const char sms_PIR1[]            PROGMEM = {"ALARM: PIR1 sensor."};                                                 // текст смс для датчика движения 1
const char sms_PIR2[]            PROGMEM = {"ALARM: PIR2 sensor."};                                                 // текст смс для датчика движения 2
const char sms_Gas[]             PROGMEM = {"ALARM: Gas sensor."};                                                  // текст смс для датчика газа/дыма

const char sms_BattPower[]       PROGMEM = {"POWER: Battery is used for powering system."};                         // текст смс для оповещения о том, что исчезло сетевое питание
const char sms_NetPower[]        PROGMEM = {"POWER: Network power has been restored."};                             // текст смс для оповещения о том, что сетевое питание возобновлено

const char sms_ErrorCommand[]    PROGMEM = {"SendSMS,\nBalance,\nTestOn(Off),\nControlOn(Off),\nRedirectOn(Off),\nSkimpy,\nStatus,\nReboot,\nButton,\nSetting,\nSensor,\nSiren,\nOutOfContr,\nOnContr,\nSmsCommand."};  // смс команда не распознана
const char sms_InfContrOff[]     PROGMEM = {"Inform: Control mode was turned OFF."};                                // информирование о снятии с охраны
const char sms_InfContrOn[]      PROGMEM = {"Inform: Control mode was turned ON."};                                 // информирование о снятии с охраны
const char sms_ErrorTension[]    PROGMEM = {"Warning: Break OnContr (TensionCable)."};                              // информирование о прырывании установки на охрану;
const char sms_TestModOn[]       PROGMEM = {"Command: Test mode is turned ON."};                                    // выполнена команда для включения тестового режима для тестирования датчиков
const char sms_TestModOff[]      PROGMEM = {"Command: Test mode is turned OFF."};                                   // выполнена команда для выключения тестового режима для тестирования датчиков
const char sms_OnContrMod[]      PROGMEM = {"Command: Control mode is turned ON."};                                 // выполнена команда для установку на охрану
const char sms_OutOfContrMod[]   PROGMEM = {"Command: Control mode is turned OFF."};                                // выполнена команда для снятие с охраны
const char sms_RedirectOn[]      PROGMEM = {"Command: SMS redirection is turned ON."};                              // выполнена команда для включения перенаправления всех смс от любого отправителя на номер SMSNUMBER
const char sms_RedirectOff[]     PROGMEM = {"Command: SMS redirection is turned OFF."};                             // выполнена команда для выключения перенаправления всех смс от любого отправителя на номер SMSNUMBER
const char sms_SkimpySiren[]     PROGMEM = {"Command: Skimpy siren was turned ON."};                                // выполнена команда для коротковременного включения сирены
const char sms_SMSRebooted[]     PROGMEM = {"Command: Device was rebooted."};                                       // устройство перегружено по причине sms команды
const char sms_WDRebooted[]      PROGMEM = {"System: Disaster rebooted."};                                          // аварийная перезагрузка по причине зависании
const char sms_GsmWasRestored[]  PROGMEM = {"System: GSM was restored."};                                           // gsm модуль был перегружен и востановлен после збоя
const char sms_WrongUssd[]       PROGMEM = {"Command: Wrong USSD code."};                                           // сообщение о неправельной USSD коде
const char sms_ErrorSendSms[]    PROGMEM = {"Command: Format should be next:\nSendSMS 'number' 'text'"};            // выполнена команда для отправки смс другому абоненту
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
const char button[]              PROGMEM = {"button"};
const char setting[]             PROGMEM = {"setting"};
const char sensor[]              PROGMEM = {"sensor"};
const char delaySiren[]          PROGMEM = {"delaysiren"};
const char _PIR1[]               PROGMEM = {"pir1"};
const char btnoncontr[]          PROGMEM = {"btnoncontr"};
const char siren[]               PROGMEM = {"siren"};
const char _SirenEnabled[]       PROGMEM = {"sirenenabled"};

// Строки для формирования смс ответов на смс команды Status и Settings
const char FirmwareVer[]         PROGMEM = {"Ver: 5.0"};
const char control[]             PROGMEM = {"On controlling: "}; 
const char test[]                PROGMEM = {"Test mode: "}; 
const char redirSms[]            PROGMEM = {"Redirect SMS: "}; 
const char power[]               PROGMEM = {"Power: "}; 
const char delSiren[]            PROGMEM = {"DelaySiren: "}; 
const char PIR1[]                PROGMEM = {"PIR1: "}; 
const char PIR2[]                PROGMEM = {"PIR2: "};
const char Gas[]                 PROGMEM = {"Gas: "}; 
const char GasCalibr[]           PROGMEM = {"GasCalibr: "};
const char GasCurr[]             PROGMEM = {"GasCurr: "};
const char tension[]             PROGMEM = {"Tension: "};
const char infContr[]            PROGMEM = {"InfContr: "};
const char sirenoff[]            PROGMEM = {"Siren Off: "};
const char gasOnlyOnContr[]      PROGMEM = {"GasOnlyOnContr: "};

const char SirenEnabled[]        PROGMEM = {"SirenEnabled: "};
const char PIR1Siren[]           PROGMEM = {"PIR1Siren: "};
const char PIR2Siren[]           PROGMEM = {"PIR2Siren: "};
const char TensionSiren[]        PROGMEM = {"TensionSiren: "};

const char idle[]                PROGMEM = {"Idle"};
const char on[]                  PROGMEM = {"on"};
const char off[]                 PROGMEM = {"off"};
const char sec[]                 PROGMEM = {" sec."};
const char minut[]               PROGMEM = {" min."};
const char hour[]                PROGMEM = {" hours."};
const char delOnContr[]          PROGMEM = {"DelayOnContr: "};
const char intervalVcc[]         PROGMEM = {"IntervalVcc: "};
const char balanceUssd[]         PROGMEM = {"BalanceUssd: "};
const char GasVal[]              PROGMEM = {"GasVal: "};
const char GasNotReady[]         PROGMEM = {"NotReady"};
const char BtnOnContr[]          PROGMEM = {"BtnOnContr: "};
const char BtnInTestMod[]        PROGMEM = {"BtnInTestMod: "};
const char BtnBalance[]          PROGMEM = {"BtnBalance: "};
const char BtnSkimpySiren[]      PROGMEM = {"BtnSkimpySiren: "};
const char BtnOutOfContr[]       PROGMEM = {"BtnOutOfContr: "};



#define deltaGasPct        10                              // дельта отклонения от нормы датчика газа привышения, которой необходимо сигнализировать об утечки газа

// паузы
#define  delayOnContrTest     7                            // время паузы от нажатие кнопки до установки режима охраны в режиме тестирования
#define  timeAfterPressBtn    3000                         // время выполнения операций после единичного нажатия на кнопку
#define  timeSiren            20000                        // время работы сирены/тревоги в штатном режиме (милисекунды).
#define  timeSirenT           1000                         // время работы сирены/тревоги в тестовом режиме (милисекунды).
#define  timeSmsPIR1          120000                       // время паузы после последнего СМС датчика движения 1 (милисекунды)
#define  timeSmsPIR2          120000                       // время паузы после последнего СМС датчика движения 2 (милисекунды)
#define  timeSmsGas           120000                       // время паузы после последнего СМС датчика газа/дыма (милисекунды)
#define  timeSkimpySiren      400                          // время короткого срабатывания модуля сирены
#define  timeAllLeds          1200                         // время горение всех светодиодов во время включения устройства (тестирования светодиодов)
#define  timeTestBlinkLed     400                          // время мерцания светодиода при включеном режима тестирования
#define  timeRejectCall       3000                         // время пауза перед збросом звонка
#define  timeCheckGas         2000                         // время паузы между измирениями датчика газа/дыма (милисекунды)
#define  timeGasReady         600000                       // время паузы для прогрева датчика газа/дыма после включения устройства или датчика (милисекунды) (10 мин.)
#define  timeTestBoardLed     3000                         // время мерцания внутреннего светодиода на плате при включеном режима тестирования
#define  timeTrigTension      1000                         // во избежании ложного срабатывании датчика расстяжки включается только если датчик срабатывает больше чем указанное время
#define  timeTrigPirs         500                          // во избежании ложного срабатывании датчики движения включаются только если датчик срабатывает больше чем указанное время

//// КОНСТАНТЫ ДЛЯ ПИНОВ /////
#define SpecerPin 8
#define gsmLED 13
#define OutOfContrLED 12
#define OnContrLED 11
#define AlarmLED 10
#define BattPowerLED 9                          // LED для сигнализации о работе от резервного питания
#define boardLED 6                              // LED для сигнализации текущего режима с помошью внутреннего светодиода на плате

#define pinBOOT 5                               // нога BOOT для включения gsm модема 
#define Button 2                                // нога на кнопку
#define SirenGenerator 7                        // нога на сирену

// Спикер
#define sysTone 98                              // системный тон спикера
#define clickTone 98                            // тон спикера при нажатии на кнопку
#define smsSpecDur 100                          // длительность сигнала при получении смс команд и отправки ответа 
#define powSpecDur 100                          // длительность сигнала при обнаружении перехода на питание от батареии или от сети

//Power control 
#define pinMeasureVcc A0                        // нога чтения типа питания (БП или батарея)
#define netVcc      10.0                        // значения питяния от сети (вольт)
#define battVcc     0.1                         // значения питяния от батареи (вольт)

//Sensores
#define pinSH1      A2                          // нога на растяжку
#define pinGas      A3                          // нога датчика газа/дыма 
#define pinGasPower A4                          // нога управления питанием датчика газа/дыма 
#define pinPIR1     3                           // нога датчика движения 1
#define pinPIR2     4                           // нога датчика движения 2

//// КОНСТАНТЫ РЕЖИМОВ РАБОТЫ //// 
#define OutOfContrMod  1                        // снята с охраны
#define OnContrMod     2                        // установлена охрана

//// КОНСТАНТЫ EEPROM ////
#define E_mode           0                      // адресс для сохранения режимов работы 
#define E_inTestMod      1                      // адресс для сохранения режима тестирования
#define E_isRedirectSms  2                      // адресс для сохранения режима перенаправления всех смс
#define E_SMSRebooted    3                      // адресс для сохранения факта перезагрузки устройства по смс команде

#define E_delaySiren     4                      // адресс для сохранения длины паузы между срабатыванием датяиков и включением сирены (в сикундах)
#define E_delayOnContr   5                      // время паузы от нажатия кнопки до установки режима охраны (в сикундах)
#define E_intervalVcc    6                      // интервал между измерениями питания (в сикундах)
#define E_infContr       7                      // адресс для хранения режима оповещение о постановки/снятие на охрану через смс
#define E_gasOnlyOnContr 8                      // адресс для хранения режима работы датчика газа (выключать ли датчик газа в режиме не на охране)  
#define E_gasCalibr      9                      // калибровка датчика газа. Значение от датчика, которое воспринимать как 0 (отсутствие утечки газа)


#define E_SirenEnabled   23
#define E_PIR1Siren      24                     
#define E_PIR2Siren      25
#define E_TensionSiren   26

#define E_IsPIR1Enabled  27                     
#define E_IsPIR2Enabled  28
#define E_IsGasEnabled   29                   
#define E_TensionEnabled 30

// Количество нажатий на кнопку для включений режимова
#define E_BtnOnContr     31                     // количество нажатий на кнопку для установки на охрану
#define E_BtnInTestMod   32                     // количество нажатий на кнопку для включение/отключения режима тестирования 
#define E_BtnBalance     33                     // количество нажатий на кнопку для запроса баланса счета
#define E_BtnSkimpySiren 34                     // количество нажатий на кнопку для кратковременного включения сирены
#define E_BtnOutOfContr  35

#define E_BalanceUssd      60                   // Ussd код для запроса баланца

#define E_NUM_RebootAns    80                   // для промежуточного хранения номера телефона, на который необходимо будет отправить после перезагрузки сообщение о перезагрузке устройства 

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
                                                // 2 - установлено на охрану                                                
                                                // при добавлении не забываем посмотреть рездел 
                                                
                                               
bool interrupt = false;                         // разрешить/запретить обработку прырывания (по умолчанию запретить, что б небыло ложного срабатывания при старте устройства)
bool inTestMod = false;                         // режим тестирования датчиков (не срабатывает сирена и не отправляются СМС)
bool isSiren = false;                           // режим сирены

bool SirEnabled = false;                        // включена/выключена сирена глобально
bool TensionSir = false;                        // включена/выключена сирена для растяжки
bool PIR1Sir = false;                           // включена/выключена сирену для датчика движения 1
bool PIR2Sir = false;                           // включена/выключена сирену для датчика движения 2

bool isAlarm = false;                           // режим тревоги
bool reqSirena = false;                         // уст. в true когда сработал датчик и необходимо включить сирену
bool isRun = true;                              // флаг для управления выполнения блока кода в loop только при старте устройства
String numberAnsUssd = "";                      // для промежуточного хранения номера телефона, от которого получено gsm код и которому необходимо отправить ответ (баланс и т.д.)


unsigned long prSiren = 0;                      // время включения сирены (милисекунды)
unsigned long prAlarm = 0;                      // время включения светодиода тревоги (милисекунды)
unsigned long prLastPressBtn = 0;               // время последнего нажатие на кнопку (милисекунды)
unsigned long prTestBlinkLed = 0;               // время мерцания светодиода при включеном режима тестирования (милисекунды)
unsigned long prRefreshVcc = 0;                 // время последнего измирения питания (милисекунды)
unsigned long prReqSirena = 0;                  // время последнего обнаружения, что необходимо включать сирену

unsigned long intrVcc = 0;                      // интервал между измерениями питания

byte countPressBtn = 0;                         // счетчик нажатий на кнопку
bool SMSRebooted = false;                       // указываем была ли последний раз перезагрузка программным путем
bool WDRebooted = false;                        // true - произошло срабатывание сторожевого таймера
int GasPct = 0;                                 // хранит отклонение от нормы (в процентах) на основании полученого от дат.газа знаяения

PowerControl powCtr (netVcc, battVcc, pinMeasureVcc);   // контроль питания

MyGSM gsm(gsmLED, boardLED, pinBOOT);           // GSM модуль


// Датчики
DigitalSensor SenTension(pinSH1, timeTrigTension);
DigitalSensor SenPIR1(pinPIR1, timeTrigPirs);
DigitalSensor SenPIR2(pinPIR2, timeTrigPirs);
GasSensor SenGas(pinGas, pinGasPower);


void setup() 
{
  //Обработка аварийного выключения
  WDRebooted = MCUSR & B00001000;            // запоминаем состояние флага (была ли перегрузка)
  MCUSR &= ~(1 << WDRF);                     // сбрасываем флаг и отключаем WatshDog (иначе будет циклическая перезагрузка при старте утсройства)
  wdt_disable();
  
  delay(1000);                               // чтобы нечего не повисало при включении 
  
 // debug.begin(19200);
  pinMode(SpecerPin, OUTPUT);
  pinMode(gsmLED, OUTPUT);
  pinMode(OutOfContrLED, OUTPUT);
  pinMode(OnContrLED, OUTPUT);
  pinMode(AlarmLED, OUTPUT);
  pinMode(BattPowerLED, OUTPUT);              // LED для сигнализации о работе от резервного питания
  pinMode(boardLED, OUTPUT);                  // LED для сигнализации текущего режима с помошью внутреннего светодиода на плате
  pinMode(pinBOOT, OUTPUT);                   // нога BOOT на модеме
  pinMode(pinSH1, INPUT_PULLUP);              // нога на растяжку
  pinMode(pinPIR1, INPUT);                    // нога датчика движения 1
  pinMode(pinPIR2, INPUT);                    // нога датчика движения 2
  pinMode(pinGas, INPUT);                     // нога датчика газа/дыма 
  pinMode(pinGasPower, OUTPUT);               // нога управления питанием датчика газа/дыма 
  pinMode(Button, INPUT_PULLUP);              // кнопка для установки режима охраны
  pinMode(SirenGenerator, OUTPUT);            // нога на сирену
  pinMode(pinMeasureVcc, INPUT);              // нога чтения типа питания (БП или батарея)      

  StopSiren();                                // при включении устройства сирена должна быть по умолчанию выключена

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
          EEPROM.write(i, 0);                        // стираем все данные с EEPROM
        // установка дефолтных параметров
        EEPROM.write(E_mode, OutOfContrMod);         // устанавливаем по умолчанию режим не на охране
        EEPROM.write(E_inTestMod, 0);                // режим тестирования по умолчанию выключен
        EEPROM.write(E_isRedirectSms, 0);            // режим перенаправления всех смс по умолчанию выключен
        EEPROM.write(E_SMSRebooted, 0);              // факт перезагрузки устройства по умолчанию выключено (устройство не перезагружалось)
        EEPROM.write(E_delaySiren, 0);               // пауза между сработкой датчиков и включением сирены отключена (0 секунд) 
        EEPROM.write(E_delayOnContr, 25);            // пауза от нажатия кнопки до установки режима охраны (25 сек)
        EEPROM.write(E_intervalVcc, 0);              // интервал между измерениями питания (0 секунд)
        EEPROM.write(E_BalanceUssd, "***");          // Ussd код для запроса баланца
        EEPROM.write(E_infContr, 0);                 // информирование о снятии с охраны по смс по умолчанию отключено
        EEPROM.write(E_IsPIR1Enabled, 1);            
        EEPROM.write(E_IsPIR2Enabled, 1);
        EEPROM.write(E_IsGasEnabled, 0);
        EEPROM.write(E_TensionEnabled, 0);
        EEPROM.write(E_BtnOnContr, 1);
        EEPROM.write(E_BtnInTestMod, 2);
        EEPROM.write(E_BtnBalance, 3);
        EEPROM.write(E_BtnSkimpySiren, 4);        
        EEPROM.write(E_BtnOutOfContr, 0);
        EEPROM.write(E_gasOnlyOnContr, 0);            // режим при котором датчик газа включен во всех режимах системы (не только в режиме на охране)
        WriteIntEEPROM(E_gasCalibr, 1023);                    
        EEPROM.write(E_SirenEnabled, 1);              // сирена по умолчанию включена
        EEPROM.write(E_PIR1Siren, 1);                 // сирена при срабатывании датчика движения 1 по умолчанию включена
        EEPROM.write(E_PIR2Siren, 1);                 // сирена при срабатывании датчика движения 2 по умолчанию включена
        EEPROM.write(E_TensionSiren, 1);              // сирена при срабатывании растяжки по умолчанию включена             
        delay(1000);                                  // эмуляция перезагрузки
    }
  } 

  if (WDRebooted) 
  {
    gsm.Shutdown(false);                              // если произошла  перезагрузка то выключаем gsm модуль    
  }
   
  // блок тестирования спикера и всех светодиодов
  PlayTone(sysTone, 100);                          
  delay(500);
  digitalWrite(gsmLED, HIGH);
  digitalWrite(OutOfContrLED, HIGH);
  digitalWrite(OnContrLED, HIGH);
  digitalWrite(AlarmLED, HIGH);
  digitalWrite(BattPowerLED, HIGH);
  digitalWrite(boardLED, HIGH);
  delay(timeAllLeds);
  digitalWrite(gsmLED, LOW);
  digitalWrite(OutOfContrLED, LOW);
  digitalWrite(OnContrLED, LOW);
  digitalWrite(AlarmLED, LOW);
  digitalWrite(BattPowerLED, LOW);
  digitalWrite(boardLED, LOW);
  
  analogReference(INTERNAL);
  
  gsm.SwitchOn();                                       // включаем модем 
  powCtr.Refresh();                                     // читаем тип питания (БП или батарея)
  digitalWrite(BattPowerLED, powCtr.IsBattPower);       // сигнализируем светодиодом состояния питания
  gsm.Initialize();                                     // инициализация gsm модуля (включения, настройка)     
 
  digitalWrite(BattPowerLED, powCtr.IsBattPower);       // сигнализируем светодиодом режим питания (от батареи - светится, от сети - не светится)
  
  if (EEPROM.read(E_IsGasEnabled))                      // если включен датчик газа/дыма
    SenGas.TurnOnPower();                               // включаем питание датчика газа/дыма 
  else SenGas.TurnOffPower();                           // иначе выключаем питание датчика газа/дыма  
  
  attachInterrupt(0, ClickButton, FALLING);             // привязываем 0-е прерывание к функции ClickButton(). 
  interrupt = true;                                     // разрешаем обработку прырывания  

  inTestMod = EEPROM.read(E_inTestMod);                 // читаем тестовый режим из еепром
  SMSRebooted = EEPROM.read(E_SMSRebooted);             // читаем перезагружалось ли последний раз устройство по sms команде
  SenGas.gasClbr = ReadIntEEPROM(E_gasCalibr);          // читаем значения калибровки датчика газа/дыма
  SirEnabled = EEPROM.read(E_SirenEnabled);             // читаем включена или выключена сирена глобально
  TensionSir = EEPROM.read(E_TensionSiren);             // читаем включена или выключена сирена для растяжки
  PIR1Sir = EEPROM.read(E_PIR1Siren);                   // читаем включена или выключена сирена для датчика движения 1
  PIR2Sir = EEPROM.read(E_PIR2Siren);                   // читаем включена или выключена сирена для датчика движения 2
  intrVcc = EEPROM.read(E_intervalVcc);                 // читаем интервал между измерениями питания (в сикундах)
  intrVcc = intrVcc * 1000;                             // конвертируем интервал между измерениями питания с сикунд в милисекунды

  // чтение конфигураций с EEPROM
  if (EEPROM.read(E_mode) == OnContrMod)                   // читаем режим из еепром 
  {
    if (WDRebooted)                                        // если устройство запускается после перезагрузки (аварийной или по смс команде)
      Set_OnContrMod(false, EEPROM.read(E_infContr));      // то устанавливаем на охрану немедленно (без паузы перед установкой на охрану)
    else                                                   // если устройство запускается не после аварийной перезагрузки
      if (!Set_OnContrMod(true, EEPROM.read(E_infContr)))  // то устанавливаем на охрану с паузой перед установкой (что б была возможность отменить установку на охрану или покинуть помещение)
        Set_OutOfContrMod(0);                              // если установка на охрану прервана то устанавливаем режим Не на охране
      
  }
    else Set_OutOfContrMod(0);  
  wdt_enable (WDTO_8S);                                    // включаем watchdog (автоконтроль работы системы, если не сбрасывать счетчик больше 8 сек. то система перезагрузится)
}


void loop() 
{   
  if (GetElapsed(prRefreshVcc) > intrVcc || prRefreshVcc == 0)                              // проверяем сколько прошло времени после последнего измерения питания (секунды) (выдерживаем паузц между измерениями что б не загружать контроллер)
  {   
    PowerControl();                                                                         // мониторим питание системы
    prRefreshVcc = millis();
  }   

  gsm.Refresh();                                                                            // читаем сообщения от GSM модема                                                
  
  if(gsm.WasRestored)
  {
    SendSms(&GetStrFromFlash(sms_GsmWasRestored), &NumberRead(E_NUM1_OutOfContr));
    gsm.WasRestored = false;
  }  

  if(WDRebooted)
  {  
    if(SMSRebooted)
    {    
      SendSms(&GetStrFromFlash(sms_SMSRebooted), &NumberRead(E_NUM_RebootAns));
      SMSRebooted = false;
      EEPROM.write(E_SMSRebooted, false);
    }
    else
    {
      SendSms(&GetStrFromFlash(sms_WDRebooted), &NumberRead(E_NUM1_OutOfContr));
    }
    WDRebooted = false;
  }
  
  if (inTestMod && !isAlarm)                                                                // если включен режим тестирования и не тревога
  {
    if (GetElapsed(prTestBlinkLed) > timeTestBlinkLed)   
    {
      digitalWrite(AlarmLED, digitalRead(AlarmLED) == LOW);                                 // то мигаем внешним светодиодом
      if(mode == OutOfContrMod)
        digitalWrite(boardLED, digitalRead(boardLED) == LOW);                               // то мигаем внутренним светодиодом на плате
      prTestBlinkLed = millis();
    }
  }

  if (isAlarm)                                                                              // если тревога и если прошло заданное время отключаем тревогу
  {
    int cAlarm;
    if (!inTestMod) cAlarm = timeSiren;                                                     // если выключен режим тестирования то сохраняем установленное время тревоги
      else cAlarm = timeSirenT;                                                             // если включен режим тестирования то время тревоги сокращаем для удобства тестирования
    if (!SenGas.IsTrig && (GetElapsed(prAlarm) > cAlarm || prAlarm == 0))                   // если тревога больше установленного времени и не надо сигнализировать о газе то выключаем светодиод о индикации тревоги
      StopAlarm();
  }
      
  if (countPressBtn != 0)
  {     
    if (GetElapsed(prLastPressBtn) > timeAfterPressBtn)
    {       
      // установка на охрану countBtnOnContrMod
      if (mode == OutOfContrMod && countPressBtn == EEPROM.read(E_BtnOnContr))              // если кнопку нажали заданное количество для включение/отключения режима тестирования
      {        
        countPressBtn = 0;  
        Set_OnContrMod(true, EEPROM.read(E_infContr));       
      }
      else
      // включение/отключения режима тестирования
      if (mode == OutOfContrMod && countPressBtn == EEPROM.read(E_BtnInTestMod))            // если кнопку нажали заданное количество для включение/отключения режима тестирования
      {
        countPressBtn = 0;  
        PlayTone(sysTone, 250);                                                             // сигнализируем об этом спикером  
        InTestMod(!inTestMod);
      }
      else
      // запрос баланса счета
      if (mode == OutOfContrMod && countPressBtn == EEPROM.read(E_BtnBalance))              // если кнопку нажали заданное количество для запроса баланса счета
      {
        countPressBtn = 0;  
        PlayTone(sysTone, 250);                                                             // сигнализируем об этом спикером                        
        if(gsm.RequestUssd(&ReadStrEEPROM(E_BalanceUssd)))
          numberAnsUssd = NumberRead(E_NUM1_OutOfContr);                                    // сохраняем номер на который необходимо будет отправить ответ          
        else
          SendSms(&GetStrFromFlash(sms_WrongUssd), &NumberRead(E_NUM1_OutOfContr));         // если ответ пустой то отправляем сообщение об ошибке команды         */          
      }                                                                                
      else
      // кратковременное включение сирены (для тестирования модуля сирены)
      if (mode == OutOfContrMod && countPressBtn == EEPROM.read(E_BtnSkimpySiren))                      
      {
        countPressBtn = 0;  
        PlayTone(sysTone, 250);                                    
        SkimpySiren();
      }        
      
      // выключение режима контроля (если настроена для этого кнопка)
      else 
      if (mode == OnContrMod && countPressBtn == EEPROM.read(E_BtnOutOfContr))      
      {
        delay(200);                                                                         // пайза, что б не сливались звуковые сигналы нажатия кнопки и установки режима
        countPressBtn = 0;          
        Set_OutOfContrMod(EEPROM.read(E_infContr));             
      }
      else
      {
        PlayTone(sysTone, 250);   
        countPressBtn = 0;  
      }                            
   }
    else
    // снятие кнопкой с охраны (работает только в тестовом режиме, когда не блокируются прерывания)
    if (mode == OnContrMod && inTestMod)                                                    // в тестовом режиме можно сниамть кнопкой с охраны
    {
      delay(200);                                                                           // пайза, что б не сливались звуковые сигналы нажатия кнопки и установки режима
      countPressBtn = 0;  
      gsm.RejectCall();                                                                     // сбрасываем вызов      
      Set_OutOfContrMod(0);       
    }                  
  }

   ////// NOT IN CONTROL MODE ///////  
  if (mode == OutOfContrMod)                                                               // если режим не на охране
  {

    if (gsm.NewRing)                                                                       // если обнаружен входящий звонок
    {
      if (NumberRead(E_NUM1_OnContr).indexOf(gsm.RingNumber) > -1 ||                       // если найден зарегистрированный звонок то ставим на охрану
          NumberRead(E_NUM2_OnContr).indexOf(gsm.RingNumber) > -1 ||
          NumberRead(E_NUM3_OnContr).indexOf(gsm.RingNumber) > -1           
         )      
      {               
        if (!isAlarm) digitalWrite(AlarmLED, LOW);                                         // если нет необходимости сигнализировать о тревоге то выключаем светодиод, который может моргать если включен тестовый режим
        digitalWrite(boardLED, LOW);
        delay(timeRejectCall);                                                             // пауза перед збросом звонка        
        gsm.RejectCall();                                                                  // сбрасываем вызов               
        Set_OnContrMod(false, EEPROM.read(E_infContr));                                    // устанавливаем на охрану без паузы              
      }
      else gsm.RejectCall();                                                               // если не найден зарегистрированный звонок то сбрасываем вызов (без паузы)      
    gsm.ClearRing();                                                                       // очищаем обнаруженный входящий звонок    
    }    
  }                                                                                        // end OutOfContrMod 
  else 
  
  ////// IN CONTROL MODE ///////  
  if (mode == OnContrMod)                                                                  // если в режиме охраны
  {
    CheckSensors();                                                                        // опрашиваем все датчики для обновления их состояний
    if (isSiren && !inTestMod)
    {
      if (GetElapsed(prSiren) > timeSiren)                                                 // если включена сирена и сирена работает больше установленного времени то выключаем ее
        StopSiren();
    } 
      
    if (SenTension.State && !SenTension.IsTrig && EEPROM.read(E_TensionEnabled))           // проверяем растяжку только если она не срабатывала ранее (что б смс и звонки совершались единоразово)
    {
       StartAlarm();                                                                       // сигнализируем светодиодом о тревоге
       if (!inTestMod && TensionSir) 
       {
         reqSirena = true;             
         if (prReqSirena == 1) prReqSirena = millis();                         
       }
       SenTension.PrTrigTime = millis();                                                   // запоминаем когда сработал датчик для отображения статуса датчика
       SenTension.IsTrig = true;            
       SenTension.IsAlarm = true;                   
    }   
    
    if (SenPIR1.State && EEPROM.read(E_IsPIR1Enabled))
    {       
      StartAlarm();                                                                        // сигнализируем светодиодом о тревоге
      if (!inTestMod && PIR1Sir) 
      {
        reqSirena = true;
        if (prReqSirena == 1) prReqSirena = millis();
      }
      SenPIR1.PrTrigTime = millis();                                                       // запоминаем когда сработал датчик для отображения статуса датчика      
      if (GetElapsed(SenPIR1.PrAlarmTime) > timeSmsPIR1 || SenPIR1.PrAlarmTime == 0)       // если выдержена пауза после последнего звонка и отправки смс 
        SenPIR1.IsAlarm = true;           
    }

    if (SenPIR2.State && EEPROM.read(E_IsPIR2Enabled))
    { 
      StartAlarm();                                                                        // сигнализируем светодиодом о тревоге
      if (!inTestMod && PIR2Sir)
      {
        reqSirena = true;
        if (prReqSirena == 1) prReqSirena = millis();
      }
      SenPIR2.PrTrigTime = millis();                                                       // запоминаем когда сработал датчик для отображения статуса датчика      
      if (GetElapsed(SenPIR2.PrAlarmTime) > timeSmsPIR2 || SenPIR2.PrAlarmTime == 0)       // если выдержена пауза после последнего звонка и отправки смс
        SenPIR2.IsAlarm = true;           
    }   
   
    CheckSensors();                                                                        // опрашиваем все датчики для обновления их состояний
         
    if (reqSirena 
      && (GetElapsed(prReqSirena)/1000 >= EEPROM.read(E_delaySiren) || prReqSirena == 0))      
    {     
      interrupt = false;                                                                   // блокируем обработку прерывания от кнопки (кнопку можно нажимать только до включения сирены)
      reqSirena = false;            
      if (!isSiren)
      {
        StartSiren();
        prReqSirena = 0;                                                                   // устанавливаем в 0 для отключения паузы между следующим срабатыванием датчиков и включением сирены
      }
      else
        prSiren = millis();      
    }      
    
    if (SenTension.IsAlarm)                                                                // проверяем состояние растяжки и если это первое обнаружение обрыва (TensionTriggered = false) то выполняем аналогичные действие
    { 
      //gsm.RejectCall();                                                                    // если уже оповещается вызовом о тревоге то прырываем звонок так как смс оповещение в приоритете
      if (gsm.IsAvailable())
      {
        if (!inTestMod)    
        {
          gsm.SendSms(&GetStrFromFlash(sms_TensionCable), &NumberRead(E_NUM1_OutOfContr));
          CheckSensors();                                                                  // чтобы нечего не пропустить перечитываем состояние датчиков после sms так как sms может отправляется долго 
        }        
        if(!SenPIR1.IsAlarm && !SenPIR2.IsAlarm)
        {
          gsm.Call(&NumberRead(E_NUM1_OutOfContr));  
          CheckSensors();                                                                  // чтобы нечего не пропустить перечитываем состояние датчиков после звонка так как звонок может отправляется долго
        }
        SenTension.IsAlarm = false;
      }                                                    
    }
    
    if (SenPIR1.IsAlarm)                                                                   // проверяем состояние 1-го датчика движения
    {                                                                 
      //gsm.RejectCall();                                                                  // если уже оповещается вызовом о тревоге то прырываем звонок так как смс оповещение в приоритете
      if (gsm.IsAvailable())              
      {  
        if (!inTestMod)  
        {
          gsm.SendSms(&GetStrFromFlash(sms_PIR1), &NumberRead(E_NUM1_OutOfContr));         // если не включен режим тестирование отправляем смс        
          CheckSensors();                                                                  // чтобы нечего не пропустить перечитываем состояние датчиков после sms так как sms может отправляется долго 
        }        
        if (!SenPIR2.IsAlarm)
        {
          gsm.Call(&NumberRead(E_NUM1_OutOfContr));  
          CheckSensors();                                                                  // чтобы нечего не пропустить перечитываем состояние датчиков после звонка так как звонок может отправляется долго
        }
        SenPIR1.PrAlarmTime = millis();
        SenPIR1.IsAlarm = false;
      }
    }
    
    if (SenPIR2.IsAlarm)                                                                   // проверяем состояние 2-го датчика движения
    {      
      //gsm.RejectCall();                                                                  // если уже оповещается вызовом о тревоге то прырываем звонок так как смс оповещение в приоритете
      if (gsm.IsAvailable())
      {  
        if (!inTestMod)
        {
          gsm.SendSms(&GetStrFromFlash(sms_PIR2), &NumberRead(E_NUM1_OutOfContr));        
          CheckSensors();                                                                  // чтобы нечего не пропустить перечитываем состояние датчиков после sms так как sms может отправляется долго 
        }
        gsm.Call(&NumberRead(E_NUM1_OutOfContr));  
        CheckSensors();                                                                    // чтобы нечего не пропустить перечитываем состояние датчиков после sms так как sms может отправляется долго 
        SenPIR2.PrAlarmTime = millis();
        SenPIR2.IsAlarm = false;
      }
    }
    
    if (gsm.NewRing)                                                                       // если обнаружен входящий звонок
    {      
      if (NumberRead(E_NUM1_OutOfContr).indexOf(gsm.RingNumber) > -1 ||                    // если найден зарегистрированный звонок то снимаем с охраны
          NumberRead(E_NUM2_OutOfContr).indexOf(gsm.RingNumber) > -1 || 
          NumberRead(E_NUM3_OutOfContr).indexOf(gsm.RingNumber) > -1 ||
          NumberRead(E_NUM4_OutOfContr).indexOf(gsm.RingNumber) > -1         
         )               
      {                    
        delay(timeRejectCall);                                                             // пауза перед збросом звонка
        Set_OutOfContrMod(EEPROM.read(E_infContr));                                        // снимаем с охраны                       
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
        prReqSirena = 0;                                                                  // устанавливаем в 0 для отключения паузы между следующим срабатыванием датчиков и включением сирены   
      }
      else gsm.RejectCall();                                                              // если не найден зарегистрированный звонок то сбрасываем вызов (без паузы)
      gsm.ClearRing();                                                                    // очищаем обнаруженный входящий звонок 
    }         
    CheckSensors();                                                                       // чтобы нечего не пропустить перечитываем состояние датчиков
  }                                                                                       // end OnContrMod
 
 
  // обработка датчика газа/дыма
  if (SenGas.IsTurnOn)                                                                    // если датчик газа/дыма включен
  {
    // обработка датчика газа/дыма
    if (SenGas.PrTrigTime != 0) 
      if (GetElapsed(SenGas.PrTrigTime)/1000 > 43200)                                     // если время последнего срабатывания больше чем 12 часов то обнуляем его 
        SenGas.PrTrigTime = 0;
    
    if (!SenGas.IsReady)
      if (GetElapsed(SenGas.PrGasTurnOn) > timeGasReady)                                  // если прошло достаточно времени для прогревания датчика газа/дыма после включения устройства то указываем что он готов к опрашиванию.     
        SenGas.IsReady = true;                                                            // то указываем что он готов к опрашиванию.          
        
    if (SenGas.IsReady) 
    { 
      if (GetElapsed(SenGas.PrCheckGas) > timeCheckGas || SenGas.PrCheckGas == 0)         // если датчик газа прогрет и готов к опрашиванию то проверяем сколько прошло времени после последнего измирения датчика газа    
      { 
        GasPct = SenGas.GetPctFromNorm();                                                 // сохраняем отклонение от нормы (в процентах) на основании полученого от дат.газа знаяения 
        SenGas.PrCheckGas = millis(); 
      }
    
      if (GasPct > deltaGasPct)                                                             // если отклонение больше заданой дельты то сигнализируем о прывышении уровня газа/дыма 
      {       
        StartAlarm();                                                                       // сигнализируем светодиодом о тревоге
        if (!SenGas.IsTrig && inTestMod) PlayTone(sysTone, 100);                            // если включен режим тестирование и это первое срабатывание то сигнализируем спикером  
        SenGas.IsTrig = true;
        SenGas.PrTrigTime = millis();                                                       // запоминаем когда сработал датчик для отображения статуса датчика
        if (GetElapsed(SenGas.PrAlarmTime) > timeSmsGas || SenGas.PrAlarmTime == 0)         // если выдержена пауза после последнего звонка и отправки смс 
          SenGas.IsAlarm = true;
      }
      else SenGas.IsTrig = false;    
  
      if (SenGas.IsAlarm)                                                                      
      {                                                                 
        if (gsm.IsAvailable())              
        {  
          if (!inTestMod)  
            gsm.SendSms(&(GetStrFromFlash(sms_Gas)+ "\n" + GetStrFromFlash(GasVal) + String(GasPct) + "%"), &NumberRead(E_NUM1_OutOfContr));     // если не включен режим тестирование отправляем смс
          gsm.Call(&NumberRead(E_NUM1_OutOfContr));                                        // сигнализируем звонком о сработке датчика
          SenGas.PrAlarmTime = millis();
          SenGas.IsAlarm = false;
        }
      }
    }
  }
  ///----------------
  
  if (gsm.NewUssd)                                                                       // если доступный новый ответ на Ussd запрос
  {
    SendSms(&gsm.UssdText, &numberAnsUssd);                                              // отправляем ответ на Ussd запрос
    gsm.ClearUssd();                                                                     // сбрасываем ответ на gsm команду 
  }
  
  if(!SenTension.IsAlarm && !SenPIR1.IsAlarm && !SenPIR2.IsAlarm && !SenGas.IsAlarm)
    ExecSmsCommand();                                                                    // если нет необработаных датчиков то проверяем доступна ли новая команда по смс и если да то выполняем ее
  
  if(mode == OnContrMod)                                                                 // если мы в режиме контроля то чтобы нечего не пропустить перечитываем состояние датчиков
    CheckSensors();                                                                         
  
  wdt_reset();                                                                           // сбрасываем счетчик watchdog (автоконтроль работы системы, если больше 8 сек. не сбрасывать то система перезагрузится)
}



//// ------------------------------- Functions --------------------------------- ////
void CheckSensors()                                                                      // процедура для опрашивание всех датчиков и обновления их состояний (свойство State) (true - датчик сработал, False - датчик в покое)
{
  SenTension.CheckSensor();
  SenPIR1.CheckSensor();
  SenPIR2.CheckSensor();
}

void ClickButton()
{ 
  if (digitalRead (Button) == LOW)                      // защита от ложного срабатывания при касании проводником к контактам
  {
    if (interrupt)                    
    {    
      static unsigned long millis_prev;
      if(millis() - 300 > millis_prev) 
      {          
        PlayTone(clickTone, 40);    
        countPressBtn++;
        prLastPressBtn = millis();         
      }       
      millis_prev = millis();           
    }
  }      
}

bool Set_OutOfContrMod(bool infContr)                   // метод для снятие с охраны
{ 
  mode = OutOfContrMod;                                 // снимаем с охраны

  if (EEPROM.read(E_gasOnlyOnContr) == 1)               // если включен режим включения датчика газа только в режиме на охране    
    if (EEPROM.read(E_IsGasEnabled) == 1)               // и датчик газа активирован в системе
      SenGas.TurnOffPower();                            // то выключаем датчик газа
  
  interrupt = true;                                     // разрешаем обрабатывать прерывания
  digitalWrite(OnContrLED, LOW);
  if (!SenGas.IsTrig)
    StopAlarm(); 
  digitalWrite(OutOfContrLED, HIGH);
  digitalWrite(boardLED, LOW);
  PlayTone(sysTone, 500);
  StopSiren();                                          // выключаем сирену                         
  reqSirena = false; 
    
  SenPIR1.ResetSensor();
  SenPIR2.ResetSensor();
  SenTension.ResetSensor();
  
  EEPROM.write(E_mode, mode);                           // пишим режим в еепром, что б при следующем включении устройства, оно оставалось в данном режиме
 
  gsm.RejectCall();                                     // сбрасываем вызов  
  
  if (!inTestMod && infContr)
    SendSms(&GetStrFromFlash(sms_InfContrOff), &NumberRead(E_NUM1_OutOfContr));  // отправляем смс о снятии с охраны;
  return true;
}

bool Set_OnContrMod(bool IsWaiting, bool infContr)      // метод для установки на охрану
{ 
  if (IsWaiting == true)                                // если включен режим ожидание перед установкой охраны, выдерживаем заданную паузу, что б успеть покинуть помещение
  { 
    digitalWrite(OutOfContrLED, LOW);   
    if (!isAlarm) digitalWrite(AlarmLED, LOW);
    digitalWrite(boardLED, LOW);                        // во время ожидание перед установкой на охрану, что б не горел внутренний светодиод, выключаем его (он моргает в режиме тестирования)
    byte timeWait = 0;
    if (inTestMod) timeWait = delayOnContrTest;         // если включен режим тестирования, устанавливаем для удобства тестирования меньшую паузу
    else timeWait = EEPROM.read(E_delayOnContr);        // если режим тестирования выклюяен, используем обычную паузу
    for(byte i = 0; i < timeWait; i++)   
    {               
      if (countPressBtn > 0)                            // если пользователь нажал на кнопку то установка на охрану прерывается
      {
        Break_OnContrMod();
        return false;
      }        
      if (i < (timeWait * 0.7))                         // первых 70% паузы моргаем медленным темпом
      {
        BlinkLEDSpecer(OnContrLED, 0, 500, 0);               
        if (countPressBtn == 0) delay(500);
      }
      else                                              // последних 30% паузы ускоряем темп
      {
        BlinkLEDSpecer(OnContrLED, 0, 250, 250); 
        if (countPressBtn == 0) BlinkLEDSpecer(OnContrLED, 0, 250, 250);              
      }         
      wdt_reset();                                      // сбрасываем счетчик watchdog (если не сбрасывать больше 8 сек. то система перезагрузится)
    }
  }
  
  if (EEPROM.read(E_TensionEnabled) &&  SenTension.CheckSensor())            // проверяем растяжку, если она нарушена (не закрыта дверь) то прырываем установку на охрану и информируем смс об этом
  {
    if (!inTestMod)                                                 // если не включен режим тестирования то отправляем смс о прырывании установки на охрану;
      SendSms(&GetStrFromFlash(sms_ErrorTension), &NumberRead(E_NUM1_OutOfContr));
    Break_OnContrMod();
    return false;
  }
  
  //установка на охрану     
  mode = OnContrMod;                                    // ставим на охрану 

  if (EEPROM.read(E_gasOnlyOnContr) == 1)               // если включен режим включения датчика газа только в режиме на охране
    if (EEPROM.read(E_IsGasEnabled) == 1)               // и датчик газа активирован в системе
      SenGas.TurnOnPower();                             // то включаем датчик газа
    
  if (!inTestMod && EEPROM.read(E_BtnOutOfContr)==0)    // если не включен режим тестирования и отключена возможность снимать с охраны кнопкой 
    interrupt = false;                                  // то запрещаем обрабатывать прерывания
  
  // установка переменных в дефолтное состояние  
  SenPIR1.ResetSensor();
  SenPIR2.ResetSensor();
  SenTension.ResetSensor();
                                                 
  digitalWrite(OutOfContrLED, LOW);
  digitalWrite(boardLED, HIGH);
  digitalWrite(OnContrLED, HIGH);
  PlayTone(sysTone, 500);  
  EEPROM.write(E_mode, mode);                           // пишим режим в еепром, что б при следующем включении устройства, оно оставалось в данном режиме
  delay (2500);                                         // дополнительная пауза так как датчик держит лог. единицу 2,5
  prReqSirena = 1;                                      // устанавливаем в 1 для активации паузы между срабатыванием датчиков и включением сирены
  
  if (!inTestMod && infContr)                         
    SendSms(&GetStrFromFlash(sms_InfContrOn), &NumberRead(E_NUM1_OutOfContr));  // отправляем смс о постановке на охрану;
  return true;
}

void Break_OnContrMod()
{
    countPressBtn = 0;        
    digitalWrite(OnContrLED, LOW);  
    digitalWrite(OutOfContrLED, HIGH);
}

void StartSiren()
{  
  if (SirEnabled)                                       // и сирена не отключена в конфигурации
    digitalWrite(SirenGenerator, HIGH);                 // включаем сирену через через транзистор или релье  
  isSiren = true; 
  prSiren = millis(); 
  prAlarm = millis();                                   // время работы светодиода тривоги увеличивается до времени сирены (сирена и светодиод должны выключаться одновременно)
}

void StopSiren()                                        // !!! этот метод не может содержать код зависимый от конфигурации с EEPROM памяти так как этот метод вызывается в Setup блоке до блока сброса всех настроек EEPROM и по этому возможное считывание не актуальных настроек 
{
  digitalWrite(SirenGenerator, LOW);                    // выключаем сирену через транзистор или релье
  isSiren = false;   
}

void StartAlarm()
{
  if (!isAlarm)                                         // если еще невклюена тревога то включаем ее (проверяем для того что бы не выполнять лишний раз метод)
  {
    isAlarm = true;
    digitalWrite(AlarmLED, HIGH);                       // сигнализируем светодиодом о тревоге
    if (inTestMod)                                      // если не включен тестовый режим и это первое 
    {
      PlayTone(sysTone, 100);                           // если включен режим тестирование то сигнализируем еще и спикером
    }   
  } 
  prAlarm = millis(); 
}

void StopAlarm()
{
  digitalWrite(AlarmLED, LOW);                          // Если не надо сигнализировать о газе то выключаем светодиод о индикации тревоги 
  isAlarm = false;  
}

void PowerControl()                                                                           // метод для обработки событий питания системы (переключение на батарею или на сетевое)
{
  powCtr.Refresh();    
  digitalWrite(BattPowerLED, powCtr.IsBattPower);

  if (!powCtr.IsBattPowerPrevious && powCtr.IsBattPower)                                      // если предыдущий раз было от сети а сейчас от батареи (пропало сетевое питание 220v)
  {
    if (mode != OnContrMod || inTestMod)                                                      // если не в режиме охраны или включен режим тестирования 
      PlayTone(sysTone, powSpecDur);                                                          // то сигнализируем спикером о переходе на резервное питание
    if (!inTestMod)                                                                           // если не включен режим тестирования
      gsm.SendSms(&GetStrFromFlash(sms_BattPower), &NumberRead(E_NUM1_OutOfContr));           // отправляем смс о переходе на резервное питание            
  }
  
  if (powCtr.IsBattPowerPrevious && !powCtr.IsBattPower)                                      // если предыдущий раз было от батареи a сейчас от сети (сетевое питание 220v возобновлено) и если не включен режим тестирования  
  {
    if (mode != OnContrMod || inTestMod)                                                      // если не в режиме охраны или включен режим тестирования 
      PlayTone(sysTone, powSpecDur);                                                          // то сигнализируем спикером о возобновлении питания от сетевое 220v 
    if(!inTestMod)                                                                            // если не включен режим тестирования
      gsm.SendSms(&GetStrFromFlash(sms_NetPower), &NumberRead(E_NUM1_OutOfContr));            // отправляем смс о возобновлении питания от сетевое 220v 
  }
}

void SkimpySiren()                                                                        // метод для кратковременного включения сирены (для теститования сирены)
{
  digitalWrite(AlarmLED, HIGH);                                                           // включаем светодиод тревоги
  digitalWrite(SirenGenerator, HIGH);                                                     // включаем сирену                                 
  delay(timeSkimpySiren);                                                                 // кратковременный период на который включается сирена
  digitalWrite(SirenGenerator, LOW);                                                      // выключаем сирену через релье  
  if (!isAlarm) digitalWrite(AlarmLED, LOW);                                              // выключаем светодиод если нет необходимости сигнализировать о тревоге
}

void InTestMod(bool state)
{
  inTestMod = state;                                                                      // включаем/выключаем режим тестирование датчиков          
  EEPROM.write(E_inTestMod, inTestMod);                                                   // пишим режим тестирование датчиков в еепром    
}


void PlayTone(byte tone, unsigned int duration) 
{  
  if (!isAlarm) digitalWrite(AlarmLED, LOW);                                              // если нет необходимости сигнализировать о тревоге то выключаем светодиод, который может моргать если включен тестовый режим
  if (mode != OnContrMod) digitalWrite(boardLED, LOW);
  for (unsigned long i = 0; i < duration * 1000L; i += tone * 2) 
  {
    digitalWrite(SpecerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(SpecerPin, LOW);
    delayMicroseconds(tone);
  }
} 

void BlinkLEDSpecer(byte pinLED,  unsigned int millisBefore,  unsigned int millisHIGH,  unsigned int millisAfter)     // метод для включения спикера и заданного светодиода на заданное время
{ 
  digitalWrite(pinLED, LOW);                          
  delay(millisBefore);  
  digitalWrite(pinLED, HIGH);                                          
  PlayTone(sysTone, millisHIGH);
  digitalWrite(pinLED, LOW);
  delay(millisAfter);
}

bool SendSms(String *text, String *phone)      // собственный метод отправки СМС (возвращает true если смс отправлена успешно) создан для инкапсуляции сигнализации об отправки смс
{
  if(gsm.SendSms(text, phone))                 // если смс отправлено успешно 
  {  
    if (mode != OnContrMod || inTestMod)       // если не в режиме охраны или включен режим тестирования то сигнализируем спикером об отправке смс
      PlayTone(sysTone, smsSpecDur);                 
    return true;
  }
  else return false;
}

// Hard перезагрузка, после которой отправляется sms о перезагрузке (полная перезагрузка устройства средствами WatshDog с перезагрузкой gsm модуля) (Примечание: Не применять если gsm модуль еще не загрузился) 
void Reboot()                       
{
  digitalWrite(AlarmLED, LOW);
  digitalWrite(OutOfContrLED, LOW);
  digitalWrite(OnContrLED, LOW);      
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}  
}

//Метод ExecSmsCommand() очень большой и по этой причине вынесен в файл ExecSmsCommand.ino
