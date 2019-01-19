#include <stdint.h>
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "input_ois.h"

void RunDemoGui();

bool VJoy_Init(char* out_error, int errorSize);
void VJoy_Shutdown();
void VJoy_Pause();
void VJoy_Update( int numAxes, float* axes, int numButtons, bool* buttons );
 
static void PrintError( const TCHAR* what, DWORD dwLastError )
{
	TCHAR   lpBuffer[256] = _T( "?" );
	if( dwLastError != 0 )
	{
		::FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,                 // It´s a system error
							NULL,                                      // No string to be formatted needed
							dwLastError,                               // Hey Windows: Please explain this error!
							MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Do it in the standard language
							lpBuffer,              // Put the message here
							256-1,                     // Number of bytes to store the message
							NULL );
		_tprintf(TEXT("%s : %s\n"), what, lpBuffer );
	}
}

#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
int main( int argc, char *argv[] )
{
	RunDemoGui();
	return 0; 
}

#define WINDOW_WIDTH 564*2
#define WINDOW_HEIGHT 1152

#define NK_INCLUDE_FIXED_TYPES
//#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_GDI_IMPLEMENTATION
#include "nuklear/nuklear.h"
#include "nuklear/nuklear_gdi.h"

void DoLogGui(struct nk_context* ctx)
{
    nk_layout_row_dynamic(ctx, 30, 1);
	if( nk_button_label(ctx, "Clear Log") )
	{
		g.log.clear();
	}
	nk_layout_row_dynamic(ctx, 200, 1);
	if( nk_group_begin(ctx, "Log", 0) )
	{
		nk_layout_row_dynamic(ctx, 30, 1);
		for( auto it = g.log.begin(); it != g.log.end(); ++it )
			nk_label(ctx, it->c_str(), NK_TEXT_LEFT);
		nk_group_end(ctx);
	}
}

void DoConnectingGui(struct nk_context* ctx)
{
    nk_layout_row_dynamic(ctx, 30, 1);
	
	if( nk_button_label(ctx, "Scan COM ports") || g.firstFrame )
	{
		g.firstFrame = false;
		g.portList.clear();
		SerialPort::EnumerateSerialPorts(g.portList, g.sb, -1);
	}

	if( g.portList.empty() )
		nk_label(ctx, "No ports scanned...", NK_TEXT_LEFT);
	else
	{
		for( auto it = g.portList.begin(); it != g.portList.end(); ++it )
		{
			std::string label = it->name + '(' + it->path + ')';
			if( nk_button_label(ctx, label.c_str()) )
			{
				delete g.device;
				delete g.port;
				g.port = new OisPortSerial(it->path.c_str());
				g.device = new OisDevice(*g.port, it->name, GAME_VERSION, GAME_NAME);
			}
		}
	}
}

