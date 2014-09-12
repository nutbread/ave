#ifndef ___PLUGIN_VNOISE_H
#define ___PLUGIN_VNOISE_H



#include "../Header.hpp"
#include "../Helpers.hpp"
FUNCTION_START();




// Noise add class
class VNoise :
	public IClip
{
private:
	PClip clip;
	VideoInfo clipInfo;
	Random random;
	int components;
	int strengthMin;
	int strengthMax;
	int shadeMin;
	int shadeMax;

	inline void randomChange(int* hsv);

	static AVSValue __cdecl create1(AVSValue args, void* user_data, IScriptEnvironment* env);

	VNoise(
		IScriptEnvironment* env,
		PClip clip,
		int strengthMin,
		int strengthMax,
		int shadeMin,
		int shadeMax,
		int seed
	);

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

