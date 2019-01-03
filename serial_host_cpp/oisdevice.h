
#ifndef OIS_INFO
#define OIS_INFO( fmt, ... ) do{}while(0)
#endif

#ifndef OIS_WARN
#define OIS_WARN( fmt, ... ) do{}while(0)
#endif

#ifndef OIS_ENABLE_ERROR_LOGGING
#define OIS_ENABLE_ERROR_LOGGING 0
#endif

#ifndef OIS_ASSERT
#define OIS_ASSERT( condition ) do{}while(0)
#endif

#ifndef OIS_ASSUME
#define OIS_ASSUME( condition )
#endif

#ifndef OIS_NO_STDINT
#include <stdint.h>
#endif 

#ifndef OIS_VECTOR
#include <vector>
#define OIS_VECTOR std::vector
#endif

#ifndef OIS_STRING
#include <string>
#define OIS_STRING std::string
#endif

#ifndef OIS_STRING_BUILDER
#include <stdarg.h>
struct OIS_STRING_BUILDER
{
	const char* FormatTemp(const char* fmt, ...)
	{
		va_list	v;
		va_start(v, fmt);
		const char* s = FormatV(temp, fmt, v);
		va_end( v );
		return s;
	}
	const char* Format(OIS_STRING& result, const char* fmt, ...)
	{
		va_list	v;
		va_start(v, fmt);
		const char* s = FormatV(result, fmt, v);
		va_end( v );
		return s;
	}
	char* AllocTemp(unsigned bytes)
	{
		temp.resize(bytes);
		return &temp[0];
	}
	void StoreTemp(OIS_STRING& result, const char* buffer)
	{
		result = buffer;
	}
private:
	OIS_STRING temp;
	const char* FormatV(OIS_STRING& result, const char* fmt, const va_list& v)
	{
		int length = _vscprintf( fmt, v ) + 1;
		result.resize(length);
		char* buffer = &result[0];
		vsnprintf(buffer, length, fmt, v);
		return buffer;
	}
};
#endif

#ifndef OIS_ARRAYSIZE
namespace OisDeviceInternal
{
	template<unsigned N> struct Sizer { char elems[N]; };
	template<class Type, unsigned N> Sizer<N> ArraySize_( Type(&)[N] );
}
#define OIS_ARRAYSIZE( a ) sizeof( OisDeviceInternal::ArraySize_( a ).elems )
#endif

#ifndef OIS_MIN
namespace OisDeviceInternal
{
	template<class T> T Min( T a, T b ) { return a<b ? a : b; }
}
#define OIS_MIN OisDeviceInternal::Min
#endif


struct PortName
{
	uint32_t id;
	OIS_STRING path;
	OIS_STRING name;
};
typedef OIS_VECTOR<PortName> OIS_PORT_LIST;

#ifndef OIS_PORT
#define OIS_PORT SerialPort
#include "serialport.hpp"
#endif

#ifndef OIS_FOURCC
#define OIS_FOURCC(str)                                                          \
	((uint32_t)(uint8_t)(str[0])        | ((uint32_t)(uint8_t)(str[1]) << 8) |   \
	((uint32_t)(uint8_t)(str[2]) << 16) | ((uint32_t)(uint8_t)(str[3]) << 24 ))  //
#endif

#ifndef OIS_MAX_COMMAND_LENGTH
const static unsigned OIS_MAX_COMMAND_LENGTH = 128;
#endif

class OisDevice
{
public:
	OisDevice(unsigned id, const OIS_STRING& path, const OIS_STRING& name, unsigned gameVersion, const char* gameName)
		: m_portName(path)
		, m_deviceName(name)
		, m_portId(id)
		, m_gameVersion(gameVersion)
		, m_gameName(gameName)
	{
		ClearState();
	}
	const char* GetDeviceName() const { return m_deviceNameOverride.empty() ? m_deviceName.c_str() : m_deviceNameOverride.c_str(); }
	uint32_t    GetProductID()  const { return m_pid; }
	uint32_t    GetVendorID()   const { return m_vid; }
	unsigned    GetPortId()     const { return m_portId; }
	bool        Connecting()    const { return m_connectionState != Handshaking; }
	bool        Connected()     const { return m_connectionState == Active; }
	