void DoOisGui(struct nk_context* ctx)
{
	if( !g.device )
		return;
    const float ratio2[] = {0.4f, 0.6f};
	
    nk_layout_row_dynamic(ctx, 30, 1);
	if( nk_button_label(ctx, "Disconnect") )
	{
		delete g.device;
		g.device = nullptr;
		return;
	}
    nk_layout_row(ctx, NK_DYNAMIC, 30, 2, ratio2);
	nk_label(ctx, "Name", NK_TEXT_LEFT);
	nk_label(ctx, g.device->GetDeviceName(), NK_TEXT_LEFT);
	nk_label(ctx, "PID", NK_TEXT_LEFT);
	nk_labelf(ctx, NK_TEXT_LEFT, "0x%8X", g.device->GetProductID());
	nk_label(ctx, "VID", NK_TEXT_LEFT);
	nk_labelf(ctx, NK_TEXT_LEFT, "0x%8X", g.device->GetVendorID());
	nk_label(ctx, "Port", NK_TEXT_LEFT);
	nk_labelf(ctx, NK_TEXT_LEFT, "%s", g.port->PortName().c_str());
	nk_label(ctx, "State", NK_TEXT_LEFT);
	if( g.device->Connected() )
		nk_label(ctx, "Active", NK_TEXT_LEFT);
	else if( g.device->Connecting() )
		nk_label(ctx, "Sync", NK_TEXT_LEFT);
	else
		nk_label(ctx, "Handshake", NK_TEXT_LEFT);
	
    nk_layout_row_dynamic(ctx, 30, 1);
	nk_label(ctx, "Numeric Inputs", NK_TEXT_LEFT);
	
	if (nk_tree_push(ctx, NK_TREE_TAB, "Events", NK_MAXIMIZED))
	{
		if( nk_button_label(ctx, "Clear Event Log") )
		{
			g.eventLog.clear();
		}
		nk_layout_row_dynamic(ctx, 200, 1);
		if( nk_group_begin(ctx, "Event Log", 0) )
		{
			nk_layout_row_dynamic(ctx, 30, 1);
			for( auto it = g.eventLog.begin(); it != g.eventLog.end(); ++it )
				nk_label(ctx, it->c_str(), NK_TEXT_LEFT);
			nk_group_end(ctx);
		}
		nk_tree_pop(ctx);
	}

	if (nk_tree_push(ctx, NK_TREE_TAB, "Inputs", NK_MAXIMIZED))
	{
		auto& inputs = g.device->DeviceInputs();
		for( auto it = inputs.begin(); it != inputs.end(); ++it )
		{
			std::string name = it->name + " : ch" + std::to_string(it->channel);
			if( !it->active )
				name = name + " (Inactive)";
			if (nk_tree_push_id(ctx, NK_TREE_TAB, name.c_str(), NK_MAXIMIZED, it->channel))
			{
				nk_layout_row(ctx, NK_DYNAMIC, 30, 2, ratio2);
				nk_label(ctx, "Value", NK_TEXT_LEFT);
				OisDevice::Value newValue;
				bool set = false;
				switch( it->type )
				{
					case OisDevice::Boolean:
					{
						int boolean = it->value.boolean ? 1 : 0;
						nk_checkbox_label(ctx, name.c_str(), &boolean);
						set = (!!boolean) != it->value.boolean;
						newValue.boolean = !!boolean;
						break;
					}
					case OisDevice::Number:
					{
						int number = it->value.number;
						nk_property_int(ctx, name.c_str(), SHRT_MIN, &number, SHRT_MAX, 1, 1);
						set = number != it->value.number;
						newValue.number = number;
						break;
					}
					case OisDevice::Fraction: 
					{
						float fraction = it->value.fraction;
						nk_property_float(ctx, name.c_str(), SHRT_MIN/100.0f, &fraction, SHRT_MAX/100.0f, 1, 1);
						set = fraction != it->value.fraction;
						newValue.fraction = fraction;
						break;
					}
				}
				if( set )
					g.device->SetInput(*it, newValue);
				nk_tree_pop(ctx);
			}
		}
		nk_tree_pop(ctx);
	}
	
	if (nk_tree_push(ctx, NK_TREE_TAB, "Outputs", NK_MAXIMIZED))
	{
		auto& outputs = g.device->DeviceOutputs();
		for( auto it = outputs.begin(); it != outputs.end(); ++it )
		{
			std::string name = it->name + " : ch" + std::to_string(it->channel);
			if( !it->active )
				name = name + " (Inactive)";
			if (nk_tree_push_id(ctx, NK_TREE_TAB, name.c_str(), NK_MAXIMIZED, it->channel))
			{
				nk_layout_row(ctx, NK_DYNAMIC, 30, 2, ratio2);
				nk_label(ctx, "Value", NK_TEXT_LEFT);
				switch( it->type )
				{
				case OisDevice::Boolean:  nk_label(ctx, it->value.boolean ? "True" : "False", NK_TEXT_LEFT); break;
				case OisDevice::Number:	  nk_labelf(ctx, NK_TEXT_LEFT, "%d", it->value.number); break;
				case OisDevice::Fraction: nk_labelf(ctx, NK_TEXT_LEFT, "%f", it->value.fraction); break;
				}
				nk_tree_pop(ctx);
			}
		}
		nk_tree_pop(ctx);
	}
}

static bool s_enableVJoyOutput = false;
static char s_vjoyError[1024] = {'\0'};
void DoVJoyGui(struct nk_context* ctx)
{
    nk_layout_row_dynamic(ctx, 30, 1);
	if( !s_enableVJoyOutput )
	{
		if( nk_button_label(ctx, "Enable vJoy output") )
			s_enableVJoyOutput = VJoy_Init(s_vjoyError, 1024);
		if( s_vjoyError[0] != '\0' )
			nk_labelf(ctx, NK_TEXT_LEFT, "Error: %s", s_vjoyError);
		return;
	}
	if( nk_button_label(ctx, "Disable vJoy output") )
	{
		s_enableVJoyOutput = false;
		VJoy_Shutdown();
		return;
	}

	if( g.numAxes )
		nk_label(ctx, "Axes", NK_TEXT_ALIGN_LEFT);
	for( int i=0; i!=g.numAxes; ++i )
		nk_slider_float(ctx, -1, &g.axisValues[i], 1, 0.1f);

	if( g.numButtons )
		nk_label(ctx, "Buttons", NK_TEXT_ALIGN_LEFT);
    nk_layout_row_dynamic(ctx, 30, 5);
	for( int i=0; i!=g.numButtons; ++i )
	{
		char label[32];
		sprintf(label, "Btn%d", i);
		nk_check_label(ctx, label, g.buttonValues[i]);
	}
}

