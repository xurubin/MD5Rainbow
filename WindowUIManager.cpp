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

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, 0, 500, 600, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	HDC dc = GetWindowDC(hWnd);
	GetClientRect(hWnd, &BufferRect);
	BufferDC = CreateCompatibleDC(dc);
	BufferBitmap = CreateCompatibleBitmap(dc, BufferRect.right-BufferRect.left, BufferRect.bottom-BufferRect.top);
	SelectObject(BufferDC, BufferBitmap);
	ReleaseDC(hWnd, dc);

	SetTimer(hWnd, 1, 200, NULL);

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
	CWindowUIManager* uim = (CWindowUIManager*)(&CUIManager::getSingleton());

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		//switch (wmId)
		//{
		//case IDM_ABOUT:
		//	DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
		//	break;
		//case IDM_EXIT:
		//	DestroyWindow(hWnd);
		//	break;
		//default:
		//	return DefWindowProc(hWnd, message, wParam, lParam);
		//}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		BitBlt(hdc, 0, 0, BufferRect.right-BufferRect.left, BufferRect.bottom-BufferRect.top, 
			BufferDC, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);
		break;
	case WM_TIMER:
		uim->RefreshUI(GetTickCount() - t0);
		t0 = GetTickCount();
		InvalidateRect(hWnd, &BufferRect, true);
		break;
	case WM_DESTROY:
		uim->InputBuffer.clear();
		uim->IsInputMode = false;
		PostQuitMessage(0);
		break;
	case WM_ERASEBKGND:
		break;
	case WM_KEYDOWN:
		if(uim->IsInputMode)
		{
			char c = wParam;
			int ctrl = HIBYTE(GetAsyncKeyState(VK_CONTROL));
			if ((ctrl)&&(wParam=='V')) //Ctrl+V: paste
			{
				char* p = NULL;
				if (OpenClipboard(NULL))
				{
					p = (char*)GetClipboardData(CF_TEXT);
					CloseClipboard();
				}
				if(p) uim->InputBuffer = string(p);
				return 0;
			}
		}
		break;
	case WM_CHAR:
		if(uim->IsInputMode)
		{
			char c = wParam;
			int ctrl = HIBYTE(GetAsyncKeyState(VK_CONTROL));
			switch(c)
			{
				case 0x16:
					break;
				case 0x0A:  // linefeed 
				case 0x0D:  // carriage return 
				case 0x1B:  // escape 
					if(c==0x1B) uim->InputBuffer.clear();
					uim->IsInputMode = false;
					break;
				case 0x08:  // backspace 
					if (uim->InputBuffer.size() > 0)
						uim->InputBuffer.erase(uim->InputBuffer.size()-1);
					break;
				default:
					uim->InputBuffer.push_back(c);
					break;
			}
			uim->RefreshUI(GetTickCount() - t0);
			t0 = GetTickCount();
			InvalidateRect(hWnd, &BufferRect, true);
		}
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



CWindowUIManager::CWindowUIManager()
{
	IsInputMode = false;
}
string CWindowUIManager::InputString(string prompt)
{
	InputModeCaption = prompt;
	InputBuffer="c457bc8dbdcd7f5be129d40f9d7e12d6";
	IsInputMode = true;
	while(IsInputMode) Sleep(200);
	return InputBuffer;
	//char line[128];
	//printf(prompt.c_str());
	//if (!fgets(line, sizeof(line),stdin)) return string("");
	//line[32] = '\0';
}

int CWindowUIManager::DrawTextOnWindow(int x, int y, string caption, int* width, int* height)
{
	RECT rect;
	rect.left = x;
	rect.top = y;
	DrawText(BufferDC, caption.c_str(), -1, &rect, DT_CALCRECT | DT_NOCLIP | DT_LEFT);
	DrawText(BufferDC, caption.c_str(), -1, &rect, DT_NOCLIP | DT_LEFT);

	if (width)
		*width = rect.right - rect.left;
	if(height)
		*height = rect.bottom - rect.top;
	return rect.bottom - rect.top;
}

