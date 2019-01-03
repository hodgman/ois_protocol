#ifdef USB_JOYSTICK
# include "joystick.h"
#endif

#ifndef OIS_BINARY
#define OIS_BINARY 1
#endif

#ifndef OIS_ASCII
#define OIS_ASCII 1
#endif

#ifndef OIS_MIN_VERSION
#define OIS_MIN_VERSION 0
#endif

#ifndef OIS_MAX_VERSION
#define OIS_MAX_VERSION 2
#endif

#if OIS_MAX_VERSION < 2
#undef OIS_BINARY
#define OIS_BINARY 0
#endif

#if !OIS_ASCII && !OIS_BINARY 
#error "You need at least either ASCII or binary!"
#endif

#if !OIS_ASCII && OIS_MIN_VERSION < 2 
#error "Version 1 requires ASCII support!"
#endif

#if OIS_MIN_VERSION > OIS_MAX_VERSION
#error "Min can't be above max..."
#endif

#ifndef OIS_MAX_COMMAND_LENGTH
#define OIS_MAX_COMMAND_LENGTH 128
#endif

//todo - toggle numeric input support
/*
  enum ClientCommandsBin
  {
    CL_TNI   = 0x05,
    CL_TNI_PAYLOAD_T = 0x10,
  };*/

#if OIS_BINARY && OIS_ASCII
# define IF_BINARY if( ois.binary )
# define IF_ASCII  if( !ois.binary )
#elif OIS_BINARY
# define IF_BINARY if(1)
# define IF_ASCII  if(0)
#else
# define IF_BINARY if(0)
# define IF_ASCII  if(1)
#endif

enum NumericType
{
  Boolean,
  Number,
  Fraction,
};
struct OisNumericInput
{
  const char* name;
  NumericType type;
  int value;
};
struct OisNumericOutput
{
  const char* name;
  NumericType type;
  int value;
};
struct OisCommand
{
  const char* name;
};
struct OisState;

void ois_setup(OisState&, const char* name, uint32_t pid, uint32_t vid, OisCommand* commands, int numCommands, OisNumericInput* inputs, int numInputs, OisNumericOutput* outputs, int numOutputs, int version);
void ois_loop(OisState&);
void ois_set(OisState&, OisNumericOutput& output, int value);
void ois_execute(OisState&, OisCommand& command);
void ois_print(OisState&, const char*);

class OisProfiler
{
public:
  void loop(OisState& ois)
  {
    unsigned long profileTime = micros();
    unsigned long profileDt = profileTime - prevTime;
    prevTime = profileTime;

    const unsigned long freq = 10;

    ++loopCount;
    timer += profileDt;
    if( timer > (freq*1000*1000) )
    {
      profileDt = (timer + (loopCount>>1)) / loopCount;
      timer = 0;
      loopCount = 0;

      bool profileMs = profileDt > 3000;
      if( profileMs )
        profileDt = (profileDt + 500) / 1000;
  
      char buf[32] = "Loop Rate: ";
      ltoa( profileDt, buf+11, 10 );
      strcat(buf+11, profileMs?"ms":"us");
      ois_print( ois, buf );
    }
  }
private:
  unsigned long timer = 0;
  unsigned long loopCount = 0;
  unsigned long prevTime = 0;
};


#define OIS_BUFFER_LENGTH (OIS_MAX_COMMAND_LENGTH*2)
#define FOURCC(str)                                                            \
  ((uint32_t)(uint8_t)(str[0])        | ((uint32_t)(uint8_t)(str[1]) << 8) |   \
  ((uint32_t)(uint8_t)(str[2]) << 16) | ((uint32_t)(uint8_t)(str[3]) << 24 ))  //






struct OisState
{
  enum DeviceState
  {
    Handshaking,
    Synchronisation,
    Active,
  } deviceState;
  int maxVersion;
  int version;
  
  long baud;
  char* gameTitle;
  int gameVersion;

