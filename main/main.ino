/// GSM сигналка c установкой по кнопке
/// датчиком на прерывателях и движения

// донат, https://money.yandex.ru/to/410012486954705

#include <EEPROM.h>

//#include <SoftwareSerial.h>                 // если программный
//SoftwareSerial gsm(7, 8); // RX, TX
#define gsm Serial                           // если аппаратный в UNO
//#define gsm Serial1                          // если аппаратный в леонардо


//// НАСТРОЕЧНЫЕ КОНСТАНТЫ /////
// номера телефонов
#define TELLNUMBER "ATD+380509151369;"                // номен на который будем звонить
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
const String smsText_PIR1         = "ALARM: PIR1 sensor";                 //текст смс для датчика движения

// паузы
const int timeSiren = 20;                     // время работы сирены 20 сек.
const int timeWaitingInContr = 25;            // Время паузы от нажатие кнопки до установки режима охраны
const int timeHoldingBtn = 2;                 // время удерживания кнопки для включения режима охраны  2 сек.

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
#define pinPIR1 6                             // нога датчика движения


//// КОНСТАНТЫ РЕЖИМОВ РАБОТЫ //// 
const byte NotInContrMod = 1;                 // снята с охраны
const byte InContrMod = 2;                    // установлена охрана


//// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ////
byte mode = NotInContrMod;          // 1 - снята с охраны                                  
                                    // 2 - установлена охрана
                                    // при добавлении не забываем посмотреть на 75 строку
bool btnIsHolding = false;
int ch = 0;
String val = "";

bool inTestMod = false;                 // режим тестирования датчиков (не срабатывает сирена и не отправляются СМС)

bool isSiren = false;                   // режим сирены
long prMillisSiren = 0;                 // здесь будет храниться время последнего включения сирены

bool controlTensionCable = true;        // включаем контроль растяжки


void setup() {
  delay(1000);                                //// !! чтобы нечего не повисало при включении
  
  gsm.begin(9600);                            /// незабываем указать скорость работы UART модема
  pinMode(SpecerPin, OUTPUT);
  pinMode(gsmLED, OUTPUT);
  pinMode(NotInContrLED, OUTPUT);
  pinMode(InContrLED, OUTPUT);
  pinMode(SirenLED, OUTPUT);
  pinMode(pinBOOT, OUTPUT);                   /// нога BOOT на модеме
  pinMode(SH1, INPUT_PULLUP);                 /// нога на растяжку
  pinMode(pinPIR1, INPUT);                    /// нога датчика движения
  pinMode(Button, INPUT_PULLUP);              /// кнопка для установки режима охраны
  pinMode(SirenGenerator, OUTPUT);            /// нога на сирену
  pinMode(power, INPUT);                      /// нога чтения типа питания (БП или батарея)    

  digitalWrite(SirenGenerator, HIGH);         /// выключаем сирену через релье
                             
  InitializeGSM();                          // Инициализируем модемом (включения, настройка)  
  mode = EEPROM.read(0);                      // читаем режим из еепром  
  if (mode == InContrMod) Set_InContrMod(1);          
  else Set_NotInContrMod();

  inTestMod = EEPROM.read(1);                 // читаем тестовый режим из еепром    
}

void loop() 
{  
  if (isSiren == 1)                           // если включена сирена проверяем время ее работы
  {
    if (GetElapsed(prMillisSiren) > (timeSiren * 1000))          // если сирена работает больше установленного времени то выключаем ее
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
    bool sPIR1 =  SensorTriggered_PIR1();
                                   
    if ((sTensionCable && controlTensionCable) || sPIR1)              // если обрыв
    {                                                                 
      if (isSiren == 0) StartSiren();                                 // включаем сирену
            
      gsm.println(TELLNUMBER);                                        // отзваниваемся

      if (sTensionCable && !inTestMod)                               // отправляем СМС если сработал обрыв растяжки и не включен режим тестирование
        SendSMS(String(smsText_TensionCable), String(SMSNUMBER));    
      if (sPIR1 && !inTestMod)                                        // отправляем СМС если сработал датчик движения и не включен режим тестирование
        SendSMS(String(smsText_PIR1), String(SMSNUMBER));            
      
      if (sTensionCable) controlTensionCable = false;                 // отключаем контроль растяжки что б сирена не работала постоянно после разрыва растяжки
    }
  }  
 
  // ищим RING
  // если нашли, опрашиваем кто это и снимаем с охраны
  
   if (gsm.available())                                     // если GSM модуль что-то послал нам, то
   {    
    while (gsm.available())                                 //сохраняем входную строку в переменную val
    {  
      ch = gsm.read();
      val += char(ch);
      delay(10);
    }
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
        BlinkLED(gsmLED, 500, 250);                         // сигнализируем об этом      
        delay(1500);                                        // небольшая пауза перед збросом звонка
        gsm.println("ATH0");                                // сбрасываем вызов        
        Set_NotInContrMod();                                // снимаем с охраны         
      }
      else
      if (mode == NotInContrMod                                // если включен режим снята с охраны и найден зарегистрированный звонок то снимаем с охраны
          && (val.indexOf(NUMBER1_InContr) > -1 
              || val.indexOf(NUMBER2_InContr) > -1 
              || val.indexOf(NUMBER3_InContr) > -1 
              || val.indexOf(NUMBER4_InContr) > -1
             )
         )       
      {  
        // DOTO
        //Serial.println("--- MASTER RING DETECTED ---");
        BlinkLED(gsmLED, 500, 250);                         // сигнализируем об этом
        delay(7000);                                        // большая пауза перед збросом звонка
        gsm.println("ATH0");                                // сбрасываем вызов        
        Set_InContrMod(0);                                  // устанавливаем на охрану без паузы      
      }
      else gsm.println("ATH0");                             // если не найден зарегистрированный звонок то сбрасываем вызов (без паузы)
      val = "";
    }
    //else Serial.println(val);                             // печатаем в монитор порта пришедшую строку         
  }      
}



