//#define USB_JOYSTICK
#define DEBUG

#define OIS_BINARY 0
#define OIS_ASCII 1
#define OIS_MIN_VERSION 0
#define OIS_MAX_VERSION 2

#include "example_hardware.h"
#include "../ois_protocol.h"

OisState ois;

struct 
{
  OisNumericInput   red{"BTN_LED0", Number};
  OisNumericInput blue1{"BTN_LED2", Number};
  OisNumericInput blue2{"BTN_LED3", Number};
  OisNumericInput blue3{"BTN_LED4", Number};
  OisNumericInput blue4{"BTN_LED5", Number};
} inputs;

struct 
{
  OisNumericOutput   red{"BTN0", Boolean};
  OisNumericOutput start{"BTN1", Boolean};
  OisNumericOutput blue1{"BTN2", Boolean};
  OisNumericOutput blue2{"BTN3", Boolean};
  OisNumericOutput blue3{"BTN4", Boolean};
  OisNumericOutput blue4{"BTN5", Boolean};
  OisNumericOutput   key{"BTN6", Boolean};
} outputs;

struct 
{
} commands;


class Panel
{
public:
  Panel()
    : red(41, 2)
    , start(39)
    , blue1(37, 3)
    , blue2(35, 4)
    , blue3(32, 5)
    , blue4(30, 6)
    , key(53)
  {
  }
  void Setup()
  {
    red.Setup();
    start.Setup();
    blue1.Setup();
    blue2.Setup();
    blue3.Setup();
    blue4.Setup();
    key.Setup();
  }
  void Loop()
  {
    int ledRed   = inputs.red.value;
    int ledBlue1 = inputs.blue1.value;
    int ledBlue2 = inputs.blue2.value;
    int ledBlue3 = inputs.blue3.value;
    int ledBlue4 = inputs.blue4.value;

    int bRed   = red.Read(ledRed);
    int bStart = start.Read();
    int bBlue1 = blue1.Read(ledBlue1);
    int bBlue2 = blue2.Read(ledBlue2);
    int bBlue3 = blue3.Read(ledBlue3);
    int bBlue4 = blue4.Read(ledBlue4);
    int bKey   = key.Read();

    ois_set(ois, outputs.red,   OPEN != bRed);
    ois_set(ois, outputs.blue1, OPEN != bBlue1);
    ois_set(ois, outputs.blue2, OPEN != bBlue2);
    ois_set(ois, outputs.blue3, OPEN != bBlue3);
    ois_set(ois, outputs.blue4, OPEN != bBlue4);
    ois_set(ois, outputs.start, OPEN != bStart);
    ois_set(ois, outputs.key,   OPEN != bKey);
  }
  
  LedSwitch2Pin<true> red;
  Switch              start;
  LedSwitch2Pin<true> blue1;
  LedSwitch2Pin<true> blue2;
  LedSwitch2Pin<true> blue3;
  LedSwitch2Pin<true> blue4;
  Switch              key;
};

Panel panel;

void setup() 
{
  ois_setup_structs(ois, "My Button Panel", FOURCC("PNL1"), FOURCC("EXPL"), commands, inputs, outputs);

  panel.Setup();
}

OisProfiler profiler;

void loop() 
{
  panel.Loop();

  ois_loop(ois);

  profiler.loop(ois);
}
