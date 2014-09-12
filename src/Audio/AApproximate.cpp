#include "AApproximate.hpp"
#include <cassert>
FUNCTION_START();



// Clip blending
AApproximate :: AApproximate(
	IScriptEnvironment* env,
	PClip clip,

	const char* waveform,
	bool crossAtSame0Point,

	double tolerance,
	double absoluteToleranceFactor,

	__int64 interpolationSize,
	__int64 interpolationSamples
) :
	IClip(),
	clip(clip),
	isSetup(false),
	crossAtSame0Point(crossAtSame0Point),
	waveform(SQUARE),
	tolerance(tolerance),
	absoluteToleranceFactor(absoluteToleranceFactor), // 1.0 = absolute, 0.0 = relative to previous peak
	interpolationSize(interpolationSize),
	interpolationSamples(interpolationSamples),
	channelGraphs(NULL)
{
	// Get info
	this->clipInfo = this->clip->GetVideoInfo();

	// Validation
	if (
		this->clipInfo.SampleType() != SAMPLE_INT8 &&
		this->clipInfo.SampleType() != SAMPLE_INT16 &&
		this->clipInfo.SampleType() != SAMPLE_INT32 &&
		this->clipInfo.SampleType() != SAMPLE_FLOAT
	) {
		env->ThrowError("AApproximate: 24bit audio not supported");
	}

	// Settings
	std::string s;
	AApproximate::cStringToLower(waveform, s);
	if (s == "tri" || s == "triangle" || s == "line" || s == "linear") this->waveform = TRIANGLE;
	else if (s == "sin" || s == "sine") this->waveform = SINE;
}
AApproximate :: ~AApproximate() {
	if (this->channelGraphs != NULL) {
		int channels = this->clipInfo.AudioChannels();
		for (int c = 0; c < channels; ++c) {
			// Delete lists
			while (this->channelGraphs[c].first != NULL) {
				this->channelGraphs[c].last = this->channelGraphs[c].first->after;
				delete this->channelGraphs[c].first;
				this->channelGraphs[c].first = this->channelGraphs[c].last;
			}
		}
		// Delete graphs
		delete [] this->channelGraphs;
	}
}

PVideoFrame __stdcall AApproximate :: GetFrame(int n, IScriptEnvironment* env) {
	return this->clip->GetFrame(n, env);
}
void __stdcall AApproximate :: GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
	if (!this->isSetup) {
		this->setup(env);
		this->isSetup = true;
	}

	// Get audio
	int channels = this->clipInfo.AudioChannels();
	float* floatAudio = new float[count * channels];

	// Modify
	for (__int64 i = 0; i < count * channels; ++i) floatAudio[i] = -1.0;
	this->modifyAudio(floatAudio, start, count, env);

	// Return
	if (this->clipInfo.SampleType() == SAMPLE_INT8) {
		this->convertFloatToAudio<__int8>(this->clip, &this->clipInfo, floatAudio, static_cast<__int8*>(buf), count, env, (1 << (sizeof(__int8) * 8 - 1)), (1 << (sizeof(__int8) * 8 - 1)) - 1);
	}
	else if (this->clipInfo.SampleType() == SAMPLE_INT16) {
		this->convertFloatToAudio<__int16>(this->clip, &this->clipInfo, floatAudio, static_cast<__int16*>(buf), count, env, (1 << (sizeof(__int16) * 8 - 1)), (1 << (sizeof(__int16) * 8 - 1)) - 1);
	}
	else if (this->clipInfo.SampleType() == SAMPLE_INT32) {
		this->convertFloatToAudio<__int32>(this->clip, &this->clipInfo, floatAudio, static_cast<__int32*>(buf), count, env, (1 << (sizeof(__int32) * 8 - 1)), (1 << (sizeof(__int32) * 8 - 1)) - 1);
	}
	else if (this->clipInfo.SampleType() == SAMPLE_FLOAT) {
		this->convertFloatToAudio<SFLOAT>(this->clip, &this->clipInfo, floatAudio, static_cast<SFLOAT*>(buf), count, env, -1.0, 1.0);
	}

	// Clear
	delete [] floatAudio;
}
const VideoInfo& __stdcall AApproximate :: GetVideoInfo() {
	return this->clipInfo;
}
bool __stdcall AApproximate :: GetParity(int n) {
	return this->clip->GetParity(n);
}
CacheHintsReturnType __stdcall AApproximate :: SetCacheHints(int cachehints, int frame_range) {
	// Do not pass cache requests upwards, only to the next filter
	return CacheHintsReturn(0);
}

