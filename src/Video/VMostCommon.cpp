#include "VMostCommon.hpp"
#include <cassert>
FUNCTION_START();



// TODO : This needs a "fast" option for lower block sizes
// Most common color per block
VMostCommon :: VMostCommon(
	IScriptEnvironment* env,
	PClip clip,

	int wBlocks,
	int hBlocks,
	int range,
	bool smallOutput
) :
	IClip(),
	clip(clip),
	wBlocks(wBlocks),
	hBlocks(hBlocks),
	range(range),
	smallOutput(smallOutput),
	tiebreaker(TIEBREAKER_RANDOM)
{
	// Get info
	this->clipInfo = this->clip->GetVideoInfo();
	this->clipInfo2 = this->clip->GetVideoInfo();

	// Validation
	if (!this->clipInfo.IsRGB24() && !this->clipInfo.IsRGB32()) {
		env->ThrowError("VMostCommon: RGB24/RGB32 data only");
	}

	// Correction
	if (this->wBlocks < 1) {
		this->wBlocks = 1;
	}
	else if (this->wBlocks > this->clipInfo.width) {
		this->wBlocks = this->clipInfo.width;
	}
	if (this->hBlocks < 1) {
		this->hBlocks = 1;
	}
	else if (this->hBlocks > this->clipInfo.height) {
		this->hBlocks = this->clipInfo.height;
	}
	if (this->range < 0) {
		this->range = 0;
	}

	// Small
	if (this->smallOutput) {
		this->clipInfo2.width = this->wBlocks;
		this->clipInfo2.height = this->hBlocks;
	}

	// Create
	this->colorCountTotal = 256 * 256 * 256;
	this->colorCounts = new unsigned int[this->colorCountTotal];

	this->topResultsLength = 256;
	this->topResults = new unsigned int[this->topResultsLength];
}
VMostCommon :: ~VMostCommon() {
	delete [] this->colorCounts;

	delete [] this->topResults;
}

void VMostCommon :: expandTopResults() {
	// Setup
	int sizeIncrement = 256;
	unsigned int* topResultsOld = this->topResults;
	this->topResults = new unsigned int[this->topResultsLength + sizeIncrement];

	// Copy old
	for (int i = 0; i < this->topResultsLength; ++i) {
		this->topResults[i] = topResultsOld[i];
	}

	// Done
	this->topResultsLength += sizeIncrement;
	delete [] topResultsOld;
}

