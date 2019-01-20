

#define OIS_DEVICE_IMPL
#define OIS_SERIALPORT_IMPL
#include "input_ois.h"

#include "../websocket_host_cpp/ois_webby.h"

AppGlobals g;

OisWebsocketHost* g_websockets = nullptr;

void InputOis_Init()
{
	delete g_websockets;
	g_websockets = new OisWebsocketHost(GAME_VERSION, GAME_NAME, 8080);
}

static void UpdateDevice( OisDevice& device, std::vector<std::pair<OisDevice*, std::vector<const OisDevice::Event*>>>& devices )
{
	devices.push_back({&device, {}});
	std::vector<const OisDevice::Event*>& eventsThisLoop = devices.back().second;
	device.Poll(g.sb);
	device.PopEvents([&](const OisDevice::Event& event)
	{
		g.eventLog.push_back(event.name);
		eventsThisLoop.push_back(&event);
	});
}

void InputOis_Update( std::vector<std::pair<OisDevice*, std::vector<const OisDevice::Event*>>>& devices )
{
	if( !g_websockets )
		return;

	if( g.device )
		UpdateDevice( *g.device, devices );

	g_websockets->Poll();
	auto& connections = g_websockets->Connections();
	for( auto* c : connections )
		UpdateDevice( c->m_device, devices );
}

void InputOis_Shutdown()
{
	delete g_websockets;
	g_websockets = 0;
}

void InputOis_Connect(const PortName& portName)
{
	delete g.device;
	delete g.port;
	g.port = new OisPortSerial(portName.path.c_str());
	g.device = new OisDevice(*g.port, portName.name, GAME_VERSION, GAME_NAME);
}
void InputOis_Disconnect(const OisDevice&)
{
	delete g.device;
	g.device = nullptr;
}

void OisLog(const char* category, const char* fmt, ...)
{
	std::string str;
	va_list	v;
	va_start(v, fmt);
	g.sb.FormatV(str, fmt, v);
	va_end( v );
	g.log.push_back(str);
}