  const char* deviceName = "";
  uint32_t pid=0, vid=0;

  bool binary;
  char messageBuffer[OIS_BUFFER_LENGTH];
  int messageLength;

  OisCommand*       commands;
  OisNumericInput*  inputs;
  OisNumericOutput* outputs;
  int numInputs;
  int numOutputs;
  int numCommands;

  uint* touchedCommandsMasks;
  uint* touchedOutputsMasks;
  int numTouchedCommandsMasks;
  int numTouchedOutputsMasks;
  
  int numTouchedCommands;
  int numTouchedOutputs;
  int touchedCommandsIterator;
  int touchedOutputsIterator;
  
  int synCount;

  
  enum ClientCommandsBin
  {
    CL_CMD   = 0x01,
    CL_NIO   = 0x02,
    CL_ACT   = 0x03,
    CL_DBG   = 0x04,
    CL_TNI   = 0x05,
    CL_PID   = 0x06,
    CL_END   = 0x07,
    CL_VAL_1 = 0x08,
    CL_VAL_2 = 0x09,
    CL_VAL_3 = 0x0A,
    CL_VAL_4 = 0x0B,
    CL_EXC_0 = 0x0C,
    CL_EXC_1 = 0x0D,
    CL_EXC_2 = 0x0E,

    CL_COMMAND_MASK  = 0x0F,
    CL_PAYLOAD_SHIFT = 4,

    CL_N_PAYLOAD_N = 0x10,
    CL_N_PAYLOAD_F = 0x20,
    CL_N_PAYLOAD_O = 0x40,
    
    CL_TNI_PAYLOAD_T = 0x10,
  };
  enum ServerCommandsBin
  {
    SV_NUL   = 0x00,
    SV_VAL_1 = 0x01,
    SV_VAL_2 = 0x02,
    SV_VAL_3 = 0x03,
    SV_VAL_4 = 0x04,
    SV_END_ = 'E',//0x45

    SV_COMMAND_MASK  = 0x07,
    SV_PAYLOAD_SHIFT = 3,
  };
};


template<class CMD, class NI, class NO>
void ois_setup_structs(OisState& ois, const char* name, uint32_t pid, uint32_t vid, CMD& commands, NI& inputs, NO& outputs, int version = OIS_MAX_VERSION)
{
  ois_setup(ois, name, pid, vid, 
            (OisCommand*)&commands, sizeof(commands)/sizeof(OisCommand),
            (OisNumericInput*)&inputs, sizeof(inputs)/sizeof(OisNumericInput),
            (OisNumericOutput*)&outputs, sizeof(outputs)/sizeof(OisNumericOutput), version);
}

template<int CMD, int NI, int NO>
void ois_setup(OisState& ois, const char* name, uint32_t pid, uint32_t vid, OisCommand(&commands)[CMD], OisNumericInput(&inputs)[NI], OisNumericOutput(&outputs)[NO], int version = OIS_MAX_VERSION)
{
  ois_setup(ois, name, pid, vid, commands, CMD, inputs, NI, outputs, NO, version);
}
template<int CMD, int NI>
void ois_setup(OisState& ois, const char* name, uint32_t pid, uint32_t vid, OisCommand(&commands)[CMD], OisNumericInput(&inputs)[NI], int version = OIS_MAX_VERSION)
{
  ois_setup(ois, name, pid, vid, commands, CMD, inputs, NI, 0, 0, version);
}
template<int CMD, int NO>
void ois_setup(OisState& ois, const char* name, uint32_t pid, uint32_t vid, OisCommand(&commands)[CMD],  OisNumericOutput(&outputs)[NO], int version = OIS_MAX_VERSION)
{
  ois_setup(ois, name, pid, vid, commands, CMD, 0, 0, outputs, NO, version);
}
template<int NI, int NO>
void ois_setup(OisState& ois, const char* name, uint32_t pid, uint32_t vid, OisNumericInput(&inputs)[NI], OisNumericOutput(&outputs)[NO], int version = OIS_MAX_VERSION)
{
  ois_setup(ois, name, pid, vid, 0, 0, inputs, NI, outputs, NO, version);
}
template<int CMD>
void ois_setup(OisState& ois, const char* name, uint32_t pid, uint32_t vid, OisCommand(&commands)[CMD], int version = OIS_MAX_VERSION)
{
  ois_setup(ois, name, pid, vid, commands, CMD, 0, 0, 0, 0, version);
}
template<int NI>
void ois_setup(OisState& ois, const char* name, uint32_t pid, uint32_t vid, OisNumericInput(&inputs)[NI], int version = OIS_MAX_VERSION)
{
  ois_setup(ois, name, pid, vid, 0, 0, inputs, NI, 0, 0, version);
}
template<int NO>
void ois_setup(OisState& ois, const char* name, uint32_t pid, uint32_t vid, OisNumericOutput(&outputs)[NO], int version = OIS_MAX_VERSION)
{
  ois_setup(ois, name, pid, vid, 0, 0, 0, 0, outputs, NO, version);
}

