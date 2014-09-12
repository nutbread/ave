#include "Graphics.hpp"
#include <cassert>



// Graphics
//{
PFNGLGETSTRINGIPROC glGetStringi = NULL;

PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffers = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffers = NULL;
PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebuffer = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatus = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameteriv = NULL;
PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmap = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2D = NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbuffer = NULL;

PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffers = NULL;
PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffers = NULL;
PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbuffer = NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorage = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameteriv = NULL;
PFNGLISRENDERBUFFEREXTPROC glIsRenderbuffer = NULL;

PFNGLGENBUFFERSPROC glGenBuffers = NULL;
PFNGLBINDBUFFERPROC glBindBuffer = NULL;
PFNGLBUFFERDATAPROC glBufferData = NULL;
PFNGLBUFFERSUBDATAPROC glBufferSubData = NULL;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = NULL;
PFNGLGETBUFFERPARAMETERIVPROC glGetBufferParameteriv = NULL;
PFNGLMAPBUFFERPROC glMapBuffer = NULL;
PFNGLUNMAPBUFFERPROC glUnmapBuffer = NULL;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = NULL;

PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsString = NULL;

PFNGLACTIVETEXTUREPROC glActiveTexture = NULL;

PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;
PFNGLUSEPROGRAMPROC glUseProgram = NULL;
PFNGLATTACHSHADERPROC glAttachShader = NULL;
PFNGLDETACHSHADERPROC glDetachShader = NULL;
PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = NULL;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
PFNGLUNIFORM1IPROC glUniform1i = NULL;
PFNGLUNIFORM2IPROC glUniform2i = NULL;
PFNGLUNIFORM3IPROC glUniform3i = NULL;
PFNGLUNIFORM4IPROC glUniform4i = NULL;
PFNGLUNIFORM1IVPROC glUniform1iv = NULL;
PFNGLUNIFORM2IVPROC glUniform2iv = NULL;
PFNGLUNIFORM3IVPROC glUniform3iv = NULL;
PFNGLUNIFORM4IVPROC glUniform4iv = NULL;
PFNGLUNIFORM1UIPROC glUniform1ui = NULL;
PFNGLUNIFORM2UIPROC glUniform2ui = NULL;
PFNGLUNIFORM3UIPROC glUniform3ui = NULL;
PFNGLUNIFORM4UIPROC glUniform4ui = NULL;
PFNGLUNIFORM1UIVPROC glUniform1uiv = NULL;
PFNGLUNIFORM2UIVPROC glUniform2uiv = NULL;
PFNGLUNIFORM3UIVPROC glUniform3uiv = NULL;
PFNGLUNIFORM4UIVPROC glUniform4uiv = NULL;
PFNGLUNIFORM1FPROC glUniform1f = NULL;
PFNGLUNIFORM2FPROC glUniform2f = NULL;
PFNGLUNIFORM3FPROC glUniform3f = NULL;
PFNGLUNIFORM4FPROC glUniform4f = NULL;
PFNGLUNIFORM1FVPROC glUniform1fv = NULL;
PFNGLUNIFORM2FVPROC glUniform2fv = NULL;
PFNGLUNIFORM3FVPROC glUniform3fv = NULL;
PFNGLUNIFORM4FVPROC glUniform4fv = NULL;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = NULL;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = NULL;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = NULL;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = NULL;
PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation = NULL;
PFNGLCREATESHADERPROC glCreateShader = NULL;
PFNGLDELETESHADERPROC glDeleteShader = NULL;
PFNGLSHADERSOURCEPROC glShaderSource = NULL;
PFNGLCOMPILESHADERPROC glCompileShader = NULL;
PFNGLGETSHADERIVPROC glGetShaderiv = NULL;
PFNGLGENSAMPLERSPROC glGenSamplers = NULL;
PFNGLDELETESAMPLERSPROC glDeleteSamplers = NULL;
PFNGLSAMPLERPARAMETERIPROC glSamplerParameteri = NULL;
PFNGLGETSAMPLERPARAMETERIVPROC glGetSamplerParameteriv = NULL;
PFNGLBINDSAMPLERPROC glBindSampler = NULL;

PFNGLFENCESYNCPROC glFenceSync = NULL;
PFNGLWAITSYNCPROC glWaitSync = NULL;
PFNGLDELETESYNCPROC glDeleteSync = NULL;
//}

