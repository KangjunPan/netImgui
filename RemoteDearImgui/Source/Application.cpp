//=================================================================================================
// Application.cpp
//
// Entry point of this application. 
// Initialize the app & window, and run the mainloop with messages processing
//=================================================================================================
#include "stdafx.h"
#include "../resource.h"
#include "../DirectX/DirectX11.h"
#include <RemoteImgui_Network.h>
#include "ServerNetworking.h"
#include "ServerInfoTab.h"

#define MAX_LOADSTRING		100
#define CLIENT_MAX			8
#define CMD_CLIENTFIRST_ID	1000
#define CMD_CLIENTLAST_ID	(CMD_CLIENTFIRST_ID + CLIENT_MAX - 1)

// Global Variables:
HINSTANCE					ghApplication;					// Current instance
HWND						ghMainWindow;					// Main Windows
int32_t						gActiveClient = -1;				// Currently selected remote Client for input & display
WCHAR						szTitle[MAX_LOADSTRING];		// The title bar text
WCHAR						szWindowClass[MAX_LOADSTRING];	// the main window class name
NetworkServer::ClientInfo	gClients[CLIENT_MAX];			// Table of all potentially connected clients to this server
RmtImgui::ImguiInput		gInput;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void UpdateActiveClient(int NewClientId)
{
	if( NewClientId != gActiveClient )
	{
		char zName[96];
		if( gActiveClient != -1 )
		{
			NetworkServer::ClientInfo& Client = gClients[gActiveClient];
			sprintf_s(zName, "    %i.%i.%i.%i:%i(%s)    ", Client.mConnectIP[0], Client.mConnectIP[1], Client.mConnectIP[2], Client.mConnectIP[3], Client.mConnectPort, Client.mName);
			ModifyMenuA(GetMenu(ghMainWindow), (UINT)Client.mMenuId, MF_BYCOMMAND|MF_ENABLED, Client.mMenuId, zName);
			Client.mbIsActive = false;
		}
		
		gActiveClient = NewClientId;
		if( gActiveClient != -1 )
		{
			NetworkServer::ClientInfo& Client = gClients[gActiveClient];
			sprintf_s(zName, "--[ %i.%i.%i.%i:%i(%s) ]--", Client.mConnectIP[0], Client.mConnectIP[1], Client.mConnectIP[2], Client.mConnectIP[3], Client.mConnectPort, Client.mName);
			ModifyMenuA(GetMenu(ghMainWindow), (UINT)Client.mMenuId, MF_BYCOMMAND|MF_GRAYED, Client.mMenuId, zName);
			Client.mbIsActive = true;
		}		
		DrawMenuBar(ghMainWindow);
	}
}

void AddRemoteClient(int32_t NewClientIndex)
{	
	gClients[NewClientIndex].mMenuId = CMD_CLIENTFIRST_ID + NewClientIndex;
	AppendMenuA(GetMenu(ghMainWindow), MF_STRING, gClients[NewClientIndex].mMenuId, "");
	UpdateActiveClient(NewClientIndex);
}

void RemoveRemoteClient(int32_t OldClientIndex)
{
	if( gClients[OldClientIndex].mMenuId != 0 )
	{
		RemoveMenu(GetMenu(ghMainWindow), (UINT)gClients[OldClientIndex].mMenuId, MF_BYCOMMAND);
		gClients[OldClientIndex].Reset();
		if( OldClientIndex == gActiveClient )
			UpdateActiveClient(0);
	}
}

BOOL Startup(HINSTANCE hInstance, int nCmdShow)
{
    // Initialize global strings	
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_REMOTEDEARIMGUI, szWindowClass, MAX_LOADSTRING);
    WNDCLASSEXW wcex;
    wcex.cbSize			= sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REMOTEDEARIMGUI));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_REMOTEDEARIMGUI);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	if( !RegisterClassExW(&wcex) )
		return false;
	
	ghApplication		= hInstance;
	ghMainWindow		= CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, ghApplication, nullptr);

	if (!ghMainWindow)
		return false;

	if( !dx::Startup(ghMainWindow) )
		return false;

	memset(&gInput, 0, sizeof(gInput));
	ShowWindow(ghMainWindow, nCmdShow);
	UpdateWindow(ghMainWindow);
	return true;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
		// Parse the menu selections:
        int wmId = LOWORD(wParam);
		if( wmId >= CMD_CLIENTFIRST_ID && wmId <= CMD_CLIENTLAST_ID )
		{
			UpdateActiveClient(wmId - CMD_CLIENTFIRST_ID);
		}
		else
		{
			switch (wmId)
			{
			case IDM_ABOUT:	DialogBox(ghApplication, MAKEINTRESOURCE(IDD_ABOUTBOX), ghMainWindow, About);	break;
			case IDM_EXIT:	DestroyWindow(ghMainWindow); break;
			default: return DefWindowProc(ghMainWindow, message, wParam, lParam);
			}
		}
    }
    break;
    case WM_DESTROY: PostQuitMessage(0); break;
	case WM_LBUTTONDOWN:	gInput.MouseDownMask |= RmtImgui::ImguiInput::kMouseBtn_Left; return true;
    case WM_LBUTTONUP:		gInput.MouseDownMask &= ~RmtImgui::ImguiInput::kMouseBtn_Left; return true;
    case WM_RBUTTONDOWN:	gInput.MouseDownMask |= RmtImgui::ImguiInput::kMouseBtn_Right; return true;
    case WM_RBUTTONUP:		gInput.MouseDownMask &= ~RmtImgui::ImguiInput::kMouseBtn_Right; return true;
    case WM_MBUTTONDOWN:	gInput.MouseDownMask |= RmtImgui::ImguiInput::kMouseBtn_Middle; return true;
    case WM_MBUTTONUP:		gInput.MouseDownMask &= RmtImgui::ImguiInput::kMouseBtn_Middle; return true;
    case WM_MOUSEWHEEL:		gInput.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f; return true;
    case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (wParam > 0 && wParam < 0x10000 && gInput.KeyCharCount < sizeof(gInput.KeyChars)/sizeof(gInput.KeyChars[0]) )
			gInput.KeyChars[ gInput.KeyCharCount++ ] = (unsigned short)wParam;        
        return true;
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

    // Perform application initialization:
    if (	!Startup (hInstance, nCmdShow) || 
			!NetworkServer::Startup(gClients, ARRAY_COUNT(gClients), RmtImgui::Network::kDefaultServerPort ) ||
			!ServerInfoTab_Startup(RmtImgui::Network::kDefaultServerPort) )
    {
		return FALSE;
    }

    // Main message loop:
	MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

		for(uint32_t ClientIdx(0); ClientIdx<ARRAY_COUNT(gClients); ClientIdx++)
		{
			auto& Client = gClients[ClientIdx];
			// Client : Added
			if( Client.mbConnected == true && Client.mMenuId == 0 && Client.mName[0] != 0 )
				AddRemoteClient(ClientIdx);
			// Client : Removed
			if( Client.mbConnected == false && Client.mMenuId != 0 )
				RemoveRemoteClient(ClientIdx);
		}

		gClients[gActiveClient].UpdateInput(ghMainWindow, gInput);
		ServerInfoTab_Draw();
		dx::Render( gClients[gActiveClient].mvTextures, gClients[gActiveClient].GetFrame() );
    }

	ServerInfoTab_Shutdown();
	NetworkServer::Shutdown();
	dx::Shutdown();
    return (int) msg.wParam;
}