void ois_setup(OisState& ois, const char* deviceName, uint32_t pid, uint32_t vid, OisCommand* commands, int numCommands, OisNumericInput* inputs, int numInputs, OisNumericOutput* outputs, int numOutputs, int version)
{
#ifdef USB_JOYSTICK
  joystick_setup();
  ois.deviceState = OisState::Active;
  ois.version = ois.maxVersion = 0;
  static char nullchar = '\0';
  ois.gameTitle = &nullchar;
  ois.gameVersion = 0;
#else
  ois.baud = 9600;
  Serial.begin(ois.baud);
  ois.deviceState = OisState::Handshaking;
  ois.version = ois.maxVersion = version;
  
  ois.gameTitle = (char*)malloc(1);
  ois.gameTitle[0] = '\0';
  ois.gameVersion = 0;
#endif

  ois.synCount = 0;
  ois.deviceName = deviceName;
  ois.pid = pid;
  ois.vid = vid;
  ois.binary = false;
  ois.inputs = inputs;
  ois.outputs = outputs;
  ois.commands = commands;
  ois.numInputs = numInputs;
  ois.numOutputs = numOutputs;
  ois.numCommands = numCommands;
  ois.numTouchedCommands = 0;
  ois.numTouchedOutputs = 0;
  ois.touchedCommandsIterator = 0;
  ois.touchedOutputsIterator = 0;

  int bitsPerMask = sizeof(int)*8;
  ois.numTouchedCommandsMasks = (ois.numCommands + bitsPerMask-1)/bitsPerMask;
  ois.numTouchedOutputsMasks = (ois.numOutputs  + bitsPerMask-1)/bitsPerMask;
  int commandsMasksSize = sizeof(int) * ois.numTouchedCommandsMasks;
  int  outputsMasksSize = sizeof(int) * ois.numTouchedOutputsMasks;
  ois.touchedCommandsMasks = ois.numCommands ? (uint*)malloc(commandsMasksSize) : nullptr;
  ois.touchedOutputsMasks  = ois.numOutputs  ? (uint*)malloc( outputsMasksSize) : nullptr;
  if( ois.numCommands ) memset(ois.touchedCommandsMasks, 0, commandsMasksSize);
  if( ois.numOutputs  ) memset(ois.touchedOutputsMasks,  0,  outputsMasksSize);
}


#ifndef USB_JOYSTICK
void ois_set(OisState& ois, OisNumericOutput& output, int data)
{
  if( output.value == data )
    return;
  output.value = data;
  int index = &output - ois.outputs;
  int slot = index / (sizeof(int)*8);
  int bitIndex = index % (sizeof(int)*8);
  uint& value = ois.touchedOutputsMasks[slot];
  uint mask = 1U << bitIndex;
  int isNew = (value & mask) == 0 ? 1 : 0;
  value |= mask;
  ois.numTouchedOutputs += isNew;
}

