#include "VGetPixel.hpp"
#include <cassert>
FUNCTION_START();



AVSValue __cdecl VGetPixel :: getPixel(AVSValue args, void* user_data, IScriptEnvironment* env) {
	// Get vars
	PClip clip = args[0].AsClip();
	VideoInfo clipInfo = clip->GetVideoInfo();
	int frameId = args[1].AsInt(0);
	int x = args[2].AsInt(0);
	int y = args[3].AsInt(0);
	bool getAlpha = args[4].AsBool(false);

	if (x < 0) x = 0;
	else if (x > clipInfo.width) x = clipInfo.width - 1;
	if (y < 0) y = 0;
	else if (y > clipInfo.height) y = clipInfo.height - 1;

	// Errors
	if (!clipInfo.IsRGB24() && !clipInfo.IsRGB32()) {
		env->ThrowError("getPixel: RGB24/RGB32 data only");
		return AVSValue(0);
	}
	int components = clipInfo.IsRGB32() ? 4 : 3;

	// Get read pointers
	PVideoFrame source = clip->GetFrame(frameId, env);
	const unsigned char* sourcePtr = source->GetReadPtr();
	int sourcePitch = source->GetPitch();

	unsigned int value = 0;
	// RGB
	int i = 0;
	for (; i < 3; ++i) {
		value |= static_cast<unsigned int>(static_cast<unsigned char>(sourcePtr[i + x * components + y * sourcePitch])) << (i * 8);
	}
	// A
	if (getAlpha) {
		if (components == 4) {
			value |= (static_cast<unsigned int>(static_cast<unsigned char>(sourcePtr[i + x * components + y * sourcePitch]))) << (i * 8);
		}
		else {
			value |= 0xFF000000UL;
		}
	}

	// Return the value
	return AVSValue(static_cast<int>(value));
}



const char* VGetPixel :: registerClass(IScriptEnvironment* env) {
	/**
		clip.VGetPixel(...)

		Get the color of a pixel of a video frame at a certain position

		@param clip
			The clip to sample
		@param frame
			The frame of the clip
		@param x
			The x coordinate
		@param y
			The y coordinate
		@param alpha
			True if alpha should be included
			If there is no alpha channel, 255 is used as the alpha
		@return
			A hex value of the pixels color'
			#[AA]RRGGBB

		@default
			clip.VGetPixel( \
				frame = 0, \
				x = 0, \
				y = 0, \
				alpha = false \
			)
	*/
	env->AddFunction(
		"VGetPixel",
		"[clip]c"
		"[frame]i[x]i[y]i[alpha]b",
		VGetPixel::getPixel,
		NULL
	);

	return "VGetPixel";
}



FUNCTION_END();
REGISTER(VGetPixel);