void DoVJoyUpdate()
{
	if( !g.device || !g.device->Connected() )
	{
		VJoy_Pause();
		return;
	}

	const auto& events = g.device->DeviceEvents();
	const auto& outputs = g.device->DeviceOutputs();

	int numButtons = events.size();
	int numAxes = 0;

	//map events onto buttons:
	for( int i=0; i!=numButtons; ++i )
		g.buttonValues[i] = false;
	for( auto it = g.eventsThisLoop.begin(); it != g.eventsThisLoop.end(); ++it )
	{
		const OisDevice::Event* event = *it;
		int index = (int)(ptrdiff_t)(event - &events.front());
		if( index >= 0 && index < numButtons )
			g.buttonValues[index] = true;
	}

	//numeric output bools to buttons, others to axes
	for( auto it = outputs.begin(); it != outputs.end(); ++it )
	{
		if( it->type == OisDevice::Boolean )
			g.buttonValues[numButtons++] = it->value.boolean;
		else
		{
			if( it->type == OisDevice::Number )
				g.axisValues[numAxes++] = it->value.number / (float)MAXSHORT;
			else
				g.axisValues[numAxes++] = it->value.fraction / (MAXSHORT/100.0f);
		}
	}
	g.numButtons = numButtons;
	g.numAxes = numAxes;

	VJoy_Update(numAxes, g.axisValues, numButtons, g.buttonValues);
}

void MainLoop(struct nk_context* ctx)
{
	g.eventsThisLoop.clear();
	if( g.device )
	{
		g.device->Poll(g.sb);
		g.device->PopEvents([](const OisDevice::Event& event)
		{
			g.eventLog.push_back(event.name);
			g.eventsThisLoop.push_back(&event);
		});
	}

	if (nk_begin(ctx, "OIS", nk_rect(0, 0, (float)gdi.width/2-2, (float)gdi.height), 0))
	{
		if (nk_tree_push(ctx, NK_TREE_TAB, "Ois Log", NK_MAXIMIZED))
		{
			DoLogGui(ctx);
			nk_tree_pop(ctx);
		}
		if( g.device )
		{
			if (nk_tree_push(ctx, NK_TREE_TAB, "Ois Input", NK_MAXIMIZED))
			{
				DoOisGui(ctx);
				nk_tree_pop(ctx);
			}
		}
		else
		{
			DoConnectingGui(ctx);
		}
	}
	nk_end(ctx);
	if (nk_begin(ctx, "VJOY", nk_rect((float)gdi.width/2+2, 0, (float)gdi.width/2-2, (float)gdi.height), 0))
	{
		DoVJoyGui(ctx);
	}
	nk_end(ctx);

	DoVJoyUpdate();
}

static LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    if (nk_gdi_handle_event(wnd, msg, wparam, lparam))
        return 0;

    return DefWindowProcW(wnd, msg, wparam, lparam);
}

void RunDemoGui()
{
    GdiFont* font;
    struct nk_context *ctx;

    WNDCLASSW wc;
    ATOM atom;
    RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = WS_EX_APPWINDOW;
    HWND wnd;
    HDC dc;
    int running = 1;

    /* Win32 */
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(0);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"Ois2vJoyWindowClass";
    atom = RegisterClassW(&wc);

    AdjustWindowRectEx(&rect, style, FALSE, exstyle);
    wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"Ois2vJoy Demo",
        style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, wc.hInstance, NULL);
    dc = GetDC(wnd);

    font = nk_gdifont_create("Arial", 24);
    ctx = nk_gdi_init(font, dc, WINDOW_WIDTH, WINDOW_HEIGHT);

    while (running)
    {
        /* Input */
        MSG msg;
        nk_input_begin(ctx);

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                running = 0;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        nk_input_end(ctx);
		
		MainLoop(ctx);

        nk_gdi_render(nk_rgb(30,30,30));

		Sleep(1);
    }

    nk_gdifont_del(font);
    ReleaseDC(wnd, dc);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
}