void ois_execute(OisState& ois, OisCommand& command)
{
  int index = &command - ois.commands;
  int slot = index / (sizeof(int)*8);
  int bitIndex = index % (sizeof(int)*8);
  uint& value = ois.touchedCommandsMasks[slot];
  uint mask = 1U << bitIndex;
  int isNew = (value & mask) == 0 ? 1 : 0;
  value |= mask;
  ois.numTouchedCommands += isNew;
}

char* ZeroDelimiter(char* str, char delimiter)
{
  char* c = str;
  for( ; *c; ++c )
  {
    if( *c == delimiter )
    {
      *c = '\0';
      return c + 1; 
    }
  }
  return c;
}
  
void ois_reset(OisState& ois)
{
  ois.binary = false;
  ois.synCount = 0;
  ois.gameTitle[0] = '\0';
  ois.gameVersion = 0;
  ois.deviceState = OisState::Handshaking;
}

void ois_parse_ascii(OisState& ois, char* cmd)
{
  if( !cmd[0] )
    return;
  u32 type = 0;
  if( cmd[1] && cmd[2] )
    type = FOURCC(cmd);
#if OIS_ASCII
  bool isKeyVal = 0!=isDigit(cmd[0]);
  if( isKeyVal )
  {
    const char* payload = ZeroDelimiter(cmd, '=');
    int channel = atoi(cmd);
    if( channel < ois.numInputs )
    {
      OisNumericInput& v = ois.inputs[channel];
      v.value = atoi(payload);
    }
  }
  else
#endif
  {
    char* payload = cmd + 4;
    switch( type )
    {
      default:
        break;
      case FOURCC("DEN\0")://v1
      {
        if( ois.version == 451 )
          ois.version = OIS_MAX_VERSION;
        else if( ois.version > 0 )
          ois.version--;
        else
          ois.version = ois.maxVersion;
        ois_reset(ois);
        break;
      }
#if OIS_MAX_VERSION >= 2
      case FOURCC("END\0")://v2
      {
        ois.version = ois.maxVersion;
        ois_reset(ois);
        break;
      }
      case FOURCC("ACK=")://v2
      {
        char* gameTitle = ZeroDelimiter(payload, ',');
        int len = strlen(gameTitle)+1;
        ois.gameTitle = (char*)realloc(ois.gameTitle, len);
        memcpy(ois.gameTitle, gameTitle, len);
        ois.gameVersion = atoi(payload);
#if OIS_BINARY
        ois.binary = true;
#endif
      }//fall-through:
#endif
      case FOURCC("ACK\0")://v1
        ois.deviceState = OisState::Synchronisation;
        break;
#if OIS_MIN_VERSION == 0
      case FOURCC("452\r")://hack for objects in space beta
      {
        ois.version = 1;
        ois.deviceState = OisState::Synchronisation;
        break;
      }
#endif
    }
  }
}

