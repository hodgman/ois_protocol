

#define OIS_DEVICE_IMPL
#define OIS_SERIALPORT_IMPL
#include "input_ois.h"

#include "../websocket_host_cpp/ois_webby.h"

AppGlobals g;

OisWebsocketHost* g_websockets;

void InputOis_Init()
{
	g_websockets = new OisWebsocketHost(GAME_VERSION, GAME_NAME, 8080);
}

void InputOis_Update( OIS_VECTOR<OisDevice*>& devices )
{
	if( !g_websockets )
		return;

	if( g.device )
		devices.push_back(g.device);

	g_websockets->Poll();
	auto& connections = g_websockets->Connections();
	for( auto* c : connections )
	{
		devices.push_back( &c->m_device );
	}
}

void InputOis_Shutdown()
{
	delete g_websockets;
	g_websockets = 0;
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
