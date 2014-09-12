#include "AVideoToWaveform.hpp"
#include <cassert>
FUNCTION_START();



AVideoToWaveform :: AVideoToWaveform(
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
) :
	IClip(),
	clip(clip),
	clipInfo(),
	pixelSampleSize(pixelSampleSize),
	blockSampleSize(blockSampleSize),
	blockSamplePeriod(blockSamplePeriod),
	blockWaveform(TRIANGLE),
	loopNearest(loopNearest),
	channelSplit(channelSplit),
	regions(10, NULL),
	emptyValue(0.0f)
{
	this->clipInfo = this->clip->GetVideoInfo();

	// Settings
	if (!this->clipInfo.HasVideo()) {
		env->ThrowError("AVideoToWaveform: Video missing");
	}
	if (!this->clipInfo.IsRGB24() && !this->clipInfo.IsRGB32()) {
		env->ThrowError("AVideoToWaveform: RGB24/RGB32 data only");
	}

	// Set vars
	if (this->pixelSampleSize < 1) this->pixelSampleSize = 1;
	if (this->blockSampleSize < 1) this->blockSampleSize = 1;
	if (this->blockSamplePeriod < 1) this->blockSamplePeriod = 1;
	this->threshold[0] = max(1, thresholdRed);
	this->threshold[1] = max(1, thresholdGreen);
	this->threshold[2] = max(1, thresholdBlue);
	std::string s = blockWaveform;
	for (int i = 0; i < s.length(); ++i) {
		if (s[i] >= 'A' && s[i] <= 'Z') {
			s[i] -= 'A' - 'a';
		}
	}
	if (s == "saw" || s == "sawtooth") this->blockWaveform = SAWTOOTH;
	else if (s == "sqr" || s == "square") this->blockWaveform = SQUARE;
	else if (s == "sin" || s == "sine") this->blockWaveform = SINE;
	else if (s == "flat") this->blockWaveform = FLAT;
	// else, TRIANGLE

	// Modify clipInfo
	if (sampleRate > 0) {
		this->clipInfo.audio_samples_per_second = sampleRate;
	}
	else if (this->clipInfo.audio_samples_per_second == 0) {
		this->clipInfo.audio_samples_per_second = 44100;
	}
	if (channels > 0) {
		this->clipInfo.nchannels = channels;
	}
	else if (this->clipInfo.nchannels == 0) {
		this->clipInfo.nchannels = 2;
	}
	this->clipInfo.sample_type = SAMPLE_FLOAT;
	assert(this->clipInfo.SampleType() == SAMPLE_FLOAT);

	// Set sample count
	this->clipInfo.num_audio_samples = static_cast<__int64>(this->clipInfo.width) * static_cast<__int64>(this->clipInfo.num_frames) * static_cast<__int64>(this->pixelSampleSize);
}

double AVideoToWaveform :: getWaveform(__int64 position) const {
	position = position % this->blockSamplePeriod;

	switch (this->blockWaveform) {
		case TRIANGLE:
		{
			int half = this->blockSamplePeriod / 2;
			return (position <= half) ?
				position / static_cast<double>(half) : // increasing
				(this->blockSamplePeriod - position) / static_cast<double>(half); // decreasing
		}
		case SAWTOOTH:
			return position / static_cast<double>(this->blockSamplePeriod);
		case SQUARE:
			return (position <= this->blockSamplePeriod / 2) ? 0.0 : 1.0;
		case SINE:
			return ::cos(position / static_cast<double>(this->blockSamplePeriod) * M_PI * 2.0) / -2.0 + 0.5;
		case FLAT:
			return 0.5;
	}

	return 0.0;
}
void AVideoToWaveform :: clearRegions() {
	for (size_t i = 0; i < this->regions.size(); ++i) {
		delete this->regions[i];
	}
	this->regions.clear();
}

