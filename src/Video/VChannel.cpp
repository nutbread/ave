#include "VChannel.hpp"
#include <cassert>
FUNCTION_START();



// Channel separation class
VChannel :: VChannel(
	IScriptEnvironment* env,
	PClip clip,
	int red,
	int green,
	int blue,
	int hue,
	int saturation,
	int value,
	int fullColor,
	int emptyColor,
	int fullAlpha,
	int emptyAlpha,
	bool normalize,
	bool maximize,
	bool perComponent
) :
	IClip(),
	clip(clip),
	normalize(normalize),
	maximize(maximize),
	perComponent(perComponent),
	normalizeValue(1.0)
{
	// Get info
	this->clipInfo = this->clip->GetVideoInfo();

	// Validation
	if (!this->clipInfo.IsRGB24() && !this->clipInfo.IsRGB32()) {
		env->ThrowError("VChannel: RGB24/RGB32 data only");
	}

	// HSV or RGB?
	this->usingHSV = (hue >= 0 || saturation >= 0 || value >= 0);
	if (this->usingHSV) {
		// HSV
		this->hsv[0] = (hue >= 0 ? hue : 0);
		this->hsv[1] = (saturation >= 0 ? saturation : 255);
		this->hsv[2] = (value >= 0 ? value : 255);

		int rgbColor = VChannel::HSVarrayToRGB(this->hsv);
		red = rgbColor & 0xFF;
		green = (rgbColor >> 8) & 0xFF;
		blue = (rgbColor >> 16) & 0xFF;
	}
	{
		// RGB
		this->rgb[0] = (red >= 0 ? red : 0);
		this->rgb[1] = (green >= 0 ? green : 0);
		this->rgb[2] = (blue >= 0 ? blue : 0);

		if (emptyColor < 0) {
			if (fullColor < 0) {
				emptyColor = 0x000000;
			}
			else {
				// Set it to the target color
				emptyColor = VChannel::RGBarrayToRGB(this->rgb);
			}
		}
		if (fullColor < 0) {
			fullColor = VChannel::RGBarrayToRGB(this->rgb);
		}

		// Set the normalize value
		if (normalize) {
			this->normalizeValue = this->rgb[0] + this->rgb[1] + this->rgb[2];
			if (this->normalizeValue < 1.0) this->normalizeValue = 1.0;

			for (int i = 0; i < 3; ++i) {
				this->rgbDivisor[i] = max(1, this->rgb[i]);
			}
		}
		else {
			this->normalizeValue = 255 * 3;

			for (int i = 0; i < 3; ++i) {
				this->rgbDivisor[i] = 255;
			}
		}
	}


	// Set to defined color
	this->fullColor[2] = (fullColor) & 0xFF;
	this->fullColor[1] = (fullColor >> 8) & 0xFF;
	this->fullColor[0] = (fullColor >> 16) & 0xFF;
	this->fullColor[3] = fullAlpha & 0xFF;

	// Set to defined color
	this->emptyColor[2] = (emptyColor) & 0xFF;
	this->emptyColor[1] = (emptyColor >> 8) & 0xFF;
	this->emptyColor[0] = (emptyColor >> 16) & 0xFF;
	this->emptyColor[3] = emptyAlpha & 0xFF;
}

