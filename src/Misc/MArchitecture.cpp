#include "MArchitecture.hpp"
#include <cassert>
FUNCTION_START();



AVSValue __cdecl MArchitecture :: getArchitecture(AVSValue args, void* user_data, IScriptEnvironment* env) {
	// Return the value
#	if defined(_M_X64) || defined(__amd64__)
	return AVSValue("x64");
#	else
	return AVSValue("x86");
#	endif
}



const char* MArchitecture :: registerClass(IScriptEnvironment* env) {
	/**
		MArchitecture(...)

		Get the runtime architecture of the plugin

		@return
			"x86" if using a 32-bit compiled version
			"x64" if using a 64-bit compiled version

		@default
			MArchitecture()
	*/
	env->AddFunction(
		"MArchitecture",
		"",
		MArchitecture::getArchitecture,
		NULL
	);

	return "MArchitecture";
}



FUNCTION_END();
REGISTER(MArchitecture);