float* AApproximate :: getFloatAudio(PClip clip, VideoInfo* clipInfo, __int64 start, __int64 count, IScriptEnvironment* env) {
	float* floatAudio = NULL;

	if (clipInfo->SampleType() == SAMPLE_INT8) {
		floatAudio = AApproximate::convertAudioToFloat<__int8>(clip, clipInfo, start, count, env, (1 << (sizeof(__int8) * 8 - 1)), (1 << (sizeof(__int8) * 8 - 1)) - 1);
	}
	else if (clipInfo->SampleType() == SAMPLE_INT16) {
		floatAudio = AApproximate::convertAudioToFloat<__int16>(clip, clipInfo, start, count, env, (1 << (sizeof(__int16) * 8 - 1)), (1 << (sizeof(__int16) * 8 - 1)) - 1);
	}
	else if (clipInfo->SampleType() == SAMPLE_INT32) {
		floatAudio = AApproximate::convertAudioToFloat<__int32>(clip, clipInfo, start, count, env, (1 << (sizeof(__int32) * 8 - 1)), (1 << (sizeof(__int32) * 8 - 1)) - 1);
	}
	else if (clipInfo->SampleType() == SAMPLE_FLOAT) {
		floatAudio = AApproximate::convertAudioToFloat<SFLOAT>(clip, clipInfo, start, count, env, -1.0, 1.0);
	}

	return floatAudio;
}
template <class Type> float* AApproximate :: convertAudioToFloat(PClip clip, VideoInfo* clipInfo, __int64 start, __int64 count, IScriptEnvironment* env, Type min, Type max) {
	// Get source audio
	int channels = clipInfo->AudioChannels();

	// Create buffer
	__int64 bufferSize = count * channels;
	Type* bufferNew = new Type[bufferSize];

	// Fill buffer
	__int64 firstSample = min(clipInfo->num_audio_samples, max(0, start));
	__int64 lastSample = min(clipInfo->num_audio_samples, start + count);
	__int64 pos = (firstSample - start) * channels;
	clip->GetAudio(static_cast<void*>(&(bufferNew[pos])), firstSample, lastSample - firstSample, env);
	//for (__int64 i = 0; i < pos; ++i) bufferNew[i] = 0; // before
	//for (__int64 i = (lastSample - firstSample) * channels; i < bufferSize; ++i) bufferNew[i] = 0; // after



	// Create new audio
	float* newAudio = new float[bufferSize];
	float* newAudioPos = newAudio;

	// Floating point max/min
	float fmin = -static_cast<float>(min);
	float fmax = static_cast<float>(max);

	// Copy
	float f;
	Type* buf = bufferNew;
	for (__int64 i = 0; i < bufferSize; ++i) {
		// Copy and convert data
		f = *buf / ((*buf < 0) ? fmin : fmax);

		// Set new
		*newAudioPos = f;

		// Continue
		++newAudioPos;
		++buf;
	}



	// Delete temp buffer
	delete [] bufferNew;



	// Done
	return newAudio;
}
template <class Type> void AApproximate :: convertFloatToAudio(PClip clip, VideoInfo* clipInfo, float* floatAudio, Type* target, __int64 count, IScriptEnvironment* env, Type min, Type max) {
	// Channel count
	int channels = clipInfo->AudioChannels();

	// Loop
	__int64 i = 0 * channels;
	__int64 iMax = (count + 0) * channels;

	// Float audio pointer
	float* newAudioPos = &(floatAudio[i]);

	// Floating point max/min
	double fmin = -static_cast<double>(min);
	double fmax = static_cast<double>(max);

	// Copy
	double f;
	for (; i < iMax; ++i) {
		// Copy and convert data
		f = *newAudioPos * ((*newAudioPos < 0) ? fmin : fmax);

		// Set new
		*target = static_cast<Type>(f);

		// Continue
		++newAudioPos;
		++target;
	}

	// Done
}

