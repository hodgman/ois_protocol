# Open Interactivity System
Custom user input/output devices for games.

## Background

This project is inspired by an an extension of the serial protocol developed by Flat Earth Games for Objects In Space:

http://objectsgame.com/the-controllers/arduino-tutorial/
http://objectsgame.com/the-controllers/ois-serial-data-protocol/

By adding support for the OIS protocol to your game, your users can create their own physical input devices using hobbyist hardware such as Arduino.

Use cases:

- Physical installations / arcades / trade show booths
- Immersive simulations
- Unique games designed for custom input devices
- Accessibility

This project defines a communication specification, plus reference implementations that can be dropped into your games / input devices.

## Protocol Overview

### Protocol states

#### Handshaking (HS)

Device  requests a connection from the host, negotiates the protocol version.

#### Synchronisation (SYNC)

Device registers inputs, outputs and commands that it can send/receive.

#### Active

Device and host send/receive values and commands.

### Protocol messages

| Command |                                                | Host send | Device send | HS state | SYNC state | Active state |
| ------- | ---------------------------------------------- | --------- | ----------- | -------- | ---------- | ------------ |
| SYN     | Begin Synchronisation stage                    |           | ✓           | ✓        |            |              |
| ACK     | Acknowledge connection                         | ✓         |             |          |            |              |
| DEN     | Deny connection                                | ✓         |             |          |            |              |
| PID     | Register device name/ID                        |           | ✓₂          |          | ✓          |              |
| CMD     | Register command                               |           | ✓           |          | ✓          | ✓₂           |
| NIB     | Add numeric input (boolean) registration       |           | ✓           |          | ✓          | ✓₂           |
| NIN     | Add numeric input (number) registration        |           | ✓           |          | ✓          | ✓₂           |
| NIF     | Add numeric input (fraction) registration      |           | ✓           |          | ✓          | ✓₂           |
| NOB     | Add numeric output (boolean) registration      |           | ✓₂          |          | ✓          | ✓            |
| NON     | Add numeric output(number) registration        |           | ✓₂          |          | ✓          | ✓            |
| NOF     | Add numeric output (fraction) registration     |           | ✓₂          |          | ✓          | ✓            |
| RNI     | Remove numeric input registration              |           | ✓₂          |          |            | ✓            |
| RNO     | Remove numeric output registration             |           | ✓₂          |          |            | ✓            |
| RCM     | Remove command registration                    |           | ✓₂          |          |            | ✓            |
| ACT     | End synchronisation state / begin active state |           | ✓           |          | ✓          |              |
| EXC     | Execute command                                |           | ✓           |          |            | ✓            |
| DBG     | Debug messaging                                |           | ✓           | ✓₂       | ✓₂         | ✓            |
| #       | Key/value updates                              | ✓         | ✓₂          |          |            | ✓            |
| END     | Return to handshake stage                      | ✓₂        | ✓₂          |          | ✓          | ✓            |

₂ = introduced in version 2

### Types

- Boolean - 0/1
- Number - 16 bit signed integer
- Fraction - 16 bit signed integer, scaled by 100 (i.e. a value of 150 is interpreted as 1.5)

# Transmission formats

TODO: Binary / ASCII 

# Implementations

TODO

- serial_device_arduino
- serial_host_cpp

# Protocol in-depth

TODO