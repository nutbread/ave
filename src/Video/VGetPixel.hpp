#ifndef ___PLUGIN_VGETPIXEL_H
#define ___PLUGIN_VGETPIXEL_H



#include "../Header.hpp"
FUNCTION_START();




// Video frame pixel getting
class VGetPixel {
private:
	VGetPixel();
	~VGetPixel();

	static AVSValue __cdecl getPixel(AVSValue args, void* user_data, IScriptEnvironment* env);

public:
	static const char* registerClass(IScriptEnvironment* env);

};



FUNCTION_END();



#endif

