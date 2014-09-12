#ifndef ___PLUGIN_VCACHE_H
#define ___PLUGIN_VCACHE_H



#include "../Header.hpp"
FUNCTION_START();



// Video caching class
class VCache :
	public IClip
{
private:
	struct CacheEntry {
		int frameNumber;
		unsigned char* pixelData;

		union {
			struct {
				CacheEntry* before;
				CacheEntry* after;
			};
			CacheEntry* relatives[2];
		};

		~CacheEntry();

	};

	PClip clip;
	VideoInfo clipInfo;

	CacheEntry* cacheEndpoints[2];
	int cacheSize;
	int cacheSizeMax;

	union {
		int count[4];

		struct {
			union {
				int count[2];

				struct {
					int before;
					int after;
				};
			} hit;
			union {
				int count[2];

				struct {
					int before;
					int after;
				};
			} miss;
		};
	} cacheIncrement;
	/*
		cacheIncrement.count[0] = cacheIncrement.hit.count[0] = cacheIncrement.hit.before
		cacheIncrement.count[1] = cacheIncrement.hit.count[1] = cacheIncrement.hit.after
		cacheIncrement.count[2] = cacheIncrement.miss.count[0] = cacheIncrement.miss.before
		cacheIncrement.count[3] = cacheIncrement.miss.count[1] = cacheIncrement.miss.after
	*/

	CacheEntry* getCachedFrame(int frameNumber) const;
	CacheEntry* bringToFrontOfCache(int frameNumber, int frameNumberMin, int frameNumberMax, IScriptEnvironment* env);
	void setupCacheEntry(int frameNumber, CacheEntry* e, IScriptEnvironment* env);

	void dump(std::ofstream* f);

	static AVSValue __cdecl create1(AVSValue args, void* user_data, IScriptEnvironment* env);
	static AVSValue __cdecl create2(AVSValue args, void* user_data, IScriptEnvironment* env);

	VCache(
		IScriptEnvironment* env,
		PClip clip,

		int cacheSizeMax,

		int cacheBeforeCountOnHit,
		int cacheAfterCountOnHit,
		int cacheBeforeCountOnMiss,
		int cacheAfterCountOnMiss
	);
	~VCache();

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

