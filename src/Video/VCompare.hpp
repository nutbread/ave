#ifndef ___PLUGIN_VCOMPARE_H
#define ___PLUGIN_VCOMPARE_H



#include "../Header.hpp"
#include "../Graphics.hpp"
#include "VGraphicsBase.hpp"
FUNCTION_START();



// Video Comparison class
class VCompare :
	public VGraphicsBase
{
private:
	class Fluctuation {
	public:
		enum Method {
			METHOD_NONE = 0x0,
			METHOD_SINE = 0x1,
			METHOD_POLYNOMIAL_LOOP = 0x2, // y = (x ^ P) * (1.0 - i) + i  on  [0,1] repeated  y_range=[0,1]
			METHOD_POLYNOMIAL_REVERSE = 0x3, // y = (x ^ P) * (1.0 - i) + i  on  [0,1]  and  y = (1 - ((2-x) ^ P) * (1.0 - i)) + i  on  [1,2] y_range=[0,1]
			METHOD_POLYNOMIAL_SMOOTH = 0x4,
			// y = ( (((-1)^p + 1^p) / 2) - ((x*2 - 1) ^ p))  on  x=0 to 1
			// y = (-(((-1)^p + 1^p) / 2) + ((x*2 - 3) ^ p))  on  x=1 to 2
			// y_range=[-1,1]
		};

	private:
		Method method;
		double period;
		double periodOffset;
		double range[2];
		int polynomialPower;

		void parseInitString(const char* initString);

	public:
		Fluctuation(const char* initString);

		void toShaderString(const std::stringstream* varName, const char* periodVariable, std::stringstream* target);

	};

	class ColorScaleSettings {
		friend class VCompare;

	private:
		double hueRange[2];
		double saturationRange[2];
		double valueRange[2];
		Fluctuation* hueRangeFluctuators[2];
		Fluctuation* saturationRangeFluctuators[2];
		Fluctuation* valueRangeFluctuators[2];

	public:
		ColorScaleSettings(
			double hueRangeMin,
			double hueRangeMax,
			double saturationRangeMin,
			double saturationRangeMax,
			double valueRangeMin,
			double valueRangeMax
		);
		~ColorScaleSettings();
	};

	enum CombinationMethod {
		COMBINE_METHOD_ADD = 0x0,
		COMBINE_METHOD_MAX = 0x1,
		COMBINE_METHOD_MIN = 0x2,
		COMBINE_METHOD_MULT = 0x3,
		COMBINE_METHOD_MULT_SQRT = 0x4,
		COMBINE_METHOD_MULT_SQRT_INV = 0x5,
		COMBINE_METHOD_COLOR_SCALE = 0x6
	};

	struct ShaderInputData {
		float position[2];
		float texCoord[2];
	};

	PClip clip[2];
	VideoInfo clipInfo[2];
	int differenceNormalize;
	double differenceRange[2];
	double differenceAlphaRange[2];
	struct {
		union {
			unsigned char color[4];
			struct {
				unsigned char cRed;
				unsigned char cGreen;
				unsigned char cBlue;
			};
		};
		double alpha;
		double clipAlpha[2];
		ColorScaleSettings* colorScale;
		CombinationMethod method;
	} layer[2];

	class GraphicsData : public VGraphicsBase::GraphicsData {
	public:
		GraphicsData(void* data);
		~GraphicsData();

		void combineValueFunction(VCompare* self, std::stringstream* target, CombinationMethod method, bool isFg, const char* prefix, const char* assignTo);

		unsigned int width;
		unsigned int height;
		unsigned int left;
		unsigned int top;
		GLuint texture[3];
		GLuint textureSamplers[3];
		GLuint rboID[3];
		GLuint fboID[3];
		GLuint pixelBuffers[3];

		union {
			struct {
				GLuint shaderVertex;
				GLuint shaderFragment;
			};
			GLuint shaderSources[2];
		};
		GLuint shaderProgram;
		GLint shaderUniformCurrentFrame;

		GLuint drawVertexBuffer;
		GLuint drawAttributeArray;
	};

	unsigned char combineValues(int value1, int value2, double factor1, double factor2, VCompare::CombinationMethod method);
	static int getMaxDifference(const unsigned char* color, int components);

	PVideoFrame transformFrame(int n, IScriptEnvironment* env);
	PVideoFrame transformFrameGPU(int n, IScriptEnvironment* env);

	void parseFluctuations(const char* str);

	VCompare(
		IScriptEnvironment* env,
		PClip clip1,
		PClip clip2,
		double diffMin,
		double diffMax,
		double diffAlphaMin,
		double diffAlphaMax,
		int diffNormalize,
		unsigned int bgColor,
		double bgAlpha,
		double bgClip1Alpha,
		double bgClip2Alpha,
		const char* bgMethod,
		unsigned int fgColor,
		double fgAlpha,
		double fgClip1Alpha,
		double fgClip2Alpha,
		const char* fgMethod,
		ColorScaleSettings* bgCSettings,
		ColorScaleSettings* fgCSettings,
		const char* fluctuations,
		bool useGPU
	);
	~VCompare();

	static AVSValue __cdecl create1(AVSValue args, void* user_data, IScriptEnvironment* env);

public:
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
	const VideoInfo& __stdcall GetVideoInfo();
	bool __stdcall GetParity(int n);
	CacheHintsReturnType __stdcall SetCacheHints(int cachehints, int frame_range);

	static const char* registerClass(IScriptEnvironment* env);

};



FUNCTION_END();



#endif