int ois_parse_binary(OisState& ois, char* start, char* end)
{
 if( end <= start )
    return 0;
  int bufferLength = (int)(ptrdiff_t)(end - start);
  
  uint16_t payload = (*start);
  uint16_t command = payload & OisState::SV_COMMAND_MASK;
  int cmdLength = 1;
  if( payload == OisState::SV_END_ )//the host send the END command as ASCII
  {
    cmdLength = 4;
  }
  else switch( command )
  {
  default:
  case OisState::SV_NUL:
  {
#ifdef DEBUG
    char buffer[32];
    sprintf(buffer, "Err: Unknown command 0x%02hhX", start[0]);
    ois_print(ois, buffer);
#endif
    return 1;
  }
  case OisState::SV_VAL_1: cmdLength = 2; break;
  case OisState::SV_VAL_2: cmdLength = 3; break;
  case OisState::SV_VAL_3: cmdLength = 4; break;
  case OisState::SV_VAL_4: cmdLength = 5; break;
  }
  if( bufferLength < cmdLength )
    return 0;//need ro read more data before we can continue
  if( payload == OisState::SV_END_ && 0==strcmp(start, "END") )
    return -1;//need to switch back to ASCII mode
    
  int channel = ois.numInputs;
  int value = 0;
  uint16_t extra = payload >> OisState::SV_PAYLOAD_SHIFT;
  switch( command )
  {
  case OisState::SV_VAL_1: value = extra;                              channel = *(uint8_t *)(start+1);              break;
  case OisState::SV_VAL_2: value = *(uint8_t *)(start+1)|(extra << 8); channel = *(uint8_t *)(start+2);              break;
  case OisState::SV_VAL_3: value = *(uint16_t*)(start+1);              channel = *(uint8_t *)(start+3)|(extra << 8); break;
  case OisState::SV_VAL_4: value = *(uint16_t*)(start+1);              channel = *(uint16_t*)(start+3);              break;
  }
  if( channel < ois.numInputs )
  {
    OisNumericInput& v = ois.inputs[channel];
    v.value = value;
  }
#ifdef DEBUG
  else
  {
    char buffer[128];
    switch( command )
    {
    case OisState::SV_VAL_1: sprintf(buffer, "Err: SV_VAL_1 %d=%d (%02hhX%02hhX)", channel, value, start[0], start[1]); break;
    case OisState::SV_VAL_2: sprintf(buffer, "Err: SV_VAL_2 %d=%d (%02hhX%02hhX%02hhX)", channel, value, start[0], start[1], start[2]); break;
    case OisState::SV_VAL_3: sprintf(buffer, "Err: SV_VAL_3 %d=%d (%02hhX%02hhX%02hhX%02hhX)", channel, value, start[0], start[1], start[2], start[3]); break;
    case OisState::SV_VAL_4: sprintf(buffer, "Err: SV_VAL_4 %d=%d (%02hhX%02hhX%02hhX%02hhX%02hhX)", channel, value, start[0], start[1], start[2], start[3], start[4]); break;
    }
    ois_print(ois, buffer);
  }
#endif
  return cmdLength;
}

void ois_recv(OisState& ois)
{
  int canRead = Serial.available();
  if( canRead == 0 )
    return;
  canRead = min(OIS_BUFFER_LENGTH - ois.messageLength, canRead);
  if( canRead > 0 )
  {
    Serial.readBytes(ois.messageBuffer + ois.messageLength, canRead);
    ois.messageLength += canRead;
  }
  else//shouldn't happen!!!
  {
#ifdef DEBUG
    ois_print(ois, "ERROR: Command buffer full. Buffer blocked?!");
#endif
    ois.messageLength = 0;
    Serial.print("END\n");
  }

  char* start = ois.messageBuffer;
  char* end = start + ois.messageLength;
  
#if OIS_MAX_VERSION >= 2
  if( ois.binary )
  {
    while( start < end )
    {
      int commandLength = ois_parse_binary(ois, start, end);
      if( commandLength == 0 )//need to read more data
        break;
      if( commandLength < 0 )//not a binary command!
      {
        ois_print(ois, "Received ASCII, resetting...");
        ois.version = ois.maxVersion;
        ois_reset(ois);
        return;
      }
      start += commandLength;//processed
    }
  }
  else
#endif
  {
    for( char* c = start; c != end; ++c )
    {
      if( *c != '\n' )
        continue;
      *c = '\0';
      ois_parse_ascii(ois, start);
      start = c+1;
    }
  }
  
  if( start == ois.messageBuffer && end == ois.messageBuffer+OIS_BUFFER_LENGTH )
  {//filled up the buffer without getting a newline!? probably garbage???
#ifdef DEBUG
    ois_print(ois, "ERROR: Command buffer full / no commands processed?!");
#endif
    ois.messageLength = 0;
    Serial.print("END\n");
  }
  else
  {
    ois.messageLength = (int)(end-start);
    if( start != ois.messageBuffer )
      memmove(ois.messageBuffer, start, ois.messageLength);
  }
}

