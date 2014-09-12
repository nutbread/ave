#include "VSlideFPS.hpp"
#include "../Helpers.hpp"
#include <cassert>
#include <cmath>
FUNCTION_START();



// Video framerate changing
VSlideFPS::Term :: Term() :
	variableExponentOuter(0),
	variableExponentInner(-1),
	offset(0.0),
	factorOuter(1.0),
	factorInner(1.0),
	exponent(0.0),
	next(NULL)
{
}
VSlideFPS::Term :: Term(const VSlideFPS::Term& other) :
	variableExponentOuter(other.variableExponentOuter),
	variableExponentInner(other.variableExponentInner),
	offset(other.offset),
	factorOuter(other.factorOuter),
	factorInner(other.factorInner),
	exponent(other.exponent),
	next(NULL)
{
}



VSlideFPS :: VSlideFPS(
	IScriptEnvironment* env,
	PClip clip,
	const char* equationString,
	const AVSValue targetFramerateNumerator,
	const AVSValue targetFramerateDenominator,
	const AVSValue initialFramerateNumerator,
	const AVSValue initialFramerateDenominator,
	const AVSValue finalFramerateNumerator,
	const AVSValue finalFramerateDenominator,
	const AVSValue targetAudioRate,
	const AVSValue initialAudioRate,
	const AVSValue finalAudioRate
) :
	IClip(),
	clip(clip),
	equationIntegrated(NULL),
	equationIntegratedAudio(NULL),
	equationIntegratedValueAt0(0.0),
	equationIntegratedAudioValueAt0(0.0)
{
	// Get info
	this->clipInfoInitial = this->clip->GetVideoInfo();
	this->clipInfo = this->clip->GetVideoInfo();

	// Validation
	if (
		this->clipInfo.HasAudio() &&
		this->clipInfo.SampleType() != SAMPLE_INT8 &&
		this->clipInfo.SampleType() != SAMPLE_INT16 &&
		this->clipInfo.SampleType() != SAMPLE_INT32 &&
		this->clipInfo.SampleType() != SAMPLE_FLOAT
	) {
		// To much hassle
		env->ThrowError("VSlideFPS: 24bit audio not supported");
	}

	// Framerates
	int framerateNumerator[2];
	int framerateDenominator[2];
	double framerates[2];
	int audiorates[2];

	VSlideFPS::getFramerates(
		&this->clipInfo,
		targetFramerateNumerator,
		targetFramerateDenominator,
		initialFramerateNumerator,
		initialFramerateDenominator,
		framerateNumerator,
		framerateDenominator,
		framerates
	);
	VSlideFPS::getAudiorates(
		&this->clipInfo,
		targetAudioRate,
		initialAudioRate,
		framerates,
		audiorates
	);

	// Create function
	int i;
	double valueAt0, fullTime;
	Term* equation = NULL;
	Term* equationAudio = NULL;

	// Load from string
	equation = VSlideFPS::normalizeEquation(VSlideFPS::loadEquationFromString(equationString));




	// Audio setup
	if (this->clipInfo.HasAudio()) {
		i = (audiorates[1] >= audiorates[0]) ? 1 : 0;


		/* Audio equation:
			equationAudio = initialAudiorate + (targetAudiorate - initialAudiorate) * equation
		*/
		equationAudio = VSlideFPS::cloneEquation(equation, audiorates[1] - audiorates[0]);
		// Update constant term
		equationAudio->factorOuter += audiorates[0];



		// Integrate
		this->equationIntegratedAudio = VSlideFPS::integrateEquation(equationAudio);
		VSlideFPS::deleteEquation(equationAudio);
		valueAt0 = VSlideFPS::evaluateEquation(this->equationIntegratedAudio, 0.0, 1.0);


		// Integrate to solve for fullTime:
		//   this->clipInfo.num_audio_samples = integrate(equationAudio, on variable "x", on range [0, fullTime])
		fullTime = VSlideFPS::solveEquation(this->equationIntegratedAudio, this->clipInfo.num_audio_samples, valueAt0);


		// Final audiorate and number of samples
		if (finalAudioRate.AsInt(-1) > 0) {
			audiorates[i] = finalAudioRate.AsInt(1);
		}
		__int64 newSampleCount = static_cast<__int64>(fullTime * audiorates[i]);
		if (newSampleCount <= 0) newSampleCount = 1;


		// Set new values
		this->clipInfo.audio_samples_per_second = audiorates[i];
		this->clipInfo.num_audio_samples = newSampleCount;


		// Update evaluation at 0
		fullTime = this->clipInfo.num_audio_samples / static_cast<double>(this->clipInfo.audio_samples_per_second);
		this->equationIntegratedAudioValueAt0 = VSlideFPS::evaluateEquation(this->equationIntegratedAudio, 0.0, fullTime);
	}


	// Video setup
	if (this->clipInfo.HasVideo()) {
		i = (framerates[1] >= framerates[0]) ? 1 : 0;


		/* Turn the equation into:
			equation = initialFramerate + (targetFramerate - initialFramerate) * equation
		*/
		VSlideFPS::multiplyEquation(equation, framerates[1] - framerates[0]);
		// Update constant term
		equation->factorOuter += framerates[0];


		// Integrate
		this->equationIntegrated = VSlideFPS::integrateEquation(equation);
		valueAt0 = VSlideFPS::evaluateEquation(this->equationIntegrated, 0.0, 1.0);


		// Integrate to solve for fullTime:
		//   this->clipInfo.num_frames = integrate(equation, on variable "x", on range [0, fullTime])
		fullTime = VSlideFPS::solveEquation(this->equationIntegrated, this->clipInfo.num_frames, valueAt0);


		// Final framerate and number of frames
		if (finalFramerateNumerator.AsInt(-1) > 0) {
			framerateNumerator[i] = finalFramerateNumerator.AsInt(1);
		}
		if (finalFramerateDenominator.AsInt(-1) > 0) {
			framerateDenominator[i] = finalFramerateDenominator.AsInt(1);
		}
		framerates[i] = framerateNumerator[i] / static_cast<double>(framerateDenominator[i]);
		int newFrameCount = static_cast<int>(fullTime * framerates[i]);
		if (newFrameCount <= 0) newFrameCount = 1;


		// Set new values
		this->clipInfo.fps_numerator = framerateNumerator[i];
		this->clipInfo.fps_denominator = framerateDenominator[i];
		this->clipInfo.num_frames = newFrameCount;


		// Update evaluation at 0
		fullTime = this->clipInfo.num_frames * this->clipInfo.fps_denominator / static_cast<double>(this->clipInfo.fps_numerator);
		this->equationIntegratedValueAt0 = VSlideFPS::evaluateEquation(this->equationIntegrated, 0.0, fullTime);

	}


	// Clean
	VSlideFPS::deleteEquation(equation);
}
VSlideFPS :: ~VSlideFPS() {
	VSlideFPS::deleteEquation(this->equationIntegrated);
	VSlideFPS::deleteEquation(this->equationIntegratedAudio);
}