PVideoFrame __stdcall AVideoToWaveform :: GetFrame(int n, IScriptEnvironment* env) {
	return this->clip->GetFrame(n, env);
}
void __stdcall AVideoToWaveform :: GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
	// Get float buffer
	SFLOAT* buffer = static_cast<SFLOAT*>(buf);
	int channels = this->clipInfo.AudioChannels();
	int components = this->clipInfo.IsRGB32() ? 4 : 3;
	int w = this->clipInfo.width;
	int h = this->clipInfo.height;
	int f, x, c, y, i;
	int bgra[4];
	double rangeMax = h / 2.0;
	double rangeMaxHalf = h;
	double range[2];
	double channelRange[2];
	double subRange[2];
	Region* region = NULL;
	int* regionIdMin = NULL;
	int* regionIdMax = NULL;
	if (this->channelSplit) {
		regionIdMin = new int[channels];
		regionIdMax = new int[channels];
	}

	// Get the pixel
	int frameStart = start / (this->pixelSampleSize * w);
	int frameEnd = (start + count) / (this->pixelSampleSize * w);
	__int64 s = start % this->pixelSampleSize;
	__int64 position = start;
	__int64 positionEnd = start + count;

	// Loop across frames
	for (f = frameStart; f <= frameEnd; ++f) {
		// Get the pixel loop region for this frame
		int pixelStart = max(f * w, start / this->pixelSampleSize);
		int pixelCount = ((start + count) / this->pixelSampleSize) - pixelStart;

		pixelStart = pixelStart % w;
		if (pixelStart + pixelCount >= w) pixelCount = w - pixelStart - 1;
		pixelCount += pixelStart;

		// Get image pointers
		PVideoFrame source = this->clip->GetFrame(f, env);
		const unsigned char* sourcePtr = source->GetReadPtr();
		int sourcePitch = source->GetPitch();

		// Loop across pixels
		for (x = pixelStart; x <= pixelCount; ++x) {
			// Enumerate regions that need drawing
			this->clearRegions();
			bool in_dark = false, pixel_dark;
			for (y = 0; y < h; ++y) {
				// Get pixel color
				for (c = 0; c < 3; ++c) {
					bgra[c] = sourcePtr[sourcePitch * y + x * components + c];
				}

				// Test
				pixel_dark = (bgra[0] < this->threshold[2] && bgra[1] < this->threshold[1] && bgra[2] < this->threshold[0]);
				if (in_dark != pixel_dark) {
					in_dark = pixel_dark;
					if (in_dark) {
						// Add a region
						region = new Region();
						region->yTop = y;
						region->yBottom = h;
						this->regions.push_back(region);
					}
					else {
						// Assign end pixel
						region->yBottom = y;
					}
				}
			}

			// Enumerate regions for channels
			if (this->channelSplit) {
				for (c = 0; c < channels; ++c) {
					regionIdMin[c] = this->regions.size();
					regionIdMax[c] = 0;
					for (i = 0; i < this->regions.size(); ++i) {
						channelRange[0] = (channels - c - 1) * h / static_cast<double>(channels);
						channelRange[1] = (channels - c - 0) * h / static_cast<double>(channels);
						if (
							this->regions[i]->yTop <= channelRange[1] &&
							this->regions[i]->yBottom >= channelRange[0]
						) {
							if (i < regionIdMin[c]) regionIdMin[c] = i;
							if (i + 1 > regionIdMax[c]) regionIdMax[c] = i + 1;
						}
					}
				}
			}

			// Loop across sample blocks
			for (; s < this->pixelSampleSize; ++s) {
				if (this->channelSplit) {
					// Loop across channels
					for (c = 0; c < channels; ++c) {
						if (regionIdMax[c] - regionIdMin[c] > 0) {
							int regionId = ((position / this->blockSampleSize) % (regionIdMax[c] - regionIdMin[c])) + regionIdMin[c];
							range[0] = this->regions[regionId]->yTop / rangeMaxHalf;
							range[1] = this->regions[regionId]->yBottom / rangeMaxHalf;

							// Assign as value
							channelRange[0] = (channels - c - 1) / static_cast<double>(channels);
							channelRange[1] = (channels - c - 0) / static_cast<double>(channels);
							for (i = 0; i < 2; ++i) {
								subRange[i] = range[i];
								if (subRange[i] < channelRange[0]) subRange[i] = channelRange[0];
								else if (subRange[i] > channelRange[1]) subRange[i] = channelRange[1];

								subRange[i] = (subRange[i] - channelRange[0]) * channels * 2.0 - 1.0;
							}
							*(buffer) = this->getWaveform(position) * (subRange[1] - subRange[0]) + subRange[0];

						}
						else {
							// Assign as 0 value
							if ((channels % 2) == 0 || c != channels / 2) {
								// Outer tracks
								*(buffer) = ((c < channels / 2) ? -1.0 : 1.0);
							}
							else {
								// Center track
								*(buffer) = 0.0;
							}
						}

						// Next
						++buffer;
					}
				}
				else {
					if (this->regions.size() > 0) {
						int regionId = (position / this->blockSampleSize) % this->regions.size();
						range[0] = this->regions[regionId]->yTop / rangeMax - 1.0;
						range[1] = this->regions[regionId]->yBottom / rangeMax - 1.0;

						for (c = 0; c < channels; ++c) {
							// Assign as value
							*(buffer) = this->getWaveform(position) * (range[1] - range[0]) + range[0];

							// Next
							++buffer;
						}
					}
					else {
						for (c = 0; c < channels; ++c) {
							// Assign as 0
							*(buffer) = this->emptyValue;

							// Next
							++buffer;
						}
					}
				}

				++position;
				if (position >= positionEnd) break;
			}
			s = 0;
			if (position >= positionEnd) break;
		}
		if (position >= positionEnd) break;
	}

	// Cleanup
	delete regionIdMin;
	delete regionIdMax;
}
const VideoInfo& __stdcall AVideoToWaveform :: GetVideoInfo() {
	return this->clipInfo;
}
bool __stdcall AVideoToWaveform :: GetParity(int n) {
	return this->clip->GetParity(n);
}
CacheHintsReturnType __stdcall AVideoToWaveform :: SetCacheHints(int cachehints, int frame_range) {
	// Do not pass cache requests upwards, only to the next filter
	return CacheHintsReturn(0);
}