int CWindowUIManager::DrawGroupCaption(int x, int y, string caption)
{
	RECT rect;
	rect.left = x;
	rect.top  = y - 1;
	rect.right = x+BufferRect.right - BufferRect.left;
	rect.bottom = y + 1;
	FillRect(BufferDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
	return DrawTextOnWindow(x, y+1, caption)+1;
}
int CWindowUIManager::PrintVariableLine(int x, int y, VariableCollection::iterator var, double interval)
{
	string caption = var->first+": ";
	int width = 0;
	int height1 = DrawTextOnWindow(x, y, caption, &width);

	int height2 = var->second->DrawWindow(interval, x+width, y, BufferRect.right - BufferRect.left - width, BufferDC);

	return height1 > height2 ? height1 : height2;
}

int CWindowUIManager::PrintLogLine(int x, int y, StringList::iterator log )
{
	return DrawTextOnWindow(x, y, *log);
}

void CWindowUIManager::RefreshUI( long delta ) 
{
	//char text[128];
	//sprintf(text, "Time: %ld", delta);
	FillRect(BufferDC, &BufferRect, (HBRUSH) (COLOR_WINDOW+1));
	//TextOut(BufferDC, 0, 0, text, strlen(text));
	double interval = delta /1000.0;
	int y = 5;

	char cacheinfo[255];
	sprintf(cacheinfo, "CacheSize:%d NumAccess:%d NumHits:%d", CUIManager::cache.cachequeue.size(),
			CUIManager::cache.numaccesses, CUIManager::cache.numhits);
	y+= DrawTextOnWindow(0, y, cacheinfo);
	//CUIManager::cache.Damp(GetTickCount());
	const int pt_sz = 2;
	int cache_width = BufferRect.right - BufferRect.left;
	cache_width = cache_width / 100 * 100 / pt_sz;

	Rectangle(BufferDC, 1, y, 1+cache_width*2+2, y+(CUIManager::cache.segments+cache_width-1)/cache_width*2+2);
	y++;
	for(int cy = 0; cy < (CUIManager::cache.segments+cache_width-1) / cache_width; cy++)
	{
		for(int cx = 0; cx < cache_width; cx++)
		{
			int o = cx+cy*cache_width;
			if (o >= CUIManager::cache.segments) break;
			int d = CUIManager::cache.data[o];
			SetPixel(BufferDC, 2+2*cx, y, RGB(255, 255 - d, 255 - d));
			SetPixel(BufferDC, 2+2*cx+1, y, RGB(255, 255 - d, 255 - d));
			SetPixel(BufferDC, 2+2*cx, y+1, RGB(255, 255 - d, 255 - d));
			SetPixel(BufferDC, 2+2*cx+1, y+1, RGB(255, 255 - d, 255 - d));
		}
		y+=2;
	}
	y+= 16;

	pthread_mutex_lock(&mutex);
	for(GroupCollection::iterator gi = groups.begin(); gi != groups.end();gi++)
	{
		y += 10;
		VariableCollection& variables = gi->second.variables;
		StringList& logs = gi->second.logs;
		int used_lines = 0;
		//Print Header
		y += DrawGroupCaption(0, y, gi->second.name);
		//Print variables
		for(VariableCollection::iterator var = variables.begin(); var != variables.end(); var++)
		{
			y += PrintVariableLine(0, y, var, interval);
		}
		//Print logs
		int max_loglines = group_height - 1 - variables.size();
		int logid = max_loglines - logs.size();
		for(StringList::iterator log = logs.begin(); log != logs.end(); log++)
		{
			logid++;
			if (logid <= 0) continue;
			y += PrintLogLine(0, y, log);
		}
	}
	pthread_mutex_unlock(&mutex);

	//Display InputBox
	if (IsInputMode)
	{
		RECT boundingrect;
		int width = BufferRect.right - BufferRect.left;
		int height = BufferRect.bottom - BufferRect.top;
		boundingrect.left = BufferRect.left + (int)(0.2*width);
		boundingrect.right = BufferRect.right - (int)(0.2*width);
		boundingrect.top = BufferRect.top + (int)(0.5*height);
		boundingrect.bottom = boundingrect.top + 100;

		//Draw bounding box
		FillRect(BufferDC, &boundingrect, (HBRUSH)GetStockObject(BLACK_BRUSH));
		boundingrect.left += 3;
		boundingrect.top  += 3;
		boundingrect.bottom -= 3;
		boundingrect.right  -= 3;
		FillRect(BufferDC, &boundingrect, (HBRUSH)(COLOR_WINDOW+1));

		//Draw Caption
		int y = boundingrect.top + 10;
		int x = boundingrect.left + 3;
		y += DrawTextOnWindow(x, y, InputModeCaption);
		y += DrawTextOnWindow(x, y, "Press ENTER to continue, ESC to cancel.");

		y+= 16; x+= 8;
		if (InputBuffer.size() == 0)
			y += DrawTextOnWindow(x, y, " ");
		else
			y += DrawTextOnWindow(x, y, InputBuffer);
		MoveToEx(BufferDC, x, y+1, NULL);
		LineTo(BufferDC, x+(int)(0.9*(boundingrect.right - boundingrect.left)), y+1);
	}
}



int IntVariable::DrawWindow(double interval, int x, int y, int width, HDC dc)
{
	const int BAR_HEIGHT=16;
	char text[128];
	int len = 0;
	int height = 0;
	int * value = (int*)var;
	switch (style)
	{
	case Progress:
		if((*value>=min)&&(*value<=max))
		{
			double percentage = ((double)*value - min) / (max-min);
			len = (int)((width-2) * percentage);
			RECT rect;
			rect.left = x;
			rect.top = y;
			rect.right = x+width;
			rect.bottom = y+BAR_HEIGHT;
			FillRect(BufferDC, &rect, (HBRUSH)(COLOR_ACTIVECAPTION + 1));
			rect.left = x+1+len;
			rect.top = y+1;
			rect.bottom --;
			rect.right --;
			FillRect(BufferDC, &rect, (HBRUSH)(COLOR_WINDOW + 1));
			height = BAR_HEIGHT;
		}
		break;
	case Raw:
		sprintf(text, "%d", *value);
		height = CWindowUIManager::DrawTextOnWindow(x, y, string(text));
		break;
	case Gradient:
		int newvalue = *value;
		len = sprintf(text, "%d", (int)((newvalue - oldvalue)/interval));
		oldvalue = newvalue;
		height = CWindowUIManager::DrawTextOnWindow(x, y, string(text));
		break;
	}
	return height;
}


#endif
