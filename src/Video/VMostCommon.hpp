#ifndef ___PLUGIN_VMOSTCOMMON_H
#define ___PLUGIN_VMOSTCOMMON_H



#include "../Header.hpp"
FUNCTION_START();



// Most common color per block
class VMostCommon :
	public IClip
{
public:
	enum Tiebreaker {
		TIEBREAKER_FIRST,
		TIEBREAKER_LAST,
		TIEBREAKER_RANDOM,
	};

private:
	PClip clip;
	VideoInfo clipInfo;
	VideoInfo clipInfo2;

	int colorCountTotal;
	unsigned int* colorCounts;

	unsigned int topResultsLength;
	unsigned int* topResults;

	int wBlocks;
	int hBlocks;
	int range;
	bool smallOutput;
	Tiebreaker tiebreaker;

	VMostCommon(
		IScriptEnvironment* env,
		PClip clip,

		int wBlocks,
		int hBlocks,
		int range,
		bool smallOutput
	);
	~VMostCommon();

	void expandTopResults();

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