//// ------------------------------- Functions --------------------------------- ////
void InitializeGSM()
{
  delay(1000);                            
  digitalWrite(gsmLED, HIGH);           // на время включаем лед  
  digitalWrite(pinBOOT, LOW);           // включаем модем 
   
  // нужно дождатся включения модема и соединения с сетью
  delay(2000);    
  gsm.println("ATE0");                  // выключаем эхо  

  delay(100);
  gsm.println("AT+CLIP=1");  //включаем АОН
    
  // настройка смс
  delay(100);
  gsm.println("AT+CMGF=1");             //режим кодировки СМС - обычный (для англ.)
  delay(100);
  gsm.println("AT+CSCS=\"GSM\"");       //режим кодировки текста
  delay(100);
    
  while(1)                              // ждем подключение модема к сети
  {                             
    gsm.println("AT+COPS?");
    if (gsm.find("+COPS: 0")) break;
    BlinkLED(gsmLED, 0, 700);         // блымаем светодиодом      
  }

  gsm.println("AT+CLIP=1");             // включаем АОН,
  
  //Serial.println("Modem OK"); 
  BlinkLED(gsmLED, 500, 150);          // блымаем светодиодом 
  BlinkLED(gsmLED, 150, 150);         // блымаем светодиодом   
  delay(200); 
}

// для подсчета сколько прошло милисикунд после последнего срабатывания события (сирена, звонок и т.д.)
unsigned long GetElapsed(long prEventMillis)
{
  unsigned long tm = millis();
  return (tm >= prEventMillis) ? tm - prEventMillis : 0xFFFFFFFF - prEventMillis + tm + 1;  //возвращаем милисикунды после последнего события
}

////// Function for setting of mods ////// 
bool Set_NotInContrMod()
{
  controlTensionCable = true;           // включаем контроль растяжки    
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
      {
        digitalWrite(InContrLED, HIGH);                 // включаем/выключаем режим тестирование датчиков 
        PlayTone(specerTone, 500);
        digitalWrite(InContrLED, LOW);
        delay (500);
      }
      else                                              // последних 30% паузы ускоряем темп
      {
        digitalWrite(InContrLED, HIGH);
        PlayTone(specerTone, 250);
        digitalWrite(InContrLED, LOW);
        delay (250);
        digitalWrite(InContrLED, HIGH);
        PlayTone(specerTone, 250);
        digitalWrite(InContrLED, LOW);
        delay (250);        
      }
      if (ButtonIsHold(timeHoldingBtn))                 // проверяем не нажата ли кнопка и если кнопка удерживается заданое время, функция вернет true и установка на охрану прерывается     
      {
        Set_NotInContrMod();
        return false;      
      }            
    }
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
  prMillisSiren = millis();
}


void  StopSiren()
{
  digitalWrite(SirenLED, LOW);
  digitalWrite(SirenGenerator, HIGH);                    // выключаем сирену через релье
  isSiren = 0;
  prMillisSiren = millis();
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

bool SensorTriggered_PIR1()                  // датчик движения
{
  if (digitalRead(pinPIR1) == HIGH) return true;
  else return false;
}

//// Методы GSM модуля ////
void SendSMS(String text, String phone)       //процедура отправки СМС
{
  //Serial.println("SMS send started");
  gsm.println("AT+CMGS=\"" + phone + "\"");
  delay(500);
  gsm.print(text);
  delay(500);
  gsm.print((char)26);
  delay(500);
  //Serial.println("SMS send complete");
  delay(2000);
}

// Блымание светодиодом
void BlinkLED(int pinLED, int millisLOW, int millisHIGH)
{ 
  digitalWrite(pinLED, LOW);                          
  delay(millisLOW);  
  digitalWrite(pinLED, HIGH);                 // блымаем светодиодом
  delay(millisHIGH); 
  digitalWrite(pinLED, LOW);
}