void AApproximate :: setup(IScriptEnvironment* env) {
	// Setup
	int channels = this->clipInfo.AudioChannels();
	this->channelGraphs = new ChannelGraph[channels];


	int s;
	float v;
	__int64 pos, posEnd;
	Region* reg;

	__int64 totalSamples = this->clipInfo.num_audio_samples;
	__int64 bufferStart, bufferSize;
	float* audioBuffer;

	double tolerance = this->tolerance;
	double absoluteToleranceFactor = this->absoluteToleranceFactor; // 1.0 = absolute, 0.0 = relative to previous peak
	double absoluteToleranceFactorInv = 1.0 - absoluteToleranceFactor;
	float previousPeak;

	// Setup each channel
	for (int c = 0; c < channels; ++c) {
		// Setup first
		bufferStart = 0;
		bufferSize = min(128 * 128, totalSamples - bufferStart);
		audioBuffer = AApproximate::getFloatAudio(this->clip, &this->clipInfo, bufferStart, bufferSize, env);

		pos = 0;
		previousPeak = 0.0;
		v = audioBuffer[pos * channels + c];
		s = AApproximate::sign(v);
		reg = new Region(pos, s, NULL, NULL);
		this->channelGraphs[c].first = reg;

		// Look forwards
		++pos;
		while (true) {
			posEnd = bufferStart + bufferSize;
			for (; pos < posEnd; ++pos) {
				// Get the value and sign
				v = audioBuffer[(pos - bufferStart) * channels + c];
				s = AApproximate::sign(v + previousPeak);

				// Mismatch?
				if (s != reg->sign) {
					// Finish
					reg->position[1] = pos;
					previousPeak = tolerance * (absoluteToleranceFactor + reg->max * absoluteToleranceFactorInv) * s;

					// Create new
					reg->after = new Region(pos, s, reg, NULL);
					reg = reg->after;
				}

				// Max
				v *= s;
				if (v > reg->max) {
					reg->max = v;
					reg->maxPos = pos;
				}
			}

			// Loop condition
			bufferStart += bufferSize;
			if (bufferStart >= totalSamples) break;

			// Get more audio
			delete [] audioBuffer;
			bufferSize = min(128 * 128, totalSamples - bufferStart);
			audioBuffer = AApproximate::getFloatAudio(this->clip, &this->clipInfo, bufferStart, bufferSize, env);
		}

		// Set last
		reg->position[1] = pos;
		this->channelGraphs[c].last = reg;

		// Clean
		delete [] audioBuffer;
	}


	// Interpolate?
	__int64 interpSize = this->interpolationSize;
	__int64 interpSamples = this->interpolationSamples;
	if (interpSize > 1 || interpSamples > 1) {
		Region* reg2;
		Region* regPre;
		int i;
		__int64 peakPosSum;
		double peakSum;
		__int64 range;

		// Interpolate
		for (int c = 0; c < channels; ++c) {
			reg = this->channelGraphs[c].first;

			while (reg != NULL) {
				// Decrease
				reg2 = reg;

				// First
				i = 1;
				peakPosSum = reg2->maxPos;
				peakSum = reg2->max;
				range = reg2->position[1] - reg2->position[0];

				// Additional
				if (reg2->sign == 0) {
					// Next
					reg2 = reg2->after;
				}
				else {
					// Next
					reg2 = reg2->after;
					if (
						reg2 != NULL &&
						(interpSamples <= 1 || range < interpSamples)
					) {
						while (true) {
							// Sign=0: do nothing
							if (reg2->sign == 0) break;

							// Update
							peakPosSum += reg2->maxPos;
							peakSum += reg2->max;
							range += reg2->position[1] - reg2->position[0];

							// Next
							++i;
							regPre = reg2;
							reg2 = reg2->after;
							delete regPre; // delete
							if (reg2 == NULL) break;

							// Loop check
							if (interpSamples > 1 && range >= interpSamples) break;
							if (interpSize > 1 && i >= interpSize) break;
						}
					}
				}

				// Modify?
				if (i > 1) {
					// Modify
					reg->max = peakSum / i;
					reg->maxPos = peakPosSum / i;
					if (reg->before != NULL && reg->before->sign != 0) {
						reg->sign = -reg->before->sign;
					}

					reg->after = reg2;
					if (reg2 == NULL) {
						reg->position[1] = regPre->position[1];
					}
					else {
						reg->position[1] = reg2->position[0];
						reg2->before = reg;
					}
				}
				else {
					// Make sign inverted from previous
					if (reg->before != NULL && reg->before->sign != 0) {
						reg->sign = -reg->before->sign;
					}
				}
				reg = reg2;
			}
		}
	}
}
void AApproximate :: modifyAudio(float* target, __int64 start, __int64 count, IScriptEnvironment* env) {
	int channels = this->clipInfo.AudioChannels();
	Region* r;

	// Variables
	__int64 pos1, pos2, p, zeroPos, posRange, posEnd = start + count;
	__int64 peakPos[2];
	float peak[2];
	bool crossAtSame0Point;

	for (int c = 0; c < channels; ++c) {
		r = this->channelGraphs[c].first;

		// Skip to the first one that contains a region
		if (r->maxPos < start) {
			while (true) {
				// No next
				if (r->after == NULL) {
					// Peaks
					peak[0] = r->max * r->sign;
					peakPos[0] = r->maxPos;
					peak[1] = 0.0; // Might not be correct in some cases
					peakPos[1] = posEnd;
					zeroPos = peakPos[0];
					crossAtSame0Point = false;
					r = NULL;
					break;
				}

				// Next
				r = r->after;

				// End?
				if (r->maxPos >= start) {
					// Peaks
					peak[0] = r->before->max * r->before->sign;
					peakPos[0] = r->before->maxPos;
					peak[1] = r->max * r->sign;
					peakPos[1] = r->maxPos;
					zeroPos = r->position[0];
					crossAtSame0Point = this->crossAtSame0Point;
					break;
				}
			}
		}
		else {
			// Peaks
			peak[0] = 0.0; // Might not be correct in some cases
			peakPos[0] = 0;
			peak[1] = r->max * r->sign;
			peakPos[1] = r->maxPos;
			zeroPos = 0;
			crossAtSame0Point = false;
		}


		// Initial limits
		pos1 = peakPos[0];
		if (pos1 < start) pos1 = start;
		pos2 = peakPos[1];
		if (pos2 > posEnd) pos2 = posEnd;

		// Loop
		while (true) {
			// More vars
			posRange = peakPos[1] - peakPos[0];
			zeroPos -= peakPos[0];

			// Set audio
			for (p = pos1; p < pos2; ++p) {
				target[(p - start) * channels + c] = this->getWaveform(p - peakPos[0], posRange, zeroPos, peak[0], peak[1], crossAtSame0Point);
			}

			// Next
			if (pos2 >= posEnd) break;

			r = r->after;
			peak[0] = peak[1];
			peakPos[0] = peakPos[1];

			// No next
			if (r == NULL) {
				// Peaks
				peak[1] = 0.0; // Might not be correct in some cases
				peakPos[1] = posEnd;
				zeroPos = peakPos[0];
				crossAtSame0Point = false;
			}
			else {
				// Peaks
				peak[1] = r->max * r->sign;
				peakPos[1] = r->maxPos;
				zeroPos = r->position[0];
				crossAtSame0Point = this->crossAtSame0Point;
			}

			// Positions
			pos1 = peakPos[0];
			pos2 = peakPos[1];
			if (pos2 > posEnd) pos2 = posEnd;
		}
	}
}

