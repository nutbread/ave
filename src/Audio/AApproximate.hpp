#ifndef ___PLUGIN_AAPPROXIMATE_H
#define ___PLUGIN_AAPPROXIMATE_H



#include "../Header.hpp"
FUNCTION_START();



// Audio approximation
class AApproximate :
	public IClip
{
private:
	struct Region {
		char sign;
		__int64 position[2];
		Region* before;
		Region* after;
		float max;
		__int64 maxPos;

		Region(__int64 pos, int sign, Region* before, Region* after) :
			sign(sign)
		{
			this->position[0] = pos;
			this->position[1] = pos + 1;
			this->before = before;
			this->after = after;
			this->max = 0.0;
			this->maxPos = pos;
		}
	};
	struct ChannelGraph {
		Region* first;
		Region* last;
	};

	enum Waveform {
		TRIANGLE,
		SQUARE,
		SINE
	};

	PClip clip;
	VideoInfo clipInfo;
	bool isSetup;
	bool crossAtSame0Point;
	Waveform waveform;
	double tolerance;
	double absoluteToleranceFactor;
	__int64 interpolationSize;
	__int64 interpolationSamples;
	ChannelGraph* channelGraphs;

	static void cStringToLower(const char* cstr, std::string& target);

	static float* getFloatAudio(PClip clip, VideoInfo* clipInfo, __int64 start, __int64 count, IScriptEnvironment* env);
	template <class Type> static float* convertAudioToFloat(PClip clip, VideoInfo* clipInfo, __int64 start, __int64 count, IScriptEnvironment* env, Type min, Type max);
	template <class Type> static void convertFloatToAudio(PClip clip, VideoInfo* clipInfo, float* floatAudio, Type* target, __int64 count, IScriptEnvironment* env, Type min, Type max);

	void setup(IScriptEnvironment* env);
	void modifyAudio(float* target, __int64 start, __int64 count, IScriptEnvironment* env);

	double getWaveform(__int64 position, __int64 range, __int64 midpoint, double low, double high, bool crossAtSame0Point);

	template <class Type> static int sign(Type x) {
		return (x > 0) ? 1 : ((x < 0) ? -1 : 0);
	}

	AApproximate(
		IScriptEnvironment* env,
		PClip clip,

		const char* waveform,
		bool crossAtSame0Point,

		double tolerance,
		double absoluteToleranceFactor,

		__int64 interpolationSize,
		__int64 interpolationSamples
	);
	~AApproximate();

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

