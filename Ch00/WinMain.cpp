/**
* windows.h introduces a large amount of code
* By using the below #define constants we can reduce the amount of code brought in by windows.h
* _CRT_SECURE_NO_WARNINGS - Preprocessor directive that suppresses warning about non-secure versions of library functions
* WIN32_LEAN_AND_MEAN - preprocessor directive that excludes a bunch of less frequently used header files from windows.h thereby reducing size
* WIN32_EXTRA_LEAN - preprocessor directive that excludes even more APIs than WIN32_LEAN_AND_MEAN
* By reducing the amount of code we can reduce the build time
*/
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#include "include/glad/glad.h"
#include <Windows.h>
#include <iostream>
#include "Application.h"

// We need to forward declare these 2 functions as they are used early on
int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

#if _DEBUG
	#pragma comment(linker,"/subsystem:console")
	int main(int argc, const char** argv)
	{
		return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWDEFAULT);
	}
#else
	#pragma comment(linker,"/subsystem:Windows")
#endif
// Instead of linking OpenGL32.lib through project properties we use this #pragma comment to do it
#pragma comment(lib, "opengl32.lib")

/**
* Few OpenGL fns need to be declared but to creation of modern OpenGL context is done using
* wglCreateContextAttribsARB 
* The fn signature is in wglext.h but we don't need to import the whole file only the relavant fns we need 
*/
#define WGL_CONTEXT_MAJOR_VERSION_ARB		0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB		0x2092
#define WGL_CONEXT_FLAGS_ARB				0x2094
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB	0x00000001
#define WGL_CONTEXT_PROFILE_MASK_ARB		0x9126
// Define a function pointer for wglCreateContextAttribsARB
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);

// Instead of including wgl.h we just define the function pointer signatures required from wgl.h
// These fns are required to work with OpenGL
typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGEXTPROC)(void);
typedef BOOL (WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);
typedef int (WINAPI* PFNWGLGETSWAPINTERVALEXTPROC)(void);

