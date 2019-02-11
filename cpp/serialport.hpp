#ifndef OIS_SERIALPORT_INCLUDED
#define OIS_SERIALPORT_INCLUDED

struct PortName
{
	uint32_t id;
	OIS_STRING path;
	OIS_STRING name;
};
typedef OIS_VECTOR<PortName> OIS_PORT_LIST;

class SerialPort
{
public:
	SerialPort();
	SerialPort(const char* portName) : SerialPort() { Connect(portName); }
	~SerialPort() { Disconnect(); }

	static void EnumerateSerialPorts(OIS_PORT_LIST& results, OIS_STRING_BUILDER&, int minPort=-1);
	
	bool IsConnected();
	void Connect();
	void Connect(const char* portName);
	void Disconnect();
	const OIS_STRING& PortName() const { return m_portName; }

	void SetBaud(int, bool purge=true);
	int  GetBaud() const { return m_baud; }

	void PurgeReadBuffer();
	int  Read(char* buffer, int size);
	int  Write(const char* buffer, int size);
private:
	SerialPort( const SerialPort& );
	SerialPort& operator=( const SerialPort& );

	void* m_handle;
	int m_baud = 0;
	OIS_STRING m_portName;
};


#ifdef OIS_SERIALPORT_IMPL
#ifdef WIN32
#include <windows.h>
#include <lmerr.h>

static HLOCAL GetWindowsErrorBuffer( HRESULT dwErrorMsgId )
{
	DWORD ret = 0;      // Temp space to hold a return value.
	HINSTANCE hInst;    // Instance handle for DLL.
	HLOCAL pBuffer = 0; // Buffer to hold the textual error description.
	if( HRESULT_FACILITY(dwErrorMsgId) == FACILITY_MSMQ )
	{ // MSMQ errors only (see winerror.h for facility info).
		// Load the MSMQ library containing the error message strings.
		hInst = LoadLibrary( "MQUTIL.DLL" );
		if(hInst != 0)
		{ // hInst not NULL if the library was successfully loaded.
			// Get the text string for a message definition
			ret = FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | // Function will handle memory allocation.
				FORMAT_MESSAGE_FROM_HMODULE | // Using a module's message table.
				FORMAT_MESSAGE_IGNORE_INSERTS, 
				hInst, // Handle to the DLL.
				dwErrorMsgId, // Message identifier.
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language.
				(LPTSTR)&pBuffer, // Buffer that will hold the text string.
				0, // Allocate at least this many chars for pBuffer.
				NULL // No insert values.
				);
		} // hInst not NULL if the library was successfully loaded.
	} // MSMQ errors only.
	else if( dwErrorMsgId >= NERR_BASE && dwErrorMsgId <= MAX_NERR )
	{ // Could be a network error.
		// Load the library containing network messages.
		hInst = LoadLibrary( TEXT("NETMSG.DLL") );
		if(hInst != 0)
		{ // Not NULL if successfully loaded.
			// Get a text string for the message definition.
			ret = FormatMessage(  
				FORMAT_MESSAGE_ALLOCATE_BUFFER | // The function will allocate memory for the message.
				FORMAT_MESSAGE_FROM_HMODULE | // Message definition is in a module.
				FORMAT_MESSAGE_IGNORE_INSERTS,  // No inserts used.
				hInst, // Handle to the module containing the definition.
				dwErrorMsgId, // Message identifier.
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language.
				(LPTSTR)&pBuffer, // Buffer to hold the text string.
				0, // Smallest size that will be allocated for pBuffer.
				NULL // No inserts.
				);
		} // Not NULL if successfully loaded.
	} // Could be a network error.
	else
	{ // Unknown message source.
		// Get the message string from the system.
		ret = FormatMessage(  
			FORMAT_MESSAGE_ALLOCATE_BUFFER | // The function will allocate space for pBuffer.
			FORMAT_MESSAGE_FROM_SYSTEM | // System wide message.
			FORMAT_MESSAGE_IGNORE_INSERTS, // No inserts.
			NULL, // Message is not in a module.
			dwErrorMsgId, // Message identifier.
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language.
			(LPTSTR)&pBuffer, // Buffer to hold the text string.
			0, // The function will allocate at least this much for pBuffer.
			NULL // No inserts.
			);
	}
	return pBuffer;
}
static void FreeWindowsErrorBuffer( HLOCAL pBuffer )
{
	if( pBuffer )
		LocalFree( pBuffer );
}

