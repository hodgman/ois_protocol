# Open Interactivity System
Custom user input/output devices for games.

## Background

This project is inspired by an an extension of the serial protocol developed by Flat Earth Games for Objects In Space, described here:

- http://objectsgame.com/the-controllers/arduino-tutorial/
- http://objectsgame.com/the-controllers/ois-serial-data-protocol/

And with existing library support here:

- https://github.com/Segwegler/OIS_Library
- https://bitbucket.org/pjhardy/arduinosinspace/src/master/

By adding support for the OIS protocol to your game, your users can create their own physical input devices using hobbyist hardware such as Arduino.

Use cases:

- Physical installations / arcades / trade show booths
- Immersive simulations
- Unique games designed for custom input devices
- Accessibility for users who are not comfortable with traditional inputs

This project defines a communication specification, plus reference implementations that can be dropped into your games / input devices.

## TODO

This library is a work in progress. Contributors welcome!

- [ ] Documentation!
- [ ] Test the Arduino device code against Objects In Space game for compatibility with their "v1" spec.
- [ ] Test the C++ host code against other Arduinos In Space for compatibility with the "v1" spec.
- [ ] Collaborate with the community to nail down an ideal "v2" spec.
- [ ] More testing of the binary communication mode.
- [ ] Example implementations in other languages (C#?).
- [ ] Other transport mechanisms besides serial ports (Websockets, HTTP, TCP Sockets, Pipes, Shared Memory?).
- [ ] Port to platforms other than Windows.
- [ ] Game Engine examples (Unity, Unreal?).
- [ ] Add an example host application that translates the OIS device into a virtual Direct Input device using vJoy.

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
| SYN     | Begin Synchronisation stage                    |           | ✓           | ✓        | ✓₂         | ✓₂        |
| ACK     | Acknowledge connection                         | ✓         |             | ✓        |            |           |
| DEN     | Deny connection                                | ✓         |             | ✓        |            |           |
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
| #       | Numeric input/output key/value                 | ✓         | ✓₂          |          |            | ✓         |
| END     | Return to handshake stage                      | ✓₂        | ✓₂          |          | ✓₂         | ✓₂        |

₂ = introduced in version 2

### Protocol messages (binary)

Handshaking still occurs in ASCII; communication swtiches to binary after a request to use the binary protocol is accepted with an ACK message from the host.

| Command | Code                                      | Bytes                    |                                          | Host send | Device send | HS state | SYN state | ACT state |
| ------- | ----------------------------------------- | ------------------------ | ---------------------------------------- | --------- | ----------- | -------- | --------- | --------- |
| CL_CMD  | 0x01                                      | 3+string (\0 terminated) | Register command                         |           | ✓           |          | ✓         | ✓         |
| CL_NIO  | 0x02 / 0x12 / 0x22 / 0x42 / 0x52 / 0x62 / | 3+string (\0 terminated) | Add numeric input or output registration |           | ✓           |          | ✓         | ✓         |
| CL_ACT  | 0x03                                      | 1                        | End sync state / begin active state      |           | ✓           |          | ✓         |           |
| CL_DBG  | 0x04                                      | 1+string (\0 terminated) | Debug messaging                          |           | ✓           | ✓        | ✓         | ✓         |
| CL_TNI  | 0x05 / 0x15                               | 3                        | Toggle numeric input activity            |           | ✓           |          | ✓         | ✓         |
| CL_PID  | 0x06                                      | 9+string (\0 terminated) | Register device name/ID                  |           | ✓           |          | ✓         |           |
| CL_EXC  | 0x0C / 0x0D / 0x0E                        | 1/2/3                    | Execute command                          |           | ✓           |          |           | ✓         |
| CL_VAL  | 0x08 / 0x09 / 0x0A / 0x0B                 | 2/3/4/5                  | Numeric output key/value                 |           | ✓           |          |           | ✓         |
| SV_VAL  | 0x01 / 0x02 / 0x03 / 0x04                 | 2/3/4/5                  | Numeric input key/value                  | ✓         |             |          |           | ✓         |
| END     | 0x45 / 'E' (END\n)                        | 4                        | ASCII END command                        | ✓         | ✓           |          | ✓         | ✓         |
| SYN     | 0x53 / 'S' / (SYN=)                       | 4+string (\n terminated) | ASCII SYN command                        |           | ✓           | ✓        | ✓         | ✓         |

Even though handshaking must complete before binary communication begins, host implementations should correctly handle an ASCII SYN command, as these can occur if a controller is power-cycled after a connection is established. 

# Transmission formats

TODO: Binary / ASCII description

# Implementations

TODO: How to use / configure

- serial_device_arduino
  - serial_device_arduino / example
- serial_host_cpp

# ASCII Protocol in-depth

TODO - details

# Binary Protocol in-depth

TODO - details

## Device to Host (Client) messages

Device to host messages take up 1 or more bytes, with a message type identifier in the lowest 4 bits of the first byte. The high 4 bits of some message types is used to store additional message data. Unless the message type contains a string, the size of a message in bytes can be determined from the type parameter alone.

```c
| Byte 0        || Byte 1+       ||
|0|1|2|3|4|5|6|7||0|1|2|3|4|5|6|7||
| Type  | Extra || Data          ||
```

Some messages contain ASCII string data, which appears immediately after the regular message bytes, and is terminated with a NULL byte (0x00). 

For example, a DBG message header is a single byte, followed by a string. A DBG message containing the two-byte string "Hi" would be encoded as the following 4 bytes:

```c
| Byte 0        || B1 || B2 || B3 ||
|0|1|2|3|4|5|6|7||    ||    ||    ||
|CL_DBG |       ||'H' ||'i' ||'\0'||
| 0x4   | 0x0   ||0x48||0x69||0x00||
| 0x04          ||
```

### Command byte layouts

| Command  | Type | Extra                                                        | Following bytes                                              |
| -------- | ---- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| CL_CMD   | 0x1  | Must be 0                                                    | Byte 1: Low byte of channel ID<br />Byte 2: High byte of channel ID<br />Byte 3+: String event name |
| CL_NIO   | 0x2  | Bitmask of:<br />0x1: Number (N\*N)<br />0x2: Fraction (N\*F)<br />0x4: Output (NO\*) | Byte 1: Low byte of channel ID<br />Byte 2: High byte of channel ID<br />Byte 3+: String input/output name |
| CL_ACT   | 0x3  | Must be 0                                                    | None                                                         |
| CL_DBG   | 0x4  | Must be 0                                                    | Byte 1: String debug message                                 |
| CL_TNI   | 0x5  | 0x0: False / deactivate<br />0x1: True / activate            | Byte 1: Low byte of hannel ID<br />Byte 2: High byte of channel ID |
| CL_PID   | 0x6  | Must be 0                                                    | Bytes [1-4]: Product ID (32bit little endian)<br />Bytes [5-8]: Vendor ID (32bit little endian)<br />Byte 9+: Device name |
| CL_EXC0  | 0xC  | Channel ID <br />(low 4 bits)                                | None                                                         |
| CL_EXC1  | 0xD  | High byte of the channel ID <br />(low 4 bits)               | Byte 1: Low byte of hannel ID                                |
| CL_EXC2  | 0xE  | Must be 0                                                    | Byte 1: Low byte of hannel ID<br />Byte 2: High byte of channel ID |
| CL_VAL_1 | 0x8  | Value<br />(low 4 bits)                                      | Byte 1: Low byte of the channel ID                           |
| CL_VAL_2 | 0x9  | High byte of value<br />(low 4 bits)                         | Byte 1: Low byte of value<br />Byte 2: Low byte of channel ID |
| CL_VAL_3 | 0xA  | High byte of channel ID<br />(low 4 bits)                    | Byte 1: Low byte of value<br />Byte 2: High byte of value<br />Byte 3: Low byte of channel ID |
| CL_VAL_4 | 0xB  | Must be 0                                                    | Byte 1: Low byte of value<br />Byte 2: High byte of value<br />Byte 3: Low byte of channel ID<br />Byte 4: High byte of channel ID |
| END      | 0x4  | 0x5                                                          | Byte 1: 'N' (0x4E)<br />Byte 2: 'D' (0x44)<br />Byte 3: '\n' (0x0A) |
| SYN      | 0x3  | 0x5                                                          | Byte 1: 'Y' (0x59)<br />Byte 2: 'N' (0x4E)<br />Byte 3: '=' (0x3D) |

## Host to Device (Server) messages

Device to host messages take up 1 or more bytes, with a message type identifier in the lowest 3 bits of the first byte. The high 5 bits of some message types is used to store additional message data. The size of a message in bytes can be determined from the type parameter alone.

```c
| Byte 0        || Byte 1+       ||
|0|1|2|3|4|5|6|7||0|1|2|3|4|5|6|7||
| Type| Extra   || Data          ||
```


### Message byte layouts 

| Command  | Type | Extra                                     | Following bytes                                              |
| -------- | ---- | ----------------------------------------- | ------------------------------------------------------------ |
| SV_VAL_1 | 0x1  | Value<br />(low 5 bits)                   | Byte 1: Low byte of the channel ID                           |
| SV_VAL_2 | 0x2  | High byte of value<br />(low 5 bits)      | Byte 1: Low byte of value<br />Byte 2: Low byte of channel ID |
| SV_VAL_3 | 0x3  | High byte of channel ID<br />(low 5 bits) | Byte 1: Low byte of value<br />Byte 2: High byte of value<br />Byte 3: Low byte of channel ID |
| SV_VAL_4 | 0x4  | Must be 0                                 | Byte 1: Low byte of value<br />Byte 2: High byte of value<br />Byte 3: Low byte of channel ID<br />Byte 4: High byte of channel ID |
| END      | 0x4  | 0x5                                       | Byte 1: 'N' (0x4E)<br />Byte 2: 'D' (0x44)<br />Byte 3: '\n' (0x0A) |

 