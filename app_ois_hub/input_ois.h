
#define GAME_VERSION 1
#define GAME_NAME    "OisHub"
#define GAME_PID 0
#define GAME_VID 0

void OisLog( const char* category, const char* fmt, ... );

#define OIS_INFO( fmt, ... ) OisLog("INFO", fmt, __VA_ARGS__);
#define OIS_WARN( fmt, ... ) OisLog("WARN", fmt, __VA_ARGS__);
#define OIS_ASSERT( condition ) if(!(condition)){OisLog("ASSERTION", "%s(%d) : %s", __FILE__, __LINE__, #condition);}
#define OIS_ENABLE_ERROR_LOGGING 1
#define OIS_ENABLE_VIRTUAL_PORT

#include "../cpp/ois_protocol.h"

struct OisDeviceEx
{
	IOisPort* port = nullptr;
	OisDevice* device = nullptr;
	std::vector<const char*> eventLog;
	std::vector<const OisState::Event*> newEvents;
	int updateCount = 0;
};

void InputOis_Init();
void InputOis_Update( std::vector<OisDeviceEx*>& devices, float deltaTime );
void InputOis_Shutdown();

void InputOis_Connect(const PortName&);
void InputOis_Disconnect(const OisDevice&);


struct OisHostEx
{
	IOisPort* port = nullptr;
	OisHost* device = nullptr;
	std::vector<const char*> eventLog;
	std::vector<const OisState::Event*> newEvents;
	int updateCount = 0;
};
void OutputOis_Connect(const PortName&);
void OutputOis_Update( OisHostEx*& device, float deltaTime );



struct AppGlobals
{
	std::vector<std::string> oisLog;
	std::vector<std::string> webbyLog;
};

extern AppGlobals g;
