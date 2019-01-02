
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
	OIS_STRING temp;
	const char* FormatTemp(const char* fmt, ...)
	{
		va_list	v;
		va_start(v, fmt);
		const char* s = Format(temp, fmt, v);
		va_end( v );
		return s;
	}
	const char* Format(OIS_STRING& result, const char* fmt, ...)
	{
		va_list	v;
		va_start(v, fmt);
		const char* s = Format(result, fmt, v);
		va_end( v );
		return s;
	}
	const char* Format(OIS_STRING& result, const char* fmt, const va_list& v)
	{
		int length = _vscprintf( fmt, v ) + 1;
		result.resize(length);
		char* buffer = &result[0];
		vsnprintf(buffer, length, fmt, v);
		return buffer;
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
	OisDevice(unsigned id, const OIS_STRING& path, const OIS_STRING& name)
		: m_portName(path)
		, m_deviceName(name)
		, m_portId(id)
	{
		ClearState();
	}
	const char* GetDeviceName() const { return m_deviceNameOverride.empty() ? m_deviceName.c_str() : m_deviceNameOverride.c_str(); }
	uint32_t    GetProductID()  const { return m_pid; }
	uint32_t    GetVendorID()   const { return m_vid; }
	unsigned    GetPortId()     const { return m_portId; }
	bool        Connecting()    const { return m_connectionState != Handshaking; }
	bool        Connected()     const { return m_connectionState == Active; }

	void Poll();
	
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
		int         channel;
		OIS_STRING  name;
		NumericType type;
		Value       value;
	};
	
	const OIS_VECTOR<NumericValue>& DeviceInputs()  { return m_numericInputs; }
	const OIS_VECTOR<NumericValue>& DeviceOutputs() { return m_numericOutputs; }

	void SetInput(const NumericValue& input, Value value);
private:
	enum ClientCommands
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
		RNI = OIS_FOURCC("RNI="),
		RNO = OIS_FOURCC("RNO="),
		RCM = OIS_FOURCC("RCM="),
		PID = OIS_FOURCC("PID="),
		END = OIS_FOURCC("END\0"),
	};
	enum DeviceState
	{
		Handshaking,
		Synchronisation,
		Active,
	};
	typedef uint32_t DeviceStateMask;
	
	static char* ZeroDelimiter(char* str, char delimiter);
	void Send(const char* cmd);
	void SendLn(const char* cmd);
	bool ExpectState(DeviceStateMask state, const char* cmd, unsigned version);
	void ClearState();
	bool Process(char* cmd);

	OIS_PORT m_port;
	OIS_STRING m_portName;
	OIS_STRING m_deviceName;

	OIS_STRING m_deviceNameOverride;
	unsigned m_portId;
	unsigned m_protocolVersion;
	uint32_t m_pid;
	uint32_t m_vid;
	DeviceState m_connectionState;
	char m_commandBuffer[OIS_MAX_COMMAND_LENGTH*2];
	unsigned m_commandLength;
	
	struct Event
	{
		int channel;
		OIS_STRING name;
	};
	OIS_VECTOR<NumericValue> m_numericInputs;
	OIS_VECTOR<NumericValue> m_numericOutputs;
	OIS_VECTOR<int>          m_queuedInputs;
	OIS_VECTOR<Event>        m_events;
	OIS_VECTOR<int>          m_eventBuffer;

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
void OisDevice::Poll()
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
		for( char* c = start; c != end; ++c )
		{
			if( *c != '\n' )
				continue;
			*c = '\0';
			Process(start);
			start = c+1;
		}
		OIS_ASSERT( start <= end );

		if( start == m_commandBuffer && end == m_commandBuffer+OIS_ARRAYSIZE(m_commandBuffer) )
		{
			OIS_WARN("OisDevice command buffer is full without a \\n present! Discarding...");
			m_commandBuffer[OIS_ARRAYSIZE(m_commandBuffer)-1] = '\0';
			Process(m_commandBuffer);
			m_commandLength = 0;
		}
		else
		{
			m_commandLength = (unsigned)(end-start);
			if( start != m_commandBuffer )
				memmove(m_commandBuffer, start, m_commandLength);
		}
	}
	OIS_STRING_BUILDER sb;
	for( int index : m_queuedInputs )
	{
		const NumericValue& v = m_numericInputs[index];
		switch( v.type )
		{
			default: OIS_ASSERT( false );
			case Boolean:
				Send(sb.FormatTemp("%d=%d\n", v.channel, v.value.boolean?1:0));
				OIS_INFO( "-> %d(%s) = %s", v.channel, v.name.c_str(), v.value.boolean ? "true" : "false" );
				break;
			case Number:
				Send(sb.FormatTemp("%d=%d\n", v.channel, v.value.number));
				OIS_INFO( "-> %d(%s) = %d", v.channel, v.name.c_str(), v.value.number );
				break;
			case Fraction:
				Send(sb.FormatTemp("%d=%d\n", v.channel, (int)(v.value.fraction*100.0f)));
				OIS_INFO( "-> %d(%s) = %.2f", v.channel, v.name.c_str(), v.value.fraction );
				break;
		}
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

bool OisDevice::Process(char* cmd)
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
				int version = atoi(payload);
				OIS_INFO( "<- SYN: %d", version );
				if( version >= 1 && version <= 2 )
				{
					switch( version )
					{
					default: OIS_ASSUME(false);
					case 1: SendLn("ACK");        break;
					case 2: SendLn("ACK=1,22RS"); break;
					}
					OIS_INFO( "-> ACK", version );
					m_protocolVersion = version;
					m_connectionState = Synchronisation;
				}
				else
				{
					OIS_INFO( "-> DEN", version );
					SendLn("DEN");
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
				int channel = atoi(ZeroDelimiter(payload, ','));
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
				if( output )
				{
					m_numericOutputs.push_back({channel, OIS_STRING(name), nt});
					m_numericOutputs.back().value.number = 0;
				}
				else
				{
					m_numericInputs.push_back({channel, OIS_STRING(name), nt});
					m_numericInputs.back().value.number = 0xcdcdcdcd;
				}
				OIS_INFO( "<- %s: %d %s", cmd, channel, name );
				break;
			}
			case RNI:
			case RNO:
			case RCM:
			{
				ExpectState( (1<<Synchronisation) | (1<<Active), cmd, 2 );
				int channel = atoi(payload);
				auto fn = [this, cmd, channel](auto& vec, const char* type)
				{
					auto* e = FindChannel(vec, channel);
					OIS_INFO( "<- %s %d (%s)", type, channel, e?e->name.c_str():"UNKNOWN CHANNEL" );
					if( e )
					{
						std::swap(*e, vec.back());
						vec.pop_back();
					}
				};
				switch( type )
				{
				case RNI: fn(m_numericInputs, "RNI"); break;
				case RNO: fn(m_numericOutputs, "RNO"); break;
				case RCM: fn(m_events, "RCM"); break;
				}
				break;
			}
			case ACT:
			{
				ExpectState( Synchronisation, cmd, 1 );
				m_connectionState = Active;
				OIS_INFO( "<- ACT" );
				break;
			}

			case EXC:
			{
				ExpectState( Active, cmd, 1 );
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
void OisDevice::Send(const char* cmd)
{
	m_port.Write(cmd, (int)strlen(cmd));
}
void OisDevice::SendLn(const char* cmd)
{
	Send(cmd);
	m_port.Write("\n", 1);
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
			Send("END\n");
		}
		return false;
	}
	return true;
}
void OisDevice::ClearState()
{
	m_connectionState = Handshaking;
	m_protocolVersion = 1;
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