	void Poll(OIS_STRING_BUILDER&);
	
	enum NumericType
	{
		Boolean,
		Number,
		Fraction,
	};
	typedef union {
		bool    boolean;
		int32_t number;
		float   fraction;
	} Value;
	struct NumericValue
	{
		OIS_STRING  name;
		uint16_t    channel;
		bool        active;
		NumericType type;
		Value       value;
	};
	
	const OIS_VECTOR<NumericValue>& DeviceInputs()  { return m_numericInputs; }
	const OIS_VECTOR<NumericValue>& DeviceOutputs() { return m_numericOutputs; }

	void SetInput(const NumericValue& input, Value value);
private:
	enum ClientCommandsAscii
	{
		//v1 commands
		SYN = OIS_FOURCC("SYN="),
		CMD = OIS_FOURCC("CMD="),
		NIB = OIS_FOURCC("NIB="),
		NIN = OIS_FOURCC("NIN="),
		NIF = OIS_FOURCC("NIF="),
		ACT = OIS_FOURCC("ACT\0"),
		EXC = OIS_FOURCC("EXC="),
		DBG = OIS_FOURCC("DBG="),
		//v2 commands
		NOB = OIS_FOURCC("NOB="),
		NON = OIS_FOURCC("NON="),
		NOF = OIS_FOURCC("NOF="),
		TNI = OIS_FOURCC("TNI="),
		PID = OIS_FOURCC("PID="),
		END = OIS_FOURCC("END\0"),
	};
	enum ClientCommandsBin
	{
		CL_NUL   = 0x00,
		CL_CMD   = 0x01,
		CL_NIO   = 0x02,
		CL_ACT   = 0x03,
		CL_SYN_  = 'S',//0x53
		CL_DBG   = 0x04,
		CL_451_  = '4',//0x34
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
	enum DeviceState
	{
		Handshaking,
		Synchronisation,
		Active,
	};
	typedef uint32_t DeviceStateMask;
	
	static char* ZeroDelimiter(char* str, char delimiter);
	void SendData(const char* cmd, int length);
	void SendText(const char* cmd);
	bool ExpectState(DeviceStateMask state, const char* cmd, unsigned version);
	void ClearState();
	bool ProcessAscii(char* cmd, OIS_STRING_BUILDER&);
	int ProcessBinary(char* start, char* end);

	OIS_PORT m_port;
	OIS_STRING m_portName;
	OIS_STRING m_deviceName;

	unsigned m_gameVersion;
	const char* m_gameName;

	OIS_STRING m_deviceNameOverride;
	unsigned m_portId;
	unsigned m_protocolVersion;
	bool     m_binary;
	uint32_t m_pid;
	uint32_t m_vid;
	DeviceState m_connectionState;
	char m_commandBuffer[OIS_MAX_COMMAND_LENGTH*2];
	unsigned m_commandLength;
	
	struct Event
	{
		uint16_t channel;
		OIS_STRING name;
	};
	OIS_VECTOR<NumericValue> m_numericInputs;
	OIS_VECTOR<NumericValue> m_numericOutputs;
	OIS_VECTOR<uint16_t>     m_queuedInputs;
	OIS_VECTOR<Event>        m_events;
	OIS_VECTOR<uint16_t>     m_eventBuffer;

