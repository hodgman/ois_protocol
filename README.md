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

### Types

- Boolean - 0/1
- Number - 16 bit signed integer
- Fraction - 16 bit signed integer, scaled by 100 (i.e. The number 1.5 is encoded as 150)

### Protocol messages (ASCII)

| Command |                                                | Host send | Device send | HS state | SYNC state | Active state |
| ------- | ---------------------------------------------- | --------- | ----------- | -------- | ---------- | ------------ |
| SYN     | Begin Synchronisation stage                    |           | ✓           | ✓        |            |              |
| ACK     | Acknowledge connection                         | ✓         |             |          |            |              |
| DEN     | Deny connection                                | ✓         |             |          |            |              |
| PID     | Register device name/ID                        |           | ✓₂          |          | ✓₂         |              |
| CMD     | Register command                               |           | ✓           |          | ✓          | ✓₂           |
| NIB     | Add numeric input (boolean) registration       |           | ✓           |          | ✓          | ✓₂           |
| NIN     | Add numeric input (number) registration        |           | ✓           |          | ✓          | ✓₂           |
| NIF     | Add numeric input (fraction) registration      |           | ✓           |          | ✓          | ✓₂           |
| NOB     | Add numeric output (boolean) registration      |           | ✓₂          |          | ✓₂         | ✓₂           |
| NON     | Add numeric output(number) registration        |           | ✓₂          |          | ✓₂         | ✓₂           |
| NOF     | Add numeric output (fraction) registration     |           | ✓₂          |          | ✓₂         | ✓₂           |
| TNI     | Toggle numeric input activity                  |           | ✓₂          |          | ✓₂         | ✓₂           |
| ACT     | End synchronisation state / begin active state |           | ✓           |          | ✓          |              |
| EXC     | Execute command                                |           | ✓           |          |            | ✓            |
| DBG     | Debug messaging                                |           | ✓           | ✓₂       | ✓₂         | ✓            |
| #       | Key/value updates                              | ✓         | ✓₂          |          |            | ✓            |
| END     | Return to handshake stage                      | ✓₂        | ✓₂          |          | ✓₂         | ✓₂           |

₂ = introduced in version 2

### Protocol messages (binary)

Binary transmission is currently WIP.

| Command    | Hex                       |                                                | Host send | Device send | HS state | SYNC state | Active state |
| ---------- | ------------------------- | ---------------------------------------------- | --------- | ----------- | -------- | ---------- | ------------ |
| PID        | 0x06                      | Register device name/ID                        |           | ✓           |          | ✓          |              |
| CMD        | 0x01                      | Register command                               |           | ✓           |          | ✓          | ✓            |
| NIO        | 0x02                      | Add numeric input or output registration       |           | ✓           |          | ✓          | ✓            |
| ACT        | 0x03                      | End synchronisation state / begin active state |           | ✓           |          | ✓          |              |
| TNI        | 0x05                      | Toggle numeric input activity                  |           | ✓           |          | ✓          | ✓            |
| EXC0/1/2   | 0x0C / 0x0D / 0x0E        | Execute command                                |           | ✓           |          |            | ✓            |
| DBG        | 0x04                      | Debug messaging                                |           | ✓           | ✓        | ✓          | ✓            |
| VAL1/2/3/4 | 0x08 / 0x09 / 0x0A / 0x0B | Key/value updates                              | ✓         | ✓           |          |            | ✓            |
| END        | 0x07                      | Return to handshake stage                      | ✓         | ✓           |          | ✓          | ✓            |
# Transmission formats

TODO: Binary / ASCII 

# Implementations

TODO

- serial_device_arduino
- serial_host_cpp

# Protocol in-depth

TODO