PVideoFrame __stdcall VMostCommon :: GetFrame(int n, IScriptEnvironment* env) {
	// Destination
	PVideoFrame dest = env->NewVideoFrame(this->clipInfo2);
	unsigned char* destPtr = dest->GetWritePtr();
	int destPitch = dest->GetPitch();

	// Get main source
	PVideoFrame source = this->clip->GetFrame(n, env);
	const unsigned char* sourcePtr = source->GetReadPtr();
	int sourcePitch = source->GetPitch();
	int bgra[4];
	unsigned int colorIndex;
	unsigned int topResultsSize, topResultsAmount, topResultIndex;

	// Loop values
	int components = this->clipInfo.IsRGB32() ? 4 : 3;
	int x, x0, y, xMax, yMax, c, bx, by;
	int rangeLoop[3][2];
	int widthSrc = this->clipInfo.width;
	int heightSrc = this->clipInfo.height;
	int widthDest = this->clipInfo2.width;
	int heightDest = this->clipInfo2.height;

	// Copy from main source
	by = 0;
	while (by < this->hBlocks) {
		bx = 0;
		while (bx < this->wBlocks) {
			// Loop across block of pixels
			x0 = widthSrc * bx / this->wBlocks;
			y = heightSrc * by / this->hBlocks;
			xMax = widthSrc * (bx + 1) / this->wBlocks;
			yMax = heightSrc * (by + 1) / this->hBlocks;
			for (; y < yMax; ++y) {
				for (x = x0; x < xMax; ++x) {
					// Read
					for (c = 0; c < components; ++c) {
						bgra[c] = static_cast<unsigned char>(sourcePtr[c + x * components + y * sourcePitch]);
					}

					// Increment counts
					if (this->range == 0) {
						colorIndex = bgra[2] + (bgra[1] + bgra[0] * 256) * 256;
						++this->colorCounts[colorIndex];
					}
					else {
						// Include range
						for (c = 0; c < 3; ++c) {
							rangeLoop[c][0] = bgra[c] - this->range;
							rangeLoop[c][1] = bgra[c] + this->range;
							if (rangeLoop[c][0] < 0) rangeLoop[c][0] = 0;
							if (rangeLoop[c][1] > 255) rangeLoop[c][1] = 255;
						}
						for (; rangeLoop[0][0] <= rangeLoop[0][1]; ++rangeLoop[0][0]) {
							for (; rangeLoop[1][0] <= rangeLoop[1][1]; ++rangeLoop[1][0]) {
								for (; rangeLoop[2][0] <= rangeLoop[2][1]; ++rangeLoop[2][0]) {
									colorIndex = (rangeLoop[2][0]) + (rangeLoop[1][0] + rangeLoop[0][0] * 256) * 256;
									++this->colorCounts[colorIndex];
								}
							}
						}
					}
				}
			}

			// Find max
			topResultsSize = 0;
			topResultsAmount = 0;

			// Check all colors
			for (c = 0; c < this->colorCountTotal; ++c) {
				if (this->colorCounts[c] > 0) {
					if (this->colorCounts[c] > topResultsAmount) {
						// New
						topResultsSize = 1;
						topResultsAmount = this->colorCounts[c];
						this->topResults[0] = c;
					}
					else if (this->colorCounts[c] == topResultsAmount) {
						// Add
						if (topResultsSize >= this->topResultsLength) this->expandTopResults();
						this->topResults[topResultsSize] = c;
						++topResultsSize;
					}

					// Reset
					this->colorCounts[c] = 0;
				}
			}

			// Convert back to color
			topResultIndex = 0; // TODO : Use tiebreaker
			bgra[2] = ((this->topResults[topResultIndex]) & 0xFF);
			bgra[1] = ((this->topResults[topResultIndex] >> 8) & 0xFF);
			bgra[0] = ((this->topResults[topResultIndex] >> 16) & 0xFF);

			// Apply
			x0 = widthDest * bx / this->wBlocks;
			y = heightDest * by / this->hBlocks;
			xMax = widthDest * (bx + 1) / this->wBlocks;
			yMax = heightDest * (by + 1) / this->hBlocks;
			for (; y < yMax; ++y) {
				for (x = x0; x < xMax; ++x) {
					// Set
					for (c = 0; c < components; ++c) {
						destPtr[c + x * components + y * destPitch] = static_cast<char>(static_cast<unsigned char>(bgra[c]));
					}
				}
			}

			// Continue
			++bx;
		}

		// Continue
		++by;
	}

	// Done
	return dest;
}
void __stdcall VMostCommon :: GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
	this->clip->GetAudio(buf, start, count, env);
}
const VideoInfo& __stdcall VMostCommon :: GetVideoInfo() {
	return this->clipInfo2;
}
bool __stdcall VMostCommon :: GetParity(int n) {
	return this->clip->GetParity(n);
}
CacheHintsReturnType __stdcall VMostCommon :: SetCacheHints(int cachehints, int frame_range) {
	// Do not pass cache requests upwards, only to the next filter
	return CacheHintsReturn(0);
}



AVSValue __cdecl VMostCommon :: create1(AVSValue args, void* user_data, IScriptEnvironment* env) {
	// VMostCommon
	return static_cast<AVSValue>(new VMostCommon(
		env,
		args[0].AsClip(), // clip

		args[1].AsInt(1), // w_blocks
		args[2].AsInt(1), // h_blocks
		args[3].AsInt(0), // range
		args[4].AsBool(false) // small
	));
}
const char* VMostCommon :: registerClass(IScriptEnvironment* env) {
	/**
		clip.VMostCommon(...)

		Get the most common color inside blocks of a video.

		@param clip
			The clip to resize
		@param w_blocks
			How many horizontal blocks
			Range: [1, clip.width]
		@param h_blocks
			How many vertical blocks
			Range: [1, clip.width]
		@param range
			The RGB range a pixel can be for it to count for a nearby color
			Range: [0,255]
			The larger the value, the longer this takes
		@param small
			Instead of using blocks that fill the size of the original, block(s) are 1x1 pixel
		@return
			The clip with the entire clip (or blocks of it) composed of a single color
			If small=true, the size of the resulting clip = ( w_blocks , h_blocks )

		@default
			clip.VMostCommon( \
				w_blocks = 1, \
				h_blocks = 1, \
				range = 0, \
				small = false \
			)
	*/
	env->AddFunction(
		"VMostCommon",
		"[clip]c"
		"[w_blocks]i[h_blocks]i[range]i"
		"[small]b",
		VMostCommon::create1,
		NULL
	);

	return "VMostCommon";
}



FUNCTION_END();
REGISTER(VMostCommon);

