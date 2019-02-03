
#define GAME_VERSION 1
#define GAME_NAME    "OisHub"

void OisLog( const char* category, const char* fmt, ... );

#define OIS_INFO( fmt, ... ) OisLog("INFO", fmt, __VA_ARGS__);
#define OIS_WARN( fmt, ... ) OisLog("WARN", fmt, __VA_ARGS__);
#define OIS_ASSERT( condition ) if(!(condition)){OisLog("ASSERTION", "%s(%d) : %s", __FILE__, __LINE__, #condition);}
#define OIS_ENABLE_ERROR_LOGGING 1
#define OIS_ENABLE_VIRTUAL_PORT

#include "../cpp/oisdevice.h"

struct OisDeviceEx
{
	IOisPort* port = nullptr;
	OisDevice* device = nullptr;
	std::vector<const char*> eventLog;
	std::vector<const OisState::Event*> newEvents;
	int updateCount = 0;
};

void InputOis_Init();
void InputOis_Update( std::vector<OisDeviceEx*>& devices );
void InputOis_Shutdown();

void InputOis_Connect(const PortName&);
void InputOis_Disconnect(const OisDevice&);



struct AppGlobals
{
	std::vector<std::string> oisLog;
	std::vector<std::string> webbyLog;
};

extern AppGlobals g;
