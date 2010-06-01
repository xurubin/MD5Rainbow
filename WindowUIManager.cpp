#define _CRT_SECURE_NO_WARNINGS
#include "UIManager.h"
#ifdef WIN32
#include "windows.h"
#include "common.h"
#include <string.h> 
#include <memory.h> 


// Global Variables:
HINSTANCE hInst;									// current instance
char szTitle[] = "MD5 Rainbow";						// The title bar text
char szWindowClass[]="MD5RainbowUIClass";			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

HDC BufferDC;
HBITMAP BufferBitmap;
RECT BufferRect;
//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, IDI_APPLICATION);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	HDC dc = GetWindowDC(hWnd);
	GetClientRect(hWnd, &BufferRect);
	BufferDC = CreateCompatibleDC(dc);
	BufferBitmap = CreateCompatibleBitmap(BufferDC, BufferRect.right-BufferRect.left, BufferRect.bottom-BufferRect.top);
	SelectObject(BufferDC, BufferBitmap);
	ReleaseDC(hWnd, dc);

	SetTimer(hWnd, 1, 500, NULL);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	long t0 = 0;
	//CWindowUIManager& uim = (CWindowUIManager)(CUIManager::getSingleton());

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		//case IDM_ABOUT:
		//	DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
		//	break;
		//case IDM_EXIT:
		//	DestroyWindow(hWnd);
		//	break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		BitBlt(hdc, 0, 0, BufferRect.right-BufferRect.left, BufferRect.bottom-BufferRect.top, 
			BufferDC, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);
		break;
	case WM_TIMER:
		//uim.RefreshUI(GetTickCount() - t0);
		t0 = GetTickCount();
		InvalidateRect(hWnd, &BufferRect, true);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_ERASEBKGND:
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


void CWindowUIManager::Internal_Handler(void)
{
	hInst = GetModuleHandle(NULL);

	// TODO: Place code here.
	MSG msg;

	// Initialize global strings
	MyRegisterClass(hInst);

	// Perform application initialization:
	if (!InitInstance (hInst, SW_SHOW))
	{
		return;
	}
	
	// Main message loop:
	while (GetMessage(&msg, NULL, 0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DeleteObject(BufferBitmap);
	DeleteDC(BufferDC);
	return;
}

void CWindowUIManager::RefreshUI( long delta ) 
{
	char text[128];
	sprintf(text, "Time: %ld", delta);
	FillRect(BufferDC, &BufferRect, (HBRUSH) (COLOR_WINDOW+1));
	TextOut(BufferDC, 0, 0, text, strlen(text));
}

#endif
