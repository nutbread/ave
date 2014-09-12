#ifndef ___PLUGIN_VSLIDEFPS_H
#define ___PLUGIN_VSLIDEFPS_H



#include "../Header.hpp"
FUNCTION_START();




// Noise add class
class VSlideFPS :
	public IClip
{
private:
	struct Term {
		// var^variableExponentOuter * factorOuter * (var^variableExponentInner * x * factorInner + offset)^(exponent)
		// x replaced with (x / full_time) in actual calculation to solve for full_time
		int variableExponentOuter;
		int variableExponentInner;
		double offset;
		double factorOuter;
		double factorInner;
		double exponent;
		Term* next;

		Term();
		Term(const Term& other);
	};

	PClip clip;
	VideoInfo clipInfoInitial;
	VideoInfo clipInfo;

	Term* equationIntegrated;
	Term* equationIntegratedAudio;
	double equationIntegratedValueAt0;
	double equationIntegratedAudioValueAt0;

	static AVSValue __cdecl create1(AVSValue args, void* user_data, IScriptEnvironment* env);

	/**
		Integrate a single Term object

		@param term
			The term to integrate
		@param result
			The term object to put the results into
	*/
	static void integrateTerm(const Term* term, Term* result);
	static Term* integrateEquation(const Term* term);
	static Term* normalizeEquation(Term* equation);
	static Term* cloneEquation(const Term* term, double factor);
	static void multiplyEquation(Term* term, double factor);
	static void deleteEquation(Term* term);
	static double evaluateEquation(const Term* equation, double x, double variable);
	static double solveEquation(const Term* equation, double equals, double valueAt0);

	VSlideFPS(
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
	);
	~VSlideFPS();

	static Term* loadEquationFromString(const char* str);
	static double loadNumberFromString(const char* str, size_t& i);
	template <class Type> void getTypedAudio(Type* buffer, __int64 start, __int64 count, IScriptEnvironment* env);

	static void getFramerates(
		const VideoInfo* clipInfo,
		const AVSValue targetFramerateNumerator,
		const AVSValue targetFramerateDenominator,
		const AVSValue initialFramerateNumerator,
		const AVSValue initialFramerateDenominator,
		int* framerateNumerator,
		int* framerateDenominator,
		double* framerates
	);
	static void getAudiorates(
		const VideoInfo* clipInfo,
		const AVSValue targetAudioRate,
		const AVSValue initialAudioRate,
		const double* framerates,
		int* audiorates
	);

public:
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
	const VideoInfo& __stdcall GetVideoInfo();
	bool __stdcall GetParity(int n);
	CacheHintsReturnType __stdcall SetCacheHints(int cachehints, int frame_range);

	static const char* registerClass(IScriptEnvironment* env);

};



FUNCTION_END();



#endif

