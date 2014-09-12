#include "VCompare.hpp"
#include "../Helpers.hpp"
#include <cassert>
FUNCTION_START();



// Debuging for graphics functions
#define GLERR(id) \
	{ \
		GLenum e = glGetError(); \
		if (e) { \
			std::stringstream ss; \
			ss << "err." << (id) << ": " << (int)e; \
			env->ThrowError(ss.str().c_str()); \
		} \
	}
// Disable
#undef GLERR
#define GLERR(id)



// Comparison
VCompare :: VCompare(
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
) :
	VGraphicsBase(env, useGPU)
{
	// Setup graphics init function and data
	this->setupInit<VCompare::GraphicsData>(static_cast<void*>(this));

	// Set clips
	this->clip[0] = clip1;
	this->clip[1] = clip2;
	this->clipInfo[0] = this->clip[0]->GetVideoInfo();
	this->clipInfo[1] = this->clip[1]->GetVideoInfo();

	// Validation
	if (!this->clipInfo[0].IsRGB24()) {
		env->ThrowError("VCompare: RGB24 data only (video 1)");
	}
	if (!this->clipInfo[1].IsRGB24()) {
		env->ThrowError("VCompare: RGB24 data only (video 2)");
	}
	if (this->clipInfo[0].width != this->clipInfo[1].width) {
		env->ThrowError("VCompare: Video dimension mismatch (width)");
	}
	if (this->clipInfo[0].height != this->clipInfo[1].height) {
		env->ThrowError("VCompare: Video dimension mismatch (height)");
	}

	// Method checking
	std::string m[2];
	m[0] = bgMethod;
	m[1] = fgMethod;
	for (int i = 0; i < 2; ++i) {
		// Lower case
		for (int j = 0; j < m[i].length(); ++j) {
			if (m[i][j] >= 'A' && m[i][j] <= 'Z') {
				m[i][j] -= 'A' - 'a';
			}
		}

		// Comparison
		if (m[i] == "max") {
			this->layer[i].method = VCompare::COMBINE_METHOD_MAX;
		}
		else if (m[i] == "min") {
			this->layer[i].method = VCompare::COMBINE_METHOD_MIN;
		}
		else if (m[i] == "mult") {
			this->layer[i].method = VCompare::COMBINE_METHOD_MULT;
		}
		else if (m[i] == "mult_sqrt") {
			this->layer[i].method = VCompare::COMBINE_METHOD_MULT_SQRT;
		}
		else if (m[i] == "mult_sqrt_inv") {
			this->layer[i].method = VCompare::COMBINE_METHOD_MULT_SQRT_INV;
		}
		else if (m[i] == "color") {
			this->layer[i].method = VCompare::COMBINE_METHOD_COLOR_SCALE;
		}
		else {
			this->layer[i].method = VCompare::COMBINE_METHOD_ADD;
		}
	}

	// Difference range
	if (diffMin < 0.0) diffMin = 0.0;
	else if (diffMin > 255.0) diffMin = 255.0;
	if (diffMax < 0.0) diffMax = 0.0;
	else if (diffMax > 255.0) diffMax = 255.0;

	if (diffMax <= diffMin) diffMax = diffMin;

	this->differenceRange[0] = diffMin / 255.0;
	this->differenceRange[1] = (diffMax - diffMin) / 255.0;

	// Difference alpha range
	if (diffAlphaMin < 0.0) diffAlphaMin = 0.0;
	else if (diffAlphaMin > 1.0) diffAlphaMin = 1.0;
	if (diffAlphaMax < 0.0) diffAlphaMax = 0.0;
	else if (diffAlphaMax > 1.0) diffAlphaMax = 1.0;

	this->differenceAlphaRange[0] = diffAlphaMin;
	this->differenceAlphaRange[1] = (diffAlphaMax - diffAlphaMin);

	if (diffNormalize < 0) diffNormalize = -1;
	else if (diffNormalize > 1) diffNormalize = 1;
	this->differenceNormalize = diffNormalize;

	// Background
	this->layer[0].cRed = (bgColor & 0xFF0000) >> 16;
	this->layer[0].cGreen = (bgColor & 0xFF00) >> 8;
	this->layer[0].cBlue = (bgColor & 0xFF);
	this->layer[0].alpha = bgAlpha;
	this->layer[0].clipAlpha[0] = bgClip1Alpha;
	this->layer[0].clipAlpha[1] = bgClip2Alpha;
	this->layer[0].colorScale = bgCSettings;

	// Foreground
	this->layer[1].cRed = (fgColor & 0xFF0000) >> 16;
	this->layer[1].cGreen = (fgColor & 0xFF00) >> 8;
	this->layer[1].cBlue = (fgColor & 0xFF);
	this->layer[1].alpha = fgAlpha;
	this->layer[1].clipAlpha[0] = fgClip1Alpha;
	this->layer[1].clipAlpha[1] = fgClip2Alpha;
	this->layer[1].colorScale = fgCSettings;

	// Fluctuations
	this->parseFluctuations(fluctuations);
}
VCompare :: ~VCompare() {
}

void __stdcall VCompare :: GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
	this->clip[0]->GetAudio(buf, start, count, env);
}
const VideoInfo& __stdcall VCompare :: GetVideoInfo() {
	return this->clipInfo[0];
}
bool __stdcall VCompare :: GetParity(int n) {
	return this->clip[0]->GetParity(n);
}
CacheHintsReturnType __stdcall VCompare :: SetCacheHints(int cachehints, int frame_range) {
	// Do not pass cache requests upwards, only to the next filter
	return CacheHintsReturn(0);
}