bool Graphics::functionPointersBound = false;
bool Graphics::alreadyInit = false;
size_t Graphics::activeGraphicsCount = 0;
Graphics::Thread* Graphics::thread = NULL;



// Thread
Graphics::Thread :: Thread() :
	graphicsInstance(NULL),
	threadHandle(INVALID_HANDLE_VALUE),
	threadId(0),
	shouldClose(false),
	requestSemaphore(INVALID_HANDLE_VALUE),
	functionQueueModificationLock(),
	functionQueue(NULL),
	functionQueueLast(NULL)
{
	// Init function queue
	this->functionQueueLast = &this->functionQueue;

	// Critical section
	InitializeCriticalSection(&this->functionQueueModificationLock);

	// Create semaphore
	this->requestSemaphore = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);

	// Create a new thread
	this->threadHandle = CreateThread(NULL, 0, &Graphics::Thread::executeThread, static_cast<void*>(this), 0, &this->threadId);

	// Init
	this->executeAction(Graphics::Thread::initFunction, this, 0);
}
Graphics::Thread :: ~Thread() {
	// Exit thread
	this->executeAction(NULL, NULL, FunctionQueue::FLAGS_QUIT);
	WaitForSingleObject(this->threadHandle, INFINITE);
	CloseHandle(this->threadHandle);
	CloseHandle(this->requestSemaphore);
	DeleteCriticalSection(&this->functionQueueModificationLock);
}
int Graphics::Thread :: initFunction(void* data) {
	// Init function
	Graphics::Thread* self = static_cast<Graphics::Thread*>(data);
	self->graphicsInstance = Graphics::create();

	// Done
	return 0;
}
DWORD __stdcall Graphics::Thread :: executeThread(void* data) {
	Graphics::Thread* self = static_cast<Graphics::Thread*>(data);
	bool loop = true;
	while (loop) {
		// Wait for a signal
		WaitForSingleObject(self->requestSemaphore, INFINITE);


		// Get the object
		EnterCriticalSection(&self->functionQueueModificationLock);
		assert(self->functionQueue != NULL);
		// Get current
		Graphics::Thread::FunctionQueue* queueItem = self->functionQueue;
		// Remove from list
		self->functionQueue = queueItem->functionQueueNext;
		if (self->functionQueue == NULL) {
			// List is now empty
			self->functionQueueLast = &self->functionQueue;
		}
		LeaveCriticalSection(&self->functionQueueModificationLock);


		// Execute
		if ((queueItem->flags & FunctionQueue::FLAGS_QUIT) != 0) {
			// Thread exit
			loop = false;
		}
		else {
			// Execute function
			queueItem->returnValue = (*queueItem->function)(queueItem->data);
		}


		// Done; deletion occurs in the "executeAction" function
		queueItem->functionQueueNext = NULL;
		ReleaseSemaphore(queueItem->completionLock, 1, NULL);
	}

	// Delete
	delete self->graphicsInstance;
	self->graphicsInstance = NULL;

	// Done
	return 0;
}
int Graphics::Thread :: executeAction(Graphics::Function function, void* data, unsigned int flags) {
	// Create
	Graphics::Thread::FunctionQueue* queueItem = new Graphics::Thread::FunctionQueue();
	queueItem->function = function;
	queueItem->data = data;
	queueItem->flags = flags;
	queueItem->returnValue = 0;
	queueItem->completionLock = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
	queueItem->functionQueueNext = NULL;

	// Add
	EnterCriticalSection(&this->functionQueueModificationLock);
	(*this->functionQueueLast) = queueItem;
	this->functionQueueLast = &queueItem->functionQueueNext;
	LeaveCriticalSection(&this->functionQueueModificationLock);

	// Signal an extra is ready
	ReleaseSemaphore(this->requestSemaphore, 1, NULL);

	// Wait for response
	WaitForSingleObject(queueItem->completionLock, INFINITE);

	// Delete
	CloseHandle(queueItem->completionLock);
	int returnValue = queueItem->returnValue;
	delete queueItem;

	// Done
	return returnValue;
}
bool Graphics::Thread :: setupOkay() const {
	return (this->graphicsInstance != NULL);
}