void VSlideFPS :: getFramerates(const VideoInfo* clipInfo, const AVSValue targetFramerateNumerator, const AVSValue targetFramerateDenominator, const AVSValue initialFramerateNumerator, const AVSValue initialFramerateDenominator, int* framerateNumerator, int* framerateDenominator, double* framerates) {
	// Defaults
	framerateNumerator[1] = targetFramerateNumerator.AsInt(-1);
	framerateDenominator[1] = targetFramerateDenominator.AsInt(-1);
	framerateNumerator[0] = initialFramerateNumerator.AsInt(-1);
	framerateDenominator[0] = initialFramerateDenominator.AsInt(-1);

	// Framerates
	for (int i = 0; i < 2; ++i) {
		// Normalize to 0/1 if either is 0
		if (framerateNumerator[i] == 0 || framerateDenominator[i] == 0) {
			framerateNumerator[i] = 0;
			framerateDenominator[i] = 1;
		}
		else {
			// Set to original if negative
			if (framerateNumerator[i] < 0) framerateNumerator[i] = clipInfo->fps_numerator;
			if (framerateDenominator[i] < 0) framerateDenominator[i] = clipInfo->fps_denominator;
		}
		framerates[i] = framerateNumerator[i] / static_cast<double>(framerateDenominator[i]);
	}

	// Both framerates cannot be 0
	if (framerateNumerator[0] == 0 && framerateNumerator[1] == 0) {
		framerateNumerator[0] = clipInfo->fps_numerator;
		framerateDenominator[0] = clipInfo->fps_denominator;
		framerates[0] = framerateNumerator[0] / static_cast<double>(framerateDenominator[0]);
	}
}
void VSlideFPS :: getAudiorates(const VideoInfo* clipInfo, const AVSValue targetAudioRate, const AVSValue initialAudioRate, const double* framerates, int* audiorates) {
	audiorates[0] = initialAudioRate.AsInt(-1);
	audiorates[1] = targetAudioRate.AsInt(-1);

	if (audiorates[0] < 0) {
		audiorates[0] = (framerates == NULL) ? clipInfo->audio_samples_per_second : static_cast<int>(clipInfo->audio_samples_per_second * (framerates[0] * clipInfo->fps_denominator / clipInfo->fps_numerator));
	}
	if (audiorates[1] < 0) {
		audiorates[1] = (framerates == NULL) ? clipInfo->audio_samples_per_second : static_cast<int>(clipInfo->audio_samples_per_second * (framerates[1] * clipInfo->fps_denominator / clipInfo->fps_numerator));
	}
}

