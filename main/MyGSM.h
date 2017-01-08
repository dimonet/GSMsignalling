#include "Arduino.h"

class MyGSM
{
  public: 
    MyGSM(int gsmLED, int pinBOOT);
    void Initialize();
    bool Available();
    String Read();
    void SendSMS(String text, String phone);
    void Call(String phone);
    void RejectCall();
    
  private:
    void BlinkLED(int millisBefore, int millisHIGH, int millisAfter);
    int _gsmLED;
    int _pinBOOT;                             // нога BOOT или K на модеме   
};