void ois_send_handshake(OisState& ois)
{/*
  ++ois.synCount;
  if( ois.synCount > 300 && ois.baud != 9600 )
  {
    Serial.println("DBG=Resetting to 9600 baud");
    Serial.flush();
    Serial.begin( ois.baud = 9600 );
    ois.synCount = 0;
  }*/
#if OIS_MIN_VERSION == 0
  Serial.print("451\n");//hack for objects in space beta
#endif

  Serial.print("SYN=");
  Serial.print(ois.version);
#if OIS_MAX_VERSION > 1
  if( ois.version > 1 )
  {
#if OIS_BINARY
    Serial.print(",B");
#else
    Serial.print(",A");
#endif
  }
#endif
  Serial.print("\n");

  delay(500);
}


#if OIS_MAX_VERSION >= 2
void ois_send_pid(OisState& ois)
{
  IF_BINARY
  {
    uint8_t data[9];
    data[0] = OisState::CL_PID;
    data[1] = (uint8_t)(ois.pid);
    data[2] = (uint8_t)(ois.pid>>8);
    data[3] = (uint8_t)(ois.pid>>16);
    data[4] = (uint8_t)(ois.pid>>24);
    data[5] = (uint8_t)(ois.vid);
    data[6] = (uint8_t)(ois.vid>>8);
    data[7] = (uint8_t)(ois.vid>>16);
    data[8] = (uint8_t)(ois.vid>>24);
    Serial.write(data, 9);
    Serial.write(ois.deviceName, strlen(ois.deviceName)+1);
  }
  IF_ASCII
  {
    Serial.print("PID=");
    Serial.print(ois.pid);
    Serial.print(",");
    Serial.print(ois.vid);
    Serial.print(",");
    Serial.println(ois.deviceName);
  }
}
#endif

void ois_send_sync(OisState& ois)
{
#if OIS_MAX_VERSION >= 2
  if( ois.version >= 2 )
    ois_send_pid(ois);
#endif
    
  int channel = 0;
  for( int i=0, end=ois.numCommands; i!=end; ++i, ++channel )
  {
    OisCommand& c = ois.commands[i];
    IF_BINARY
    {
      uint8_t data[3];
      data[0] = OisState::CL_CMD;
      data[1] = (uint8_t)(channel);
      data[2] = (uint8_t)(channel>>8);
      Serial.write(data, 3);
      Serial.write(c.name, strlen(c.name)+1);
    }
    IF_ASCII
    {
      Serial.print("CMD=");
      Serial.print(c.name); Serial.print(","); Serial.print(channel); Serial.print("\n");
    }
  }
  for( int i=0, end=ois.numInputs; i!=end; ++i, ++channel )
  {
    OisNumericInput& c = ois.inputs[i];
    IF_BINARY
    {
      uint8_t data[3];
      data[0] = OisState::CL_NIO | (c.type==Number ? OisState::CL_N_PAYLOAD_N : (c.type==Fraction ? OisState::CL_N_PAYLOAD_F : 0));
      data[1] = (uint8_t)(channel);
      data[2] = (uint8_t)(channel>>8);
      Serial.write(data, 3);
      Serial.write(c.name, strlen(c.name)+1);
    }
    IF_ASCII
    {
      switch( c.type )
      {
        case Boolean:  Serial.print("NIB="); break;
        case Number:   Serial.print("NIN="); break;
        case Fraction: Serial.print("NIF="); break;
      }
      Serial.print(c.name); Serial.print(","); Serial.print(channel); Serial.print("\n");
    }
  }
  if( ois.version >= 2 )
  {
    for( int i=0, end=ois.numOutputs; i!=end; ++i, ++channel )
    {
      OisNumericOutput& c = ois.outputs[i];
      IF_BINARY
      {
        uint8_t data[3];
        data[0] = OisState::CL_NIO | OisState::CL_N_PAYLOAD_O | (c.type==Number ? OisState::CL_N_PAYLOAD_N : (c.type==Fraction ? OisState::CL_N_PAYLOAD_F : 0));
        data[1] = (uint8_t)(channel);
        data[2] = (uint8_t)(channel>>8);
        Serial.write(data, 3);
        Serial.write(c.name, strlen(c.name)+1);
      }
      IF_ASCII
      {
        switch( c.type )
        {
          case Boolean:  Serial.print("NOB="); break;
          case Number:   Serial.print("NON="); break;
          case Fraction: Serial.print("NOF="); break;
        }
        Serial.print(c.name); Serial.print(","); Serial.print(channel); Serial.print("\n");
      }
    }
  }
  IF_BINARY
  {
    uint8_t data[1];
    data[0] = OisState::CL_ACT;
    Serial.write(data, 1);
  }
  IF_ASCII
  {
    Serial.print("ACT\n");
  }
  ois.deviceState = OisState::Active;
}

