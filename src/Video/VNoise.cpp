#include "VNoise.hpp"
#include <cassert>
FUNCTION_START();




// Video noise
VNoise :: VNoise(
	IScriptEnvironment* env,
	PClip clip,
	int strengthMin,
	int strengthMax,
	int shadeMin,
	int shadeMax,
	int seed
) :
	IClip(),
	clip(clip),
	random(),
	components(3),
	strengthMin(strengthMin),
	strengthMax(strengthMax),
	shadeMin(shadeMin),
	shadeMax(shadeMax)
{
	// Get info
	this->clipInfo = this->clip->GetVideoInfo();

	// Validation
	if (this->clipInfo.IsRGB24()) {
		this->components = 3;
	}
	else if (this->clipInfo.IsRGB32()) {
		this->components = 4;
	}
	else {
		env->ThrowError("VNoise: RGB24/RGB32 data only");
	}

	// Values
	if (seed >= 0) this->random.seed(seed);

	if (this->strengthMin < 0) this->strengthMin = 0;
	else if (this->strengthMin > 255) this->strengthMin = 255;

	if (this->strengthMax < 0) this->strengthMax = 0;
	else if (this->strengthMax > 255) this->strengthMax = 255;

	if (this->strengthMax < this->strengthMin) this->strengthMax = this->strengthMin;

	// Shades
	if (this->shadeMin < 0) this->shadeMin = 0;
	else if (this->shadeMin > 255) this->shadeMin = 255;

	if (this->shadeMax < 0) this->shadeMax = 0;
	else if (this->shadeMax > 255) this->shadeMax = 255;

	if (this->shadeMax < this->shadeMin) this->shadeMax = this->shadeMin;

}

PVideoFrame __stdcall VNoise :: GetFrame(int n, IScriptEnvironment* env) {
	// Destination
	PVideoFrame dest = env->NewVideoFrame(this->clipInfo);
	unsigned char* destPtr = dest->GetWritePtr();
	int destPitch = dest->GetPitch();

	// Get main source
	PVideoFrame sourceMain = this->clip->GetFrame(n, env);
	const unsigned char* sourceMainPtr = sourceMain->GetReadPtr();
	int sourceMainPitch = sourceMain->GetPitch();
	int rgba[4];

	// Loop values
	int x, y, j;
	int width = this->clipInfo.width;
	int height = this->clipInfo.height;
	int rowSize = width * this->components;

	// Copy from main source
	for (y = 0; y < height; ++y) {
		// Loop over line
		for (x = 0; x < rowSize; x += this->components) {
			// Get old
			for (j = 0; j < this->components; ++j) {
				rgba[j] = *(sourceMainPtr + x + j);
			}

			// Modify
			this->randomChange(rgba);

			// Set new
			for (j = 0; j < this->components; ++j) {
				*(destPtr + x + j) = rgba[j];
			}
		}

		// Next line
		sourceMainPtr += sourceMainPitch;
		destPtr += destPitch;
	}

	// Done
	return dest;
}
void __stdcall VNoise :: GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
	this->clip->GetAudio(buf, start, count, env);
}
const VideoInfo& __stdcall VNoise :: GetVideoInfo() {
	return this->clipInfo;
}
bool __stdcall VNoise :: GetParity(int n) {
	return this->clip->GetParity(n);
}
CacheHintsReturnType __stdcall VNoise :: SetCacheHints(int cachehints, int frame_range) {
	// Do not pass cache requests upwards, only to the next filter
	return CacheHintsReturn(0);
}

void VNoise :: randomChange(int* rgb) {
	// Hue
	int shade = this->random.nextInt(this->shadeMax - this->shadeMin + 1) + this->shadeMin;
	int factor = this->random.nextInt(this->strengthMax - this->strengthMin + 1) + this->strengthMin;
	int f_1 = 255 - factor;

	// Merge
	for (int i = 0; i < 3; ++i) {
		rgb[i] = static_cast<int>((rgb[i] * f_1 / 255.) + (shade * factor / 255.));
	}
}




AVSValue __cdecl VNoise :: create1(AVSValue args, void* user_data, IScriptEnvironment* env) {
	// New video reversal
	return static_cast<AVSValue>(new VNoise(
		env,
		args[0].AsClip(), // clip
		args[1].AsInt(0), // strength_min
		args[2].AsInt(40), // strength_max
		args[3].AsInt(0), // shade_min
		args[4].AsInt(255), // shade_max
		args[5].AsInt(0) // seed
	));
}
const char* VNoise :: registerClass(IScriptEnvironment* env) {
	/**
		clip.VNoise(...)

		Add black and white noise to a video clip.

		@param clip
			The clip to add noise to
		@param strength_min
			The minimum strength to apply the noise
			Range: [0,255]
		@param strength_max
			The maximum strength to apply the noise
			Range: [0,255]
		@param shade_min
			The minimum shade (0=black)
			Range: [0,255]
		@param shade_max
			The maximum shade (255=white)
			Range: [0,255]
		@param seed
			The random number generator seed
			If less than 0, it's always seeded with the time
		@return
			The video clip with black and white noise added

		@default
			clip.VNoise( \
				strength_min = 0, \
				strenght_max = 40, \
				shade_min = 0, \
				shade_max = 255, \
				seed = 0 \
			)
	*/
	env->AddFunction(
		"VNoise",
		"[clip]c"
		"[strength_min]i[strength_max]i[shade_min]i[shade_max]i[seed]i",
		VNoise::create1,
		NULL
	);

	return "VNoise";
}



FUNCTION_END();
REGISTER(VNoise);

