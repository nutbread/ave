#ifndef ___PLUGIN_MARCHITECTURE_H
#define ___PLUGIN_MARCHITECTURE_H



#include "../Header.hpp"
FUNCTION_START();




// Video frame pixel getting
class MArchitecture {
private:
	MArchitecture();
	~MArchitecture();

	static AVSValue __cdecl getArchitecture(AVSValue args, void* user_data, IScriptEnvironment* env);

public:
	static const char* registerClass(IScriptEnvironment* env);

};



FUNCTION_END();



#endif

