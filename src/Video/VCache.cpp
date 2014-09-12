#include "VCache.hpp"
#include <cassert>
#include <fstream>
FUNCTION_START();



// Clip caching
VCache :: VCache(
	IScriptEnvironment* env,
	PClip clip,

	int cacheSizeMax,
	int cacheBeforeCountOnHit,
	int cacheAfterCountOnHit,
	int cacheBeforeCountOnMiss,
	int cacheAfterCountOnMiss
) :
	IClip(),
	clip(clip),
	cacheSize(0),
	cacheSizeMax(cacheSizeMax)
{
	// Get info
	this->clipInfo = this->clip->GetVideoInfo();

	// Validate
	if (!this->clipInfo.IsRGB24() && !this->clipInfo.IsRGB32() && !this->clipInfo.IsYUY2() && !this->clipInfo.IsYUV()) {
		env->ThrowError("VCache: Invalid video type");
	}

	// Setup
	this->cacheEndpoints[0] = new CacheEntry();
	this->cacheEndpoints[1] = new CacheEntry();

	this->cacheEndpoints[0]->before = NULL;
	this->cacheEndpoints[0]->after = this->cacheEndpoints[1];
	this->cacheEndpoints[1]->before = this->cacheEndpoints[0];
	this->cacheEndpoints[1]->after = NULL;

	this->cacheEndpoints[0]->frameNumber = -1;
	this->cacheEndpoints[1]->frameNumber = -1;
	this->cacheEndpoints[0]->pixelData = NULL;
	this->cacheEndpoints[1]->pixelData = NULL;

	this->cacheIncrement.hit.before = cacheBeforeCountOnHit;
	this->cacheIncrement.hit.after = cacheAfterCountOnHit;
	this->cacheIncrement.miss.before = cacheBeforeCountOnMiss;
	this->cacheIncrement.miss.after = cacheAfterCountOnMiss;

	// Cache size
	if (this->cacheSizeMax < 1) this->cacheSizeMax = 1;
}
VCache :: ~VCache() {
	// Delete all
	CacheEntry* e = this->cacheEndpoints[0];
	while (e != NULL) {
		this->cacheEndpoints[0] = e;
		e = e->after;

		delete this->cacheEndpoints[0];
	}
}

PVideoFrame __stdcall VCache :: GetFrame(int n, IScriptEnvironment* env) {
	// Check if the frame is cached
	CacheEntry* e = this->getCachedFrame(n);
	if (e == NULL) {
		// Not cached
		e = this->bringToFrontOfCache(n, n - this->cacheIncrement.miss.before, n + this->cacheIncrement.miss.after, env);

		// Make sure there is a main cache entry
		assert(e != NULL);
	}
	else {
		// Cached
		this->bringToFrontOfCache(n, n - this->cacheIncrement.hit.before, n + this->cacheIncrement.hit.after, env);
	}

	// Create a new frame
	PVideoFrame dest = env->NewVideoFrame(this->clipInfo);

	// Assign the data
	int destRowSize, destHeight, destPitchChange;
	unsigned char* destPtr;
	const unsigned char* sourcePtr = e->pixelData;
	assert(sourcePtr != NULL);
	if (this->clipInfo.IsPlanar()) {
		// Loop over planes
		int planes[] = { PLANAR_Y , PLANAR_V , PLANAR_U };

		// Fill array
		for (int plane = 0; plane < 3; ++plane) {
			// Sizes
			destRowSize = dest->GetRowSize(planes[plane]);
			destHeight = dest->GetHeight(planes[plane]);
			destPitchChange = dest->GetPitch(planes[plane]) - destRowSize;

			// Pointer
			destPtr = dest->GetWritePtr(planes[plane]);

			// Loop
			int i = 0;
			for (int y = 0; y < destHeight; ++y) {
				for (int x = 0; x < destRowSize; ++x) {
					*(destPtr) = *(sourcePtr);
					++destPtr;
					++sourcePtr;
				}

				// Additional offset if necessary
				destPtr += destPitchChange;
			}
		}
	}
	else { // if (this->clipInfo.IsRGB24() || this->clipInfo.IsRGB32() || this->clipInfo.IsYUY2()) {
		// Sizes
		destRowSize = dest->GetRowSize();
		destHeight = dest->GetHeight();
		destPitchChange = dest->GetPitch() - destRowSize;

		// Pointer
		destPtr = dest->GetWritePtr();

		// Loop
		int i = 0;
		for (int y = 0; y < destHeight; ++y) {
			for (int x = 0; x < destRowSize; ++x) {
				*(destPtr) = *(sourcePtr);
				++destPtr;
				++sourcePtr;
			}

			// Additional offset if necessary
			destPtr += destPitchChange;
		}
	}

	// Return
	return dest;
}
void __stdcall VCache :: GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
	this->clip->GetAudio(buf, start, count, env);
}
const VideoInfo& __stdcall VCache :: GetVideoInfo() {
	return this->clipInfo;
}
bool __stdcall VCache :: GetParity(int n) {
	return this->clip->GetParity(n);
}
CacheHintsReturnType __stdcall VCache :: SetCacheHints(int cachehints, int frame_range) {
	// Do not pass cache requests upwards, only to the next filter
	return CacheHintsReturn(0);
}