AVSValue __cdecl AVideoToWaveform :: create1(AVSValue args, void* user_data, IScriptEnvironment* env) {
	// Setup arguments
	PClip clip = args[0].AsClip();

	int sampleRate = args[1].AsInt(0);
	int channels = args[2].AsInt(0);

	bool loopNearest = false;
	bool channelSplit = args[3].AsBool(true);

	int pixelSampleSize = args[4].AsInt(128);
	int blockSampleSize = args[5].AsInt(4);
	int blockSamplePeriod = args[6].AsInt(4);
	const char* blockWaveform = args[7].AsString("");

	int thresholdRed = args[8].AsInt(128);
	int thresholdGreen = args[9].AsInt(thresholdRed);
	int thresholdBlue = args[10].AsInt(thresholdGreen);

	// Return
	return static_cast<AVSValue>(new AVideoToWaveform(
		env,
		clip,

		sampleRate,
		channels,

		pixelSampleSize,
		blockSampleSize,
		blockSamplePeriod,
		blockWaveform,

		loopNearest,
		channelSplit,

		thresholdRed,
		thresholdGreen,
		thresholdBlue
	));
}
const char* AVideoToWaveform :: registerClass(IScriptEnvironment* env) {
	/**
		clip.AVideoToWaveform(...)

		Convert an image/video frame into an audio waveform.
		When opened in an audio viewer, the audio waveform should resemble the original image.
		(Black and white images work best)

		@param clip
			The image to sample
		@param sample_rate
			The rate of the resulting audio
		@param channels
			The number of channels in the resulting audio
		@param channel_split
			True if the resulting waveform image should be split across the total channels
			False if it should be replicated on each channel
		@param pixel_sample_size
			The number of audio samples that will form a single pixel
		@param block_sample_size
			The number of samples that form a "block"
			A "block" is a vertical section of audio which is used when multiple pixels exist in the same vertical scanline
		@param block_sample_period
			The number of samples that form a "block"
		@param waveform
			The waveform to use for the audio
			Available values are:
				"TRIANGLE"
				"SAWTOOTH"
				"SQUARE"
				"SINE"
				"FLAT"
		@param threshold_red
			The cutoff level for a color to be considered "black", and thus used as an audio block (red component)
			Range: [0,255]
		@param threshold_green
			The cutoff level for a color to be considered "black", and thus used as an audio block (green component)
			Range: [0,255]
		@param threshold_blue
			The cutoff level for a color to be considered "black", and thus used as an audio block (blue component)
			Range: [0,255]
		@return
			The video + a new audio track

		@default
			clip.AVideoToWaveform( \
				sample_rate = clip.AudioRate (or 44100 if no audio), \
				channels = clip.AudioChannels (or 2 if no audio), \
				channel_split = true, \
				pixel_sample_size = 128, \
				block_sample_size = 4, \
				block_sample_period = 4, \
				waveform = "TRIANGLE", \
				threshold_red = 128, \
				threshold_green = threshold_red, \
				threshold_blue = threshold_red \
			) \
	*/
	env->AddFunction(
		"AVideoToWaveform",
		"[clip]c"
		"[sample_rate]i[channels]i[channel_split]b"
		"[pixel_sample_size]i[block_sample_size]i[block_sample_period]i[waveform]s"
		"[threshold_red]i[threshold_green]i[threshold_blue]i",
		AVideoToWaveform::create1,
		NULL
	);

	return "AVideoToWaveform";
}



FUNCTION_END();
REGISTER(AVideoToWaveform);


