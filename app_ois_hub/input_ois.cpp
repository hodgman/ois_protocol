
void OisWebbyLog( const char* fmt, ... );
#define OIS_WEBBY_INFO( fmt, ... ) OisWebbyLog(fmt, __VA_ARGS__);

#define OIS_PROTOCOL_IMPL
#include "input_ois.h"
#include <algorithm>

#include "../cpp/ois_webby.h"


class OisSerialConnection
{
public:
	OisSerialConnection(const char* portName, const OIS_STRING& name, unsigned gameVersion, const char* gameName)
		: m_port(portName)
		, m_device(m_port, name, gameVersion, gameName)
	{
	}
	OisPortSerial m_port;
	OisDevice m_device;
};

class OisSerialConnectionList
{
public:
	OisSerialConnectionList() {}
	~OisSerialConnectionList() { Clear(); }
	void Clear()
	{
		for( auto* d : devices )
			delete d;
		devices.clear();
	}
	
	void New(const char* portPath, const OIS_STRING& portName)
	{
		devices.push_back(new OisSerialConnection(portPath, portName, GAME_VERSION, GAME_NAME));
	}
	
	auto Find(const OisDevice& d)
	{
		return std::find_if(devices.begin(), devices.end(), [&d](OisSerialConnection* item)
		{
			return &item->m_device == &d;
		});
	}

	bool Delete(const OisDevice& d)
	{
		auto it = Find(d);
		if( it == devices.end() )
			return false;
		delete *it;
		std::swap( *it, devices.back() );
		devices.pop_back();
		return true;
	}

	auto begin() { return devices.begin(); }
	auto end() { return devices.end(); }
private:
	OisSerialConnectionList(const OisSerialConnectionList&);
	OisSerialConnectionList& operator=(const OisSerialConnectionList&);
	
	std::vector<OisSerialConnection*> devices;
};

class OisAllConnectionList
{
public:
	std::vector<OisDeviceEx> devices;

	OisDeviceEx& Find(IOisPort* p, OisDevice* d)
	{
		auto it = std::find_if(devices.begin(), devices.end(), [d](const OisDeviceEx& item)
		{
			return item.device == d;
		});
		if( it == devices.end() )
		{
			OisDeviceEx e;
			e.port = p;
			e.device = d;
			e.updateCount = updateCount;
			devices.push_back(e);
			return devices.back();
		}
		else
		{
			it->updateCount = updateCount;
			return *it;
		}
	}
	void GarbageCollect()
	{
		for( size_t i=0; i!=devices.size(); )
		{
			if( devices[i].updateCount < updateCount )
			{
				std::swap(devices[i], devices.back());
				devices.pop_back();
			}
			else
				++i;
		}
		++updateCount;
	}
private:
	int updateCount = 0;
};

static OIS_STRING_BUILDER sb;
static OisAllConnectionList    g_allDevices;
static OisSerialConnectionList g_serialConnections;
static OisWebHost*             g_websockets = nullptr;
AppGlobals g;

static OisWebWhitelist g_webFiles[] =
{
	{ "/ois_protocol.js", "../javascript/ois_protocol.js" },
	{ "/example/",        "../javascript/example/index.html" },
	{ "/example/",        "../javascript/", true },
};

void InputOis_Init()
{
	delete g_websockets;
	g_websockets = new OisWebHost(GAME_VERSION, GAME_NAME, g_webFiles, true, 8080);
}

static void UpdateDevice( OisDeviceEx& d )
{
	d.device->Poll(sb);
	d.newEvents.clear();
	d.device->PopEvents([&](const OisState::Event& event)
	{
		d.eventLog.push_back(event.name.c_str());
		d.newEvents.push_back(&event);
	});
}

void InputOis_Update( std::vector<OisDeviceEx*>& devices, float deltaTime )
{
	if( !g_websockets )
		return;
	
	for( auto* c : g_serialConnections )
	{
		OisDeviceEx& d = g_allDevices.Find( &c->m_port, &c->m_device );
		UpdateDevice( d );
	}

	g_websockets->Poll();
	for( auto* c : g_websockets->Connections() )
	{
		OisDeviceEx& d = g_allDevices.Find( &c->m_port, &c->m_device );
		UpdateDevice( d );
	}

	g_allDevices.GarbageCollect();
	
	for( auto& d : g_allDevices.devices )
		devices.push_back(&d);
}

void InputOis_Shutdown()
{
	delete g_websockets;
	g_websockets = 0;
	g_serialConnections.Clear();
	g_allDevices.GarbageCollect();
}

void InputOis_Connect(const PortName& portName)
{
	g_serialConnections.New(portName.path.c_str(), portName.name);
}

void InputOis_Disconnect(const OisDevice& d)
{
	if( !g_serialConnections.Delete(d) )
	{
		g_websockets->Disconnect(d);
	}
}



OisHostEx g_out;

void OutputOis_Connect(const PortName& name)
{
	delete g_out.device;
	delete g_out.port;

	g_out.port = new OisPortSerial(name.path.c_str());
	g_out.device = new OisHost(*g_out.port, name.name, GAME_PID, GAME_VID);

	for( auto& d : g_allDevices.devices )
	{
		for( auto& e : d.device->DeviceEvents() )
			g_out.device->AddEvent(e.name);

		for( auto& o : d.device->DeviceOutputs() )
			g_out.device->AddOutput(o.name, o.type);
	}
}

void OutputOis_Update(OisHostEx*& device, float deltaTime)
{
	if( !g_out.device )
	{
		device = 0;
		return;
	}
	device = &g_out;

	OIS_STRING_BUILDER sb;
	g_out.device->Poll(sb, deltaTime);
}



void OisLog(const char* category, const char* fmt, ...)
{
	std::string str;
	va_list	v;
	va_start(v, fmt);
	sb.FormatV(str, fmt, v);
	va_end( v );
	g.oisLog.push_back(str);
}

void OisWebbyLog(const char* fmt, ...)
{
	std::string str;
	va_list	v;
	va_start(v, fmt);
	sb.FormatV(str, fmt, v);
	va_end( v );
	g.webbyLog.push_back(str);
}
