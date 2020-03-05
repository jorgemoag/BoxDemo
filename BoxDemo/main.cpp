#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "DemoApp.h"

static const LPCTSTR WindowClassName = L"DirectXTest";
static const LPCTSTR WindowTitle = L"aprendeunrealengine.com";

static const int Width = 800;
static const int Height = 600;

// Paso 1: Declarar una función que será la encargada de gestionar los eventos
LRESULT CALLBACK WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	// Paso 2: Registrar una estructura WNDCLASSEX
	WNDCLASSEX WindowClass;
	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = WndProc; // pasarle la función
	WindowClass.cbClsExtra = NULL;
	WindowClass.cbWndExtra = NULL;
	WindowClass.hInstance = hInstance;
	WindowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WindowClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	WindowClass.lpszMenuName = NULL;
	WindowClass.lpszClassName = WindowClassName;
	WindowClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// Registrar la estructura
	if (!RegisterClassEx(&WindowClass))
	{
		OutputDebugString(L"[ERROR] !RegisterClassEx(&WindowClass)");
		return 1;
	}

	// Paso 3. Crear una ventana usando la estructura anterior.
	// devuelve un handle que identifica la ventana creada.
	HWND hWnd = CreateWindowEx(
		NULL, // dwExTyle
		WindowClassName, // ClassName
		WindowTitle, // WindowTitle
		WS_OVERLAPPEDWINDOW, // dwStyle
		CW_USEDEFAULT, CW_USEDEFAULT, // (X, Y)
		Width, Height, // (Width, Height)
		NULL, // WndParent
		NULL, // Menu
		hInstance, // hInstance
		NULL // lpParam
	);

	if (!hWnd)
	{
		OutputDebugString(L"[ERROR] !CreateWindowEx");
		return 1;
	}

	// Paso 4. Mostrar la ventana
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Paso 5. Preguntar a Windows por eventos.
	// los eventos se llaman mensajes en windows.
	MSG Message; ZeroMemory(&Message, sizeof(Message));

	// DemoApp
	DemoApp App{ hWnd, Width, Height };

	// bucle infinito para preguntar por eventos
	while (true)
	{
		// conseguir el evento y eliminarlo de la cola de eventos
		if (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
		{
			if (Message.message == WM_QUIT)
			{
				break;
			}
			App.Tick();
		}
		// distribuir el evento para que podamos procesarlo en WndProc.
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_KEYDOWN:
	{
		if (wParam == VK_ESCAPE)
		{
			DestroyWindow(hWnd);
		}
		return 0;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	default:
	{
		// si no procesamos el evento, se lo pasamos al gestor por
		// defecto que trae Windows
		return DefWindowProc(hWnd, Message, wParam, lParam);
	}
	}
}