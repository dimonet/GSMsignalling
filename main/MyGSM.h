#include "Arduino.h"

class MyGSM
{
  public: 
    MyGSM(byte gsmLED, byte boardLED, byte pinBOOT);
    bool Initialize();             
    void Shutdown(bool ledIndicator);               // выключение gsm модуля             
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
    
  private:
    unsigned long GetElapsed(unsigned long prEventMillis);
    void SwitchOn();                                // включение gsm модуля   
    void Configure();                               // настройка gsm модуля
    bool IsWorking;                                   // true - модуль включен, false - модуль выключен или не с конфигурирован (возможно из за збоев)
    bool ModuleIsCorrect;    
    bool WaitingAvailable();                        // ожидание готовности gsm модуля
    void BlinkLED(unsigned int millisBefore, unsigned int millisHIGH, unsigned int millisAfter);
    void SetString(String *source, String *target, char firstSymb, int offsetFirst, char secondSymb, int offsetSecond);
    byte _gsmLED;
    byte _boardLED;
    byte _pinBOOT;                                   // нога BOOT или K на модеме   
    String _sigStrength;
    String GetStrFromFlash(char* addr);    
};