void ois_send_command(OisState& ois, int index)
{
  IF_BINARY
  {
    const unsigned extraBits = 8 - OisState::CL_PAYLOAD_SHIFT;
    const unsigned channelLimit1 = 1U<<extraBits;
    const unsigned channelLimit2 = 1U<<(8+extraBits);
      
    uint8_t data[3];
    int length;
    if( index < channelLimit1 )
    {
      length = 1;
      data[0] = OisState::CL_EXC_0 | (uint8_t)(index << OisState::CL_PAYLOAD_SHIFT);
    }
    else if( index < channelLimit2 )
    {
      length = 2;
      data[0] = OisState::CL_EXC_1 | (uint8_t)((index>>8) << OisState::CL_PAYLOAD_SHIFT);
      data[1] = (uint8_t)(index);
    }
    else
    {
      length = 3;
      data[0] = OisState::CL_EXC_2;
      data[1] = (uint8_t)(index);
      data[2] = (uint8_t)(index>>8);
    }
    Serial.write(data, length);
  }
  IF_ASCII
  {
    Serial.print("EXC=");
    Serial.print(index);
    Serial.print("\n");
  }
}

#if OIS_MAX_VERSION >= 2
void ois_send_output(OisState& ois, int index)
{
  int base = ois.numCommands + ois.numInputs;
  int channel = index + base;
  int value = ois.outputs[index].value;
  IF_BINARY
  {
    uint8_t data[5];
    int length = 0;
    uint16_t u = (uint16_t)value;
    const unsigned extraBits = 8 - OisState::CL_PAYLOAD_SHIFT;
    const unsigned valueLimit1   = 1U<<extraBits;
    const unsigned valueLimit2   = 1U<<(8+extraBits);
    const unsigned channelLimit3 = 1U<<(8+extraBits);
    if( channel < 256 && u < valueLimit1 )
    {
      data[0] = (uint8_t)(OisState::CL_VAL_1 | (u << OisState::CL_PAYLOAD_SHIFT));
      data[1] = (uint8_t)(channel);
      length = 2;
    }
    else if( channel < 256 && u < valueLimit2 )
    {
      data[0] = (uint8_t)(OisState::CL_VAL_2 | ((u>>8) << OisState::CL_PAYLOAD_SHIFT));
      data[1] = (uint8_t)(u);
      data[2] = (uint8_t)(channel);
      length = 3;
    }
    else if( channel < channelLimit3 )
    {
      data[0] = (uint8_t)(OisState::CL_VAL_3 | ((channel>>8) << OisState::CL_PAYLOAD_SHIFT));
      data[1] = (uint8_t)(u);
      data[2] = (uint8_t)(u>>8);
      data[3] = (uint8_t)(channel);
      length = 4;
    }
    else
    {
      data[0] = (uint8_t)OisState::CL_VAL_4;
      data[1] = (uint8_t)(u);
      data[2] = (uint8_t)(u>>8);
      data[3] = (uint8_t)(channel);
      data[4] = (uint8_t)(channel>>8);
      length = 5;
    }
    Serial.write(data, length);
  }
  IF_ASCII
  {
    Serial.print(channel);
    Serial.print("=");
    Serial.print(value);
    Serial.print("\n");
  }
}
#endif

