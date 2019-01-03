# Ois2vJoy
vJoy is a project that adds a virtual game input device to windows, which most games (using Direct Input or Raw Input) will detect as a joystick/gamepad.

This application connects to an OIS device via the serial port, and translates outputs from that device to vJoy inputs. This app can also be useful for simply debugging / validating an OIS device.

To use this application as a virtual joystick/gamepad, first install the vJoy driver from:
 http://vjoystick.sourceforge.net/site/

![](ois2vjoy.png)

## License

[MIT](../../COPYING)