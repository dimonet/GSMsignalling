#include "Arduino.h"

// Настройки gsm модуля
#define AVAILABLE_TIMEOUT  1000                 // врем ожидание готовности модема (мили сек.)  
#define TIME_DIAGNOSTIC    900000               // время проверки gsm модуля(мили сек.) (1800000 = раз в пол часа, 900000 = раз в 15 мин, 180000 - раз в пол часа). Если обнаружено что gsm не отвечает или потерял сеть то устройство перезагружается автоматически
#define TIMEOUT_LOADING    120000               // время таймаута ожидания поиска сети gsm модуля после его перезагрузки если обнаружено сбой в его работе (мили сек.)
#define INITIALIZE_TIMEOUT 120                  // врем ожидания поиска сети модема (сек.)  
#define SMS_LIMIT          150                  // максимальное куличество символов в смс (большео лимита все символы обрезается)

// Статусы gsm модуля
#define Shutdowned         1                    // модуль выключен по причине отсутствия сети
#define Loading            2                    // модуль ищет сеть
#define Registered         3                    // модуль нашел сеть и работает в штатном режиме
#define Fail               4                    // модуль потерял сеть или перестал отвечать
#define NotFound           5                    // модуль или сим карта не обнаружена

class MyGSM
{
  public: 
    MyGSM(byte gsmLED, byte boardLED, byte pinBOOT);
    void InitUART();                                // инициализация UART gsm модуля
    bool Initialize();             
    void Shutdown(bool ledIndicator);               // выключение gsm модуля            
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
    bool CheckSIMCard();                            // возвращает true если модуль обнаружил SIM карту
    
  private:   
    void Configure();                               // настройка gsm модуля 
    bool WaitingAvailable();                        // ожидание готовности gsm модуля
    void BlinkLED(unsigned int millisBefore, unsigned int millisHIGH, unsigned int millisAfter);
    void SetString(String *source, String *target, char firstSymb, int offsetFirst, char secondSymb, int offsetSecond);
    byte _gsmLED;
    byte _boardLED;
    byte _pinBOOT;                                   // нога BOOT или K на модеме   
    String _sigStrength;   
};