#include <setupapi.h>
#include <winioctl.h>
#include <atlbase.h>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "advapi32.lib")

void SerialPort::EnumerateSerialPorts(OIS_PORT_LIST& results, OIS_STRING_BUILDER& sb, int minPort)
{
	HDEVINFO hDeviceInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if( INVALID_HANDLE_VALUE == hDeviceInfo )
		return;

	SP_DEVINFO_DATA info = { sizeof(SP_DEVINFO_DATA) };
	BOOL ok = TRUE;
	for( DWORD index = 0; ok = SetupDiEnumDeviceInfo(hDeviceInfo, index, &info); ++index )
	{
		ATL::CRegKey deviceKey;
		deviceKey.Attach(SetupDiOpenDevRegKey(hDeviceInfo, &info, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE));
		if( INVALID_HANDLE_VALUE == deviceKey )
			continue;
		unsigned port = 0; // get port number
		{
			char* portName;
			{
				ULONG length = 0;
				if( ERROR_SUCCESS != deviceKey.QueryStringValue("PortName", nullptr, &length) )
					continue;
				++length;//for the terminator
				portName = sb.AllocTemp(length);
				DWORD regType = 0;
				if( ERROR_SUCCESS != RegQueryValueEx(deviceKey, "PortName", nullptr, &regType, (BYTE*)portName, &length) )
					continue;
				if( (regType != REG_SZ) && (regType != REG_EXPAND_SZ) )
					continue;
				portName[length-1] = '\0';
			}
			const DWORD length = (DWORD)strlen(portName);
			if( length <= 3 )//can't be COM#
				continue;
			if( _strnicmp(portName, "COM", 3) != 0 )
				continue;
			bool bNumeric = true;
			for( DWORD i=3; i<length && bNumeric; ++i )
				bNumeric = isdigit(portName[i]) != 0;
			if( !bNumeric )
				continue;
			port = (unsigned)atoi(portName + 3);
			if( (int)port < minPort )
				continue;
		}
		OIS_STRING name; //get friendly name
		{
			DWORD regType = 0, length = 0;
			if( !SetupDiGetDeviceRegistryProperty(hDeviceInfo, &info, SPDRP_DEVICEDESC, &regType, nullptr, 0, &length) )
			{
				if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
					continue;
			}
			char* buffer = sb.AllocTemp(length);
			if( !SetupDiGetDeviceRegistryProperty(hDeviceInfo, &info, SPDRP_DEVICEDESC, &regType, (BYTE*)buffer, length, &length) )
				continue;
			if( regType != REG_SZ )
				continue;
			sb.StoreTemp(name, buffer);
		}
		OIS_STRING path;
		sb.Format(path, "\\\\.\\COM%d", port);
		results.push_back({port, path, name});
	}

	SetupDiDestroyDeviceInfoList(hDeviceInfo);
}


SerialPort::SerialPort() 
	: m_handle(INVALID_HANDLE_VALUE)
{}

void SerialPort::Connect(const char* portName)
{
	m_portName = portName;
	Connect();
}

