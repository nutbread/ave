#ifndef ___PLUGIN_VNOCLEAR_H
#define ___PLUGIN_VNOCLEAR_H



#include "../Header.hpp"
FUNCTION_START();



// Most common color per block
class VNoClear :
	public IClip
{
private:
	PClip clip;
	VideoInfo clipInfo;

	double colorInitial[4];
	double colorRefresh[4];
	double colorRefreshFactor;
	bool clearOnSkip;

	double* previous;
	
	int frameIdPrevious;
	
	VNoClear(
		IScriptEnvironment* env,
		PClip clip,

		unsigned int colorInitial,
		unsigned int colorRefresh,
		double colorRefreshFactor,
		bool clearOnSkip
	);
	~VNoClear();

	void clearPrevious();
	
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