void ois_send_active(OisState& ois)
{
  int sent = 0;
  int sendLimit = 2;//todo
  if( ois.numTouchedCommands )
  {
    int i, end;
    for( i=ois.touchedCommandsIterator, end=ois.numTouchedCommandsMasks; i<end && sent < sendLimit; ++i )
    {
      uint& value = ois.touchedCommandsMasks[i];
      if( !value )
        continue;
      for( int j=0, mask=1; j != 16; ++j, mask <<= 1 )
      {
        if( value & mask )
        {
          ois_send_command(ois, i * (sizeof(int)*8) + j);
          value &= ~mask;
          --ois.numTouchedCommands;
          ++sent;
          if( sent >= sendLimit )
            break; 
        }
      }
    }
    ois.touchedCommandsIterator = i >= end ? 0 : i;
  }
  
#if OIS_MAX_VERSION >= 2
  if( ois.numTouchedOutputs && ois.version >= 2 )
  {
    int i, end;
    for( i=ois.touchedOutputsIterator, end=ois.numTouchedOutputsMasks; i<end && sent < sendLimit; ++i )
    {
      uint& value = ois.touchedOutputsMasks[i];
      if( !value )
        continue;
      for( uint j=0, mask=1; j != 16; ++j, mask <<= 1 )
      {
        if( value & mask )
        {
          ois_send_output(ois, i * (sizeof(int)*8) + j);
          value &= ~mask;
          --ois.numTouchedOutputs;
          ++sent;
          if( sent >= sendLimit )
            break; 
        }
      }
    }
    ois.touchedOutputsIterator = i >= end ? 0 : i;
  }
#endif
}
#else
void ois_set(OisState& ois, OisNumericOutput& output, int data)
{
  if( output.value == data )
    return;
  output.value = data;
  int index = ois.numCommands + (&output - ois.outputs);
  int slot = index / 8;
  int bitIndex = index % 8;
  uint8_t mask = 1U << bitIndex;
  //todo - some outputs might be axes...
  if( data )
    joyReport.button[index] |= mask;
  else
    joyReport.button[index] &= ~mask;
}

void ois_execute(OisState& ois, OisCommand& command)
{
  int index = &command - ois.commands;
  int slot = index / 8;
  int bitIndex = index % 8;
  uint8_t mask = 1U << bitIndex;
  joyReport.button[index] |= mask;
}
#endif


void ois_loop(OisState& ois)
{
#ifdef USB_JOYSTICK
  sendJoyReport(&joyReport);
  //todo - for each command, clear button bit
  delay(1);
#else
  ois_recv(ois);
  switch( ois.deviceState )
  {
    default:
    case OisState::Handshaking:     return ois_send_handshake(ois);
    case OisState::Synchronisation: return ois_send_sync(ois);
    case OisState::Active:          return ois_send_active(ois);
  }
#endif
}

void ois_print(OisState& ois, const char* text)
{
#if !defined USB_JOYSTICK
  IF_BINARY
  {
    uint8_t data[1] = { OisState::CL_DBG };
    Serial.write(data, 1);
    Serial.write(text, strlen(text)+1);
  }
  IF_ASCII
  {
    Serial.print("DBG=");
    Serial.print(text);
    Serial.print("\n");
  }
#elif defined _DEBUG
  Serial.print(text);
#endif
}

