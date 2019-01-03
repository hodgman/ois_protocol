
enum SwitchState
{
  CLOSED = 0,
  OPEN = 1,
};
typedef int KnobState10;
typedef int LedState8;

typedef unsigned int uint;

struct Switch
{
  int pin;
  Switch(int pin) : pin(pin) {}
  void Setup()
  {
    pinMode(pin, INPUT_PULLUP);
  }
  SwitchState Read()
  {
    return (SwitchState)digitalRead(pin);
  }
};

struct Knob
{
  int pin;
  Knob(int pin) : pin(pin) {}
  void Setup()
  {
    pinMode(pin, INPUT);
  }
  KnobState10 Read()
  {
    //pinMode(pin, INPUT);
    int k = analogRead(pin);
    //pinMode(pin, OUTPUT);
    //digitalWrite(pin, LOW);
    return k;
  }
};

struct LedSwitchInline
{
  int pin;
  LedSwitchInline(int pin) : pin(pin) {}
  void Setup(int ledState=0)
  {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, ledState);
  }
  SwitchState Read(LedState8 ledState=0)
  {
    digitalWrite(pin, LOW);
    pinMode(pin, INPUT_PULLUP);
    int b = digitalRead(pin);
    pinMode(pin, OUTPUT);
    //digitalWrite(pin, ledState);
    analogWrite(pin, ledState);
    return (SwitchState)b;
  }
};

template<bool ANALOG>
struct LedSwitch2Pin
{
  int pinSwitch, pinLed;
  LedSwitch2Pin(int pinSwitch, int pinLed) : pinSwitch(pinSwitch) , pinLed(pinLed) {}
  void Setup(int ledState=0)
  {
    pinMode(pinSwitch, INPUT_PULLUP);
    pinMode(pinLed, OUTPUT);
  }
  SwitchState Read(LedState8 ledState=0)
  {
    digitalWrite(pinLed, LOW);
    digitalWrite(pinSwitch, HIGH);
    int b = digitalRead(pinSwitch);
    if( ANALOG )
      analogWrite(pinLed, ledState);
    else if( ledState > 127 )
      digitalWrite(pinLed, HIGH);
    return (SwitchState)b;
  }
};


int AxisFrom10Bit( int k )
{
  return k;
  unsigned long u = k;
  u = (u<<6) | (u>>4);
  long s = u;
  s = s - 32768;
  return (int)s;
}
LedState8 LedFrom10Bit( int k )
{
  long l = k&0x3FF;
  return (l*l)>>12;
}
LedState8 LedFrom8Bit( int k )
{
  long l = k&0xFF;
  return (l*l)>>8;
}