PVideoFrame VCompare :: transformFrame(int n, IScriptEnvironment* env) {
	// Sources
	PVideoFrame sources[2];
	const unsigned char* sourcePtrs[2];
	int sourcePitches[2];

	sources[0] = this->clip[0]->GetFrame(n, env);
	sources[1] = this->clip[1]->GetFrame(n, env);
	sourcePtrs[0] = sources[0]->GetReadPtr();
	sourcePtrs[1] = sources[1]->GetReadPtr();
	sourcePitches[0] = sources[0]->GetPitch();
	sourcePitches[1] = sources[1]->GetPitch();

	// Destination
	PVideoFrame dest = env->NewVideoFrame(this->clipInfo[0]);
	unsigned char* destPtr = dest->GetWritePtr();
	int destPitch = dest->GetPitch();

	// Loop values
	int width = sources[0]->GetRowSize();
	int height = sources[0]->GetHeight();
	int x, y, i, iInv, j, components = 3, sourceCount = 2;
	double difference, alphaDifference;
	int diff[4];
	unsigned char bg[4];
	unsigned char fg[4];
	unsigned char sourceColors[2][4];

	// Loop over full frame
	for (y = 0; y < height; ++y) {
		// Loop over line
		for (x = 0; x < width; x += components) {
			// Calculate pixel difference
			difference = 0.0;
			for (i = 0; i < components; ++i) {
				for (j = 0; j < sourceCount; ++j) {
					sourceColors[j][i] = *(sourcePtrs[j] + x + i);
				}
				diff[i] = sourceColors[0][i] - sourceColors[1][i];
				difference += Math::absoluteValue(diff[i]);
			}
			difference /= components;
			difference /= (this->differenceNormalize >= 0 ? VCompare::getMaxDifference(sourceColors[this->differenceNormalize], components) / static_cast<double>(components) : 255.0); // now in range of [0.0, 1.0]
			difference -= this->differenceRange[0];
			if (difference < 0.0) {
				difference = 0.0;
			}
			else {
				if (this->differenceRange[1] == 0.0) {
					difference = 1.0;
				}
				else {
					difference = difference / this->differenceRange[1];
					if (difference > 1.0) difference = 1.0;
				}
			}

			// difference should end in the range [0.0,1.0]

			// Alpha range
			if (this->differenceAlphaRange[1] > 0.0) {
				alphaDifference = difference / this->differenceAlphaRange[1] + this->differenceAlphaRange[0];
			}
			else {
				alphaDifference = (difference >= 0.0) ? 1.0 : 0.0;
			}


			// Colors
			for (i = 0; i < components; ++i) {
				iInv = components - 1 - i;

				// Background
				bg[i] = this->combineValues(sourceColors[0][i], sourceColors[1][i], this->layer[0].clipAlpha[0], this->layer[0].clipAlpha[1], this->layer[0].method);
				bg[i] = static_cast<int>((this->layer[0].alpha * this->layer[0].color[iInv]) + ((1.0 - this->layer[0].alpha) * bg[i]));

				// Combine
				if (alphaDifference > 0.0) {
					// Foreground
					fg[i] = this->combineValues(sourceColors[0][i], sourceColors[1][i], this->layer[1].clipAlpha[0], this->layer[1].clipAlpha[1], this->layer[1].method);
					fg[i] = static_cast<int>((this->layer[1].alpha * this->layer[1].color[iInv]) + ((1.0 - this->layer[1].alpha) * fg[i]));

					// Alpha range
					bg[i] = static_cast<int>(((1.0 - alphaDifference) * bg[i]) + (alphaDifference * fg[i]));
				}

				// Final
				*(destPtr + x + i) = bg[i];
			}
		}

		// Next line
		sourcePtrs[0] += sourcePitches[0];
		sourcePtrs[1] += sourcePitches[1];
		destPtr += destPitch;
	}

	// Return new frame
	return dest;
}
PVideoFrame VCompare :: transformFrameGPU(int n, IScriptEnvironment* env) {
	// Vars
	int x, y, i, j;
	VCompare::GraphicsData* g = static_cast<VCompare::GraphicsData*>(this->graphics);


	// Destination
	PVideoFrame dest = env->NewVideoFrame(this->clipInfo[0]);
	unsigned char* destPtr = dest->GetWritePtr();

	// Get main source
	PVideoFrame sourceMain[2];
	const unsigned char* sourceMainPtr[2];
	const unsigned char* sourceMainPtrCurrent;
	for (i = 0; i < 2; ++i) {
		sourceMain[i] = this->clip[i]->GetFrame(n, env);
		sourceMainPtr[i] = sourceMain[i]->GetReadPtr();
	}

	// Loop values
	int width = this->clipInfo[0].width;
	int height = this->clipInfo[0].height;
	const int components = 3;
	int bgra[4];
	GLubyte* ptr;


	// Copy sources into GPU
	for (i = 0; i < 2; ++i) {
		// Bind texture and pixel buffer, get pointer
		glActiveTexture(GL_TEXTURE0 + i); GLERR(4213423)
		glBindTexture(GL_TEXTURE_2D, g->texture[i]);
		glBindSampler(i, g->textureSamplers[i]);

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, g->pixelBuffers[i]); GLERR(100)
		ptr = static_cast<GLubyte*>(glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY)); GLERR(103)

		if (ptr == NULL) {
			env->ThrowError("VCompare: Error getting buffer pointer (write)");
		}
		else {
			// RGB
			sourceMainPtrCurrent = sourceMainPtr[i];
			bgra[3] = 255;
			for (y = 0; y < height; ++y) {
				// Loop over line
				for (x = 0; x < width; ++x) {
					// Get old
					for (j = 0; j < components; ++j) {
						bgra[j] = *(sourceMainPtrCurrent++);
					}

					// Modify
					*(ptr++) = bgra[2]; // Red
					*(ptr++) = bgra[1]; // Green
					*(ptr++) = bgra[0]; // Blue
					*(ptr++) = bgra[3]; // Alpha
				}
			}

			// Unmap
			glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); GLERR(104)

			// Save
			glTexSubImage2D(GL_TEXTURE_2D, 0, g->left, g->top, width, height, GL_RGBA, GL_UNSIGNED_BYTE, NULL); GLERR(105)
		}
	}
	// Unbind
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); GLERR(112)


	// Modify the texture
	glBindFramebuffer(GL_FRAMEBUFFER, g->fboID[2]); GLERR(1120)
	glViewport(0, 0, g->width, g->height); GLERR(1121)
	glUseProgram(g->shaderProgram); GLERR(1122)
	if (g->shaderUniformCurrentFrame >= 0) {
		glUniform1i(g->shaderUniformCurrentFrame, n);
	}
	glBindVertexArray(g->drawAttributeArray); GLERR(1123)
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4); GLERR(1124)
	glBindVertexArray(0); GLERR(1125)
	glUseProgram(0); GLERR(1126)
	glBindFramebuffer(GL_FRAMEBUFFER, 0); GLERR(1127)


	// Read texture from GPU
	glBindTexture(GL_TEXTURE_2D, g->texture[2]);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, g->pixelBuffers[2]); GLERR(106)
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); GLERR(108)
	ptr = static_cast<GLubyte*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY)); GLERR(109)

	if (ptr == NULL) {
		env->ThrowError("VCompare: Error getting buffer pointer (read)");
	}
	else {
		// Offset to starting pixel
		ptr += (g->top * g->width + g->left) * 4;
		size_t ptrNextLineOffset = (g->width - width) * 4;

		for (y = 0; y < height; ++y) {
			// Loop over line
			for (x = 0; x < width; ++x) {
				// Read from buffer
				bgra[2] = *(ptr++); // Red
				bgra[1] = *(ptr++); // Green
				bgra[0] = *(ptr++); // Blue
				bgra[3] = *(ptr++); // Alpha

				// Set new
				for (j = 0; j < components; ++j) {
					*(destPtr++) = bgra[j];
				}
			}

			// Next line
			ptr += ptrNextLineOffset;
		}

		// Release
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER); GLERR(110)
	}


	// Unbind
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); GLERR(111)
	glBindTexture(GL_TEXTURE_2D, 0); GLERR(113)
	glActiveTexture(GL_TEXTURE0);


	// Done
	return dest;
}