VSlideFPS::Term* VSlideFPS :: loadEquationFromString(const char* str) {
	Term* equation = NULL;
	Term* term;
	Term** termPtr = &equation;

	// Values { factorOuter, factorInner, offset, exponent }
	double factors[4] = { 1.0 , 1.0 , 0.0 , 0.0 };

	// Read string
	bool done = false, constant = true;
	int j = 0;
	size_t i = 0;

	while (str[i] != '\0') {
		// State checking
		if (str[i] == '+') {
			if (j > 0) done = true;
		}
		else if (str[i] == '.' || (str[i] >= '0' && str[i] <= '9')) {
			factors[j] = VSlideFPS::loadNumberFromString(str, i);
			done = (++j >= 4);
		}
		else if (str[i] == '(') {
			constant = false;
			if (j < 1) j = 1;
		}
		else if (str[i] == 'x') {
			constant = false;
			if (j < 2) j = 2;
		}
		else if (str[i] == '^') {
			constant = false;
			if (j < 3) j = 3;
		}
		else if (str[i] == '-') {
			constant = false;
			if (j < 2) j = 2;
		}
		// else, ignore

		// Term done or end of string
		if ((str[++i] == '\0' && j > 0) || done) {
			// Complete term
			if (!constant && j < 4) factors[3] = 1.0; // default exponent to 1.0 if not specified

			term = new Term();
			term->factorOuter = factors[0];
			term->factorInner = factors[1];
			term->offset = -factors[2];
			term->exponent = factors[3];

			*termPtr = term;
			termPtr = &term->next;


			// Reset for next
			constant = true;
			done = false;
			factors[0] = 1.0;
			factors[1] = 1.0;
			factors[2] = 0.0;
			factors[3] = 0.0;
			j = 0;
		}
	}


	// Default
	if (equation == NULL) {
		equation = new Term();
		equation->exponent = 1.0;
	}

	// Error and/or return
	return equation;
}
double VSlideFPS :: loadNumberFromString(const char* str, size_t& i) {
	size_t start = i, j;
	bool decimal = false;

	while (true) {
		if (str[i] == '.' && !decimal) {
			decimal = true;
		}
		else if (!(str[i] >= '0' && str[i] <= '9')) {
			break;
		}
		++i;
	}

	// Get the value
	char* evalStr = new char[i - start + 1];

	for (j = start; j < i; ++j) evalStr[j - start] = str[j];
	evalStr[j - start] = '\0';

	double value = atof(evalStr);

	delete [] evalStr;
	return value;
}

PVideoFrame __stdcall VSlideFPS :: GetFrame(int n, IScriptEnvironment* env) {
	// Get frame number
	double fullTime = this->clipInfo.num_frames * this->clipInfo.fps_denominator / static_cast<double>(this->clipInfo.fps_numerator);
	double frameTime = n * this->clipInfo.fps_denominator / static_cast<double>(this->clipInfo.fps_numerator);

	int newFrame = static_cast<int>(VSlideFPS::evaluateEquation(this->equationIntegrated, frameTime, fullTime) - this->equationIntegratedValueAt0);
	if (newFrame <= 0) newFrame = 0;

	return this->clip->GetFrame(newFrame, env);
}
void __stdcall VSlideFPS :: GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
	if (this->clipInfo.SampleType() == SAMPLE_INT8) {
		VSlideFPS::getTypedAudio<__int8>(static_cast<__int8*>(buf), start, count, env);
	}
	else if (this->clipInfo.SampleType() == SAMPLE_INT16) {
		VSlideFPS::getTypedAudio<__int16>(static_cast<__int16*>(buf), start, count, env);
	}
	else if (this->clipInfo.SampleType() == SAMPLE_INT32) {
		VSlideFPS::getTypedAudio<__int32>(static_cast<__int32*>(buf), start, count, env);
	}
	else { // if (this->clipInfo.SampleType() == SAMPLE_FLOAT) {
		VSlideFPS::getTypedAudio<SFLOAT>(static_cast<SFLOAT*>(buf), start, count, env);
	}
}
const VideoInfo& __stdcall VSlideFPS :: GetVideoInfo() {
	return this->clipInfo;
}
bool __stdcall VSlideFPS :: GetParity(int n) {
	return this->clip->GetParity(n);
}
CacheHintsReturnType __stdcall VSlideFPS :: SetCacheHints(int cachehints, int frame_range) {
	// Do not pass cache requests upwards, only to the next filter
	return CacheHintsReturn(0);
}

