#ifndef ___PLUGIN_VCHANNEL_H
#define ___PLUGIN_VCHANNEL_H



#include "../Header.hpp"
FUNCTION_START();



// Channel separation class
class VChannel :
	public IClip
{
private:
	PClip clip;
	VideoInfo clipInfo;

	bool usingHSV;
	bool normalize;
	bool maximize;
	bool perComponent;
	union {
		int rgb[3];
		int hsv[3];
	};
	union {
		struct {
			unsigned char fullColor[4];
			unsigned char emptyColor[4];
		};
		unsigned char colors[2][4];
	};
	double normalizeValue;
	double rgbDivisor[3];

	static int bound(int value, int vmin, int vmax);
	static int loop(int value, int range);
	static int HSVarrayToRGB(int* hsv);
	static int RGBarrayToRGB(int* rgb);

	VChannel(
		IScriptEnvironment* env,
		PClip clip,
		int red,
		int green,
		int blue,
		int hue,
		int saturation,
		int value,
		int fullColor,
		int emptyColor,
		int fullAlpha,
		int emptyAlpha,
		bool normalize,
		bool maximize,
		bool perComponent
	);

	static AVSValue __cdecl create1(AVSValue args, void* user_data, IScriptEnvironment* env);

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

