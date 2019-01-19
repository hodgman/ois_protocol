

#define OIS_DEVICE_IMPL
#define OIS_SERIALPORT_IMPL
#include "input_ois.h"

#include "../websocket_host_cpp/ois_webby.h"

AppGlobals g;

void OisLog(const char* category, const char* fmt, ...)
{
	std::string str;
	va_list	v;
	va_start(v, fmt);
	g.sb.FormatV(str, fmt, v);
	va_end( v );
	g.log.push_back(str);
}
