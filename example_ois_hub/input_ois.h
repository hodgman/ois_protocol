
void OisLog( const char* category, const char* fmt, ... );

#define OIS_INFO( fmt, ... ) OisLog("INFO", fmt, __VA_ARGS__);
#define OIS_WARN( fmt, ... ) OisLog("WARN", fmt, __VA_ARGS__);
#define OIS_ASSERT( condition ) if(!(condition)){OisLog("ASSERTION", "%s(%d) : %s", __FILE__, __LINE__, #condition);}
#define OIS_ENABLE_ERROR_LOGGING 1
#define OIS_VIRTUAL_PORT

#include "../serial_host_cpp/oisdevice.h"


struct AppGlobals
{
	OisPortSerial* port = nullptr;
	OisDevice* device = nullptr;
	OIS_PORT_LIST portList;
	bool firstFrame = true;
	
	OIS_STRING_BUILDER sb;
	std::vector<std::string> log;

	std::vector<std::string> eventLog;
	std::vector<const OisDevice::Event*> eventsThisLoop;
	
	float axisValues[8] = {};
	bool buttonValues[128] = {};
	int numButtons = 0;
	int numAxes = 0;
};

extern AppGlobals g;