void AApproximate :: cStringToLower(const char* cstr, std::string& target) {
	assert(cstr != NULL);

	target = cstr;
	for (size_t i = 0; i < target.length(); ++i) {
		if (target[i] >= 'A' && target[i] <= 'Z') {
			target[i] -= 'A' - 'a';
		}
	}
}
double AApproximate :: getWaveform(__int64 position, __int64 range, __int64 midpoint, double low, double high, bool crossAtSame0Point) {
	if (crossAtSame0Point) {
		switch (this->waveform) {
			case TRIANGLE:
				return (position < midpoint) ?
					((midpoint - position) / static_cast<double>(midpoint)) * low :
					((position - midpoint) / static_cast<double>(range - midpoint)) * high;
			case SQUARE:
				return (position < midpoint) ? low : high;
			case SINE:
			{
				if (position < midpoint) {
					// First half
					return ::cos(position * M_PI / (midpoint * 2)) * low;
				}
				else {
					// Second half
					range -= midpoint;
					return -::cos((position - midpoint + range) * M_PI / (range * 2)) * high;
				}
			}
		}
	}
	else {
		switch (this->waveform) {
			case TRIANGLE:
				return (position / static_cast<double>(range)) * (high - low) + low;
			case SQUARE:
				return (position < range / 2) ? low : high;
			case SINE:
				return ((1.0 - ::cos(position * M_PI / range)) / 2.0) * (high - low) + low;
		}
	}

	return 0.0;
}