unsigned char VCompare :: combineValues(int value1, int value2, double factor1, double factor2, VCompare::CombinationMethod method) {
	// Value
	double newValue = 0.0;

	// Composite method
	switch (method) {
		case VCompare::COMBINE_METHOD_MAX:
		{
			double v1 = value1 * factor1;
			double v2 = value2 * factor2;
			newValue = (v1 > v2) ? v1 : v2;
		}
		break;
		case VCompare::COMBINE_METHOD_MIN:
		{
			double v1 = value1 * factor1;
			double v2 = value2 * factor2;
			newValue = (v1 < v2) ? v1 : v2;
		}
		break;
		case VCompare::COMBINE_METHOD_MULT:
		{
			newValue = (value1 / 255.0 * factor1) * (value2 / 255.0 * factor2) * 255.0;
		}
		break;
		case VCompare::COMBINE_METHOD_MULT_SQRT:
		{
			newValue = sqrt((value1 / 255.0 * factor1) * (value2 / 255.0 * factor2)) * 255.0;
		}
		break;
		case VCompare::COMBINE_METHOD_MULT_SQRT_INV:
		{
			newValue = (1.0 - sqrt(1.0 - (value1 / 255.0 * factor1) * (value2 / 255.0 * factor2))) * 255.0;
		}
		break;
		//case VCompare::COMBINE_METHOD_ADD:
		//case VCompare::COMBINE_METHOD_COLOR: // only on GPU
		default:
		{
			newValue = value1 * factor1 + value2 * factor2;
		}
		break;
	}

	// Return and bounding
	int r = static_cast<int>(newValue);
	if (r < 0) r = 0;
	else if (r > 255) r = 255;
	return r;
}
int VCompare :: getMaxDifference(const unsigned char* color, int components) {
	int diff = 0;
	int inverse;
	while (--components >= 0) {
		inverse = 255 - color[components];
		diff += (inverse >= color[components] ? inverse : color[components]);
	}
	return diff;
}

void VCompare :: parseFluctuations(const char* str) {
	// Struct of init list
	struct FluxInit {
		const char* key;
		Fluctuation** target;
	};


	FluxInit initTargets[] = {
		{ "bg_cs_hue_min" , &this->layer[0].colorScale->hueRangeFluctuators[0] },
		{ "bg_cs_hue_max" , &this->layer[0].colorScale->hueRangeFluctuators[1] },
		{ "bg_cs_saturation_min" , &this->layer[0].colorScale->saturationRangeFluctuators[0] },
		{ "bg_cs_saturation_max" , &this->layer[0].colorScale->saturationRangeFluctuators[1] },
		{ "bg_cs_value_min" , &this->layer[0].colorScale->valueRangeFluctuators[0] },
		{ "bg_cs_value_max" , &this->layer[0].colorScale->valueRangeFluctuators[1] },

		{ "fg_cs_hue_min" , &this->layer[1].colorScale->hueRangeFluctuators[0] },
		{ "fg_cs_hue_max" , &this->layer[1].colorScale->hueRangeFluctuators[1] },
		{ "fg_cs_saturation_min" , &this->layer[1].colorScale->saturationRangeFluctuators[0] },
		{ "fg_cs_saturation_max" , &this->layer[1].colorScale->saturationRangeFluctuators[1] },
		{ "fg_cs_value_min" , &this->layer[1].colorScale->valueRangeFluctuators[0] },
		{ "fg_cs_value_max" , &this->layer[1].colorScale->valueRangeFluctuators[1] },
	};
	int initTargetCount = 12;

	int pos = 0;
	bool loop = true;
	std::stringstream ss;
	while (loop) {
		// End of string checking / empty line removal
		while (true) {
			// Remove whitespace
			for (; str[pos] < ' '; ++pos) {
				if (str[pos] == '\0') {
					loop = false;
					break;
				}
			}

			// Comment?
			if (str[pos] == '#') {
				// Remove comment
				while (str[pos] != '\0' && str[pos] != '\n') ++pos;
			}
			else {
				// Else, done
				break;
			}
		}

		// Get search key
		ss.str("");
		for (; str[pos] != '\0' && str[pos] != '\n' && str[pos] != ':'; ++pos) {
			if (str[pos] > ' ') {
				if (str[pos] >= 'A' && str[pos] <= 'Z') {
					ss << static_cast<char>(str[pos] + ('a' - 'A'));
				}
				else {
					ss << str[pos];
				}
			}
		}
		if (str[pos] > ' ') ++pos;

		// Check for match
		if (ss.str().length() > 0) {
			for (int i = 0; i < initTargetCount; ++i) {
				// Check
				if (ss.str() == initTargets[i].key) {
					// Add
					delete *initTargets[i].target;
					*initTargets[i].target = new VCompare::Fluctuation(&(str[pos]));
					break;
				}
			}
		}

		// Continue to next line
		for (; str[pos] != '\0' && str[pos] != '\n'; ++pos);
	}

}



