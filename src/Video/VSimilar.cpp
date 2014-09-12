#include "VSimilar.hpp"
#include <cassert>
FUNCTION_START();


VSimilar :: VSimilar(
	IScriptEnvironment* env,
	PClip clip,

	bool groupComponents,

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
) :
	clip(clip),
	clipInfo(),
	pixelReferencesFrame(-1),
	pixelReferences(NULL),
	groupComponents(groupComponents)
{
	// Get info
	this->clipInfo = this->clip->GetVideoInfo();

	// Validation
	if (!this->clipInfo.IsRGB24() && !this->clipInfo.IsRGB32()) {
		env->ThrowError("VSimilar: RGB24/RGB32 data only");
	}

	// Memory
	this->pixelReferences = new unsigned char[(3 + this->clipInfo.IsRGB32()) * this->clipInfo.width * this->clipInfo.height];

	// Vars
	this->componentIgnore[2] = ignoreRed;
	this->componentIgnore[1] = ignoreGreen;
	this->componentIgnore[0] = ignoreBlue;
	this->componentIgnore[3] = ignoreAlpha;

	this->componentThreshold[2] = thresholdRed;
	this->componentThreshold[1] = thresholdGreen;
	this->componentThreshold[0] = thresholdBlue;
	this->componentThreshold[3] = thresholdAlpha;

	this->componentNormalize[2] = normalizeRed;
	this->componentNormalize[1] = normalizeGreen;
	this->componentNormalize[0] = normalizeBlue;
	this->componentNormalize[3] = normalizeAlpha;
}
VSimilar :: ~VSimilar() {
	delete [] this->pixelReferences;
}

PVideoFrame __stdcall VSimilar :: GetFrame(int n, IScriptEnvironment* env) {

	// Destination
	PVideoFrame dest = env->NewVideoFrame(this->clipInfo);
	unsigned char* destPtr = dest->GetWritePtr();

	// Get main source
	PVideoFrame source = this->clip->GetFrame(n, env);
	const unsigned char* sourcePtr = source->GetReadPtr();

	// Loop values
	int x, y, c, c2,
		width = this->clipInfo.width,
		height = this->clipInfo.height,
		components = 3 + this->clipInfo.IsRGB32(),
		sourcePitchDiff = source->GetPitch() - width * components,
		destPitchDiff = dest->GetPitch() - width * components;

	if (this->pixelReferencesFrame >= 0 && this->pixelReferencesFrame == n - 1) {
		// Update relative
		unsigned char* refPtr = this->pixelReferences;
		int componentValue, referenceValue, diff, thresholdRange;
		double threshold;
		unsigned char bgra[4];
		int updateCountIgnored, updateCountNotIgnored;


		// Copy from main source
		for (y = 0; y < height; ++y) {
			// Loop over line
			for (x = 0; x < width; ++x) {
				// Get old
				updateCountIgnored = 0;
				updateCountNotIgnored = 0;
				for (c = 0; c < components; ++c) {
					// Normalize
					if (this->componentIgnore[c]) {
						*refPtr = *sourcePtr;
						*destPtr = *sourcePtr;
						++updateCountIgnored;
					}
					else {
						// Values
						componentValue = *sourcePtr;
						referenceValue = *refPtr;

						// Threshold
						threshold = this->componentThreshold[c];
						if (this->componentNormalize[c]) {
							thresholdRange = (255 - referenceValue);
							if (thresholdRange < referenceValue) thresholdRange = referenceValue;
							threshold *= thresholdRange / 255.0;
						}

						// Difference
						diff = componentValue - referenceValue;
						if (diff < 0) diff = -diff;

						if (diff >= threshold) {
							// Update
							*destPtr = componentValue;
							*refPtr = componentValue;
							++updateCountNotIgnored;
						}
						else {
							// Use old
							*destPtr = referenceValue;
						}
					}

					// Continue
					++refPtr;
					++sourcePtr;
					++destPtr;
				}

				// Update all if some (but not all) were updated
				if (updateCountNotIgnored > 0 && updateCountIgnored + updateCountNotIgnored < components && this->groupComponents) {
					refPtr -= components;
					sourcePtr -= components;
					destPtr -= components;

					for (c = 0; c < components; ++c) {
						*refPtr = *sourcePtr;
						*destPtr = *sourcePtr;

						++refPtr;
						++sourcePtr;
						++destPtr;
					}
				}
			}

			// Continue
			sourcePtr += sourcePitchDiff;
			destPtr += destPitchDiff;
		}
	}
	else {
		// Update absolute
		unsigned char* refPtr = this->pixelReferences;

		// Copy from main source
		for (y = 0; y < height; ++y) {
			// Loop over line
			for (x = 0; x < width; ++x) {
				// Get old
				for (c = 0; c < components; ++c) {
					// Normalize
					*refPtr = *sourcePtr;
					*destPtr = *sourcePtr;

					++refPtr;
					++sourcePtr;
					++destPtr;
				}
			}

			// Continue
			sourcePtr += sourcePitchDiff;
			destPtr += destPitchDiff;
		}
	}

	// Update
	this->pixelReferencesFrame = n;


	// New frame
	return dest;
}
void __stdcall VSimilar :: GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
	this->clip->GetAudio(buf, start, count, env);
}
const VideoInfo& __stdcall VSimilar :: GetVideoInfo() {
	return this->clipInfo;
}
bool __stdcall VSimilar :: GetParity(int n) {
	return this->clip->GetParity(n);
}
CacheHintsReturnType __stdcall VSimilar :: SetCacheHints(int cachehints, int frame_range) {
	// Do not pass cache requests upwards, only to the next filter
	return CacheHintsReturn(0);
}