// Graphics
Graphics :: Graphics() :
	hInstance(NULL),
	windowClass(),
	hWindow(NULL),
	hDeviceContext(NULL),
	hRenderContext(NULL),
	registered(false),
	threadId(0),
	referenceCount(0)
{
	this->threadId = GetCurrentThreadId();
}
Graphics :: ~Graphics() {
	// Unregister window class
	if (this->registered) {
		if (this->hRenderContext != NULL) {
			wglDeleteContext(this->hRenderContext);
		}

		if (this->hDeviceContext != NULL) {
			ReleaseDC(this->hWindow, this->hDeviceContext);
		}

		if (this->hWindow != NULL) {
			DestroyWindow(this->hWindow);
		}

		UnregisterClass(this->windowClass.lpszClassName, this->hInstance);
	}
}

void Graphics :: bindFunctionPointers() {
	// Already done?
	if (Graphics::functionPointersBound) return;
	Graphics::functionPointersBound = true;


	// String get
	glGetStringi = (PFNGLGETSTRINGIPROC)wglGetProcAddress("glGetStringi");


	// Framebuffer Object supported
	(
		(glGenFramebuffers = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT")) != NULL &&
		(glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT")) != NULL &&
		(glBindFramebuffer = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT")) != NULL &&
		(glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT")) != NULL &&
		(glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)wglGetProcAddress("glGetFramebufferAttachmentParameterivEXT")) != NULL &&
		(glGenerateMipmap = (PFNGLGENERATEMIPMAPEXTPROC)wglGetProcAddress("glGenerateMipmapEXT")) != NULL &&
		(glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT")) != NULL &&
		(glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT")) != NULL &&
		(glGenRenderbuffers = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT")) != NULL &&
		(glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSEXTPROC)wglGetProcAddress("glDeleteRenderbuffersEXT")) != NULL &&
		(glBindRenderbuffer = (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT")) != NULL &&
		(glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glRenderbufferStorageEXT")) != NULL &&
		(glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)wglGetProcAddress("glGetRenderbufferParameterivEXT")) != NULL &&
		(glIsRenderbuffer = (PFNGLISRENDERBUFFEREXTPROC)wglGetProcAddress("glIsRenderbufferEXT")) != NULL
	);

	// Vertex buffers
	(
		(glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers")) != NULL &&
		(glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer")) != NULL &&
		(glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData")) != NULL &&
		(glBufferSubData = (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData")) != NULL &&
		(glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers")) != NULL &&
		(glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)wglGetProcAddress("glGetBufferParameteriv")) != NULL &&
		(glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer")) != NULL &&
		(glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer")) != NULL &&

		(glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray")) != NULL &&
		(glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays")) != NULL &&
		(glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)wglGetProcAddress("glDeleteVertexArrays")) != NULL
	);

	// Multitexturing
	(
		(glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture")) != NULL
	);

	// Shaders
	(
		((glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram")) != NULL) &&
		((glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram")) != NULL) &&
		((glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram")) != NULL) &&
		((glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader")) != NULL) &&
		((glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader")) != NULL) &&
		((glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram")) != NULL) &&
		((glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv")) != NULL) &&
		((glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog")) != NULL) &&
		((glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation")) != NULL) &&
		((glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i")) != NULL) &&
		((glUniform2i = (PFNGLUNIFORM2IPROC)wglGetProcAddress("glUniform2i")) != NULL) &&
		((glUniform3i = (PFNGLUNIFORM3IPROC)wglGetProcAddress("glUniform3i")) != NULL) &&
		((glUniform4i = (PFNGLUNIFORM4IPROC)wglGetProcAddress("glUniform4i")) != NULL) &&
		((glUniform1iv = (PFNGLUNIFORM1IVPROC)wglGetProcAddress("glUniform1iv")) != NULL) &&
		((glUniform2iv = (PFNGLUNIFORM2IVPROC)wglGetProcAddress("glUniform2iv")) != NULL) &&
		((glUniform3iv = (PFNGLUNIFORM3IVPROC)wglGetProcAddress("glUniform3iv")) != NULL) &&
		((glUniform4iv = (PFNGLUNIFORM4IVPROC)wglGetProcAddress("glUniform4iv")) != NULL) &&
		((glUniform1ui = (PFNGLUNIFORM1UIPROC)wglGetProcAddress("glUniform1ui")) != NULL) &&
		((glUniform2ui = (PFNGLUNIFORM2UIPROC)wglGetProcAddress("glUniform2ui")) != NULL) &&
		((glUniform3ui = (PFNGLUNIFORM3UIPROC)wglGetProcAddress("glUniform3ui")) != NULL) &&
		((glUniform4ui = (PFNGLUNIFORM4UIPROC)wglGetProcAddress("glUniform4ui")) != NULL) &&
		((glUniform1uiv = (PFNGLUNIFORM1UIVPROC)wglGetProcAddress("glUniform1uiv")) != NULL) &&
		((glUniform2uiv = (PFNGLUNIFORM2UIVPROC)wglGetProcAddress("glUniform2uiv")) != NULL) &&
		((glUniform3uiv = (PFNGLUNIFORM3UIVPROC)wglGetProcAddress("glUniform3uiv")) != NULL) &&
		((glUniform4uiv = (PFNGLUNIFORM4UIVPROC)wglGetProcAddress("glUniform4uiv")) != NULL) &&
		((glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f")) != NULL) &&
		((glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f")) != NULL) &&
		((glUniform3f = (PFNGLUNIFORM3FPROC)wglGetProcAddress("glUniform3f")) != NULL) &&
		((glUniform4f = (PFNGLUNIFORM4FPROC)wglGetProcAddress("glUniform4f")) != NULL) &&
		((glUniform1fv = (PFNGLUNIFORM1FVPROC)wglGetProcAddress("glUniform1fv")) != NULL) &&
		((glUniform2fv = (PFNGLUNIFORM2FVPROC)wglGetProcAddress("glUniform2fv")) != NULL) &&
		((glUniform3fv = (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv")) != NULL) &&
		((glUniform4fv = (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv")) != NULL) &&
		((glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv")) != NULL) &&
		((glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation")) != NULL) &&

		((glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer")) != NULL) &&
		((glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray")) != NULL) &&
		((glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray")) != NULL) &&
		((glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)wglGetProcAddress("glBindAttribLocation")) != NULL) &&
		((glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog")) != NULL) &&
		((glBindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC)wglGetProcAddress("glBindFragDataLocation")) != NULL) &&

		((glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader")) != NULL) &&
		((glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader")) != NULL) &&
		((glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource")) != NULL) &&
		((glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader")) != NULL) &&
		((glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv")) != NULL)
	);

	// Sampler
	(
		((glGenSamplers = (PFNGLGENSAMPLERSPROC)wglGetProcAddress("glGenSamplers")) != NULL) &&
		((glDeleteSamplers = (PFNGLDELETESAMPLERSPROC)wglGetProcAddress("glDeleteSamplers")) != NULL) &&
		((glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)wglGetProcAddress("glSamplerParameteri")) != NULL) &&
		((glGetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIVPROC)wglGetProcAddress("glGetSamplerParameteriv")) != NULL) &&
		((glBindSampler = (PFNGLBINDSAMPLERPROC)wglGetProcAddress("glBindSampler")) != NULL)
	);

	// Sync
	(
		((glFenceSync = (PFNGLFENCESYNCPROC)wglGetProcAddress("glFenceSync")) != NULL) &&
		((glWaitSync = (PFNGLWAITSYNCPROC)wglGetProcAddress("glWaitSync")) != NULL) &&
		((glDeleteSync = (PFNGLDELETESYNCPROC)wglGetProcAddress("glDeleteSync")) != NULL)
	);
}

LRESULT CALLBACK Graphics :: graphicsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// Default
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

Graphics* Graphics :: create() {
	// Get window instance
	HINSTANCE hInstance = GetModuleHandle(NULL);
	if (hInstance == NULL) {
		// Failure
		return NULL;
	}


	// Setup the window class
	Graphics* g = new Graphics();
	g->hInstance = hInstance;

	g->windowClass.cbSize			= sizeof(WNDCLASSEX);
	g->windowClass.style			= (CS_HREDRAW | CS_VREDRAW | CS_OWNDC);
	g->windowClass.lpfnWndProc		= static_cast<WNDPROC>(Graphics::graphicsWindowProc);
	g->windowClass.cbClsExtra		= 0;
	g->windowClass.cbWndExtra		= 0;
	g->windowClass.hInstance		= g->hInstance;
	g->windowClass.hIcon			= NULL;
	g->windowClass.hCursor			= NULL;
	g->windowClass.hbrBackground	= NULL;
	g->windowClass.lpszMenuName		= NULL;
	g->windowClass.lpszClassName	= "AVExtensions";
	g->windowClass.hIconSm			= NULL;

	// Register
	if (RegisterClassEx(&g->windowClass) == 0) {
		// Failure
		delete g;
		return NULL;
	}
	g->registered = true;

	// Create
	DWORD windowStyle = WS_OVERLAPPEDWINDOW;
	DWORD windowStyleExtended = WS_EX_APPWINDOW;
	g->hWindow = CreateWindowEx(
		windowStyleExtended,
		g->windowClass.lpszClassName,
		NULL,
		windowStyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		HWND_DESKTOP,
		NULL,
		g->hInstance,
		NULL
	);
	if (g->hWindow == NULL) {
		// Failure
		delete g;
		return NULL;
	}



	// Device context
	g->hDeviceContext = GetDC(g->hWindow);
	if (g->hDeviceContext == NULL) {
		delete g;
		return NULL;
	}


	// Pixel format
	PIXELFORMATDESCRIPTOR pfd =											// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),									// Size Of This Pixel Format Descriptor
		1,																// Version Number
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,		// Must Support Window, OpenGL, and Double Buffering
		PFD_TYPE_RGBA,													// Request An RGBA Format
		32,																// Select Our Color Depth
		0, 0, 0, 0, 0, 0,												// Color Bits Ignored
		0,																// No Alpha Buffer
		0,																// Shift Bit Ignored
		0,																// No Accumulation Buffer
		0, 0, 0, 0,														// Accumulation Bits Ignored
		24,																// 16Bit Z-Buffer (Depth Buffer)
		0,																// No Stencil Buffer
		0,																// No Auxiliary Buffer
		PFD_MAIN_PLANE,													// Main Drawing Layer
		0,																// Reserved
		0, 0, 0															// Layer Masks Ignored
	};

	GLuint pixelFormat = ChoosePixelFormat(g->hDeviceContext, &pfd);
	if (pixelFormat == 0) {
		// Invalid
		delete g;
		return NULL;
	}


	// Apply the pixel format
	if (!SetPixelFormat(g->hDeviceContext, pixelFormat, &pfd)) {
		// Invalid
		delete g;
		return NULL;
	}

	// Render context
	g->hRenderContext = wglCreateContext(g->hDeviceContext);
	if (g->hRenderContext == NULL) {
		// Invalid
		delete g;
		return NULL;
	}


	// Apply the render context
	if (!wglMakeCurrent(g->hDeviceContext, g->hRenderContext)) {
		// Invalid
		delete g;
		return NULL;
	}


	// Get function pointer
	if (wglCreateContextAttribsARB == NULL) {
		wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	}


	// Get a 3.x context
	if (wglCreateContextAttribsARB != NULL) {
		// New context attributes
		int attributes[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_FLAGS_ARB, 0,
			0, 0
		};

		HGLRC newContext = wglCreateContextAttribsARB(g->hDeviceContext, 0, attributes);
		if (newContext != NULL) {
			// Change context to nothing
			if (wglMakeCurrent(NULL, NULL)) {
				// Change to new context
				if (wglMakeCurrent(g->hDeviceContext, newContext)) {
					// Delete old
					wglDeleteContext(g->hRenderContext);
					// Set to new
					g->hRenderContext = newContext;
				}
				else {
					// Return to old context on failure
					if (!wglMakeCurrent(g->hDeviceContext, g->hRenderContext)) {
						// Invalid
						delete g;
						return NULL;
					}
				}
			}
		}
	}


	// Bind function pointers
	g->bindFunctionPointers();


	// Okay
	return g;
}

const char* Graphics :: init() {
	if (Graphics::alreadyInit) return NULL;
	Graphics::alreadyInit = true;

	// Create
	Graphics::thread = new Graphics::Thread();

	// Error
	if (!Graphics::thread->setupOkay()) {
		delete Graphics::thread;
		Graphics::thread = NULL;
		return "Graphics setup failed";
	}

	// Done
	return NULL;
}
const char* Graphics :: deinit() {
	if (!Graphics::alreadyInit) return NULL;
	Graphics::alreadyInit = false;

	// Destroy
	delete Graphics::thread;
	Graphics::thread = NULL;

	// Done
	return NULL;
}
bool Graphics :: okay() {
	return (Graphics::thread != NULL && Graphics::thread->setupOkay());
}
int Graphics :: execute(Function function, void* data) {
	assert(Graphics::thread != NULL);
	assert(function != NULL);
	return Graphics::thread->executeAction(function, data, 0);
}


