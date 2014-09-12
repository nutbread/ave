#ifndef ___PLUGIN_VBLUR_H
#define ___PLUGIN_VBLUR_H



#include "../Header.hpp"
#include "../Graphics.hpp"
#include "VGraphicsBase.hpp"
FUNCTION_START();



// Video Blur class
class VBlur :
	public VGraphicsBase
{
private:
	enum EdgeMethods {
		EDGE_CLAMP,
		EDGE_COLOR
	};
	enum BlurMethods {
		BLUR_GAUSSIAN,
		BLUR_POLYNOMIAL
	};

	struct ShaderInputData {
		float position[2];
		float texCoord[2];
	};

	PClip clip;
	VideoInfo clipInfo;
	int components;
	struct {
		BlurMethods blurMethod;
		double radius;
		double interval;
		struct {
			// y = yIntercept + ((1.0 - x / radius) ^ (power)) * (1.0 - yIntercept)
			// "integrated" on the range [0 , ceil(radius) - 1] with an interval of 1
			double power;
			double yIntercept;
		} polynomial;
		EdgeMethods edgeMethod;
		union {
			struct {
				unsigned char r;
				unsigned char g;
				unsigned char b;
				unsigned char a;
			};
			unsigned char rgba[4];
		} edgeColor;
	} blur[2];

	class GraphicsData : public VGraphicsBase::GraphicsData {
	public:
		GraphicsData(void* data);
		~GraphicsData();

		unsigned int width;
		unsigned int height;
		unsigned int left;
		unsigned int top;

		GLuint clipTexture;
		GLuint clipTextureSampler;
		GLuint rboID[2];
		GLuint fboID[2];
		GLuint fboTextures[2];
		GLuint fboTextureSamplers[2];
		union {
			struct {
				GLuint pixelBufferIn;
				GLuint pixelBufferOut;
			};
			GLuint pixelBuffers[2];
		};

		struct {
			union {
				struct {
					GLuint vertex;
					GLuint fragment;
				};
				GLuint sources[2];
			};
			GLuint program;
			GLuint drawAttributeArray;
		} shaders[2];

		GLuint drawVertexBuffer;
	};

	void graphicsInitShaderSources(GraphicsData* gData, std::stringstream* vertex, std::stringstream* fragment, bool vertical);
	void graphicsInitShaderSourcesBasic(GraphicsData* gData, std::stringstream* vertex, std::stringstream* fragment);

	PVideoFrame transformFrame(int n, IScriptEnvironment* env);
	PVideoFrame transformFrameGPU(int n, IScriptEnvironment* env);

	VBlur(
		IScriptEnvironment* env,
		PClip clip,
		double blurX,
		double blurY,
		const char* edgeMethodX,
		const char* edgeMethodY,
		unsigned int edgeColorX,
		unsigned int edgeColorY,
		double intervalX,
		double intervalY,
		const char* functionX,
		const char* functionY,
		double polynomialPowerX,
		double polynomialPowerY,
		double polynomialInterceptX,
		double polynomialInterceptY,
		bool useGPU
	);
	~VBlur();

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