// 2 global variables for easy window cleanup
Application* gApplication = 0; // Pointer to currently running Application 
GLuint gVertexArrayObject = 0; // Handle to the global OpenGL Vertex Array Object (VAO)
/**
* Note: Instead of each draw call having its own VAO, we'll use 1 to bound for the entire duration of the sample
*/

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	// Create a new instance of application & store it in the global pointer
	gApplication = new Application();

	// Window Creation
	// Standard window definition - Just make sure WndProc fn is set correctly
	WNDCLASSEX wndclass;
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wndclass.lpszMenuName = 0;
	wndclass.lpszClassName = L"Win32 Game Window";
	RegisterClassEx(&wndclass);

	/**
	* To launch the application in the center of the screen offset it's position
	* by half screen height & width
	*/
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int clientWidth = 800;
	int clientHeight = 600;
	RECT windowRect;
	SetRect(&windowRect,
		(screenWidth / 2) - (clientWidth / 2), // x1
		(screenHeight / 2) - (clientHeight / 2), // y1
		(screenWidth / 2) + (clientWidth / 2), // x2
		(screenHeight / 2) + (clientHeight / 2) // y2
	);

	/**
	* To find the dimensions of the screen the style needs to be known
	* Here we define the possible sizes of the window - minimized or maximized
	* We don't make resize possible but we can use WS_THICKFRAME to enable resizing
	*/ 
	DWORD style = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
	
	// Once we define the window, we need to adjust the size of the client window to fit our window
	AdjustWindowRectEx(&windowRect, style, FALSE, 0);
	// Then we create the actual window
	HWND hwnd = CreateWindowEx(0, wndclass.lpszClassName, L"Game Window", style,
		windowRect.left, windowRect.top, screenWidth, screenHeight, NULL, NULL,
		hInstance, szCmdLine);
	HDC hdc = GetDC(hwnd);

	// Window Creation Completed
	// OpenGL Context creation

	/**
	* First we find the right pixel format to use
	* Then we apply it to the device context of the window
	*/
	PIXELFORMATDESCRIPTOR pfd;
	/**
	* memset is a fn that copies a single character for a specific no: of times to an object
	* It's useful for filling a number of bytes with a given value starting from a specified location
	* Here sizeof(PIXELFORMATDESCRIPTOR) number of bytes are given value = 0 starting from the location pointed by pfd
	*/
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 32;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;
	int pixelFormat = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, pixelFormat, &pfd);

	/**
	* Next we create a temperory OpenGL context
	* this is used to get a pointer to wglCreateContextAttribsARB 
	* which can then be used to create a modern context
	*/
	HGLRC tempRC = wglCreateContext(hdc);
	wglMakeCurrent(hdc, tempRC);
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

	/**
	* Now we call wglCreateContextAttribARB to get the OpenGL Core Context
	* Once we get it, we bind it, delete the legacy context & make it the current context
	*/
	const int attribList[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONEXT_FLAGS_ARB, 0,
		WGL_CONTEXT_PROFILE_MASK_ARB,
		WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0
	};
	HGLRC hglrc = wglCreateContextAttribsARB(hdc, 0, attribList);
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(tempRC);
	wglMakeCurrent(hdc, hglrc);

	// OpenGL Context Creation Completed
	// Use glad to load all OpenGL Core functions
	if (!gladLoadGL()) { std::cout << "Couldn't initialize GLAD\n"; }
	else { std::cout << "OpenGL Version:" + GLVersion.major << "." << GLVersion.minor << "\n"; }

	// Enabling VSync
	/**
	* vsync is not a built-in function, it's an extension so it needs to be called using wglGetExtensionStringEXT
	* The extension string for vsync is WGL_EXT_swap_control
	* First we check if the extension string is present in the list of extension strings being used
	*/
	PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
	/*
	* strstr - finds the first occurence of a sub-string in a string
	* It returns a ptr to the first character of the sub-string found in the string
	* It returns a nullptr if the substring isn't present in the string
	*/
	bool swapControlSupported = strstr(_wglGetExtensionsStringEXT(), "WGL_EXT_swap_control") != 0;

	/**
	* If WGL_EXT_swap_control extension is available it needs to be loaded
	* The actual fn name is wglSwapIntervalEXT & is present in wgl.h passing an argument to this fn turns on vsynch
	*/
	int vsynch = 0;
	if (swapControlSupported)
	{
		PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
		PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

		if (wglSwapIntervalEXT(1))
		{
			std::cout << "Enabled vsynch\n";
			vsynch = wglGetSwapIntervalEXT();
		}
		else { std::cout << "Couldn't enable vsynch \n"; }
	}
	else { std::cout << "WGL_EXT_swap_control not enabled\n"; } // !swapControlSupported

	//Completed vsynch 
	
	/*
	* OpenGL requires a VAO to be bound for all draw calls
	* Instead of creating a VAO for each draw call, we create 1 global VAO that's bound to WinMain & is never unbound until the window is destroyed
	*/
	glGenVertexArrays(1, &gVertexArrayObject);
	glBindVertexArray(gVertexArrayObject);

	// Display the current window
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	/**
	* Initialize the global application
	* Note: Depending on the amount of work done when Initialize() is called the application might freeze for a few seconds
	*/
	gApplication->Initialize();

	// Game loop implementation
	DWORD lastTick = GetTickCount(); // Keep track of last frame to calculate delta time
	MSG msg;
	while (true)
	{
		// Process window events
		// Handle window events by peeking at the current message stack & dispatching msg accordingly
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) { break; }
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		// Update application based on the delta time
		DWORD thisTick = GetTickCount();
		float dt = (thisTick - lastTick) * 0.001f;
		lastTick = thisTick;
		if (gApplication != 0) { gApplication->Update(dt); }

		// Render Application
		if (gApplication != 0)
		{
			RECT clientRect;
			GetClientRect(hwnd, &clientRect);
			clientWidth = clientRect.right - clientRect.left;
			clientHeight = clientRect.bottom - clientRect.top;
			glViewport(0, 0, clientWidth, clientHeight);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glPointSize(5.0f);
			glBindVertexArray(gVertexArrayObject);

			// Clear color, depth & stencil buffers
			glClearColor(0.5f, 0.6f, 0.7f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			// Find aspect ratio
			float aspect = (float)clientWidth / (float)clientHeight;
			gApplication->Render(aspect);
		}

		// After application has been updated & rendered, the buffer needs to be presented
		if (gApplication != 0)
		{
			SwapBuffers(hdc);
			if (vsynch != 0) { glFinish(); } // If vsynch is enabled, glFinish() needs to be called right after buffer swap
		}
	} // End of game loop

	// Once the game loop (window loop) finishes executing it's safe to return from the WinMain fn
	if (gApplication != 0)
	{
		std::cout << "Expected application to be null on exit \n";
		delete gApplication;
	}

	return (int)msg.wParam;
}

/**
* In order to have a properly functioning window or to even compile the application,
* an event processing function WndProc needs to be defined
* The implementation here is simple & focused on destroying the window
*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	/**
	* When WM_CLOSE msg is received, shut down Application class & emit a destroy window msg
	* Once the application is shut down we need to delete it
	*/
	case WM_CLOSE:
		if (gApplication != 0)
		{
			gApplication->Shutdown();
			delete gApplication;
			gApplication = 0;
			DestroyWindow(hwnd);
		}
		else { std::cout << "Already shut down gApplication!\n"; }
		break;

	/**
	* If a destroy msg is received OpenGL resources need to be released
	* This means destroying gVertexArrayObject & deleting the OpenGL context
	*/
	case WM_DESTROY:
		if (gVertexArrayObject != 0)
		{
			HDC hdc = GetDC(hwnd);
			HGLRC hglrc = wglGetCurrentContext();

			glBindVertexArray(0);
			glDeleteVertexArrays(1, &gVertexArrayObject);
			gVertexArrayObject = 0;

			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(hglrc);
			ReleaseDC(hwnd, hdc);

			PostQuitMessage(0);
		}
		else { std::cout << "Display multiple error msgs\n"; }
		break;

	/**
	* We can ignore the paint & erase background msgs as OpenGL is managing the rendering to the window
	*/
	case WM_PAINT:
	case WM_ERASEBKGND:
		return 0;
	}

	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}