VCompare::GraphicsData :: GraphicsData(void* data) :
	VGraphicsBase::GraphicsData(data),
	width(0),
	height(0),
	left(0),
	top(0)
{
	// Init
	int i;
	for (i = 0; i < 3; ++i) {
		this->texture[i] = 0;
		this->textureSamplers[i] = 0;
		this->rboID[i] = 0;
		this->fboID[i] = 0;
		this->pixelBuffers[i] = 0;
	}
	this->shaderVertex = 0;
	this->shaderFragment = 0;
	this->shaderProgram = 0;
	this->drawVertexBuffer = 0;
	this->drawAttributeArray = 0;


	// Setup
	VCompare* self = static_cast<VCompare*>(data);
	const int count = 3;


	// Graphics targets
	this->width = self->clipInfo[0].width;
	this->height = self->clipInfo[0].height;
	this->width = static_cast<int>(::pow(2.0, static_cast<int>(::ceil(Math::log2(static_cast<double>(this->width))))));
	this->height = static_cast<int>(::pow(2.0, static_cast<int>(::ceil(Math::log2(static_cast<double>(this->height))))));

	this->left = (this->width - self->clipInfo[0].width) / 2;
	this->top = (this->height - self->clipInfo[0].height) / 2;


	// Generate
	glGenBuffers(count, this->pixelBuffers);// Pixel buffers
	glGenTextures(count, this->texture);// Textures
	glGenSamplers(count, this->textureSamplers); // Texture samplers
	glGenFramebuffers(count, this->fboID);// Framebuffers
	glGenRenderbuffers(count, this->rboID);// Renderbuffers
	this->shaderVertex = glCreateShader(GL_VERTEX_SHADER);
	this->shaderFragment = glCreateShader(GL_FRAGMENT_SHADER);
	this->shaderProgram = glCreateProgram();


	// Empty texture
	unsigned char* textureData = new unsigned char[this->width * this->height * 4];
	unsigned char* textureDataPixel = textureData;
	for (int y = 0; y < this->height; ++y) {
		for (int x = 0; x < this->width; ++x) {
			*textureDataPixel = 0; // Red
			++textureDataPixel;
			*textureDataPixel = 0; // Green
			++textureDataPixel;
			*textureDataPixel = 0; // Blue
			++textureDataPixel;
			*textureDataPixel = 0; // Alpha
			++textureDataPixel;
		}
	}


	// Graphics
	for (i = 0; i < count; ++i) {
		// Create texture
		glBindTexture(GL_TEXTURE_2D, this->texture[i]);

		// Empty texture
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);

		// Samplers
		glSamplerParameteri(this->textureSamplers[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(this->textureSamplers[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(this->textureSamplers[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameteri(this->textureSamplers[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// Bind framebuffer / renderbuffer
		glBindFramebuffer(GL_FRAMEBUFFER, this->fboID[i]);
		glBindRenderbuffer(GL_RENDERBUFFER, this->rboID[i]);

		// Create/attach depth
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, this->width, this->height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, this->rboID[i]);

		// Attach color
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->texture[i], 0);

		// Make sure it was setup properly
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			if (this->fboID[i] != 0) {
				glDeleteFramebuffers(1, &this->fboID[i]);
				this->fboID[i] = 0;
			}

			if (this->rboID[i] != 0) {
				glDeleteRenderbuffers(1, &this->rboID[i]);
				this->rboID[i] = 0;
			}
		}
	}


	// Delete empty texture
	delete [] textureData;


	// Create pixel buffer memory
	size_t data_size = self->clipInfo[0].width * self->clipInfo[0].height * 4;
	for (i = 0; i < 2; ++i) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, this->pixelBuffers[i]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, data_size, NULL, GL_DYNAMIC_DRAW);
	}

	data_size = this->width * this->height * 4;
	glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pixelBuffers[i]);
	glBufferData(GL_PIXEL_PACK_BUFFER, data_size, NULL, GL_DYNAMIC_READ);


	//{ Create shaders
	// Shader sources
	std::stringstream ssVertex;
	std::stringstream ssFragment;
	ssVertex << //{
	"#version 330\n"
	"precision highp float;"

	"layout (location = 0) in vec2 in_Vertex;"
	"layout (location = 1) in vec2 in_TexCoord;"

	"out vec2 frag_TexCoord;"

	"void main(void) {"
		"gl_Position = vec4(in_Vertex, 0.0, 1.0);"

		"frag_TexCoord = in_TexCoord;"
	"}";
	//}
	ssFragment << //{
	"#version 330\n"
	"#define PI 3.141592653589793238462643383279\n"
	"precision highp float;"

	"uniform sampler2D in_TextureUnits[2];"

	"uniform vec4 in_Colors[2];"
	"uniform float in_InverseColorAlphas[2];"
	"uniform float in_TexAlphas[4];"
	"uniform float in_DifferenceRange[2];"
	"uniform float in_DifferenceAlphaRange[2];"
	"uniform float in_HueRange[4];"
	"uniform float in_SaturationRange[4];"
	"uniform float in_ValueRange[4];"
	"uniform int in_FrameNumber;"

	"in vec2 frag_TexCoord;"

	"out vec4 out_FragColor;"

	"void main(void) {"
		"vec4 tx0 = texture2D(in_TextureUnits[0], frag_TexCoord);"
		"vec4 tx1 = texture2D(in_TextureUnits[1], frag_TexCoord);"

		// Difference
		"float difference = (abs(tx0.r - tx1.r) + abs(tx0.g - tx1.g) + abs(tx0.b - tx1.b)) / 3.0;";

		if (self->differenceNormalize >= 0) {
			ssFragment << "difference /= ("
				"max(tx" << self->differenceNormalize << ".r, 1.0 - tx" << self->differenceNormalize << ".r) + "
				"max(tx" << self->differenceNormalize << ".g, 1.0 - tx" << self->differenceNormalize << ".g) + "
				"max(tx" << self->differenceNormalize << ".b, 1.0 - tx" << self->differenceNormalize << ".b)) / 3.0;";
		}

		// Background
		this->combineValueFunction(self, &ssFragment, self->layer[0].method, false, "bg", "vec4 bg");
		ssFragment << "bg = bg * in_InverseColorAlphas[0] + in_Colors[0];";

		// Foreground
		this->combineValueFunction(self, &ssFragment, self->layer[1].method, true, "fg", "vec4 fg");
		ssFragment << "fg = fg * in_InverseColorAlphas[1] + in_Colors[1];"

		// Mix
		//"difference = clamp((difference - in_DifferenceRange[0]) / in_DifferenceRange[1], 0.0, 1.0);"
		"bool noRange = (in_DifferenceRange[1] == 0.0);"
		"difference -= in_DifferenceRange[0];"
		"difference = float(noRange) * float(difference >= 0.0) + float(!noRange) * clamp(difference / in_DifferenceRange[1], 0.0, 1.0);"
		"difference = difference * in_DifferenceAlphaRange[1] + in_DifferenceAlphaRange[0] * float(difference > 0.0);"

		// Output
		"out_FragColor = (1.0 - difference) * bg + (difference) * fg;"
	"}";
	//}

	const char* shaderOutputs[] = {
		"out_FragColor",
		NULL
	};
	const char* shaderInputs[] = {
		"in_Vertex",
		"in_TexCoord",
		NULL
	};
	unsigned int shaderInputsAttributes[] = {
		// count, type, sizeof(type), normalize_to_float
		2, GL_FLOAT, 4, false,
		2, GL_FLOAT, 4, false
	};

	// Set sources
	std::string temp = ssVertex.str();
	const char* sSource = temp.c_str();
	glShaderSource(this->shaderVertex, 1, &static_cast<const GLchar*>(sSource), NULL);
	temp = ssFragment.str();
	sSource = temp.c_str();
	glShaderSource(this->shaderFragment, 1, &static_cast<const GLchar*>(sSource), NULL);

	// Compile
	for (i = 0; i < 2; ++i) {
		glCompileShader(this->shaderSources[i]);
		// Compile check
		GLint compiled;
		glGetShaderiv(this->shaderSources[i], GL_COMPILE_STATUS, &compiled);
		if (compiled == GL_FALSE) {
			// Error check
			this->error() << "VCompare: Error compiling shader[" << i << "]";
			int logLength = 0;
			glGetShaderiv(this->shaderSources[i], GL_INFO_LOG_LENGTH, &logLength);
			if (logLength > 0) {
				// Error
				char* infoLog = new char[logLength];
				glGetShaderInfoLog(this->shaderSources[i], logLength, &logLength, infoLog);
				this->error() << ":\n" << infoLog;
				delete [] infoLog;
			}
			return;
		}
	}

	// Attach
	glAttachShader(this->shaderProgram, this->shaderVertex);
	glAttachShader(this->shaderProgram, this->shaderFragment);

	// Bindings
	for (i = 0; shaderOutputs[i] != NULL; ++i) {
		glBindFragDataLocation(this->shaderProgram, i, shaderOutputs[i]);
	}

	// Link
	glLinkProgram(this->shaderProgram);
	GLint linked;
	glGetProgramiv(this->shaderProgram, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE) {
		// Not linked
		this->error() << "VCompare: Error linking shader";

		int logLength = 0;
		glGetProgramiv(this->shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 0) {
			// Error
			char* infoLog = new char[logLength];
			glGetProgramInfoLog(this->shaderProgram, logLength, &logLength, infoLog);
			this->error() << ":\n" << infoLog;
			delete [] infoLog;
		}

		// Done
		return;
	}

	// Uniforms
	{
		glUseProgram(this->shaderProgram);

		// Texture units
		GLint uniform = glGetUniformLocation(this->shaderProgram, "in_TextureUnits");
		if (uniform >= 0) {
			int values[2];
			values[0] = 0;
			values[1] = 1;
			glUniform1iv(uniform, 2, values);
		}

		// Colors
		uniform = glGetUniformLocation(this->shaderProgram, "in_Colors");
		if (uniform >= 0) {
			int j;
			float values[8];
			for (i = 0; i < 2; ++i) {
				for (j = 0; j < 3; ++j) {
					values[i * 4 + j] = self->layer[i].color[j] / 255.0 * self->layer[i].alpha;
				}
				values[i * 4 + j] = self->layer[i].alpha;
			}
			glUniform4fv(uniform, 2, values);
		}

		// Colors
		uniform = glGetUniformLocation(this->shaderProgram, "in_InverseColorAlphas");
		if (uniform >= 0) {
			float values[2];
			for (i = 0; i < 2; ++i) {
				values[i] = 1.0 - self->layer[i].alpha;
			}
			glUniform1fv(uniform, 2, values);
		}

		// Alphas
		uniform = glGetUniformLocation(this->shaderProgram, "in_TexAlphas");
		if (uniform >= 0) {
			float values[4];
			for (i = 0; i < 4; ++i) {
				values[i] = self->layer[i / 2].clipAlpha[i % 2];
			}
			glUniform1fv(uniform, 4, values);
		}

		// Range
		uniform = glGetUniformLocation(this->shaderProgram, "in_DifferenceRange");
		if (uniform >= 0) {
			float values[2];
			for (i = 0; i < 2; ++i) {
				values[i] = self->differenceRange[i];
			}
			glUniform1fv(uniform, 2, values);
		}

		// Alpha range
		uniform = glGetUniformLocation(this->shaderProgram, "in_DifferenceAlphaRange");
		if (uniform >= 0) {
			float values[2];
			for (i = 0; i < 2; ++i) {
				values[i] = self->differenceAlphaRange[i];
			}
			glUniform1fv(uniform, 2, values);
		}

		// Color scale
		uniform = glGetUniformLocation(this->shaderProgram, "in_HueRange");
		if (uniform >= 0) {
			float values[4];
			for (i = 0; i < 4; ++i) values[i] = self->layer[i / 2].colorScale->hueRange[i % 2];
			glUniform1fv(uniform, 4, values);
		}
		uniform = glGetUniformLocation(this->shaderProgram, "in_SaturationRange");
		if (uniform >= 0) {
			float values[4];
			for (i = 0; i < 4; ++i) values[i] = self->layer[i / 2].colorScale->saturationRange[i % 2];
			glUniform1fv(uniform, 4, values);
		}
		uniform = glGetUniformLocation(this->shaderProgram, "in_ValueRange");
		if (uniform >= 0) {
			float values[4];
			for (i = 0; i < 4; ++i) values[i] = self->layer[i / 2].colorScale->valueRange[i % 2];
			glUniform1fv(uniform, 4, values);
		}

		this->shaderUniformCurrentFrame = glGetUniformLocation(this->shaderProgram, "in_FrameNumber");

		glUseProgram(0);
	}
	//}


	// Vertex buffers
	size_t size = sizeof(ShaderInputData) * 4;
	glGenBuffers(1, &this->drawVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, this->drawVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
	ShaderInputData* vData = static_cast<ShaderInputData*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
	if (vData != NULL) {
		double xPos[2];
		double yPos[2];
		xPos[0] = (this->left / static_cast<double>(this->width));
		yPos[0] = (this->top / static_cast<double>(this->height));
		xPos[1] = ((this->left + self->clipInfo[0].width) / static_cast<double>(this->width));
		yPos[1] = ((this->top + self->clipInfo[0].height) / static_cast<double>(this->height));

		vData[0].position[0] = xPos[0] * 2.0 - 1.0;
		vData[0].position[1] = yPos[0] * 2.0 - 1.0;
		vData[0].texCoord[0] = xPos[0];
		vData[0].texCoord[1] = yPos[0];

		vData[1].position[0] = xPos[1] * 2.0 - 1.0;
		vData[1].position[1] = yPos[0] * 2.0 - 1.0;
		vData[1].texCoord[0] = xPos[1];
		vData[1].texCoord[1] = yPos[0];

		vData[2].position[0] = xPos[1] * 2.0 - 1.0;
		vData[2].position[1] = yPos[1] * 2.0 - 1.0;
		vData[2].texCoord[0] = xPos[1];
		vData[2].texCoord[1] = yPos[1];

		vData[3].position[0] = xPos[0] * 2.0 - 1.0;
		vData[3].position[1] = yPos[1] * 2.0 - 1.0;
		vData[3].texCoord[0] = xPos[0];
		vData[3].texCoord[1] = yPos[1];
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	// Vertex attribute array
	glGenVertexArrays(1, &this->drawAttributeArray);
	glBindVertexArray(this->drawAttributeArray);
	glBindBuffer(GL_ARRAY_BUFFER, this->drawVertexBuffer);
	size_t vaOffset = 0;
	for (i = 0; shaderInputs[i] != NULL; ++i) {
		GLuint shaderAttr = glGetAttribLocation(this->shaderProgram, shaderInputs[i]);
		glEnableVertexAttribArray(shaderAttr);
		glVertexAttribPointer(shaderAttr, shaderInputsAttributes[i * 4 + 0], shaderInputsAttributes[i * 4 + 1], shaderInputsAttributes[i * 4 + 3], sizeof(ShaderInputData), (GLvoid*) (vaOffset));
		vaOffset += shaderInputsAttributes[i * 4 + 0] * shaderInputsAttributes[i * 4 + 2];
	}
	glBindVertexArray(0);


	// Reset target
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Error check
	GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		// Error
		this->error() << "VCompare: error in graphics setup: " << static_cast<int>(e);
	}
}
VCompare::GraphicsData :: ~GraphicsData() {
	// TODO : Delete resources
}
void VCompare::GraphicsData :: combineValueFunction(VCompare* self, std::stringstream* target, CombinationMethod method, bool isFg, const char* prefix, const char* assignTo) {
	//self->layer[i / 2].colorScale->saturationRange[i % 2]
	int arrayOffset = isFg * 2;

	if (method == VCompare::COMBINE_METHOD_ADD) {
		*target << assignTo << " = ("
			"tx0 * in_TexAlphas[" << (arrayOffset) << "] + tx1 * in_TexAlphas[" << (arrayOffset + 1) << "]"
			");";
	}
	else if (method == VCompare::COMBINE_METHOD_MAX) {
		*target << assignTo << " = ("
			"max(tx0 * in_TexAlphas[" << (arrayOffset) << "], tx1 * in_TexAlphas[" << (arrayOffset + 1) << "])"
			");";
	}
	else if (method == VCompare::COMBINE_METHOD_MIN) {
		*target << assignTo << " = ("
			"min(tx0 * in_TexAlphas[" << (arrayOffset) << "], tx1 * in_TexAlphas[" << (arrayOffset + 1) << "])"
			");";
	}
	else if (method == VCompare::COMBINE_METHOD_MULT) {
		*target << assignTo << " = ("
			"tx0 * in_TexAlphas[" << (arrayOffset) << "] * tx1 * in_TexAlphas[" << (arrayOffset + 1) << "]"
			");";
	}
	else if (method == VCompare::COMBINE_METHOD_MULT_SQRT) {
		*target << assignTo << " = ("
			"sqrt(tx0 * in_TexAlphas[" << (arrayOffset) << "] * tx1 * in_TexAlphas[" << (arrayOffset + 1) << "])"
			");";
	}
	else if (method == VCompare::COMBINE_METHOD_MULT_SQRT_INV) {
		*target << assignTo << " = ("
			"1.0 - sqrt(1.0 - tx0 * in_TexAlphas[" << (arrayOffset) << "] * tx1 * in_TexAlphas[" << (arrayOffset + 1) << "])"
			");";
	}
	else { // if (method == VCompare::COMBINE_METHOD_COLOR_SCALE) {
		VCompare::Fluctuation* flux[2];
		std::stringstream fluxVarNames[2];
		// Hue
		for (int i = 0; i < 2; ++i) {
			fluxVarNames[i].str("");
			if ((flux[i] = self->layer[isFg].colorScale->hueRangeFluctuators[i]) != NULL) {
				// Use that variable
				fluxVarNames[i] << prefix << "HueRangeFlux" << i;

				// Set a new value
				flux[i]->toShaderString(&(fluxVarNames[i]), "in_FrameNumber", target);
			}
			else {
				fluxVarNames[i] << "in_HueRange[" << (arrayOffset + i) << "]";
			}
		}
		*target << "float " << prefix << "HueRange = "
			"(" << fluxVarNames[0].str() << " + (" << fluxVarNames[1].str() << " - " << fluxVarNames[0].str() << ") * difference) * 6.0;";

		// Saturation
		for (int i = 0; i < 2; ++i) {
			fluxVarNames[i].str("");
			if ((flux[i] = self->layer[isFg].colorScale->saturationRangeFluctuators[i]) != NULL) {
				// Use that variable
				fluxVarNames[i] << prefix << "SaturationRangeFlux" << i;

				// Set a new value
				flux[i]->toShaderString(&(fluxVarNames[i]), "in_FrameNumber", target);
			}
			else {
				fluxVarNames[i] << "in_SaturationRange[" << (arrayOffset + i) << "]";
			}
		}
		*target << "float " << prefix << "SaturationRange = "
			"(" << fluxVarNames[0].str() << " + (" << fluxVarNames[1].str() << " - " << fluxVarNames[0].str() << ") * difference);";

		// Value
		for (int i = 0; i < 2; ++i) {
			fluxVarNames[i].str("");
			if ((flux[i] = self->layer[isFg].colorScale->valueRangeFluctuators[i]) != NULL) {
				// Use that variable
				fluxVarNames[i] << prefix << "ValueRangeFlux" << i;

				// Set a new value
				flux[i]->toShaderString(&(fluxVarNames[i]), "in_FrameNumber", target);
			}
			else {
				fluxVarNames[i] << "in_ValueRange[" << (arrayOffset + i) << "]";
			}
		}
		*target << "float " << prefix << "ValueRange = "
			"(" << fluxVarNames[0].str() << " + (" << fluxVarNames[1].str() << " - " << fluxVarNames[0].str() << ") * difference);";

		// Mix
		*target << assignTo << " = ("
			"("
			"vec4("
				"clamp(abs(mod(" << prefix << "HueRange      , 6.0) - 3.0) - 1.0, 0.0, 1.0),"
				"clamp(abs(mod(" << prefix << "HueRange + 4.0, 6.0) - 3.0) - 1.0, 0.0, 1.0),"
				"clamp(abs(mod(" << prefix << "HueRange + 2.0, 6.0) - 3.0) - 1.0, 0.0, 1.0),"
				"1.0"
			") * clamp(" << prefix << "ValueRange, 0.0, 1.0)"
			") * clamp(" << prefix << "SaturationRange, 0.0, 1.0) + (vec4(1.0, 1.0, 1.0, 1.0)) * (1.0 - clamp(" << prefix << "SaturationRange, 0.0, 1.0))"
			");";
	}
}



VCompare::ColorScaleSettings :: ColorScaleSettings(
	double hueRangeMin,
	double hueRangeMax,
	double saturationRangeMin,
	double saturationRangeMax,
	double valueRangeMin,
	double valueRangeMax
)
{
	this->hueRange[0] = hueRangeMin;
	this->hueRange[1] = hueRangeMax;
	this->saturationRange[0] = saturationRangeMin;
	this->saturationRange[1] = saturationRangeMax;
	this->valueRange[0] = valueRangeMin;
	this->valueRange[1] = valueRangeMax;

	this->hueRangeFluctuators[0] = NULL;
	this->hueRangeFluctuators[1] = NULL;
	this->saturationRangeFluctuators[0] = NULL;
	this->saturationRangeFluctuators[1] = NULL;
	this->valueRangeFluctuators[0] = NULL;
	this->valueRangeFluctuators[1] = NULL;
}
VCompare::ColorScaleSettings :: ~ColorScaleSettings() {
	delete this->hueRangeFluctuators[0];
	delete this->hueRangeFluctuators[1];
	delete this->saturationRangeFluctuators[0];
	delete this->saturationRangeFluctuators[1];
	delete this->valueRangeFluctuators[0];
	delete this->valueRangeFluctuators[1];
}



VCompare::Fluctuation :: Fluctuation(const char* initString) :
	method(METHOD_NONE),
	period(1.0),
	periodOffset(0.0),
	polynomialPower(1)
{
	this->range[0] = 0.0;
	this->range[1] = 1.0;
	this->parseInitString(initString);
}
void VCompare::Fluctuation :: parseInitString(const char* str) {
	// Format : label:method,period,period_offset,range_min,range_max[,polynomial_power]
	int i = 0;
	std::stringstream ss;

	// Method
	for (; str[i] != '\0' && str[i] != '\n' && str[i] != '#' && str[i] != ','; ++i) {
		if (str[i] > ' ') {
			if (str[i] >= 'A' && str[i] <= 'Z') {
				ss << static_cast<char>(str[i] + ('a' - 'A'));
			}
			else {
				ss << str[i];
			}
		}
	}
	if (ss.str() == "none") {
		this->method = VCompare::Fluctuation::METHOD_NONE;
	}
	else if (ss.str() == "sine") {
		this->method = VCompare::Fluctuation::METHOD_SINE;
	}
	else if (ss.str() == "poly_loop") {
		this->method = VCompare::Fluctuation::METHOD_POLYNOMIAL_LOOP;
	}
	else if (ss.str() == "poly_reverse") {
		this->method = VCompare::Fluctuation::METHOD_POLYNOMIAL_REVERSE;
	}
	else if (ss.str() == "poly_smooth") {
		this->method = VCompare::Fluctuation::METHOD_POLYNOMIAL_SMOOTH;
	}

	// Period
	ss.str("");
	if (str[i] > ' ' && str[i] != '#') ++i;
	for (; str[i] != '\0' && str[i] != '\n' && str[i] != '#' && str[i] != ','; ++i) {
		if (str[i] > ' ') ss << str[i];
	}
	this->period = atof(ss.str().c_str());

	// Period offset
	ss.str("");
	if (str[i] > ' ' && str[i] != '#') ++i;
	for (; str[i] != '\0' && str[i] != '\n' && str[i] != '#' && str[i] != ','; ++i) {
		if (str[i] > ' ') ss << str[i];
	}
	this->periodOffset = atof(ss.str().c_str());

	// Range min
	ss.str("");
	if (str[i] > ' ' && str[i] != '#') ++i;
	for (; str[i] != '\0' && str[i] != '\n' && str[i] != '#' && str[i] != ','; ++i) {
		if (str[i] > ' ') ss << str[i];
	}
	this->range[0] = atof(ss.str().c_str());

	// Range max
	ss.str("");
	if (str[i] > ' ' && str[i] != '#') ++i;
	for (; str[i] != '\0' && str[i] != '\n' && str[i] != '#' && str[i] != ','; ++i) {
		if (str[i] > ' ') ss << str[i];
	}
	this->range[1] = atof(ss.str().c_str());

	// [Polynomial power]
	ss.str("");
	if (str[i] > ' ' && str[i] != '#') ++i;
	for (; str[i] != '\0' && str[i] != '\n' && str[i] != '#' && str[i] != ','; ++i) {
		if (str[i] > ' ') ss << str[i];
	}
	if (ss.str().length() > 0) {
		this->polynomialPower = atoi(ss.str().c_str());
	}

}
void VCompare::Fluctuation :: toShaderString(const std::stringstream* varName, const char* periodVariable, std::stringstream* target) {
	int fcnMin, fcnMax;
	std::stringstream ss;
	std::stringstream vars;

	ss.setf(std::ios::fixed, std::ios::floatfield);
	ss.precision(5);
	vars.setf(std::ios::fixed, std::ios::floatfield);
	vars.precision(5);

	ss << "((";

	switch (this->method) {
		case METHOD_SINE:
		{
			fcnMin = -1;
			fcnMax = 1;

			ss << "sin(2 * PI * (" << periodVariable << " + " << this->periodOffset << ") / " << this->period << ")";
		}
		break;
		case METHOD_POLYNOMIAL_LOOP:
		{
			fcnMin = 0;
			fcnMax = 1;

			ss << "pow(mod((" << periodVariable << " + " << this->periodOffset << ") / " << this->period << ", 1.0), " << this->polynomialPower << ")";
		}
		break;
		case METHOD_POLYNOMIAL_REVERSE:
		{
			fcnMin = 0;
			fcnMax = 1;

			vars << "float " << varName->str() << "_x = mod((" << periodVariable << " + " << this->periodOffset << ") / " << this->period << ", 1.0);";
			vars << "bool " << varName->str() << "_b = (" << varName->str() << "_x >= 0.5);";

			ss << "pow(((" << varName->str() << "_x) * int(!" << varName->str() << "_b) + (1.0 - " << varName->str() << "_x) * int(" << varName->str() << "_b)) * 2, " << this->polynomialPower << ")";
		}
		break;
		case METHOD_POLYNOMIAL_SMOOTH:
		{
			double offset = (pow(-1.0, this->polynomialPower) + pow(1.0, this->polynomialPower)) / 2.0;
			fcnMin = -1;
			fcnMax = 1;

			vars << "float " << varName->str() << "_x = mod((" << periodVariable << " + " << this->periodOffset << ") / " << this->period << ", 1.0);";
			vars << "bool " << varName->str() << "_b = (" << varName->str() << "_x >= 0.5);";

			ss <<
				"((" << offset << ") - pow(" << varName->str() << "_x * 4 - 1, " << this->polynomialPower << ")) * int(!" << varName->str() << "_b) + "
				"(pow(" << varName->str() << "_x * 4 - 3, " << this->polynomialPower << ") - (" << offset << ")) * int(" << varName->str() << "_b)";
		}
		break;
		default: // METHOD_NONE
		{
			fcnMin = 0;
			fcnMax = 1;
			ss << "1.0";
		}
		break;
	}

	ss << ") / (" << static_cast<double>(fcnMax - fcnMin) << ")";
	ss << ") * " << (this->range[1] - this->range[0]) << " + " << this->range[0] << "";

	//std::ofstream f("Z:\\desktop\\o3.txt",std::ofstream::app);f << vars.str() << "float " << varName->str() << " = " << ss.str() << ";";f.close();

	*target << vars.str() << "float " << varName->str() << " = " << ss.str() << ";";
}



AVSValue __cdecl VCompare :: create1(AVSValue args, void* user_data, IScriptEnvironment* env) {
	// New video comparison
	return static_cast<AVSValue>(new VCompare(
		env,
		args[0].AsClip(), // clip
		args[1].AsClip(), // to

		args[2].AsFloat(0.0), // diff_min
		args[3].AsFloat(128.0), // diff_max
		args[4].AsFloat(0.0), // diff_alpha_min
		args[5].AsFloat(1.0), // diff_alpha_max
		args[6].AsInt(-1), // diff_normalize

		args[7].AsInt(0x000000), // bg_color
		args[8].AsFloat(1.0), // bg_alpha
		args[9].AsFloat(0.0), // bg_video1_alpha
		args[10].AsFloat(0.0), // bg_video2_alpha
		args[11].AsString("add"), // bg_method

		args[12].AsInt(0xffffff), // fg_color
		args[13].AsFloat(0.0), // fg_alpha
		args[14].AsFloat(1.0), // fg_video1_alpha
		args[15].AsFloat(0.0), // fg_video2_alpha
		args[16].AsString("add"), // fg_method

		new VCompare::ColorScaleSettings(
			args[17].AsFloat(0.0), // bg_cs_hue_min
			args[18].AsFloat(1.0), // bg_cs_hue_max
			args[19].AsFloat(1.0), // bg_cs_saturation_min
			args[20].AsFloat(1.0), // bg_cs_saturation_max
			args[21].AsFloat(1.0), // bg_cs_value_min
			args[22].AsFloat(1.0) // bg_cs_value_max
		),
		new VCompare::ColorScaleSettings(
			args[23].AsFloat(0.0), // fg_cs_hue_min
			args[24].AsFloat(1.0), // fg_cs_hue_max
			args[25].AsFloat(1.0), // fg_cs_saturation_min
			args[26].AsFloat(1.0), // fg_cs_saturation_max
			args[27].AsFloat(1.0), // fg_cs_value_min
			args[28].AsFloat(1.0) // fg_cs_value_max
		),

		args[29].AsString(""), // fluctuations
		args[30].AsBool(true) // gpu
	));
}
const char* VCompare :: registerClass(IScriptEnvironment* env) {
	/**
		clip.VCompare(...)

		Perform a pixel-by-pixel difference of two videos, with lots of options

		@param clip
			The first clip to be compared
		@param to
			The second clip to be compared to

		@param diff_min
			Minimum difference required for a change to be shown
			Range: [0.0 - 255.0]
		@param diff_max
			Maximum value for the full change to be shown
			Range: [0.0 - 255.0]
		@param diff_alpha_min
			Minimum alpha for foreground
			Range: [0.0 - 1.0]
		@param diff_alpha_max
			Maximum alpha for foreground
			Range: [0.0 - 1.0]
		@param diff_normalize
			Normalize to be in the range of the maximum possible difference for a pixel

		@param bg_color
			The default background color
		@param bg_alpha
			The default background color (alpha component)
		@param bg_video1_alpha
			The alpha of the original clip1 (clip) color (background)
		@param bg_video2_alpha
			The alpha of the original clip2 (to) color (background)
		@param bg_method
			The comparison method for the background; available methods are:
				"add"
				"max"
				"min"
				"mult"
				"mult_sqrt"
				"mult_sqrt_inv"
				"color"

		@param fg_color
			The default foreground color
		@param fg_alpha
			The default foreground color (alpha component)
		@param fg_video1_alpha
			The alpha of the original clip1 (clip) color (foreground)
		@param fg_video2_alpha
			The alpha of the original clip2 (to) color (foreground)
		@param fg_method
			The comparison method for the foreground; available methods are:
				"add"
				"max"
				"min"
				"mult"
				"mult_sqrt"
				"mult_sqrt_inv"
				"color"

		@param bg_cs_hue_min
			When bg_method=="color":
			The minimum hue
			Range: [0.0 - 1.0]
		@param bg_cs_hue_max
			When bg_method=="color":
			The maximum hue
		@param bg_cs_saturation_min
			When bg_method=="color":
			The minimum saturation
		@param bg_cs_saturation_max
			When bg_method=="color":
			The maximum saturation
		@param bg_cs_value_min
			When bg_method=="color":
			The minimum value
		@param bg_cs_value_max
			When bg_method=="color":
			The maximum value

		@param fg_cs_hue_min
			When fg_method=="color":
			The minimum hue
			Range: [0.0 - 1.0]
		@param fg_cs_hue_max
			When fg_method=="color":
			The maximum hue
		@param fg_cs_saturation_min
			When fg_method=="color":
			The minimum saturation
		@param fg_cs_saturation_max
			When fg_method=="color":
			The maximum saturation
		@param fg_cs_value_min
			When fg_method=="color":
			The minimum value
		@param fg_cs_value_max
			When fg_method=="color":
			The maximum value

		@param fluctuations
			A list of fluctuations to apply to parameters over time
			@todo : Document this more

		@param gpu
			Use the gpu to render
		@note
			When using gpu=false, a small amount of functionality is not available compared to using gpu=true


		@default:
			video1.VCompare( \
				to = video2, \
				diff_min = 0.0, \
				diff_max = 128.0, \
				diff_alpha_min = 0.0, \
				diff_alpha_max = 1.0, \
				diff_normalize = -1, \
				\
				bg_color = $000000, \
				bg_alpha = 1.0, \
				bg_video1_alpha = 0.0, \
				bg_video2_alpha = 0.0, \
				bg_method = "add", \
				\
				fg_color = $ffffff, \
				fg_alpha = 0.0, \
				fg_video1_alpha = 1.0, \
				fg_video2_alpha = 0.0, \
				fg_method = "add", \
				\
				bg_cs_hue_min = 0.0, \
				bg_cs_hue_max = 1.0, \
				bg_cs_saturation_min = 1.0, \
				bg_cs_saturation_max = 1.0, \
				bg_cs_value_min = 1.0, \
				bg_cs_value_max = 1.0, \
				\
				fg_cs_hue_min = 0.0, \
				fg_cs_hue_max = 1.0, \
				fg_cs_saturation_min = 1.0, \
				fg_cs_saturation_max = 1.0, \
				fg_cs_value_min = 1.0, \
				fg_cs_value_max = 1.0, \
				\
				fluctuations = "", \
				\
				gpu = true \
			)
	*/
	env->AddFunction(
		"VCompare",
		"[clip]c[to]c"
		"[diff_min]f[diff_max]f[diff_alpha_min]f[diff_alpha_max]f[diff_normalize]i"
		"[bg_color]i[bg_alpha]f[bg_video1_alpha]f[bg_video2_alpha]f[bg_method]s"
		"[fg_color]i[fg_alpha]f[fg_video1_alpha]f[fg_video2_alpha]f[fg_method]s"

		"[bg_cs_hue_min]f[bg_cs_hue_max]f[bg_cs_saturation_min]f[bg_cs_saturation_max]f[bg_cs_value_min]f[bg_cs_value_max]f"
		"[fg_cs_hue_min]f[fg_cs_hue_max]f[fg_cs_saturation_min]f[fg_cs_saturation_max]f[fg_cs_value_min]f[fg_cs_value_max]f"

		"[fluctuations]s"
		"[gpu]b",
		VCompare::create1,
		NULL
	);

	return "VCompare";
}



FUNCTION_END();
REGISTER(VCompare);



