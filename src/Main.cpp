#include <sstream>
#undef PLUGIN_AVISYNTH_VERSION
#define PLUGIN_AVISYNTH_VERSION 25
#include "Header.hpp"
#undef PLUGIN_AVISYNTH_VERSION
#define PLUGIN_AVISYNTH_VERSION 26
#include "Header.hpp"
#include "Graphics.hpp"
FUNCTION_START();
const AVS_Linkage* AVS_linkage = NULL;
FUNCTION_END();



// Plugin name
char* pluginName = NULL;



// Function headers
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(::AviSynthV25::IScriptEnvironment* env);
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(::AviSynthV26::IScriptEnvironment* env, const ::AviSynthV26::AVS_Linkage* const vectors);
void AvisynthPluginDeinit2(void* user_data, ::AviSynthV25::IScriptEnvironment* env);
void AvisynthPluginDeinit3(void* user_data, ::AviSynthV26::IScriptEnvironment* env);



// Vesion 2.5x init
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(::AviSynthV25::IScriptEnvironment* env) {
	// Setup exit function
	env->AtExit(AvisynthPluginDeinit2, NULL);

	// Setup functions
	std::stringstream ss;
	ss << "AVExtensions[2.5x]:";
	::Registration<::AviSynthV25::IScriptEnvironment>::setup(env, &ss, ",");
	std::string pn = ss.str();

	// Plugin name c-string
	pluginName = new char[pn.length() + 1];
	for (size_t i = 0; i < pn.length(); ++i) {
		pluginName[i] = pn[i];
	}
	pluginName[pn.length()] = '\0';

	// Return
	return pluginName;
}



// Version 2.6x init
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(::AviSynthV26::IScriptEnvironment* env, const ::AviSynthV26::AVS_Linkage* const vectors) {
	// Link
	::AviSynthV26::AVS_linkage = vectors;

	// Setup exit function
	env->AtExit(AvisynthPluginDeinit3, NULL);

	// Setup functions
	std::stringstream ss;
	ss << "AVExtensions[2.6x]:";
	::Registration<::AviSynthV26::IScriptEnvironment>::setup(env, &ss, ",");
	std::string pn = ss.str();

	// Plugin name c-string
	pluginName = new char[pn.length() + 1];
	for (size_t i = 0; i < pn.length(); ++i) {
		pluginName[i] = pn[i];
	}
	pluginName[pn.length()] = '\0';

	// Return
	return pluginName;
}



// Vesion 2.5x de-init
void AvisynthPluginDeinit2(void* user_data, ::AviSynthV25::IScriptEnvironment* env) {
	// De-init graphics
	Graphics::deinit();

	// Delete plugin name info
	delete [] pluginName;
}



// Vesion 2.6x de-init
void AvisynthPluginDeinit3(void* user_data, ::AviSynthV26::IScriptEnvironment* env) {
	// De-init graphics
	Graphics::deinit();

	// Delete plugin name info
	delete [] pluginName;
}