void VSlideFPS :: integrateTerm(const VSlideFPS::Term* term, VSlideFPS::Term* result) {
	/* Standard integration, used to solve for V
		V = fullTime (to be solved for)
		q1 = variableExponentOuter
		q2 = variableExponentInner
		f1 = factorOuter
		f2 = factorInner
		o = offset
		p = exponent
		x = integration variable (framesPerSecond) (will be integrated from 0 to fullTime)

		function: V^q1 * f1 * (V^q2 * f2 * x + o) ^ p
		integration: V^(q1-q2) * f1 / (f2 * (p + 1)) * (V^q2 * f2 * x + o)^(p+1)
	*/

	// New values
	result->variableExponentOuter = term->variableExponentOuter - term->variableExponentInner;
	result->variableExponentInner = term->variableExponentInner;

	result->exponent = term->exponent + 1;
	result->offset = term->offset;

	result->factorOuter = term->factorOuter / (term->factorInner * result->exponent);
	result->factorInner = term->factorInner;


	// Next
	result->next = NULL;
}
VSlideFPS::Term* VSlideFPS :: integrateEquation(const VSlideFPS::Term* term) {
	// Integrate
	Term* equationIntegrated = NULL;
	Term** termPtr = &equationIntegrated;
	while (term != NULL) {
		// Create new term and integrate
		*termPtr = new Term();
		VSlideFPS::integrateTerm(term, *termPtr);

		// Next
		termPtr = &(*termPtr)->next;
		term = term->next;
	}

	return equationIntegrated;
}
VSlideFPS::Term* VSlideFPS :: normalizeEquation(VSlideFPS::Term* equation) {
	/* Normalize factors, and add an offset term
		Essentially performs this:
		equation = equation / (equation(1) - equation(0)) - equation(0)
	*/
	double valueAt0 = VSlideFPS::evaluateEquation(equation, 0.0, 1.0);
	double valueAt1 = VSlideFPS::evaluateEquation(equation, 1.0, 1.0);
	double valueDifference = valueAt1 - valueAt0;
	Term* term;

	if (valueDifference != 1.0 && Math::absoluteValue(valueDifference) > 1.0e-5) {
		// Normalize
		for (term = equation; term != NULL; term = term->next) {
			term->factorOuter /= valueDifference;
		}
	}
	// Add constant term
	term = new Term();
	term->factorOuter = -valueAt0;

	term->next = equation;
	equation = term;

	// Done
	return equation;
}
VSlideFPS::Term* VSlideFPS :: cloneEquation(const VSlideFPS::Term* term, double factor) {
	Term* equation = NULL;
	Term** termPtr = &equation;

	for (; term != NULL; term = term->next) {
		(*termPtr) = new Term(*term);
		(*termPtr)->factorOuter *= factor;

		termPtr = &(*termPtr)->next;
	}

	return equation;
}
void VSlideFPS :: deleteEquation(VSlideFPS::Term* term) {
	VSlideFPS::Term* next;

	while (term != NULL) {
		next = term->next;
		delete term;
		term = next;
	}
}
void VSlideFPS :: multiplyEquation(VSlideFPS::Term* term, double factor) {
	for (; term != NULL; term = term->next) {
		term->factorOuter *= factor;
	}
}
double VSlideFPS :: evaluateEquation(const VSlideFPS::Term* equation, double x, double variable) {
	double value = 0.0;
	const Term* term;

	for (term = equation; term != NULL; term = term->next) {
		value += term->factorOuter * ::pow(variable, term->variableExponentOuter) * ::pow(x * term->factorInner * ::pow(variable, term->variableExponentInner) + term->offset, term->exponent);
	}

	return value;
}
double VSlideFPS :: solveEquation(const VSlideFPS::Term* equation, double equals, double valueAt0) {
	double varFactor = -valueAt0;
	const Term* term;

	for (term = equation; term != NULL; term = term->next) {
		assert(term->variableExponentInner == -1);
		assert(term->variableExponentOuter == 1);
		varFactor += term->factorOuter * ::pow(1.0 * term->factorInner + term->offset, term->exponent);
	}

	// solve V*varFactor = equals for V
	return equals / varFactor;
}

