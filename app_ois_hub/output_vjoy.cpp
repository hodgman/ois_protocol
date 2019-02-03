
#include <windows.h> 
#include <stdio.h> 
#include "vjoy/public.h"
#include "vjoy/vjoyinterface.h"

template<class T> T Clamp( T x, T min, T max ) { return x < min ? min : (x > max ? max : x); }

void ChangedCB(int Removed, int First, void* data)
{
//	HWND hDlg = (HWND)data;
//	PostMessage(hDlg, WM_VJOYCHANGED, (WPARAM)Removed, (LPARAM)First);
}

static bool m_isEnabled = false;
static int m_iInterface = -1;

bool VJoy_Init(char* out_error, int errorSize)
{
	if( m_isEnabled && vJoyEnabled() )
		return true;

	m_isEnabled = !!vJoyEnabled();
	printf( "VJoy is %s\n", m_isEnabled ?"enabled":"NOT enabled");

	if( !m_isEnabled )
	{
		sprintf_s(out_error, errorSize, "vJoy is NOT enabled");
		return false;
	}

//	printf("Version: %d\n Vendor: %S\nProduct :%S\nVersion Number:%S\n", GetvJoyVersion(), GetvJoyManufacturerString(),  GetvJoyProductString(),  GetvJoySerialNumberString());

	WORD VerDll, VerDrv;
	if (!DriverMatch(&VerDll, &VerDrv))
	{
		sprintf_s(out_error, errorSize, "Failed\r\nvJoy Driver (version %04x) does not match vJoyInterface DLL (version %04x)\n", VerDrv, VerDll);
		printf("%s", out_error);
		return false;
	}
	else
		printf("OK - vJoy Driver and vJoyInterface DLL match. vJoyInterface DLL (version %04x)\n", VerDll);

	BOOL ffbsupp;
	int maxcnt;
	int enabledcnt;

	vJoyFfbCap(&ffbsupp);	// Is this version of vJoy capable of FFB?
	GetvJoyMaxDevices(&maxcnt);	// What is the maximum possible number of vJoy devices
	GetNumberExistingVJD(&enabledcnt);	// What is the number of vJoy devices currently enabled
	printf("VJoy is FFB capable:%s\n maxcnt:%d enabled:%d\n", m_isEnabled ? "enabled" : "NOT enabled", maxcnt, enabledcnt);

	//TODO: determine correct joyID.

	m_iInterface = 1;
	VjdStat status = GetVJDStatus(m_iInterface);
	switch (status) {
	case VJD_STAT_OWN:
		printf("vJoy Device %d is already owned by this feeder", m_iInterface);
		break;
	case VJD_STAT_FREE:
		printf("vJoy Device %d is free", m_iInterface);
		break;
	case VJD_STAT_BUSY:
		sprintf_s(out_error, errorSize, "vJoy Device %d is already owned by another feeder\nCannot continue", m_iInterface);
		printf("%s", out_error);
		return false;
	case VJD_STAT_MISS:
		sprintf_s(out_error, errorSize, "vJoy Device %d is not installed or disabled\nCannot continue", m_iInterface);
		printf("%s", out_error);
		return false;
	default:
		sprintf_s(out_error, errorSize, "vJoy Device %d general error\nCannot continue", m_iInterface);
		printf("%s", out_error);
		return false;
	};

	// Check which axes are supported
	BOOL AxisX  = GetVJDAxisExist(m_iInterface, HID_USAGE_X);
	BOOL AxisY  = GetVJDAxisExist(m_iInterface, HID_USAGE_Y);
	// Get the number of buttons supported by this vJoy device
	int nButtons  = GetVJDButtonNumber(m_iInterface);
	// Print results
	printf("\nvJoy Device %d capabilities", m_iInterface);
	printf("Button count\t\t%d", nButtons);
	printf("Axis X\t\t%s", AxisX?"Yes":"No");
	printf("Axis Y\t\t%s", AxisX?"Yes":"No");

	// Acquire the target
	if ((status == VJD_STAT_OWN) || ((status == VJD_STAT_FREE) && (! AcquireVJD(m_iInterface))))
	{
		sprintf_s(out_error, errorSize, "Failed to acquire vJoy device number %d.\n", m_iInterface);
		printf("%s", out_error);
		return false;
	} 
	else
	{
		if( errorSize )
			out_error[0] = '\0';
		printf("Acquired: vJoy device number %d.\n", m_iInterface);
		return true;
	}
}

