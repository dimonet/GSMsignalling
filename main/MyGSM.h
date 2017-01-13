#include "Arduino.h"

class MyGSM
{
  public: 
    MyGSM(byte gsmLED, byte pinBOOT);
    void Initialize();
    void ReInitialize();
    bool Available();
    String Read();
    void SendSMS(String text, String phone);
    void Call(String phone);
    void RejectCall();
    
  private:
    void BlinkLED(unsigned int millisBefore, unsigned int millisHIGH, unsigned int millisAfter);
    int _gsmLED;
    int _pinBOOT;                             // нога BOOT или K на модеме   
};