VCache::CacheEntry* VCache :: getCachedFrame(int frameNumber) const {
	CacheEntry* e = this->cacheEndpoints[0]->after;
	while (e != this->cacheEndpoints[1]) {
		// Check if frame matches
		if (e->frameNumber == frameNumber) return e;

		// Next
		assert(e->after != e);
		e = e->after;
	}

	// No match
	return NULL;
}
VCache::CacheEntry* VCache :: bringToFrontOfCache(int frameNumber, int frameNumberMin, int frameNumberMax, IScriptEnvironment* env) {
	// Returns CacheEntry with frameNumber==frameNumber
	assert(frameNumber >= 0);
	assert(frameNumberMax >= 0);
	assert(frameNumber < this->clipInfo.num_frames);
	assert(frameNumberMin < this->clipInfo.num_frames);

	if (frameNumberMin < 0) frameNumberMin = 0;
	if (frameNumberMax >= this->clipInfo.num_frames) frameNumberMax = this->clipInfo.num_frames - 1;

	// Find "new" cache entries
	int count = frameNumberMax - frameNumberMin + 1;
	CacheEntry* e;
	CacheEntry** ceArray = new CacheEntry*[count];

	for (int i = 0, j, fId; i < count; ++i) {
		// Get the frame id and array index
		fId = i + frameNumberMin;
		j = (fId == frameNumber) ? 0 : (i + (fId < frameNumber));

		// Set the frame if it already exists
		e = this->getCachedFrame(fId);
		if (e == NULL) {
			// Create a new cache entry
			e = new CacheEntry();
			this->setupCacheEntry(fId, e, env);
			++this->cacheSize;
		}
		else {
			// Unlink its relatives
			e->after->before = e->before;
			e->before->after = e->after;
		}

		ceArray[j] = e;
	}

	// Link first and last
	--count;
	e = this->cacheEndpoints[0]->after;

	ceArray[0]->before = this->cacheEndpoints[0];
	this->cacheEndpoints[0]->after = ceArray[0];
	ceArray[count]->after = e;
	e->before = ceArray[count];

	// Link middle
	for (int i = 0; i < count; ++i) {
		ceArray[i]->after = ceArray[i + 1];
		ceArray[i + 1]->before = ceArray[i];
	}

	// Bound cache size
	if (this->cacheSize > this->cacheSizeMax) {
		// Delete overflow
		CacheEntry* e = this->cacheEndpoints[1]->before;
		CacheEntry* eNext;
		for (int i = this->cacheSizeMax; i < this->cacheSize; ++i) {
			assert(e != NULL);
			eNext = e->before;

			delete e;
			e = eNext;
		}

		// Re-link
		assert(e != NULL);
		this->cacheEndpoints[1]->before = e;
		e->after = this->cacheEndpoints[1];

		// Update size
		this->cacheSize = this->cacheSizeMax;
	}

	// Clean
	delete [] ceArray;

	// Return
	return this->cacheEndpoints[0]->after;
}
void VCache :: setupCacheEntry(int frameNumber, CacheEntry* e, IScriptEnvironment* env) {
	// Set the frame number
	e->frameNumber = frameNumber;

	// Get the pixel data
	size_t pdSize;
	unsigned char* destPtr;
	const unsigned char* sourcePtr;
	int sourceRowSize, sourceHeight, sourcePitchChange;
	PVideoFrame source = this->clip->GetFrame(frameNumber, env);


	if (this->clipInfo.IsPlanar()) {
		// Loop over planes
		int planes[] = { PLANAR_Y , PLANAR_V , PLANAR_U };

		// Get size
		pdSize = 0;
		for (int plane = 0; plane < 3; ++plane) {
			pdSize += source->GetRowSize(planes[plane]) * source->GetHeight(planes[plane]);
		}

		// Create new array
		e->pixelData = new unsigned char[pdSize];
		destPtr = e->pixelData;

		// Fill array
		for (int plane = 0; plane < 3; ++plane) {
			// Sizes
			sourceRowSize = source->GetRowSize(planes[plane]);
			sourceHeight = source->GetHeight(planes[plane]);
			sourcePitchChange = source->GetPitch(planes[plane]) - sourceRowSize;

			// Pointers
			sourcePtr = source->GetReadPtr(planes[plane]);

			// Loop
			int i = 0;
			for (int y = 0; y < sourceHeight; ++y) {
				for (int x = 0; x < sourceRowSize; ++x) {
					*(destPtr) = *(sourcePtr);
					++destPtr;
					++sourcePtr;
				}

				// Additional offset if necessary
				sourcePtr += sourcePitchChange;
			}
		}
	}
	else { // if (this->clipInfo.IsRGB24() || this->clipInfo.IsRGB32() || this->clipInfo.IsYUY2()) {
		// Sizes
		sourceRowSize = source->GetRowSize();
		sourceHeight = source->GetHeight();
		sourcePitchChange = source->GetPitch() - sourceRowSize;

		// Create new array
		pdSize = sourceRowSize * sourceHeight;
		e->pixelData = new unsigned char[pdSize];

		// Pointers
		sourcePtr = source->GetReadPtr();
		destPtr = e->pixelData;

		// Loop
		int i = 0;
		for (int y = 0; y < sourceHeight; ++y) {
			for (int x = 0; x < sourceRowSize; ++x) {
				*(destPtr) = *(sourcePtr);
				++destPtr;
				++sourcePtr;
			}

			// Additional offset if necessary
			sourcePtr += sourcePitchChange;
		}
	}
}
void VCache :: dump(std::ofstream* f) {
	*f << this->cacheSize << " / " << this->cacheSizeMax << '\n';

	int i = 0;
	for (CacheEntry* e = this->cacheEndpoints[0]->after; e->after != NULL; e = e->after) {
		*f << '[' << i << "] : frame_number=" << e->frameNumber << '\n';
		++i;
	}

	*f << "\n\n";
}



