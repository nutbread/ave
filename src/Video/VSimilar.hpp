#ifndef ___PLUGIN_VSIMILAR_H
#define ___PLUGIN_VSIMILAR_H



#include "../Header.hpp"
FUNCTION_START();



// Video caching class
class VSimilar :
	public IClip
{
private:
	PClip clip;
	VideoInfo clipInfo;

	int pixelReferencesFrame;
	unsigned char* pixelReferences;

	bool groupComponents;
	bool componentNormalize[4];
	bool componentIgnore[4];
	int componentThreshold[4];

	static AVSValue __cdecl create1(AVSValue args, void* user_data, IScriptEnvironment* env);
	static AVSValue __cdecl create2(AVSValue args, void* user_data, IScriptEnvironment* env);

	VSimilar(
		IScriptEnvironment* env,
		PClip clip,

		bool groupThresholds,

		bool ignoreRed,
		bool ignoreGreen,
		bool ignoreBlue,
		bool ignoreAlpha,

		bool normalizeRed,
		bool normalizeGreen,
		bool normalizeBlue,
		bool normalizeAlpha,

		int thresholdRed,
		int thresholdGreen,
		int thresholdBlue,
		int thresholdAlpha
	);
	~VSimilar();

public:
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
	const VideoInfo& __stdcall GetVideoInfo();
	bool __stdcall GetParity(int n);
	CacheHintsReturnType __stdcall SetCacheHints(int cachehints, int frame_range);

	static const char* registerClass(IScriptEnvironment* env);

};



FUNCTION_END();



#endif

