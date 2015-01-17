#include "VNoClear.hpp"
#include <cassert>
FUNCTION_START();



VNoClear :: VNoClear(
	IScriptEnvironment* env,
	PClip clip,
	unsigned int colorInitial,
	unsigned int colorRefresh,
	double colorRefreshFactor,
	bool clearOnSkip
) :
	IClip(),
	clip(clip),
	colorRefreshFactor(colorRefreshFactor),
	clearOnSkip(clearOnSkip),
	previous(NULL),
	frameIdPrevious(-1)
{
	// Get info
	this->clipInfo = this->clip->GetVideoInfo();

	// Validation
	if (!this->clipInfo.IsRGB32()) {
		env->ThrowError("VNoClear: RGB32 data only");
	}

	// Correction
	this->colorInitial[0] = ((colorInitial) & 0xFF) / 255.0;
	this->colorInitial[1] = ((colorInitial >> 8) & 0xFF) / 255.0;
	this->colorInitial[2] = ((colorInitial >> 16) & 0xFF) / 255.0;
	this->colorInitial[3] = ((colorInitial >> 24) & 0xFF) / 255.0;

	this->colorRefresh[0] = ((colorRefresh) & 0xFF) / 255.0;
	this->colorRefresh[1] = ((colorRefresh >> 8) & 0xFF) / 255.0;
	this->colorRefresh[2] = ((colorRefresh >> 16) & 0xFF) / 255.0;
	this->colorRefresh[3] = ((colorRefresh >> 24) & 0xFF) / 255.0;

	// Create
	size_t preLen = 4 * this->clipInfo.width * this->clipInfo.height;
	this->previous = new double[preLen];
	this->clearPrevious();
}
VNoClear :: ~VNoClear() {
	delete [] this->previous;
}

void VNoClear :: clearPrevious() {
	size_t preLen = 4 * this->clipInfo.width * this->clipInfo.height;
	size_t i;
	for (i = 0; i < preLen; i += 4) {
		this->previous[i] = this->colorInitial[0];
		this->previous[i + 1] = this->colorInitial[1];
		this->previous[i + 2] = this->colorInitial[2];
		this->previous[i + 3] = this->colorInitial[3];
	}
}

PVideoFrame __stdcall VNoClear :: GetFrame(int n, IScriptEnvironment* env) {
	// Destination
	PVideoFrame dest = env->NewVideoFrame(this->clipInfo);
	unsigned char* destPtr = dest->GetWritePtr();
	int destPitch = dest->GetPitch();

	// Get main source
	PVideoFrame source = this->clip->GetFrame(n, env);
	const unsigned char* sourcePtr = source->GetReadPtr();
	int sourcePitch = source->GetPitch();

	// Size
	int i, j, iMax, x, y;
	int widthX4 = this->clipInfo.width * 4;
	int height = this->clipInfo.height;
	double alpha, alphaInv;
	double* pre;

	// Clearing
	if (this->clearOnSkip && this->frameIdPrevious >= 0 && this->frameIdPrevious != n - 1) {
		this->clearPrevious();
	}
	this->frameIdPrevious = n;

	// Merge
	if (this->colorRefreshFactor > 0.0) {
		iMax = widthX4 * height;
		for (i = 0; i < iMax; i += 4) {
			for (j = 0; j < 4; ++j) {
				this->previous[i + j] = this->previous[i + j] * (1.0 - this->colorRefreshFactor) + this->colorRefresh[0 + j] * this->colorRefreshFactor;
			}
		}
	}

	// New frame
	pre = this->previous;
	for (y = 0; y < height; ++y) {
		for (x = 0; x < widthX4; x += 4) {
			alpha = sourcePtr[x + 3] / 255.0;
			alphaInv = 1.0 - alpha;
			for (i = 0; i < 4; ++i) {
				*pre = (*pre) * alphaInv + (sourcePtr[x + i] / 255.0) * alpha;
				destPtr[x + i] = static_cast<unsigned char>(*pre * 255 + 0.5);
				++pre;
			}
		}

		sourcePtr += sourcePitch;
		destPtr += destPitch;
	}

	// Done
	return dest;
}
void __stdcall VNoClear :: GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
	this->clip->GetAudio(buf, start, count, env);
}
const VideoInfo& __stdcall VNoClear :: GetVideoInfo() {
	return this->clipInfo;
}
bool __stdcall VNoClear :: GetParity(int n) {
	return this->clip->GetParity(n);
}
CacheHintsReturnType __stdcall VNoClear :: SetCacheHints(int cachehints, int frame_range) {
	// Do not pass cache requests upwards, only to the next filter
	return CacheHintsReturn(0);
}



AVSValue __cdecl VNoClear :: create1(AVSValue args, void* user_data, IScriptEnvironment* env) {
	// VNoClear
	return static_cast<AVSValue>(new VNoClear(
		env,
		args[0].AsClip(), // clip

		args[1].AsInt(0x00000000), // color_initial
		args[2].AsInt(0x00000000), // color_refresh
		args[3].AsFloat(0.0), // color_refresh_factor
		args[4].AsBool(false) // clear_on_skip
	));
}
const char* VNoClear :: registerClass(IScriptEnvironment* env) {
	/**
		clip.VNoClear(...)

		Render a RGB32 video, simulating no background clearing.
		(ie: transparent sections will show the previous frame)

		@param clip
			The clip to resize
		@param color_initial
			The initial background color
			Default: $00000000 (RGBA=0,0,0,0)
		@param color_refresh
			The color overlayed on the previous frame before overlaying the new frame
			Default: $00000000 (RGBA=0,0,0,0)
		@param color_refresh_factor
			The factor at which the colors should be merged
			Range: [0, 1]
				let: color_refresh_factor=CRF, color_refresh=CR; ranges of everything normalized to [0, 1]
				final_R = (pre_R * (1-CRF) + CR_R * CRF) * (1.0 - new_A) + new_R * new_A // same for G,B,A
		@param clear_on_skip
			True: if a seek occured, the background should be cleared to color_initial
			False: ignore seeks
			A seek in this case means a non-sequential frame access; ie frame 3 to frame 5
		@return
			The clip with simulated no background clearing

		@default
			clip.VNoClear( \
				color_initial = $00000000, \
				color_refresh = $00000000, \
				color_refresh_factor = 0.0, \
				clear_on_skip = false \
			)
	*/
	env->AddFunction(
		"VNoClear",
		"[clip]c"
		"[color_initial]i[color_refresh]i"
		"[color_refresh_factor]f"
		"[clear_on_skip]b",
		VNoClear::create1,
		NULL
	);

	return "VNoClear";
}



FUNCTION_END();
REGISTER(VNoClear);