inline __int64 roundSample(double sample) {
	return static_cast<__int64>(sample + 0.5);
}
template <class Type> void VSlideFPS :: getTypedAudio(Type* buffer, __int64 start, __int64 count, IScriptEnvironment* env) {
	int channels = this->clipInfo.AudioChannels();

	// Get sample number
	double rate = this->clipInfo.audio_samples_per_second;
	double fullTime = this->clipInfo.num_audio_samples / rate;
	double startTime = (start) / rate;
	double endTime = (start + count) / rate;

	// Get endpoint samples
	__int64 startSample = roundSample(VSlideFPS::evaluateEquation(this->equationIntegratedAudio, startTime, fullTime) - this->equationIntegratedAudioValueAt0);
	__int64 sampleCount = roundSample(VSlideFPS::evaluateEquation(this->equationIntegratedAudio, endTime, fullTime) - this->equationIntegratedAudioValueAt0) - startSample;
	if (startSample < 0) startSample = 0;
	if (sampleCount <= 0) sampleCount = 1;


	// Get original audio
	Type* initialBuffer = new Type[channels * sampleCount];
	this->clip->GetAudio(initialBuffer, startSample, sampleCount, env);

	// Copy into new buffer
	double time;
	int c;
	__int64 sample, i;
	for (i = 0; i < count; ++i) {
		for (c = 0; c < channels; ++c) {
			// Time and sample
			time = (i + start) / rate;
			sample = roundSample(VSlideFPS::evaluateEquation(this->equationIntegratedAudio, time, fullTime) - this->equationIntegratedAudioValueAt0) - startSample;
			if (sample < 0) sample = 0;
			else if (sample >= sampleCount) sample = sampleCount - 1;

			// Copy into place
			buffer[i * channels + c] = initialBuffer[sample * channels + c];
		}
	}

	// Cleanup
	delete [] initialBuffer;
}



AVSValue __cdecl VSlideFPS :: create1(AVSValue args, void* user_data, IScriptEnvironment* env) {
	// New video reversal
	return static_cast<AVSValue>(new VSlideFPS(
		env,
		args[0].AsClip(), // clip
		args[1].AsString(""), // equation
		args[2], // framerate_numerator
		args[3], // framerate_denominator
		args[4], // framerate_numerator_initial
		args[5], // framerate_denominator_initial
		args[6], // framerate_numerator_final
		args[7], // framerate_denominator_final
		args[8], // audiorate
		args[9], // audiorate_initial
		args[10] // audiorate_final
	));
}
const char* VSlideFPS :: registerClass(IScriptEnvironment* env) {
	/**
		clip.VSlideFPS(...)

		Slide the framerate from one value to another across a clip

		@param clip
			The clip to add noise to
		@param framerate_numerator
			The target framerate numerator
		@param framerate_denominator
			The target framerate denominator
		@param framerate_numerator_initial
			The initial framerate numerator
		@param framerate_denominator_initial
			The initial framerate denominator
		@param framerate_numerator_final
			The final framerate numerator used in the output
		@param framerate_denominator_final
			The final framerate denominator used in the output
		@param audiorate
			The target audiorate
		@param audiorate_initial
			The initial audiorate
		@param audiorate_final
			The final audiorate used in the output

		@default
			clip.VSlideFPS( \
				framerate_numerator = -1, \
				framerate_denominator = -1, \
				framerate_numerator_initial = -1, \
				framerate_denominator_initial = -1, \
				framerate_numerator_final = -1, \
				framerate_denominator_final = -1, \
				audiorate = -1, \
				audiorate_initial = -1, \
				audiorate_final = -1 \
			)
	*/
	env->AddFunction(
		"VSlideFPS",
		"[clip]c"
		"[equation]s"
		"[framerate_numerator]i[framerate_denominator]i"
		"[framerate_numerator_initial]i[framerate_denominator_initial]i"
		"[framerate_numerator_final]i[framerate_denominator_final]i"
		"[audiorate]i"
		"[audiorate_initial]i"
		"[audiorate_final]i",
		VSlideFPS::create1,
		NULL
	);

	return "VSlideFPS";
}



FUNCTION_END();
REGISTER(VSlideFPS);