void SerialPort::Connect()
{
	const char* portName = m_portName.c_str();
	Disconnect();
	m_handle = CreateFileA(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if( m_handle != INVALID_HANDLE_VALUE )
		SetBaud(9600);
	else
	{
#if OIS_ENABLE_ERROR_LOGGING
		HLOCAL hError = GetWindowsErrorBuffer( GetLastError() );
		const char* error = static_cast<LPCSTR>((LPTSTR)hError);
		OIS_WARN("ERROR opening serial port %s : %s", portName, error);
		FreeWindowsErrorBuffer( hError );
#else
		OIS_WARN("ERROR opening serial port %s", portName);
#endif
	}
}

void SerialPort::SetBaud(int baud, bool purge)
{
	DCB serialParameters = {};
	if( !GetCommState(m_handle, &serialParameters) )
	{
		OIS_WARN("failed to get current serial parameters");
		return Disconnect();
	}

	m_baud = baud;
	switch(baud)
	{
	case 110:    baud = CBR_110;	break;
	case 300:    baud = CBR_300;	break;
	case 600:    baud = CBR_600;	break;
	case 1200:   baud = CBR_1200;	break;
	case 2400:   baud = CBR_2400;	break;
	case 4800:   baud = CBR_4800;	break;
	default: m_baud = 9600;
	case 9600:   baud = CBR_9600;	break;
	case 14400:  baud = CBR_14400;	break;
	case 19200:  baud = CBR_19200;	break;
	case 38400:  baud = CBR_38400;	break;
	case 56000:  baud = CBR_56000;	break;
	case 57600:  baud = CBR_57600;	break;
	case 115200: baud = CBR_115200;	break;
	case 128000: baud = CBR_128000;	break;
	case 256000: baud = CBR_256000; break;        
	}

	if( serialParameters.BaudRate == baud && !purge )
		return;
		
	serialParameters.BaudRate    = baud;
	serialParameters.ByteSize    = 8;
	serialParameters.fDtrControl = DTR_CONTROL_ENABLE;
	serialParameters.StopBits    = ONESTOPBIT;
	serialParameters.Parity      = NOPARITY;
		
	// Set COM port timeout settings
	COMMTIMEOUTS timeouts = {};
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	if(SetCommTimeouts(m_handle, &timeouts) == 0)
	{
		OIS_WARN("Error setting timeouts");
		return Disconnect();
	}

	if( !SetCommState(m_handle, &serialParameters) )
	{
		OIS_WARN("could not set Serial port parameters");
		return Disconnect();
	}
	else if( purge )
		PurgeComm(m_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

bool SerialPort::IsConnected()
{
	bool connected = m_handle != INVALID_HANDLE_VALUE;
	if( connected && !ClearCommError(m_handle, 0, 0) )
	{
		Disconnect();
		return false;
	}
	else
		return connected;
}

void SerialPort::Disconnect()
{
	if( m_handle != INVALID_HANDLE_VALUE )
	{
		CloseHandle(m_handle);
		m_handle = INVALID_HANDLE_VALUE;
	}
}

void SerialPort::PurgeReadBuffer()
{
	if( m_handle != INVALID_HANDLE_VALUE )
		PurgeComm(m_handle, PURGE_RXCLEAR);
}

int SerialPort::Read(char* buffer, int bufferSize)
{
	if( m_handle == INVALID_HANDLE_VALUE || bufferSize <= 0 )
		return 0;

	COMSTAT status = {};
	if( !ClearCommError(m_handle, 0, &status) )
	{
		Disconnect();
		return 0;
	}

	DWORD toRead = status.cbInQue < (DWORD)bufferSize ? status.cbInQue : (DWORD)bufferSize;
	DWORD bytesRead;
	if( ReadFile(m_handle, buffer, toRead, &bytesRead, NULL) )
		return bytesRead;
	return 0;
}

int SerialPort::Write(const char* buffer, int bufferSize)
{
	if( m_handle == INVALID_HANDLE_VALUE || bufferSize <= 0 )
		return false;
	
	COMSTAT status = {};
	if( !ClearCommError(m_handle, 0, &status) )
	{
		Disconnect();
		return 0;
	}

	DWORD bytesSent;
	if( !WriteFile(m_handle, (void*)buffer, bufferSize, &bytesSent, 0) )
	{
		if( !ClearCommError(m_handle, 0, 0) )
			Disconnect();
		return -1;
	}
	else
	{
		return (int)bytesSent;
	}
}


#else//WIN32
#error Requires porting to this platform...
#endif
#endif//OIS_SERIALPORT_IMPL
#endif // OIS_SERIALPORT_INCLUDED

