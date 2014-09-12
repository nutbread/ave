#ifndef ___PLUGIN_AVIDEOTOWAVEFORM_H
#define ___PLUGIN_AVIDEOTOWAVEFORM_H



#include "../Header.hpp"
FUNCTION_START();




// Video to audio converter (convert an image to audio waveform; i.e. try and spell out words and such)
class AVideoToWaveform :
	public IClip
{
private:
	enum Waveform {
		TRIANGLE,
		SAWTOOTH,
		SQUARE,
		SINE,
		FLAT
	};
	union Region {
		struct {
			int yTop;
			int yBottom;
		};
		int y[2];
	};

	PClip clip;
	VideoInfo clipInfo;

	int pixelSampleSize;
	int blockSampleSize;
	int blockSamplePeriod;
	Waveform blockWaveform;
	bool loopNearest;
	bool channelSplit;

	int threshold[3];
	std::vector<Region*> regions;

	float emptyValue;

	double getWaveform(__int64 position) const;
	void clearRegions();

	AVideoToWaveform(
		IScriptEnvironment* env,
		PClip clip,

		int sampleRate,
		int channels,

		int pixelSampleSize,
		int blockSampleSize,
		int blockSamplePeriod,
		const char* blockWaveform,
		bool loopNearest,
		bool channelSplit,

		int thresholdRed,
		int thresholdGreen,
		int thresholdBlue
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

