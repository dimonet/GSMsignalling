#include "Arduino.h"

// Настройки gsm модуля
#define AVAILABLE_TIMEOUT  1000                 // врем ожидание готовности модема (мили сек.)  
#define TIME_DIAGNOSTIC    60000                // время проверки gsm модуля(мили сек.) (60000 = раз в минуту, 900000 = раз в 15 мин, 180000 - раз в пол часа). Если обнаружен что gsm потерял сеть то постоянно горит gsm led сигнализируя об этом
#define INITIALIZE_TIMEOUT 120000               // врем ожидания поиска сети модема (мили сек.)   
#define SMS_LIMIT          150                  // максимальное куличество символов в смс (большео лимита все символы обрезается)

// Статусы gsm модуля
#define NotResponding      1                    // модуль не отвечает или отсутствует
#define NotRegistered      2                    // модуль потерял сеть
#define Registered         3                    // модуль нашел сеть и работает в штатном режиме
#define NotAvailable       4                    // Модуль не возможно проверить так как он не доступен (возможно занят над другой задачей)


class MyGSM
{
  public: 
    MyGSM(byte gsmLED, byte boardLED, byte pinBOOT);    
    void Initialize();                
    byte Status;                                    // текущий статус gsm модуля (1-Shutdowned, 2-Loading, 3-Registered, 4-Fail, 5-NotFound)
    bool NewRing;
    bool NewSms;
    bool NewUssd;    
    String RingNumber;
    String SmsNumber;
    String SmsText;
    String UssdText;    
    String Read();
    bool SendSms(String *text, String *phone);      // метод возвращает true если смс отправлен успешно
    bool Call(String *phone);
    void RejectCall();        
    bool RequestUssd(String *code);                 // запрос gsm кода (*#)
    void Refresh();
    void ClearRing();
    void ClearSms();
    void ClearUssd();   
    bool IsAvailable();                             // оправшивает готовность gsm модуля (возвращает true если модуль не занят)
    void SwitchOn();                                // включение gsm модуля    
    int GetSignalStrength();                        // возвращает уровень сигнала   

    //Events                                        // ивенты указывают на какие-то измининия на, которые нужно отреагировать в основной программе, которая обязана после обработки сбросить их в false
    bool e_IsRestored;
    
  private:   
    void Configure();                               // настройка gsm модуля 
    bool WaitingAvailable();                        // ожидание готовности gsm модуля
    void BlinkLED(unsigned int millisBefore, unsigned int millisHIGH, unsigned int millisAfter);
    void SetString(String *source, String *target, char firstSymb, int offsetFirst, char secondSymb, int offsetSecond);
    void RefreshStatus();                           // проверяет и обновляет статус модуля    
    bool IsResponded();                             // проверяет отвечает ли модуль (включен, апаратно присутствует)
    byte IsNetworkRegistered();                     // проверяет зарегистрирован ли модуль в сети (готов ли модуль к работе) 
    byte _gsmLED;
    byte _boardLED;
    byte _pinBOOT;                                   // нога BOOT или K на модеме   
    int _sigStrength;   
};