AVSValue __cdecl VSimilar :: create1(AVSValue args, void* user_data, IScriptEnvironment* env) {
	PClip clip = args[0].AsClip();

	int threshold = args[1].AsInt(1);
	bool normalize = args[2].AsBool(false);

	bool ignoreRed = args[3].AsBool(false);
	bool ignoreGreen = args[4].AsBool(false);
	bool ignoreBlue = args[5].AsBool(false);
	bool ignoreAlpha = args[6].AsBool(false);

	// VChannel
	return static_cast<AVSValue>(new VSimilar(
		env,
		clip,
		true,
		ignoreRed,
		ignoreGreen,
		ignoreBlue,
		ignoreAlpha,
		normalize,
		normalize,
		normalize,
		normalize,
		threshold,
		threshold,
		threshold,
		threshold
	));
}
AVSValue __cdecl VSimilar :: create2(AVSValue args, void* user_data, IScriptEnvironment* env) {
	PClip clip = args[0].AsClip();

	bool group = args[1].AsBool(true);

	int thresholdRed = args[2].AsInt(1);
	int thresholdGreen = args[3].AsInt(thresholdRed);
	int thresholdBlue = args[4].AsInt(thresholdGreen);
	int thresholdAlpha = args[5].AsInt(thresholdBlue);

	bool normalizeRed = args[6].AsBool(false);
	bool normalizeGreen = args[7].AsBool(false);
	bool normalizeBlue = args[8].AsBool(false);
	bool normalizeAlpha = args[9].AsBool(false);

	bool ignoreRed = args[10].AsBool(false);
	bool ignoreGreen = args[11].AsBool(false);
	bool ignoreBlue = args[12].AsBool(false);
	bool ignoreAlpha = args[13].AsBool(false);

	// VChannel
	return static_cast<AVSValue>(new VSimilar(
		env,
		clip,
		group,
		ignoreRed,
		ignoreGreen,
		ignoreBlue,
		ignoreAlpha,
		normalizeRed,
		normalizeGreen,
		normalizeBlue,
		normalizeAlpha,
		thresholdRed,
		thresholdGreen,
		thresholdBlue,
		thresholdAlpha
	));
}
const char* VSimilar :: registerClass(IScriptEnvironment* env) {
	/**
		clip.VSimilar(...)

		Replace similar colors with the same color across frames

		@param clip
			The clip to modify
		@param threshold
			The threshold range for pixels to be considered different
		@param normalize
			True if the threshold ranges should be normalized to the maximum possible range
			Example: If a pixel's value is 128, and the threshold is 160, this value can never be reached,
				because it would need a value of (128-160) or (128+160), and that's outside the range of [0,255]
				This would normalize it to still be possible in at least one direction
		@param ignore_red
			False if the red component should keep its original value no matter what
		@param ignore_blue
			False if the blue component should keep its original value no matter what
		@param ignore_green
			False if the green component should keep its original value no matter what
		@param ignore_alpha
			False if the alpha component should keep its original value no matter what
		@return
			The same clip, with colors similar colors across frames changed to be the same color

		@default
			clip.VSimilar( \
				threshold = 1, \
				normalize = false, \
				ignore_red = false, \
				ignore_green = false, \
				ignore_blue = false, \
				ignore_alpha = false \
			)
	*/
	env->AddFunction(
		"VSimilar",
		"[clip]c"
		"[threshold]i"
		"[normalize]b"
		"[ignore_red]b[ignore_green]b[ignore_blue]b[ignore_alpha]b",
		VSimilar::create1,
		NULL
	);
	/**
		clip.VSimilar(...)

		Replace similar colors with the same color across frames

		@param clip
			The clip to modify
		@param group
			Group all components of a pixel together.
			i.e. if one changes, they all change.
		@param threshold_red
			The threshold range for pixels to be considered different (red component)
			i.e. if the difference in components is greater than or equal to this value, it will be classified as "different"
			Range: [0, 255]
		@param threshold_green
			The threshold range for pixels to be considered different (green component)
			Range: [0, 255]
		@param threshold_blue
			The threshold range for pixels to be considered different (green component)
			Range: [0, 255]
		@param normalize_red
			True if the threshold ranges should be normalized to the maximum possible range (red component)
		@param normalize_green
			True if the threshold ranges should be normalized to the maximum possible range (green component)
		@param normalize_blue
			True if the threshold ranges should be normalized to the maximum possible range (blue component)
		@param normalize_alpha
			True if the threshold ranges should be normalized to the maximum possible range (alpha component)
		@param ignore_red
			False if the red component should keep its original value no matter what
		@param ignore_blue
			False if the blue component should keep its original value no matter what
		@param ignore_green
			False if the green component should keep its original value no matter what
		@param ignore_alpha
			False if the alpha component should keep its original value no matter what
		@return
			The same clip, with colors similar colors across frames changed to be the same color

		@default
			clip.VSimilar( \
				group = true, \
				threshold_red = 1, \
				threshold_green = 1, \
				threshold_blue = 1, \
				threshold_alpha = 1, \
				normalize_red = false, \
				normalize_green = false, \
				normalize_blue = false, \
				normalize_alpha = false, \
				ignore_red = false, \
				ignore_green = false, \
				ignore_blue = false, \
				ignore_alpha = false \
			)
	*/
	env->AddFunction(
		"VSimilar",
		"[clip]c"
		"[group]b"
		"[threshold_red]i[threshold_green]i[threshold_blue]i[threshold_alpha]i"
		"[normalize_red]b[normalize_green]b[normalize_blue]b[normalize_alpha]b"
		"[ignore_red]b[ignore_green]b[ignore_blue]b[ignore_alpha]b",
		VSimilar::create2,
		NULL
	);


	return "VSimilar";
}




FUNCTION_END();
REGISTER(VSimilar);