VCache::CacheEntry :: ~CacheEntry() {
	// Delete any data
	delete [] this->pixelData;
}



AVSValue __cdecl VCache :: create1(AVSValue args, void* user_data, IScriptEnvironment* env) {
	int cacheBeforeCountOnHit = args[4].AsInt(0);
	int cacheAfterCountOnHit = args[5].AsInt(0);
	int cacheBeforeCountOnMiss = args[2].AsInt(0);
	int cacheAfterCountOnMiss = args[3].AsInt(0);

	int maxCacheSize = args[1].Defined() ?
		max(1, args[1].AsInt(1)) :
		1 + max(cacheBeforeCountOnHit, cacheBeforeCountOnMiss) + max(cacheAfterCountOnHit, cacheAfterCountOnMiss);

	// VCache
	return static_cast<AVSValue>(new VCache(
		env,
		args[0].AsClip(), // clip

		maxCacheSize, // cache_size

		cacheBeforeCountOnHit, // before_hit
		cacheAfterCountOnHit, // after_hit
		cacheBeforeCountOnMiss, // before
		cacheAfterCountOnMiss // after
	));
}
AVSValue __cdecl VCache :: create2(AVSValue args, void* user_data, IScriptEnvironment* env) {
	int cacheBeforeCountOnHit = 0;
	int cacheAfterCountOnHit = 0;
	int cacheBeforeCountOnMiss = args[2].AsInt(0);
	int cacheAfterCountOnMiss = args[3].AsInt(0);

	int maxCacheSize = args[1].Defined() ?
		max(1, args[1].AsInt(1)) :
		1 + max(cacheBeforeCountOnHit, cacheBeforeCountOnMiss) + max(cacheAfterCountOnHit, cacheAfterCountOnMiss);

	// VCache
	return static_cast<AVSValue>(new VCache(
		env,
		args[0].AsClip(), // clip

		maxCacheSize, // cache_size

		cacheBeforeCountOnHit, // before_hit
		cacheAfterCountOnHit, // after_hit
		cacheBeforeCountOnMiss, // before_miss
		cacheAfterCountOnMiss // after_miss
	));
}
const char* VCache :: registerClass(IScriptEnvironment* env) {
	/**
		clip.VCache(...)

		Cache video frames in memory, so they don't have to be re-decoded or re-filtered if they are
			used multiple times in a row
		This is particularly useful when using Reverse() on a video clip. Example:
			clip.VCache(cache_size=100, before=99).Reverse()
			runs much faster than:
			clip.Reverse()
			because it caches multiple successive frames.

		@param clip
			The clip to cache frames of
		@param cache_size
			The maximum cache size
			Minimum of 1
		@param before
			The number of frames to cache in front of the current_frame
			when the current_frame is NOT already cached
		@param after
			The number of frames to cache behind the current_frame
			when the current_frame is NOT already cached
		@param before_hit
			The number of frames to cache in front of the current_frame
			when the current_frame is already cached
		@param after_hit
			The number of frames to cache behind the current_frame
			when the current_frame is already cached
		@return
			A video clip that looks identical to the original, but potentially allows the script to run faster

		@default
			clip.VCache( \
				cache_size = 1 + max(before, before_hit) + max(after, after_hit), \
				before = 0, \
				after = 0, \
				before_hit = 0, \
				after_hit = 0 \
			)
	*/
	env->AddFunction(
		"VCache",
		"[clip]c"
		"[cache_size]i"
		"[before]i[after]i[before_hit]i[after_hit]i",
		VCache::create1,
		NULL
	);
	/**
		clip.VCache(...)

		Cache video frames in memory, so they don't have to be re-decoded or re-filtered if they are
			used multiple times in a row
		This is particularly useful when using Reverse() on a video clip. Example:
			clip.VCache(cache_size=100, before=99).Reverse()
			runs much faster than:
			clip.Reverse()
			because it caches multiple successive frames.

		@param clip
			The clip to cache frames of
		@param cache_size
			The maximum cache size
			Minimum of 1
		@param before
			The number of frames to cache in front of the current_frame
			when the current_frame is NOT already cached
		@param after
			The number of frames to cache behind the current_frame
			when the current_frame is NOT already cached
		@return
			A video clip that looks identical to the original, but potentially allows the script to run faster

		@default
			clip.VCache( \
				cache_size = 1 + before + after, \
				before = 0, \
				after = 0 \
			)
	*/
	env->AddFunction(
		"VCache",
		"[clip]c"
		"[cache_size]i"
		"[before]i[after]i",
		VCache::create2,
		NULL
	);

	return "VCache";
}




FUNCTION_END();
REGISTER(VCache);