	template<class T>
	static typename T::value_type* FindChannel(T& values, int channel)
	{
		for( auto& v : values )
			if( v.channel == channel )
				return &v;
		return nullptr;
	}
};

#ifdef OIS_DEVICE_IMPL
void OisDevice::Poll(OIS_STRING_BUILDER& sb)
{
	if( !m_port.IsConnected() )
	{
		if( m_connectionState != Handshaking )
			ClearState();
		m_port.Connect(m_portName.c_str());
		return;
	}
	for( ;; )
	{
		int len = m_port.Read(m_commandBuffer+m_commandLength, OIS_ARRAYSIZE(m_commandBuffer)-m_commandLength);
		if( !len )
			break;
		m_commandLength += len;
		OIS_ASSERT( m_commandLength <= OIS_ARRAYSIZE(m_commandBuffer) );

		char* start = m_commandBuffer;
		char* end = start + m_commandLength;
		if( m_binary )
		{
			while( start < end )
			{
				int commandLength = ProcessBinary(start, end);
				if( commandLength == 0 )//need to read more data
					break;
				if( commandLength < 0 )//not a binary command!
				{
					ClearState();
					return;
				}
				start += commandLength;//processed
			}
		}
		else
		{
			for( char* c = start; c != end; ++c )
			{
				if( *c != '\n' )
					continue;
				*c = '\0';
				ProcessAscii(start, sb);
				start = c+1;
			}
		}
		OIS_ASSERT( start <= end );

		if( start == m_commandBuffer && end == m_commandBuffer+OIS_ARRAYSIZE(m_commandBuffer) )
		{
			OIS_WARN("OisDevice command buffer is full without a valid command present! Ending...");
			OIS_INFO( "-> END" );
			SendText("END\n");
			ClearState();
		}
		else
		{
			m_commandLength = (unsigned)(end-start);
			if( start != m_commandBuffer )
				memmove(m_commandBuffer, start, m_commandLength);
		}
	}
	for( int index : m_queuedInputs )
	{
		const NumericValue& v = m_numericInputs[index];
		int16_t data = 0;
		switch( v.type )
		{
		default: OIS_ASSERT( false );
		case Boolean:
			data = v.value.boolean ? 1 : 0;
			OIS_INFO( "-> %d(%s) = %s", v.channel, v.name.c_str(), v.value.boolean ? "true" : "false" );
			break;
		case Number:
			data = (int16_t)Clamp(v.value.number, -32768, 32767);
			OIS_INFO( "-> %d(%s) = %d", v.channel, v.name.c_str(), v.value.number );
			break;
		case Fraction:
			data = (int16_t)Clamp((int)(v.value.fraction*100.0f), -32768, 32767);
			OIS_INFO( "-> %d(%s) = %.2f", v.channel, v.name.c_str(), v.value.fraction );
			break;
		}
		if( m_binary )
		{
			uint8_t cmd[5];
			int cmdLength = 0;
			uint16_t u = (uint16_t)data;
			const unsigned extraBits = 8 - SV_PAYLOAD_SHIFT;
			const unsigned valueLimit1   = 1U<<extraBits;
			const unsigned valueLimit2   = 1U<<(8+extraBits);
			const unsigned channelLimit3 = 1U<<(8+extraBits);
			if( v.channel < 256 && u < valueLimit1 )
			{
				cmd[0] = (uint8_t)(0xFF & (SV_VAL_1 | (u << SV_PAYLOAD_SHIFT)));
				cmd[1] = (uint8_t)(0xFF & v.channel);
				cmdLength = 2;
			}
			else if( v.channel < 256 && u < valueLimit2 )
			{
				cmd[0] = (uint8_t)(0xFF & (SV_VAL_2 | ((u>>8) << SV_PAYLOAD_SHIFT)));
				cmd[1] = (uint8_t)(0xFF & u);
				cmd[2] = (uint8_t)(0xFF & v.channel);
				cmdLength = 3;
			}
			else if( v.channel < channelLimit3 )
			{
				cmd[0] = (uint8_t)(0xFF & (SV_VAL_3 | ((v.channel>>8) << SV_PAYLOAD_SHIFT)));
				cmd[1] = (uint8_t)(0xFF &  u);
				cmd[2] = (uint8_t)(0xFF & (u>>8));
				cmd[3] = (uint8_t)(0xFF & v.channel);
				cmdLength = 4;
			}
			else
			{
				cmd[0] = (uint8_t)SV_VAL_4;
				cmd[1] = (uint8_t)(0xFF &  u);
				cmd[2] = (uint8_t)(0xFF & (u>>8));
				cmd[3] = (uint8_t)(0xFF &  v.channel);
				cmd[4] = (uint8_t)(0xFF & (v.channel>>8));
				cmdLength = 5;
			}
			SendData((char*)cmd, cmdLength);
		}
		else
			SendText(sb.FormatTemp("%d=%d\n", v.channel, data));
	}
	m_queuedInputs.clear();
}

char* OisDevice::ZeroDelimiter(char* str, char delimiter)
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

static int CmdStrLength(const char* c, const char* end, char terminator)
{
	int length = 0;
	bool foundNull = false;
	for( ; c != end && !foundNull; ++c, ++length )
		foundNull = *c == terminator;
	return length + (foundNull ? 0 : OIS_MAX_COMMAND_LENGTH*2);
}

int OisDevice::ProcessBinary(char* start, char* end)
{
	if( end <= start )
		return 0;

	int bufferLength = (int)(end - start);
	
	const char* startString = 0;
	char strTerminator = '\0';
	uint32_t payload = (*start);
	int command = payload & CL_COMMAND_MASK;
	int cmdLength = 1;
	if( payload == CL_SYN_ || payload == CL_451_ )//has the device reset and is sending us ASCII commands?
	{
		cmdLength = 0;
		startString = start;
		strTerminator = '\n';
	}
	else switch( command )
	{
		default:
		case CL_NUL:
			OIS_WARN( "Unknown command: 0x%x", payload);
			break;
		case CL_ACT:
		case CL_END:
		case CL_EXC_0:
			break;
		case CL_CMD:
		case CL_NIO:
			cmdLength += 2;//channel
			startString = start + cmdLength;
			break;
		case CL_DBG:
			startString = start + cmdLength;
			break;
		case CL_PID:
			cmdLength += 8;//pid/vid
			startString = start + cmdLength;
			break;
		case CL_EXC_1:
		case CL_VAL_1:
			cmdLength += 1;
			break;
		case CL_TNI:
		case CL_EXC_2:
		case CL_VAL_2:
			cmdLength +=2;
			break;
		case CL_VAL_3:
			cmdLength += 3;
			break;
		case CL_VAL_4:
			cmdLength += 4;
			break;
	}

	if( startString )
	{
		if( startString >= end )
			return 0;
		cmdLength += CmdStrLength(startString, end, strTerminator);
	}

	if( bufferLength < cmdLength )
		return 0;

	//has the device reset and is sending us ASCII commands?
	if( (payload == CL_SYN_ && 0==strcmp(startString, "SYN")) ||
	    (payload == CL_451_ && 0==strcmp(startString, "451")) )
	{
		return -1;
	}

	switch( command )
	{
		case CL_PID:
		{
			ExpectState( 1<<Synchronisation, "PID", 2 );
			m_pid = *(uint32_t*)(start+1);
			m_vid = *(uint32_t*)(start+5);
			char* name = start+9;
			m_deviceNameOverride = OIS_STRING(name);
			OIS_INFO( "<- PID: %d/%d %s", m_pid, m_vid, name );
			break;
		}
		case CL_CMD:
		{
			if( !ExpectState( (1<<Synchronisation)|(1<<Active), "CMD", 2 ) )
				return false;
			uint16_t channel = *(uint16_t*)(start+1);
			char* name = start+3;
			m_events.push_back({channel, OIS_STRING(name)});
			OIS_INFO( "<- CMD: %d %s", channel, name );
			break;
		}
		case CL_NIO:
		{
			if( !ExpectState( (1<<Synchronisation)|(m_protocolVersion>1?1<<Active:0), "NIO", 2 ) )
				return false;
			bool output = payload & CL_N_PAYLOAD_O ? true :false;
			NumericType nt = payload & CL_N_PAYLOAD_F ? Fraction :
			                (payload & CL_N_PAYLOAD_N ? Number : Boolean);
			uint16_t channel = *(uint16_t*)(start+1);
			char* name = start+3;
			if( output )
			{
				m_numericOutputs.push_back({OIS_STRING(name), channel, true, nt});
				m_numericOutputs.back().value.number = 0;
			}
			else
			{
				m_numericInputs.push_back({OIS_STRING(name), channel, true, nt});
				m_numericInputs.back().value.number = 0xcdcdcdcd;
			}
			OIS_INFO( "<- NIO: %d %s (%s %s)", channel, name, output?"Out":"In", nt==Fraction?"Fraction":(nt==Number?"Number":"Boolean") );
			break;
		}
		case CL_ACT:
		{
			ExpectState( 1<<Synchronisation, "ACT", 2 );
			m_connectionState = Active;
			OIS_INFO( "<- ACT" );
			break;
		}
		case CL_TNI:
		{
			ExpectState( (1<<Synchronisation) | (1<<Active), "TNI", 2 );
			uint16_t channel = *(uint16_t*)(start+1);
			NumericValue* v = FindChannel(m_numericInputs, channel);
			OIS_INFO( "<- TNI %d (%s)", channel, v?v->name.c_str():"UNKNOWN CHANNEL" );
			if( v )
				v->active = (payload & CL_TNI_PAYLOAD_T) ? true: false;
			break;
		}
		case CL_DBG:
		{
			OIS_INFO( "<- DBG: %s", startString);
			break;
		}
		case CL_EXC_0:
		case CL_EXC_1:
		case CL_EXC_2:
		{
			ExpectState( 1<<Active, "EXC", 2 );
			uint16_t channel = 0;
			uint16_t extra = (uint16_t)(payload >> CL_PAYLOAD_SHIFT);
			switch( command )
			{
			default: eiASSUME(false);
			case CL_EXC_0: channel = extra; break;
			case CL_EXC_1: channel = (*(uint8_t *)(start+1)) | (extra<<8); break;
			case CL_EXC_2: channel = (*(uint16_t*)(start+1)); break;
			}
			Event* e = FindChannel(m_events, channel);
			if( e )
				m_eventBuffer.push_back(channel);
			OIS_INFO( "<- EXC: %d (%s)", channel, e ? e->name.c_str() : "INVALID CHANNEL" );
			break;
		}
		case CL_VAL_1:
		case CL_VAL_2:
		case CL_VAL_3:
		case CL_VAL_4:
		{
			if( !ExpectState( 1<<Active, "VAL", 2 ) )
				return false;
			int16_t channel = 0;
			uint32_t extra = payload >> CL_PAYLOAD_SHIFT;
			int16_t value = 0;
			switch( command )
			{
			default: eiASSUME(false);
			case CL_VAL_1: value = extra;                              channel = *(uint8_t *)(start+1);              break;
			case CL_VAL_2: value = *(uint8_t *)(start+1)|(extra << 8); channel = *(uint8_t *)(start+2);              break;
			case CL_VAL_3: value = *(uint16_t*)(start+1);              channel = *(uint8_t *)(start+3)|(extra << 8); break;
			case CL_VAL_4: value = *(uint16_t*)(start+1);              channel = *(uint16_t*)(start+3);              break;
			}
			NumericValue* v = FindChannel(m_numericOutputs, channel);
			if( v )
			{
				switch( v->type )
				{
				default: OIS_ASSERT( false );
				case Boolean:  v->value.boolean = !!value; 
					OIS_INFO( "<- %d(%s) = %s", channel, v->name.c_str(), v->value.boolean ? "true" : "false" );
					break;
				case Number:   v->value.number = value; 
					OIS_INFO( "<- %d(%s) = %d", channel, v->name.c_str(), v->value.number );
					break;
				case Fraction: v->value.fraction = value/100.0f;
					OIS_INFO( "<- %d(%s) = %.2f", channel, v->name.c_str(), v->value.fraction );
					break;
				}
			}
			else
				OIS_WARN( "Received key/value message for unregistered channel %d", channel);
			break;
		}
		case CL_END:
		{
			OIS_INFO( "<- END");
			if( m_connectionState != Handshaking )
				ClearState();
			m_port.Disconnect();
			break;
		}
	}

	return cmdLength;
}

bool OisDevice::ProcessAscii(char* cmd, OIS_STRING_BUILDER& sb)
{
	if( !cmd[0] )
		return false;
	uint32_t type = 0;
	if( cmd[1] && cmd[2] )
		type = OIS_FOURCC(cmd);
	bool isKeyVal = 0!=isdigit(cmd[0]);
	if( isKeyVal )
	{
		if( !ExpectState( 1<<Active, cmd, 2 ) )
			return false;
		const char* payload = ZeroDelimiter(cmd, '=');
		int channel = atoi(cmd);
		NumericValue* v = FindChannel(m_numericOutputs, channel);
		if( v )
		{
			switch( v->type )
			{
			default: OIS_ASSERT( false );
			case Boolean:  v->value.boolean = !!atoi(payload); 
				OIS_INFO( "<- %d(%s) = %s", channel, v->name.c_str(), v->value.boolean ? "true" : "false" );
				break;
			case Number:   v->value.number = atoi(payload); 
				OIS_INFO( "<- %d(%s) = %d", channel, v->name.c_str(), v->value.number );
				break;
			case Fraction: v->value.fraction = atoi(payload)/100.0f;
				OIS_INFO( "<- %d(%s) = %.2f", channel, v->name.c_str(), v->value.fraction );
				break;
			}
		}
		else
			OIS_WARN( "Received key/value message for unregistered channel %d", channel);
	}
	else
	{
		char* payload = cmd + 4;
		switch( type )
		{
			default:
			{
				OIS_WARN( "Unknown command: %s", cmd);
				break;
			}
			case SYN:
			{
				if( !ExpectState(1<<Handshaking, cmd, 1) )
					ClearState();
				m_port.PurgeReadBuffer();
				char* mode = ZeroDelimiter(payload, ',');
				bool binary = *mode == 'B';
				int version = atoi(payload);
				OIS_INFO( "<- SYN: %d/%s", version, binary?"B":"A" );
				if( !(version == 1 && binary) && version >= 1 && version <= 2 )
				{
					m_binary = binary;
					m_protocolVersion = version;
					m_connectionState = Synchronisation;
					switch( version )
					{
					default: OIS_ASSUME(false);
					case 1: SendText("ACK\n");        break;
					case 2: SendText(sb.FormatTemp("ACK=%d,%s\n", m_gameVersion, m_gameName)); break;
					}
					OIS_INFO( "-> ACK", version );
				}
				else
				{
					OIS_INFO( "-> DEN", version );
					SendText("DEN\n");
					ClearState();
				}
				break;
			}
			case PID:
			{
				ExpectState(1<<Synchronisation, cmd, 2);
				char* pid = payload;
				char* vid = ZeroDelimiter(pid, ',');
				char* name = ZeroDelimiter(vid, ',');
				m_pid = atoi(pid);
				m_vid = atoi(vid);
				m_deviceNameOverride = name;
				OIS_INFO( "<- PID: %d/%d %s", m_pid, m_vid, name );
				break;
			}
			case CMD:
			{
				if( !ExpectState( (1<<Synchronisation)|(m_protocolVersion>1?1<<Active:0), cmd, 1 ) )
					return false;
				char* name = payload;
				uint16_t channel = (uint16_t)(0xFFFF & atoi(ZeroDelimiter(payload, ',')));
				m_events.push_back({channel, OIS_STRING(name)});
				OIS_INFO( "<- CMD: %d %s", channel, name );
				break;
			}
			case NIN: case NIF: case NIB:
			case NON: case NOF: case NOB:
			{
				bool output = false;
				NumericType nt;
				switch( type )
				{
				default: OIS_ASSUME( false );
				case NON: output = true;
				case NIN: nt = Number;	 break;
				case NOF: output = true;
				case NIF: nt = Fraction; break;
				case NOB: output = true;
				case NIB: nt = Boolean;	 break;
				}
				if( !ExpectState( (1<<Synchronisation)|(m_protocolVersion>1?1<<Active:0), cmd, output ? 2 : 1 ) )
					return false;
				char* name = payload;
				int channel = atoi(ZeroDelimiter(payload, ','));
				uint16_t channel16 = (uint16_t)(channel & 0xFFFFU);
				if( output )
				{
					m_numericOutputs.push_back({OIS_STRING(name), channel16, true, nt});
					m_numericOutputs.back().value.number = 0;
				}
				else
				{
					m_numericInputs.push_back({OIS_STRING(name), channel16, true, nt});
					m_numericInputs.back().value.number = 0xcdcdcdcd;
				}
				OIS_INFO( "<- %s: %d %s", cmd, channel16, name );
				break;
			}
			case TNI:
			{
				ExpectState( (1<<Synchronisation) | (1<<Active), cmd, 2 );
				const char* active = ZeroDelimiter(payload, ',');
				int channel = atoi(payload);
				NumericValue* v = FindChannel(m_numericInputs, channel);
				OIS_INFO( "<- TNI %d (%s)", channel, v?v->name.c_str():"UNKNOWN CHANNEL" );
				if( v )
					v->active = atoi(active) ? true: false;
			}
			case ACT:
			{
				ExpectState( 1<<Synchronisation, cmd, 1 );
				m_connectionState = Active;
				OIS_INFO( "<- ACT" );
				break;
			}

			case EXC:
			{
				ExpectState( 1<<Active, cmd, 1 );
				int channel = atoi(payload);
				Event* e = FindChannel(m_events, channel);
				if( e )
					m_eventBuffer.push_back(channel);
				OIS_INFO( "<- EXC: %d (%s)", channel, e ? e->name.c_str() : "INVALID CHANNEL" );
				break;
			}
			case DBG:
			{
				OIS_INFO( "<- DBG: %s", payload);
				break;
			}
			case END:
			{
				OIS_INFO( "<- END");
				if( m_connectionState != Handshaking )
					ClearState();
				m_port.Disconnect();
				break;
			}
		}
	}
	return true;
}
	
void OisDevice::SetInput(const NumericValue& input, Value value)
{
	if( m_numericInputs.empty() )
		return;
	unsigned index = (unsigned)(&input - &m_numericInputs.front());
	if( index >= m_numericInputs.size() )
		return;
	if( m_numericInputs[index].value.number == value.number )
		return;
	m_numericInputs[index].value = value;
	m_queuedInputs.push_back(index);
}

void OisDevice::SendData(const char* cmd, int length)
{
	m_port.Write(cmd, length);
}

void OisDevice::SendText(const char* cmd)
{
	m_port.Write(cmd, (int)strlen(cmd));
}

bool OisDevice::ExpectState(DeviceStateMask state, const char* cmd, unsigned version)
{
	if( m_protocolVersion < version )
		OIS_WARN("Did not expect command under version #%d: %s", m_protocolVersion, cmd);
	bool badState = 0 == ((1<<m_connectionState) & state);
	if( badState )
	{
		OIS_WARN("Did not expect command at this time: %s", cmd);
		if( m_connectionState == Handshaking )
		{
			ClearState();
			SendText("END\n");
		}
		return false;
	}
	return true;
}

void OisDevice::ClearState()
{
	m_connectionState = Handshaking;
	m_protocolVersion = 1;
	m_binary = false;
	m_pid = OIS_FOURCC("NULL");
	m_vid = OIS_FOURCC("OIS\0");
	m_deviceNameOverride = "";
	m_commandLength = 0;
	m_numericInputs.clear();
	m_numericOutputs.clear();
	m_queuedInputs.clear();
	m_events.clear();
	m_eventBuffer.clear();
	if( m_port.IsConnected() )
		m_port.SetBaud(9600);
}
#endif
