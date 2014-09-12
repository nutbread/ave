#ifndef ___PLUGIN_VBLEND_H
#define ___PLUGIN_VBLEND_H



#include "../Header.hpp"
FUNCTION_START();



// Blending class
class VBlend :
	public IClip
{
private:
	struct InitData {
		const char* factors[2];
		const char* equation;
		int offsets[4];
	};

	enum EquationType {
		EQ_ADD,
		EQ_SUBTRACT,
		EQ_MULTIPLY,
		EQ_DIVIDE,
		EQ_MAX,
		EQ_MIN
	};
	enum FactorType {
		FT_VALUE,
		FT_COMPONENT,
	};

	PClip clip[2];
	VideoInfo clipInfo[2];

	struct {
		FactorType factors[2]; // Factor type
		double factorValues[2]; // Direct factor value (FT_VALUE)
		unsigned char factorComponent[2]; // Factor component id [0-3] (FT_COMPONENT)
		bool factorSource[2]; // Factor source id [0,1] (FT_COMPONENT)
		bool factorInvert[2]; // True if mult factor should be inverted

		EquationType equation;
		bool equationReversed;

		int offset[2][2];
	} components[4];

	unsigned char borderColor[2][4];

	bool hasOffset;

	static void cStringToLower(const char* cstr, std::string& target);
	static bool getWord(const std::string& str, std::string& word, int& pos);
	static bool isStringInt(const std::string& str);

	int modify(int component, int* source1Values, int* source2Values);
	static int sign(double value);

	PVideoFrame getFrameWithoutOffset(int n, IScriptEnvironment* env);
	PVideoFrame getFrameWithOffset(int n, IScriptEnvironment* env);

	static AVSValue __cdecl create1(AVSValue args, void* user_data, IScriptEnvironment* env);

	VBlend(
		IScriptEnvironment* env,
		PClip clip1,
		PClip clip2,

		InitData* r,
		InitData* g,
		InitData* b,
		InitData* a,

		unsigned int borderColor1,
		unsigned int borderColor2
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