PVideoFrame __stdcall VChannel :: GetFrame(int n, IScriptEnvironment* env) {
	// Pretty sure the compile optimizes these loops pretty well, so no need to template it and move the if's outwards

	// Destination
	PVideoFrame dest = env->NewVideoFrame(this->clipInfo);
	unsigned char* destPtr = dest->GetWritePtr();
	int destPitch = dest->GetPitch();

	// Get main source
	PVideoFrame source = this->clip->GetFrame(n, env);
	const unsigned char* sourcePtr = source->GetReadPtr();
	union {
		int bgra[4];
		struct {
			int blue;
			int green;
			int red;
			int alpha;
		};
	};

	// Loop values
	int x, y, c, c2;
	int width = this->clipInfo.width;
	int height = this->clipInfo.height;
	bool hasAlpha = this->clipInfo.IsRGB32();
	int components = 3 + hasAlpha;
	double factor, factorInv;

	// Copy from main source
	for (y = 0; y < height; ++y) {
		// Loop over line
		for (x = 0; x < width; ++x) {
			// Get old
			for (c = 0; c < components; ++c) {
				// Normalize
				bgra[c] = *(sourcePtr + c);
			}

			// Modify
			if (this->perComponent) {
				for (c = 0; c < 3; ++c) {
					c2 = 2 - c;

					if (this->maximize) {
						factor = (max(0, this->rgb[c] - (255 - bgra[c2]))) / this->rgbDivisor[c];
					}
					else {
						factor = ((this->rgb[c] - max(0, this->rgb[c] - bgra[c2]))) / this->rgbDivisor[c];
					}
					factorInv = 1.0 - factor;

					bgra[c2] = static_cast<int>(this->fullColor[c] * factor) + static_cast<int>(this->emptyColor[c] * factorInv);
				}
			}
			else {
				if (this->maximize) {
					factor = (
						max(0, this->rgb[0] - (255 - red)) +
						max(0, this->rgb[1] - (255 - green)) +
						max(0, this->rgb[2] - (255 - blue))
					) / this->normalizeValue;
				}
				else {
					factor = (
						(this->rgb[0] - max(0, this->rgb[0] - red)) +
						(this->rgb[1] - max(0, this->rgb[1] - green)) +
						(this->rgb[2] - max(0, this->rgb[2] - blue))
					) / this->normalizeValue;
				}
				factorInv = 1.0 - factor;

				red = static_cast<int>(this->fullColor[0] * factor) + static_cast<int>(this->emptyColor[0] * factorInv);
				green = static_cast<int>(this->fullColor[1] * factor) + static_cast<int>(this->emptyColor[1] * factorInv);
				blue = static_cast<int>(this->fullColor[2] * factor) + static_cast<int>(this->emptyColor[2] * factorInv);
				if (hasAlpha) alpha = static_cast<int>(this->fullColor[3] * factor) + static_cast<int>(this->emptyColor[3] * factorInv);
			}

			// Set new
			for (c = 0; c < components; ++c) {
				*(destPtr + c) = bgra[c];
			}

			// Continue
			destPtr += components;
			sourcePtr += components;
		}
	}

	// Done
	return dest;
}
void __stdcall VChannel :: GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
	this->clip->GetAudio(buf, start, count, env);
}
const VideoInfo& __stdcall VChannel :: GetVideoInfo() {
	return this->clipInfo;
}
bool __stdcall VChannel :: GetParity(int n) {
	return this->clip->GetParity(n);
}
CacheHintsReturnType __stdcall VChannel :: SetCacheHints(int cachehints, int frame_range) {
	// Do not pass cache requests upwards, only to the next filter
	return CacheHintsReturn(0);
}

int VChannel :: bound(int value, int vmin, int vmax) {
	if (value < vmin) return vmin;
	if (value > vmax) return vmax;
	return value;
}
int VChannel :: loop(int value, int range) {
	assert(range > 0);

	while (value < 0) value += range;
	return value % range;
}
int VChannel :: HSVarrayToRGB(int* hsv) {
	assert(hsv != NULL);

	unsigned char rgb[3];
	int hue = hsv[0];

//	unsigned char vMin = hsv[2] - static_cast<unsigned char>((hsv[2] / 255.0 * hsv[1] / 255.0) * 255);
	unsigned char vMin = hsv[2] - ((hsv[2] * hsv[1]) / 255);
	unsigned char vMax = hsv[2];
	unsigned char vSpan = vMax - vMin;

	if (hue < 256 * 1) {
		rgb[0] = vMax;
		rgb[1] = vMin + ((hue - 0) * vSpan) / 255;
		rgb[2] = vMin;
	}
	else if (hue < 256 * 2) {
		rgb[0] = vMax - ((hue - 256) * vSpan) / 255;
		rgb[1] = vMax;
		rgb[2] = vMin;
	}
	else if (hue < 256 * 3) {
		rgb[0] = vMin;
		rgb[1] = vMax;
		rgb[2] = vMin + ((hue - 256 * 2) * vSpan) / 255;
	}
	else if (hue < 256 * 4) {
		rgb[0] = vMin;
		rgb[1] = vMax - ((hue - 256 * 3) * vSpan) / 255;
		rgb[2] = vMax;
	}
	else if (hue < 256 * 5) {
		rgb[0] = vMin + ((hue - 256 * 4) * vSpan) / 255;
		rgb[1] = vMin;
		rgb[2] = vMax;
	}
	else {
		rgb[0] = vMax;
		rgb[1] = vMin;
		rgb[2] = vMax - ((hue - 256 * 5) * vSpan) / 255;
	}

	return rgb[0] | (rgb[1] << 8) | (rgb[2] << 16);
}
int VChannel :: RGBarrayToRGB(int* rgb) {
	assert(rgb != NULL);

	return rgb[0] | (rgb[1] << 8) | (rgb[2] << 16);
}



