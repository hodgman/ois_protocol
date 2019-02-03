//Designed to be used with the firmware from:
//https://github.com/harlequin-tech/arduino-usb

void joystick_setup()
{
#if defined DEBUG
  Serial.begin(9600);
#else
  Serial.begin(115200);
  delay(200);
#endif
}

#define NUM_BUTTONS  40
#define NUM_AXES  8        // 8 axes, X, Y, Z, etc

typedef struct joyReport_t {
    int16_t axis[NUM_AXES];
    uint8_t button[(NUM_BUTTONS+7)/8]; // 8 buttons per byte
} joyReport_t;

joyReport_t joyReport = {};

void sendJoyReport(struct joyReport_t *report)
{
#ifndef DEBUG
  Serial.write((uint8_t *)report, sizeof(joyReport_t));
#else
  // dump human readable output for debugging
  for (uint8_t ind=0; ind<NUM_AXES; ind++)
  {
    Serial.print("axis[");
    Serial.print(ind);
    Serial.print("]= ");
    Serial.print(report->axis[ind]);
    Serial.print(" ");
  }
  Serial.println();
  for (uint8_t ind=0; ind<NUM_BUTTONS/8; ind++)
  {
    Serial.print("button[");
    Serial.print(ind);
    Serial.print("]= ");
    Serial.print(report->button[ind], HEX);
    Serial.print(" ");
  }
  Serial.println();
#endif
}

