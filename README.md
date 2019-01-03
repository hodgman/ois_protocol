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

## TODO

This library is a work in progress. Contributors welcome!

- Test the Arduino device code against Objects In Space game for compatibility with their "v1" spec.
- Test the C++ host code against other Arduinos In Space for compatibility with the "v1" spec.
- Collaborate with the community to nail down an ideal "v2" spec.
- More testing of the binary communication mode.
- Example implementations in other languages (C#?).
- Other transport mechanisms besides serial ports (Websockets, HTTP, TCP Sockets, Pipes, Shared Memory?).
- Port to platforms other than Windows.
- Game Engine examples (Unity, Unreal?).
- Add an example host application that translates the OIS device into a virtual Direct Input device using vJoy.

## Protocol Overview

### Protocol states

#### Handshaking (HS)

Device  requests a connection from the host, negotiates the protocol version.

#### Synchronisation (SYN)

Device registers inputs, outputs and commands that it can send/receive.

#### Active (ACT)

Device and host send/receive values and commands.

### Types

- Boolean - 0/1
- Number - 16 bit signed integer
- Fraction - 16 bit signed integer, scaled by 100 (i.e. The number 1.5 is encoded as 150)

### Protocol messages (ASCII)

| Command |                                                | Host send | Device send | HS state | SYNC state | ACT state |
| ------- | ---------------------------------------------- | --------- | ----------- | -------- | ---------- | --------- |
| SYN     | Begin Synchronisation stage                    |           | ✓           | ✓        |            |           |
| ACK     | Acknowledge connection                         | ✓         |             |          |            |           |
| DEN     | Deny connection                                | ✓         |             |          |            |           |
| PID     | Register device name/ID                        |           | ✓₂          |          | ✓₂         |           |
| CMD     | Register command                               |           | ✓           |          | ✓          | ✓₂        |
| NIB     | Add numeric input (boolean) registration       |           | ✓           |          | ✓          | ✓₂        |
| NIN     | Add numeric input (number) registration        |           | ✓           |          | ✓          | ✓₂        |
| NIF     | Add numeric input (fraction) registration      |           | ✓           |          | ✓          | ✓₂        |
| NOB     | Add numeric output (boolean) registration      |           | ✓₂          |          | ✓₂         | ✓₂        |
| NON     | Add numeric output(number) registration        |           | ✓₂          |          | ✓₂         | ✓₂        |
| NOF     | Add numeric output (fraction) registration     |           | ✓₂          |          | ✓₂         | ✓₂        |
| TNI     | Toggle numeric input activity                  |           | ✓₂          |          | ✓₂         | ✓₂        |
| ACT     | End synchronisation state / begin active state |           | ✓           |          | ✓          |           |
| EXC     | Execute command                                |           | ✓           |          |            | ✓         |
| DBG     | Debug messaging                                |           | ✓           | ✓₂       | ✓₂         | ✓         |
| #       | Key/value updates                              | ✓         | ✓₂          |          |            | ✓         |
| END     | Return to handshake stage                      | ✓₂        | ✓₂          |          | ✓₂         | ✓₂        |

₂ = introduced in version 2

### Protocol messages (binary)

Handshaking still occurs in ASCII; communication swtiches to binary after a request to use the binary protocol is accepted with an ACK message from the host.

| Command        | Hex                                       | Bytes    |                                          | Host send | Device send | HS state | SYN state | ACT state |
| -------------- | ----------------------------------------- | -------- | ---------------------------------------- | --------- | ----------- | -------- | --------- | --------- |
| CL_CMD         | 0x01                                      | 3+string | Register command                         |           | ✓           |          | ✓         | ✓         |
| CL_NIO         | 0x02 / 0x12 / 0x22 / 0x42 / 0x52 / 0x62 / | 3+string | Add numeric input or output registration |           | ✓           |          | ✓         | ✓         |
| CL_ACT         | 0x03                                      | 1        | End sync state / begin active state      |           | ✓           |          | ✓         |           |
| CL_DBG         | 0x04                                      | 1+string | Debug messaging                          |           | ✓           | ✓        | ✓         | ✓         |
| CL_TNI         | 0x05 / 0x15                               | 3        | Toggle numeric input activity            |           | ✓           |          | ✓         | ✓         |
| CL_PID         | 0x06                                      | 9+string | Register device name/ID                  |           | ✓           |          | ✓         |           |
| CL_EXC 0/1/2   | 0x0C / 0x0D / 0x0E                        | 1/2/3    | Execute command                          |           | ✓           |          |           | ✓         |
| CL_VAL 1/2/3/4 | 0x08 / 0x09 / 0x0A / 0x0B                 | 2/3/4/5  | Key/value updates                        |           | ✓           |          |           | ✓         |
| SV_VAL 1/2/3/4 | 0x01 / 0x02 / 0x03 / 0x04                 | 2/3/4/5  | Key/value updates                        | ✓         |             |          |           | ✓         |
# Transmission formats

TODO: Binary / ASCII description

# Implementations

TODO: How to use / configure

- serial_device_arduino
- serial_host_cpp

# Protocol in-depth

TODO - details