AVSValue __cdecl AApproximate :: create1(AVSValue args, void* user_data, IScriptEnvironment* env) {
	// Setup arguments
	PClip clip = args[0].AsClip();

	const char* waveform = args[1].AsString("");
	bool crossAtSame0Point = args[2].AsBool(true);

	double tolerance = args[3].AsFloat(0.0);
	double absoluteToleranceFactor = args[4].AsFloat(0.0);

	__int64 interpolationSize = args[5].AsInt(1);
	__int64 interpolationSamples = args[6].AsInt(0);

	// Return
	return static_cast<AVSValue>(new AApproximate(
		env,
		clip,

		waveform,
		crossAtSame0Point,
		tolerance,
		absoluteToleranceFactor,
		interpolationSize,
		interpolationSamples
	));
}
const char* AApproximate :: registerClass(IScriptEnvironment* env) {
	/**
		clip.AApproximate(...)

		Returns an "approximation" of the original audio track.
		Basically, this is a way of distorting audio.
		It attempts to map all the peaks of the audio track, and then create a waveform inbetween the peaks.

		@param clip
			The image to sample
		@param waveform
			The waveform to approximate with
			Available values are:
				"SQUARE"
				"TRIANGLE"
				"SINE"
		@param same_zeros
			True if the waveform should intersect the x-axis at the same location,
			False otherwise
		@param tolerance
			How much tolerance is used for a sign change to occur
			Range: [0,1]
		@param absoluteness
			How absolute the tolerance is
			0 = Relative to the previous peak
			1 = Relative to 1 (the maximum amplitude)
			Range: [0,1]
		@param interpolation
			The number of peaks that should be interpolated into a single peak
			1 or less means no interpolation
		@param interpolate_samples
			The number of samples that should be interpolated into a single sample
			1 or less means no interpolation
			If this is specified along with "interpolation", the minimum interpolation distance is used
		@return
			A distorted audio track

		@default
			clip = clip.AApproximate( \
				waveform = "SQUARE", \
				same_zeros = true, \
				tolerance = 0.0, \
				absoluteness = 0.0, \
				interpolation = 1, \
				interpolate_samples = 0 \
			)
	*/
	env->AddFunction(
		"AApproximate",
		"[clip]c"
		"[waveform]s[same_zeros]b"
		"[tolerance]f[absoluteness]f"
		"[interpolation]i[interpolate_samples]i",
		AApproximate::create1,
		NULL
	);

	return "AApproximate";
}



FUNCTION_END();
REGISTER(AApproximate);


