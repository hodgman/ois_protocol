#ifdef USB_JOYSTICK
# include "joystick.h"
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



#ifndef OIS_MAX_COMMAND_LENGTH
#define OIS_MAX_COMMAND_LENGTH 128
#endif

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
  unsigned long pid=0, vid=0;
  
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
};


template<class CMD, class NI, class NO>
void ois_setup_structs(OisState& ois, const char* name, uint32_t pid, uint32_t vid, CMD& commands, NI& inputs, NO& outputs, int version = 2)
{
  ois_setup(ois, name, pid, vid, 
            (OisCommand*)&commands, sizeof(commands)/sizeof(OisCommand),
            (OisNumericInput*)&inputs, sizeof(inputs)/sizeof(OisNumericInput),
            (OisNumericOutput*)&outputs, sizeof(outputs)/sizeof(OisNumericOutput), version);
}

template<int CMD, int NI, int NO>
void ois_setup(OisState& ois, const char* name, uint32_t pid, uint32_t vid, OisCommand(&commands)[CMD], OisNumericInput(&inputs)[NI], OisNumericOutput(&outputs)[NO], int version = 2)
{
  ois_setup(ois, name, pid, vid, commands, CMD, inputs, NI, outputs, NO, version);
}
template<int CMD, int NI>
void ois_setup(OisState& ois, const char* name, uint32_t pid, uint32_t vid, OisCommand(&commands)[CMD], OisNumericInput(&inputs)[NI], int version = 2)
{
  ois_setup(ois, name, pid, vid, commands, CMD, inputs, NI, 0, 0, version);
}
template<int CMD, int NO>
void ois_setup(OisState& ois, const char* name, uint32_t pid, uint32_t vid, OisCommand(&commands)[CMD],  OisNumericOutput(&outputs)[NO], int version = 2)
{
  ois_setup(ois, name, pid, vid, commands, CMD, 0, 0, outputs, NO, version);
}
template<int NI, int NO>
void ois_setup(OisState& ois, const char* name, uint32_t pid, uint32_t vid, OisNumericInput(&inputs)[NI], OisNumericOutput(&outputs)[NO], int version = 2)
{
  ois_setup(ois, name, pid, vid, 0, 0, inputs, NI, outputs, NO, version);
}
template<int CMD>
void ois_setup(OisState& ois, const char* name, uint32_t pid, uint32_t vid, OisCommand(&commands)[CMD], int version = 2)
{
  ois_setup(ois, name, pid, vid, commands, CMD, 0, 0, 0, 0, version);
}
template<int NI>
void ois_setup(OisState& ois, const char* name, uint32_t pid, uint32_t vid, OisNumericInput(&inputs)[NI], int version = 2)
{
  ois_setup(ois, name, pid, vid, 0, 0, inputs, NI, 0, 0, version);
}
template<int NO>
void ois_setup(OisState& ois, const char* name, uint32_t pid, uint32_t vid, OisNumericOutput(&outputs)[NO], int version = 2)
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
  ois.synCount = 0;
  ois.gameTitle[0] = '\0';
  ois.gameVersion = 0;
  ois.deviceState = OisState::Handshaking;
}

void ois_parse(OisState& ois, char* cmd)
{
  if( !cmd[0] )
    return;
  u32 type = 0;
  if( cmd[1] && cmd[2] )
    type = FOURCC(cmd);
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
  {
    char* payload = cmd + 4;
    switch( type )
    {
      default:
        break;
      case FOURCC("END\0")://v2
      {
        ois.version = ois.maxVersion;
        ois_reset(ois);
        break;
      }
      case FOURCC("DEN\0")://v1
      {
        if( ois.version == 451 )
          ois.version = 2;
        else if( ois.version > 0 )
          ois.version--;
        else
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
      }//fall-through:
      case FOURCC("ACK\0")://v1
        ois.deviceState = OisState::Synchronisation;
        break;
      case FOURCC("452\r")://hack for objects in space beta
      {
        ois.version = 1;
        ois.deviceState = OisState::Synchronisation;
        break;
      }
    }
  }
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
    ois.messageBuffer[OIS_BUFFER_LENGTH-1] = '\n';
    ois.messageLength = OIS_BUFFER_LENGTH;
  }

  char* start = ois.messageBuffer;
  char* end = start + ois.messageLength;
  for( char* c = start; c != end; ++c )
  {
    if( *c != '\n' )
      continue;
    *c = '\0';
    ois_parse(ois, start);
    start = c+1;
  }
  
  if( start == ois.messageBuffer && end == ois.messageBuffer+OIS_BUFFER_LENGTH )
  {//filled up the buffer without getting a newline!? probably garbage???
    ois.messageBuffer[OIS_BUFFER_LENGTH-1] = '\0';
    ois_parse(ois, ois.messageBuffer);
    ois.messageLength = 0;
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
  if( ois.maxVersion == 451 )//hack for objects in space beta
    Serial.print("541\n");

  Serial.print("SYN=");
  Serial.print(ois.version);
  Serial.print("\n");

  delay(500);
}


void ois_send_pid(OisState& ois)
{
  Serial.print("PID=");
  Serial.print(ois.pid);
  Serial.print(",");
  Serial.print(ois.vid);
  Serial.print(",");
  Serial.println(ois.deviceName);
}

void ois_send_sync(OisState& ois)
{
  ois_send_pid(ois);
    
  int channel = 0;
  for( int i=0, end=ois.numCommands; i!=end; ++i, ++channel )
  {
    OisCommand& c = ois.commands[i];
    Serial.print("CMD=");
    Serial.print(c.name); Serial.print(","); Serial.print(channel); Serial.print("\n");
  }
  for( int i=0, end=ois.numInputs; i!=end; ++i, ++channel )
  {
    OisNumericInput& c = ois.inputs[i];
    switch( c.type )
    {
      case Boolean:  Serial.print("NIB="); break;
      case Number:   Serial.print("NIN="); break;
      case Fraction: Serial.print("NIF="); break;
    }
    Serial.print(c.name); Serial.print(","); Serial.print(channel); Serial.print("\n");
  }
  if( ois.version >= 2 )
  {
    for( int i=0, end=ois.numOutputs; i!=end; ++i, ++channel )
    {
      OisNumericOutput& c = ois.outputs[i];
      switch( c.type )
      {
        case Boolean:  Serial.print("NOB="); break;
        case Number:   Serial.print("NON="); break;
        case Fraction: Serial.print("NOF="); break;
      }
      Serial.print(c.name); Serial.print(","); Serial.print(channel); Serial.print("\n");
    }
  }
  Serial.print("ACT\n");
  ois.deviceState = OisState::Active;
}

void ois_send_command(OisState& ois, int index)
{
  Serial.print("EXC=");
  Serial.print(index);
  Serial.print("\n");
}

void ois_send_output(OisState& ois, int index)
{
  int base = ois.numCommands + ois.numInputs;
  Serial.print(index + base);
  Serial.print("=");
  Serial.print(ois.outputs[index].value);
  Serial.print("\n");
}

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
  Serial.print("DBG=");
  Serial.print(text);
  Serial.print("\n");
#elif defined _DEBUG
  Serial.print(text);
#endif
}