void VJoy_Shutdown()
{
	if( m_isEnabled )
		RelinquishVJD(m_iInterface);
	m_isEnabled = false;
}

static float g_axes[8] = {0,0,0,0,0,0,0,0};

void VJoy_Update(int numAxes, float* axes, int numButtons, bool* buttons)
{
	if( !m_isEnabled )
		return;

	// complete mode
	JOYSTICK_POSITION_V2 iReport = {}; // The structure that holds the full position data
	iReport.bDevice = (BYTE)m_iInterface;

	for( int i=0; i<min(8, numAxes); ++i )
		g_axes[i] = axes[i];

	//convert -1 <- x <- 1  to 0 <- x<- maxvalue
	if( numAxes > 0 ) iReport.wAxisX    = (LONG)Clamp((axes[0] * 0.5f + 0.5f) * (0x8000-1)+1, 1.0f, (float)0x8000);
	if( numAxes > 1 ) iReport.wAxisY    = (LONG)Clamp((axes[1] * 0.5f + 0.5f) * (0x8000-1)+1, 1.0f, (float)0x8000);
	if( numAxes > 2 ) iReport.wAxisZ    = (LONG)Clamp((axes[2] * 0.5f + 0.5f) * (0x8000-1)+1, 1.0f, (float)0x8000);
	if( numAxes > 3 ) iReport.wAxisXRot = (LONG)Clamp((axes[3] * 0.5f + 0.5f) * (0x8000-1)+1, 1.0f, (float)0x8000);
	if( numAxes > 4 ) iReport.wAxisYRot = (LONG)Clamp((axes[4] * 0.5f + 0.5f) * (0x8000-1)+1, 1.0f, (float)0x8000);
	if( numAxes > 5 ) iReport.wAxisZRot = (LONG)Clamp((axes[5] * 0.5f + 0.5f) * (0x8000-1)+1, 1.0f, (float)0x8000);
	if( numAxes > 6 ) iReport.wSlider   = (LONG)Clamp((axes[6] * 0.5f + 0.5f) * (0x8000-1)+1, 1.0f, (float)0x8000);
	if( numAxes > 7 ) iReport.wDial     = (LONG)Clamp((axes[7] * 0.5f + 0.5f) * (0x8000-1)+1, 1.0f, (float)0x8000);

	int buttonIndex = 0;
	iReport.lButtons = 0;
	iReport.lButtonsEx1 = 0;
	iReport.lButtonsEx2 = 0;
	iReport.lButtonsEx3 = 0;
	for( ULONG mask = 1; buttonIndex < 32 && buttonIndex < numButtons; ++buttonIndex, mask<<=1 )
	{
		if( buttons[buttonIndex] )
			iReport.lButtons |= mask;
	}
	for( ULONG mask = 1; buttonIndex < 64 && buttonIndex < numButtons; ++buttonIndex, mask<<=1 )
	{
		if( buttons[buttonIndex] )
			iReport.lButtonsEx1 |= mask;
	}
	for( ULONG mask = 1; buttonIndex < 96 && buttonIndex < numButtons; ++buttonIndex, mask<<=1 )
	{
		if( buttons[buttonIndex] )
			iReport.lButtonsEx2 |= mask;
	}
	for( ULONG mask = 1; buttonIndex < 128 && buttonIndex < numButtons; ++buttonIndex, mask<<=1 )
	{
		if( buttons[buttonIndex] )
			iReport.lButtonsEx3 |= mask;
	}

	UpdateVJD(m_iInterface, (void*)&iReport);
}

void VJoy_Pause()
{
	if( !m_isEnabled )
		return;
	
	for( int i=0; i<6; ++i )
		g_axes[i] *= 0.95f;
	
	VJoy_Update(8, g_axes, 0, 0);
}
