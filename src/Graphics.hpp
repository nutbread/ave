#ifndef ___PLUGIN_GRAPHICS_H
#define ___PLUGIN_GRAPHICS_H



#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include "Headers/glext.h"
#include "Headers/wglext.h"



// Graphics class

//{
extern PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
extern PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsString;

extern PFNGLGETSTRINGIPROC glGetStringi;

extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffers;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffers;
extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebuffer;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatus;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameteriv;
extern PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmap;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2D;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbuffer;

extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffers;
extern PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffers;
extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbuffer;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorage;
extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameteriv;
extern PFNGLISRENDERBUFFEREXTPROC glIsRenderbuffer;

extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLBUFFERSUBDATAPROC glBufferSubData;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
extern PFNGLGETBUFFERPARAMETERIVPROC glGetBufferParameteriv;
extern PFNGLMAPBUFFERPROC glMapBuffer;
extern PFNGLUNMAPBUFFERPROC glUnmapBuffer;
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;

extern PFNGLACTIVETEXTUREPROC glActiveTexture;

extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLDETACHSHADERPROC glDetachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUNIFORM2IPROC glUniform2i;
extern PFNGLUNIFORM3IPROC glUniform3i;
extern PFNGLUNIFORM4IPROC glUniform4i;
extern PFNGLUNIFORM1IVPROC glUniform1iv;
extern PFNGLUNIFORM2IVPROC glUniform2iv;
extern PFNGLUNIFORM3IVPROC glUniform3iv;
extern PFNGLUNIFORM4IVPROC glUniform4iv;
extern PFNGLUNIFORM1UIPROC glUniform1ui;
extern PFNGLUNIFORM2UIPROC glUniform2ui;
extern PFNGLUNIFORM3UIPROC glUniform3ui;
extern PFNGLUNIFORM4UIPROC glUniform4ui;
extern PFNGLUNIFORM1UIVPROC glUniform1uiv;
extern PFNGLUNIFORM2UIVPROC glUniform2uiv;
extern PFNGLUNIFORM3UIVPROC glUniform3uiv;
extern PFNGLUNIFORM4UIVPROC glUniform4uiv;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORM2FPROC glUniform2f;
extern PFNGLUNIFORM3FPROC glUniform3f;
extern PFNGLUNIFORM4FPROC glUniform4f;
extern PFNGLUNIFORM1FVPROC glUniform1fv;
extern PFNGLUNIFORM2FVPROC glUniform2fv;
extern PFNGLUNIFORM3FVPROC glUniform3fv;
extern PFNGLUNIFORM4FVPROC glUniform4fv;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
extern PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation;
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGENSAMPLERSPROC glGenSamplers;
extern PFNGLDELETESAMPLERSPROC glDeleteSamplers;
extern PFNGLSAMPLERPARAMETERIPROC glSamplerParameteri;
extern PFNGLGETSAMPLERPARAMETERIVPROC glGetSamplerParameteriv;
extern PFNGLBINDSAMPLERPROC glBindSampler;

extern PFNGLFENCESYNCPROC glFenceSync;
extern PFNGLWAITSYNCPROC glWaitSync;
extern PFNGLDELETESYNCPROC glDeleteSync;
//}

class Graphics {
public:
	typedef int (*Function)(void*);

	class Thread {
		friend class Graphics;

	private:
		struct FunctionQueue {
			enum {
				FLAGS_QUIT = 0x1
			};

			Function function;
			void* data;
			unsigned int flags;
			int returnValue;
			HANDLE completionLock;
			FunctionQueue* functionQueueNext;
		};

		Graphics* graphicsInstance;
		HANDLE threadHandle;
		DWORD threadId;
		bool shouldClose;
		HANDLE requestSemaphore;
		CRITICAL_SECTION functionQueueModificationLock;
		FunctionQueue* functionQueue;
		FunctionQueue** functionQueueLast;

		Thread();
		~Thread();

		static DWORD __stdcall executeThread(void* data);

		static int initFunction(void* unused);

	public:
		int executeAction(Graphics::Function function, void* data, unsigned int flags);

		bool setupOkay() const;

	};

private:
	static bool alreadyInit;
	static bool functionPointersBound;
	static size_t activeGraphicsCount;
	static Graphics::Thread* thread;

	HINSTANCE hInstance;
	WNDCLASSEX windowClass;
	HWND hWindow;
	HDC hDeviceContext;
	HGLRC hRenderContext;
	bool registered;
	unsigned int threadId;
	unsigned int referenceCount;

	Graphics();
	~Graphics();

	static LRESULT CALLBACK graphicsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void bindFunctionPointers();

	static Graphics* create();

public:
	static const char* init();
	static const char* deinit();
	static bool okay();
	static int execute(Function function, void* data);

};



#endif