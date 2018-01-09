#include "Arduino.h"

class MyGSM
{
  public: 
    MyGSM(byte gsmLED, byte boardLED, byte pinBOOT);
    void Initialize();    
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
    void Shutdown();                                // выключения gsm модула (может использоваться при перезагрузке всего устройства)
    bool IsAvailable();                             // оправшивает готовность gsm модуля (возвращает true если модуль не занят)
    
  private:
    bool WaitingAvailable();                        // ожидание готовности gsm модуля
    void BlinkLED(unsigned int millisBefore, unsigned int millisHIGH, unsigned int millisAfter);
    void SetString(String *source, String *target);
    byte _gsmLED;
    byte _boardLED;
    byte _pinBOOT;                                   // нога BOOT или K на модеме   
    String GetStrFromFlash(char* addr);    
};