AVSValue __cdecl VChannel :: create1(AVSValue args, void* user_data, IScriptEnvironment* env) {
	// VChannel
	return static_cast<AVSValue>(new VChannel(
		env,
		args[0].AsClip(), // clip
		args[1].Defined() ? VChannel::bound(args[1].AsInt(0), 0, 255) : -1, // red
		args[2].Defined() ? VChannel::bound(args[2].AsInt(0), 0, 255) : -1, // green
		args[3].Defined() ? VChannel::bound(args[3].AsInt(0), 0, 255) : -1, // blue
		args[4].Defined() ? VChannel::loop(args[4].AsInt(0), 256 * 6) : -1, // hue
		args[5].Defined() ? VChannel::bound(args[5].AsInt(0), 0, 255) : -1, // saturation
		args[6].Defined() ? VChannel::bound(args[6].AsInt(0), 0, 255) : -1, // value
		args[7].AsInt(-1), // color_full
		args[8].AsInt(-1), // color_empty
		args[9].AsInt(255), // alpha_full
		args[10].AsInt(255), // alpha_empty
		args[11].AsBool(true), // normalize
		args[12].AsBool(false), // maximize
		args[13].AsBool(false) // per_component
	));
}
const char* VChannel :: registerClass(IScriptEnvironment* env) {
	/**
		clip.VChannel(...)

		Extract a color channel from a video.
		This can be something such as the red (#ff0000), green (#00ff00), or blue (#0000ff) channel,
			or something more complex like an "orange" (#ff8000) channel

		@param clip
			The clip to modify
		@param red
			The red component of the channel to extract
		@param green
			The green component of the channel to extract
		@param blue
			The blue component of the channel to extract
		@param hue
			The hue of the channel to extract
		@param saturation
			The saturation of the channel to extract
		@param value
			The value of the channel to extract
		@note
			If hue, saturation, or value is defined, any red/green/blue values are ignored
		@param color_full
			The color to use if the distance factor is minimum
			If set to -1, it assumes the value of the defined [red,green,blue] or [hue,saturation,value] color
		@param color_empty
			The color to use if the distance factor is maximum
			If set to -1, it assumes the value of the defined [red,green,blue] or [hue,saturation,value] color
		@param alpha_full
			The alpha to use if the distance factor is minimum
		@param alpha_empty
			The alpha to use if the distance factor is maximum
		@param normalize
			Normalize the distance factor to the range [0,1]
		@param maximize
			If true, the following equation is used to calculate the distance factor:
				max(0, red - (255 - pixel.red))
				... (similar for green and blue)
			If false:
				(red - max(0, red - pixel.red))
				... (similar for green and blue)
		@param per_component
			Apply on a per-component basis if true
		@return
			An extracted color channel video clip

		@default
			clip.VChannel( \
				red = 0, \
				green = 0, \
				blue = 0, \
				hue = 0, \
				saturation = 0, \
				value = 0, \
				color_full = -1, \
				color_empty = -1, \
				alpha_full = 255, \
				alpha_empty = 255, \
				normalize = true, \
				maximize = false, \
				per_component = false \
			)
	*/
	env->AddFunction(
		"VChannel",
		"[clip]c"
		"[red]i[green]i[blue]i"
		"[hue]i[saturation]i[value]i"
		"[color_full]i[color_empty]i[alpha_full]i[alpha_empty]i"
		"[normalize]b[maximize]b[per_component]b",
		VChannel::create1,
		NULL
	);

	return "VChannel";
}



FUNCTION_END();
REGISTER(VChannel);


