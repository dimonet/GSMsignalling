#include "Arduino.h"

// Настройки gsm модуля
#define AVAILABLE_TIMEOUT  1000                 // врем ожидание готовности модема (мили сек.)  
#define TIME_DIAGNOSTIC    60000                // время проверки gsm модуля(мили сек.) (60000 = раз в минуту, 900000 = раз в 15 мин, 180000 - раз в пол часа). Если обнаружен что gsm потерял сеть то постоянно горит gsm led сигнализируя об этом
#define INITIALIZE_TIMEOUT 120                  // врем ожидания поиска сети модема (сек.)  
#define SMS_LIMIT          150                  // максимальное куличество символов в смс (большео лимита все символы обрезается)

// Статусы gsm модуля
#define Loading            1                    // модуль ищет сеть
#define Registered         2                    // модуль нашел сеть и работает в штатном режиме
#define Fail               3                    // модуль потерял сеть или перестал отвечать

class MyGSM
{
  public: 
    MyGSM(byte gsmLED, byte boardLED, byte pinBOOT);    
    bool Initialize();                
    byte Status;                                    // текущий статус gsm модуля (1-Shutdowned, 2-Loading, 3-Registered, 4-Fail, 5-NotFound)
    bool NewRing;
    bool NewSms;
    bool NewUssd;
    bool WasRestored;
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
    bool isNetworkRegistered();                     // проверяет зарегистрирован ли модуль в сети (готов ли модуль к работе)   
    void SwitchOn();                                // включение gsm модуля    
    int GetSignalStrength();                        // возвращает уровень сигнала   
    
  private:   
    void Configure();                               // настройка gsm модуля 
    bool WaitingAvailable();                        // ожидание готовности gsm модуля
    void BlinkLED(unsigned int millisBefore, unsigned int millisHIGH, unsigned int millisAfter);
    void SetString(String *source, String *target, char firstSymb, int offsetFirst, char secondSymb, int offsetSecond);
    byte _gsmLED;
    byte _boardLED;
    byte _pinBOOT;                                   // нога BOOT или K на модеме   
    int _sigStrength;   
};
