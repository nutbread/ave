#include "VBlur.hpp"
#include "../Helpers.hpp"
#include <cassert>
#include <cmath>
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




// TODO : This may not blur properly when semi-transparent video is used
// Blur
VBlur :: VBlur(
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
) :
	VGraphicsBase(env, useGPU),
	components(3)
{
	// Setup graphics init function and data
	this->setupInit<VBlur::GraphicsData>(static_cast<void*>(this));

	// Set clips
	this->clip = clip;
	this->clipInfo = this->clip->GetVideoInfo();

	// Validation
	if (this->clipInfo.IsRGB24()) {
		this->components = 3;
	}
	else if (this->clipInfo.IsRGB32()) {
		this->components = 4;
	}
	else {
		env->ThrowError("VBlur: RGB24/RGB32 data only");
	}


	// Method checking
	std::string m[2];
	m[0] = edgeMethodX;
	m[1] = edgeMethodY;
	for (int i = 0; i < 2; ++i) {
		// Lower case
		for (int j = 0; j < m[i].length(); ++j) {
			if (m[i][j] >= 'A' && m[i][j] <= 'Z') {
				m[i][j] -= 'A' - 'a';
			}
		}

		// Comparison
		if (m[i] == "color") {
			this->blur[i].edgeMethod = VBlur::EDGE_COLOR;
		}
		else { // clamp
			this->blur[i].edgeMethod = VBlur::EDGE_CLAMP;
		}
	}


	// Function checking
	m[0] = functionX;
	m[1] = functionY;
	for (int i = 0; i < 2; ++i) {
		// Lower case
		for (int j = 0; j < m[i].length(); ++j) {
			if (m[i][j] >= 'A' && m[i][j] <= 'Z') {
				m[i][j] -= 'A' - 'a';
			}
		}

		// Comparison
		if (m[i] == "polynomial") {
			this->blur[i].blurMethod = VBlur::BLUR_POLYNOMIAL;
		}
		else { // gaussian
			this->blur[i].blurMethod = VBlur::BLUR_GAUSSIAN;
		}
	}


	// Blur
	if (blurX < 0.0) blurX = 0.0;
	if (blurY < 0.0) blurY = 0.0;
	this->blur[0].radius = blurX;
	this->blur[1].radius = blurY;

	if (intervalX <= 0.0) intervalX = 1.0;
	if (intervalY <= 0.0) intervalY = 1.0;
	this->blur[0].interval = intervalX;
	this->blur[1].interval = intervalY;


	// Edge colors
	this->blur[0].edgeColor.r = (edgeColorX & 0xFF0000) >> 16;
	this->blur[0].edgeColor.g = (edgeColorX & 0xFF00) >> 8;
	this->blur[0].edgeColor.b = (edgeColorX & 0xFF);
	this->blur[0].edgeColor.a = 255;
	this->blur[1].edgeColor.r = (edgeColorY & 0xFF0000) >> 16;
	this->blur[1].edgeColor.g = (edgeColorY & 0xFF00) >> 8;
	this->blur[1].edgeColor.b = (edgeColorY & 0xFF);
	this->blur[1].edgeColor.a = 255;


	// Function
	if (polynomialPowerX <= 0.0) polynomialPowerX = 1.0;
	if (polynomialPowerY <= 0.0) polynomialPowerY = 1.0;
	if (polynomialInterceptX < 0.0) polynomialInterceptX = 0.0;
	else if (polynomialInterceptX > 1.0) polynomialInterceptX = 1.0;
	if (polynomialInterceptY < 0.0) polynomialInterceptY = 0.0;
	else if (polynomialInterceptY > 1.0) polynomialInterceptY = 1.0;

	this->blur[0].polynomial.power = polynomialPowerX;
	this->blur[0].polynomial.yIntercept = polynomialInterceptX;
	this->blur[1].polynomial.power = polynomialPowerY;
	this->blur[1].polynomial.yIntercept = polynomialInterceptY;
}
VBlur :: ~VBlur() {
}

void __stdcall VBlur :: GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
	this->clip->GetAudio(buf, start, count, env);
}
const VideoInfo& __stdcall VBlur :: GetVideoInfo() {
	return this->clipInfo;
}
bool __stdcall VBlur :: GetParity(int n) {
	return this->clip->GetParity(n);
}
CacheHintsReturnType __stdcall VBlur :: SetCacheHints(int cachehints, int frame_range) {
	// Do not pass cache requests upwards, only to the next filter
	return CacheHintsReturn(0);
}

void VBlur :: graphicsInitShaderSources(GraphicsData* gData, std::stringstream* vertex, std::stringstream* fragment, bool vertical) {
	*vertex << //{
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

	// Format
	std::streamsize precision = fragment->precision();
	fragment->setf(std::ios::fixed, std::ios::floatfield);
	fragment->precision(5);

	// Fragment
	*fragment << //{
	"#version 330\n"
	"precision highp float;"

	"uniform sampler2D in_TextureUnit;"
	"uniform vec2 in_TextureRangeMin;"
	"uniform vec2 in_TextureRangeMax;"

	"in vec2 frag_TexCoord;"

	"out vec4 out_FragColor;"

	"void main(void) {"
		// Output
		"out_FragColor = (";

		double sum = 0.0;
		double x = 0.0;
		int vCount = 0;
		double* values = NULL;

		// Method
		if (this->blur[vertical].radius == 0.0) {
			// Default
			*fragment << "texture2D(in_TextureUnit, frag_TexCoord)";
		}
		else {
			if (this->blur[vertical].blurMethod == VBlur::BLUR_POLYNOMIAL) {
				// Summation
				vCount = static_cast<int>(::ceil(this->blur[vertical].radius / this->blur[vertical].interval));
				if (vCount < 1) vCount = 1;
				values = new double[vCount];
				for (int i = 0; i < vCount; ++i) {
					// Get value
					values[i] = this->blur[vertical].polynomial.yIntercept + ::pow((1.0 - x / this->blur[vertical].radius), (this->blur[vertical].polynomial.power)) * (1.0 - this->blur[vertical].polynomial.yIntercept);

					// Add
					sum += (i == 0 ? values[i] : values[i] * 2.0);

					// Next
					x += this->blur[vertical].interval;
				}
			}
			else { // if (this->blur[vertical].blurMethod == VBlur::BLUR_GAUSSIAN) {
				// Summation
				vCount = static_cast<int>(::ceil(this->blur[vertical].radius / this->blur[vertical].interval));
				if (vCount < 1) vCount = 1;
				values = new double[vCount];
				double sigma = this->blur[vertical].radius / 3.0;
				double factor = 1.0 / (::sqrt(2.0 * M_PI) * sigma);
				double denom = -2.0 * sigma * sigma;
				for (int i = 0; i < vCount; ++i) {
					// Get value
					values[i] = factor * ::exp(x * x / denom);

					// Add
					sum += (i == 0 ? values[i] : values[i] * 2.0);

					// Next
					x += this->blur[vertical].interval;
				}

			}


			// Texture manipulation
			x = this->blur[vertical].interval;
			*fragment << "(texture2D(in_TextureUnit, frag_TexCoord) * " << values[0];
			for (int i = 1; i < vCount; ++i) {
				// Clamped/colored edges
				*fragment << " + (texture2D(in_TextureUnit, vec2(";
				if (vertical) {
					*fragment << "frag_TexCoord.x, clamp(frag_TexCoord.y + " << x << " / " << gData->height << ".0, in_TextureRangeMin.y, in_TextureRangeMax.y)";
				}
				else {
					*fragment << "clamp(frag_TexCoord.x + " << x << " / " << gData->width << ".0, in_TextureRangeMin.x, in_TextureRangeMax.x), frag_TexCoord.y";
				}
				*fragment << ")) + texture2D(in_TextureUnit, vec2(";
				if (vertical) {
					*fragment << "frag_TexCoord.x, clamp(frag_TexCoord.y + " << (-x) << " / " << gData->height << ".0, in_TextureRangeMin.y, in_TextureRangeMax.y)";
				}
				else {
					*fragment << "clamp(frag_TexCoord.x + " << (-x) << " / " << gData->width << ".0, in_TextureRangeMin.x, in_TextureRangeMax.x), frag_TexCoord.y";
				}
				*fragment << "))) * " << values[i];

				// Next
				x += this->blur[vertical].interval;
			}
			*fragment << ") / " << sum;

			// Clean
			delete [] values;
		}

		// Output complete
		*fragment <<
		");"
	"}";
	//}

	// Unformat
	fragment->unsetf(std::ios::floatfield);
	fragment->precision(precision);
}
void VBlur :: graphicsInitShaderSourcesBasic(GraphicsData* gData, std::stringstream* vertex, std::stringstream* fragment) {
	*vertex << //{
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

	// Fragment
	*fragment << //{
	"#version 330\n"
	"precision highp float;"

	"uniform sampler2D in_TextureUnit;"

	"in vec2 frag_TexCoord;"

	"out vec4 out_FragColor;"

	"void main(void) {"
		// Output
		"out_FragColor = texture2D(in_TextureUnit, frag_TexCoord);"
	"}";
	//}
}

PVideoFrame VBlur :: transformFrame(int n, IScriptEnvironment* env) {
	// Destination
	PVideoFrame dest = env->NewVideoFrame(this->clipInfo);
	unsigned char* destPtr = dest->GetWritePtr();
	int destPitch = dest->GetPitch();

	// Get main source
	PVideoFrame sourceMain = this->clip->GetFrame(n, env);
	const unsigned char* sourceMainPtr = sourceMain->GetReadPtr();
	int sourceMainPitch = sourceMain->GetPitch();
	int rgba[4];

	// Loop values
	int x, y, j;
	int width = this->clipInfo.width;
	int height = this->clipInfo.height;
	int rowSize = width * this->components;

	// Copy from main source
	for (y = 0; y < height; ++y) {
		// Loop over line
		for (x = 0; x < rowSize; x += this->components) {
			// Get old
			for (j = 0; j < this->components; ++j) {
				rgba[j] = *(sourceMainPtr + x + j);
			}

			// Modify
			// TODO
			//this->randomChange(rgba);

			// Set new
			for (j = 0; j < this->components; ++j) {
				*(destPtr + x + j) = rgba[j];
			}
		}

		// Next line
		sourceMainPtr += sourceMainPitch;
		destPtr += destPitch;
	}

	// Done
	return dest;
}
PVideoFrame VBlur :: transformFrameGPU(int n, IScriptEnvironment* env) {
	// Vars
	int x, y, i, j;
	VBlur::GraphicsData* g = static_cast<VBlur::GraphicsData*>(this->graphics);

	// Destination
	PVideoFrame dest = env->NewVideoFrame(this->clipInfo);
	unsigned char* destPtr = dest->GetWritePtr();

	// Get main source
	PVideoFrame sourceMain = this->clip->GetFrame(n, env);
	const unsigned char* sourceMainPtr = sourceMain->GetReadPtr();

	// Loop values
	int width = this->clipInfo.width;
	int height = this->clipInfo.height;
	const int components = this->components;
	int bgra[4];
	GLubyte* ptr;


	// Copy source into GPU
	glActiveTexture(GL_TEXTURE0); GLERR(4213423)
	glBindTexture(GL_TEXTURE_2D, g->clipTexture);
	glBindSampler(0, g->clipTextureSampler);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, g->pixelBufferIn); GLERR(100)
	ptr = static_cast<GLubyte*>(glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY)); GLERR(103)

	if (ptr == NULL) {
		env->ThrowError("VCompare: Error getting buffer pointer (write)");
	}
	else {
		// RGB
		bgra[3] = 255;
		for (y = 0; y < height; ++y) {
			// Loop over line
			for (x = 0; x < width; ++x) {
				// Get old
				for (j = 0; j < components; ++j) {
					bgra[j] = *(sourceMainPtr++);
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
	// Unbind
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); GLERR(112)


	// Horizontal blur
	glBindFramebuffer(GL_FRAMEBUFFER, g->fboID[0]); GLERR(1120)
	glViewport(0, 0, g->width, g->height); GLERR(1121)
	glUseProgram(g->shaders[0].program); GLERR(1122)
	glBindVertexArray(g->shaders[0].drawAttributeArray); GLERR(1123)
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear this in v-color
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4); GLERR(1124)
	glBindVertexArray(0); GLERR(1125)
	glUseProgram(0); GLERR(1126)
	glBindFramebuffer(GL_FRAMEBUFFER, 0); GLERR(1127)



	// Horizontal blur
	glBindTexture(GL_TEXTURE_2D, g->fboTextures[0]);
	glBindSampler(0, g->fboTextureSamplers[0]);
	glBindFramebuffer(GL_FRAMEBUFFER, g->fboID[1]); GLERR(1120)
	glViewport(0, 0, g->width, g->height); GLERR(1121)
	glUseProgram(g->shaders[1].program); GLERR(1122)
	glBindVertexArray(g->shaders[1].drawAttributeArray); GLERR(1123)
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4); GLERR(1124)
	glBindVertexArray(0); GLERR(1125)
	glUseProgram(0); GLERR(1126)
	glBindFramebuffer(GL_FRAMEBUFFER, 0); GLERR(1127)


	// Read texture from GPU
	glBindTexture(GL_TEXTURE_2D, g->fboTextures[1]);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, g->pixelBufferOut); GLERR(106)
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



VBlur::GraphicsData :: GraphicsData(void* data) :
	VGraphicsBase::GraphicsData(data),
	width(0),
	height(0),
	left(0),
	top(0)
{
	// Init
	int i, x, y;
	this->clipTexture = 0;
	this->clipTextureSampler = 0;
	for (i = 0; i < 2; ++i) {
		this->rboID[i] = 0;
		this->fboID[i] = 0;
		this->fboTextures[i] = 0;
		this->fboTextureSamplers[i] = 0;
		this->pixelBuffers[i] = 0;
		this->shaders[i].sources[0] = 0;
		this->shaders[i].sources[1] = 0;
		this->shaders[i].program = 0;
		this->shaders[i].drawAttributeArray = 0;
	}
	this->drawVertexBuffer = 0;

	// Setup
	VBlur* self = static_cast<VBlur*>(data);


	// Graphics targets
	this->width = self->clipInfo.width + (self->blur[0].edgeMethod != VBlur::EDGE_CLAMP ? 2 : 0);
	this->height = self->clipInfo.height + (self->blur[1].edgeMethod != VBlur::EDGE_CLAMP ? 2 : 0);
	this->width = static_cast<int>(::pow(2.0, static_cast<int>(::ceil(Math::log2(static_cast<double>(this->width))))));
	this->height = static_cast<int>(::pow(2.0, static_cast<int>(::ceil(Math::log2(static_cast<double>(this->height))))));

	this->left = (this->width - self->clipInfo.width) / 2;
	this->top = (this->height - self->clipInfo.height) / 2;


	// Generate
	glGenTextures(1, &this->clipTexture); // Textures
	glGenSamplers(1, &this->clipTextureSampler); // Texture samplers

	glGenBuffers(2, this->pixelBuffers); // Pixel buffers
	glGenFramebuffers(2, this->fboID); // Framebuffers
	glGenRenderbuffers(2, this->rboID); // Renderbuffers

	glGenTextures(2, this->fboTextures); // FBO textures
	glGenSamplers(2, this->fboTextureSamplers); // FBO texture samplers


	// Empty texture
	unsigned char* textureData = new unsigned char[this->width * this->height * 4];
	unsigned char* textureDataPixel = textureData;
	for (y = 0; y < this->height; ++y) {
		for (x = 0; x < this->width; ++x) {
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
	{ // Fill with color
		// Define regions
		struct {
			int xMin, xMax, yMin, yMax;
		} rectangles[4];
		int offset;
		int bId;

		// Technically, this could be done with only 1 row/column of colored pixels, but whatever
		// Left
		rectangles[0].xMin = 0;
		rectangles[0].xMax = this->left;
		rectangles[0].yMin = this->top;
		rectangles[0].yMax = this->top + self->clipInfo.height;

		// Right
		rectangles[1].xMin = this->left + self->clipInfo.width;
		rectangles[1].xMax = this->width;
		rectangles[1].yMin = this->top;
		rectangles[1].yMax = this->top + self->clipInfo.height;

		// Top
		rectangles[2].xMin = this->left;
		rectangles[2].xMax = this->left + self->clipInfo.width;
		rectangles[2].yMin = 0;
		rectangles[2].yMax = this->top;

		// Bottom
		rectangles[3].xMin = this->left;
		rectangles[3].xMax = this->left + self->clipInfo.width;
		rectangles[3].yMin = this->top + self->clipInfo.height;
		rectangles[3].yMax = this->height;

		// Loop
		for (i = 0; i < 4; ++i) {
			bId = i / 2;
			for (y = rectangles[i].yMin; y < rectangles[i].yMax; ++y) {
				for (x = rectangles[i].xMin; x < rectangles[i].xMax; ++x) {
					offset = (y * this->width + x) * 4;
					textureData[offset + 0] = self->blur[bId].edgeColor.rgba[0];
					textureData[offset + 1] = self->blur[bId].edgeColor.rgba[1];
					textureData[offset + 2] = self->blur[bId].edgeColor.rgba[2];
					textureData[offset + 3] = self->blur[bId].edgeColor.rgba[3];
				}
			}
		}
	}

	// Graphics
	for (i = 0; i < 2; ++i) {
		// Create texture
		glBindTexture(GL_TEXTURE_2D, this->fboTextures[i]);

		// Empty texture
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);

		// Samplers
		glSamplerParameteri(this->fboTextureSamplers[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(this->fboTextureSamplers[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(this->fboTextureSamplers[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST); // interpolated, no mipmaps
		glSamplerParameteri(this->fboTextureSamplers[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST); // interpolated, no mipmaps

		// Bind framebuffer / renderbuffer
		glBindFramebuffer(GL_FRAMEBUFFER, this->fboID[i]);
		glBindRenderbuffer(GL_RENDERBUFFER, this->rboID[i]);

		// Create/attach depth
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, this->width, this->height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, this->rboID[i]);

		// Attach color
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->fboTextures[i], 0);

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


	// Graphics part 2
	glBindTexture(GL_TEXTURE_2D, this->clipTexture);
	// Empty texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
	// Samplers
	glSamplerParameteri(this->clipTextureSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->clipTextureSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->clipTextureSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // interpolated, no mipmaps
	glSamplerParameteri(this->clipTextureSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // interpolated, no mipmaps


	// Delete empty texture
	delete [] textureData;


	// Create pixel buffer memory
	size_t data_size = self->clipInfo.width * self->clipInfo.height * 4;
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, this->pixelBufferIn);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, data_size, NULL, GL_DYNAMIC_DRAW);

	data_size = this->width * this->height * 4;
	glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pixelBufferOut);
	glBufferData(GL_PIXEL_PACK_BUFFER, data_size, NULL, GL_DYNAMIC_READ);


	//{ Vertex buffer
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
		xPos[1] = ((this->left + self->clipInfo.width) / static_cast<double>(this->width));
		yPos[1] = ((this->top + self->clipInfo.height) / static_cast<double>(this->height));

		for (i = 0; i < 4; ++i) {
			vData[i].position[0] = xPos[((i + 1) / 2) % 2] * 2.0 - 1.0;
			vData[i].position[1] = yPos[i / 2] * 2.0 - 1.0;
			vData[i].texCoord[0] = xPos[((i + 1) / 2) % 2];
			vData[i].texCoord[1] = yPos[i / 2];
		}
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//}


	// Shaders
	for (int sId = 0; sId < 2; ++sId) {
		// Create shaders
		this->shaders[sId].vertex = glCreateShader(GL_VERTEX_SHADER);
		this->shaders[sId].fragment = glCreateShader(GL_FRAGMENT_SHADER);
		this->shaders[sId].program = glCreateProgram();

		// Shader sources
		std::stringstream ssVertex;
		std::stringstream ssFragment;
		self->graphicsInitShaderSources(this, &ssVertex, &ssFragment, sId != 0);
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
		glShaderSource(this->shaders[sId].vertex, 1, &static_cast<const GLchar*>(sSource), NULL);
		temp = ssFragment.str();
		sSource = temp.c_str();
		glShaderSource(this->shaders[sId].fragment, 1, &static_cast<const GLchar*>(sSource), NULL);

		// Compile
		for (i = 0; i < 2; ++i) {
			glCompileShader(this->shaders[sId].sources[i]);
			// Compile check
			GLint compiled;
			glGetShaderiv(this->shaders[sId].sources[i], GL_COMPILE_STATUS, &compiled);
			if (compiled == GL_FALSE) {
				// Error check
				this->error() << "VBlur: Error compiling shader[" << sId << "][" << i << "]";
				int logLength = 0;
				glGetShaderiv(this->shaders[sId].sources[i], GL_INFO_LOG_LENGTH, &logLength);
				if (logLength > 0) {
					// Error
					char* infoLog = new char[logLength];
					glGetShaderInfoLog(this->shaders[sId].sources[i], logLength, &logLength, infoLog);
					this->error() << ":\n" << infoLog;
					delete [] infoLog;
				}

				return;
			}
		}

		// Attach
		glAttachShader(this->shaders[sId].program, this->shaders[sId].vertex);
		glAttachShader(this->shaders[sId].program, this->shaders[sId].fragment);

		// Bindings
		for (i = 0; shaderOutputs[i] != NULL; ++i) {
			glBindFragDataLocation(this->shaders[sId].program, i, shaderOutputs[i]);
		}

		// Link
		glLinkProgram(this->shaders[sId].program);
		GLint linked;
		glGetProgramiv(this->shaders[sId].program, GL_LINK_STATUS, &linked);
		if (linked == GL_FALSE) {
			// Not linked
			this->error() << "VBlur: Error linking shader[" << sId << "]";

			int logLength = 0;
			glGetProgramiv(this->shaders[sId].program, GL_INFO_LOG_LENGTH, &logLength);
			if (logLength > 0) {
				// Error
				char* infoLog = new char[logLength];
				glGetProgramInfoLog(this->shaders[sId].program, logLength, &logLength, infoLog);
				this->error() << ":\n" << infoLog;
				delete [] infoLog;
			}

			// Done
			return;
		}

		// Uniforms
		{
			glUseProgram(this->shaders[sId].program);

			// Texture units
			GLint uniform = glGetUniformLocation(this->shaders[sId].program, "in_TextureUnit");
			if (uniform >= 0) {
				glUniform1i(uniform, 0);
			}

			uniform = glGetUniformLocation(this->shaders[sId].program, "in_TextureRangeMin");
			if (uniform >= 0) {
				glUniform2f(uniform,
					static_cast<float>(this->left - (self->blur[0].edgeMethod != VBlur::EDGE_CLAMP ? 1 : 0) + 0.5) / this->width,
					static_cast<float>(this->top - (self->blur[1].edgeMethod != VBlur::EDGE_CLAMP ? 1 : 0) + 0.5) / this->height // flipped
				);
			}

			uniform = glGetUniformLocation(this->shaders[sId].program, "in_TextureRangeMax");
			if (uniform >= 0) {
				glUniform2f(uniform,
					static_cast<float>(this->left + self->clipInfo.width + (self->blur[0].edgeMethod != VBlur::EDGE_CLAMP ? 1 : 0) - 0.5) / this->width,
					static_cast<float>(this->top + self->clipInfo.height + (self->blur[1].edgeMethod != VBlur::EDGE_CLAMP ? 1 : 0) - 0.5) / this->height // flipped
				);
			}

			glUseProgram(0);
		}


		// Vertex attribute array
		glGenVertexArrays(1, &this->shaders[sId].drawAttributeArray);
		glBindVertexArray(this->shaders[sId].drawAttributeArray);
		glBindBuffer(GL_ARRAY_BUFFER, this->drawVertexBuffer);
		size_t vaOffset = 0;
		for (i = 0; shaderInputs[i] != NULL; ++i) {
			GLuint shaderAttr = glGetAttribLocation(this->shaders[sId].program, shaderInputs[i]);
			glEnableVertexAttribArray(shaderAttr);
			glVertexAttribPointer(shaderAttr, shaderInputsAttributes[i * 4 + 0], shaderInputsAttributes[i * 4 + 1], shaderInputsAttributes[i * 4 + 3], sizeof(ShaderInputData), (GLvoid*) (vaOffset));
			vaOffset += shaderInputsAttributes[i * 4 + 0] * shaderInputsAttributes[i * 4 + 2];
		}
		glBindVertexArray(0);
	}


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
		this->error() << "VBlur: error in graphics setup: " << static_cast<int>(e);
	}
}
VBlur::GraphicsData :: ~GraphicsData() {
	// TODO : Delete resources
}



AVSValue __cdecl VBlur :: create1(AVSValue args, void* user_data, IScriptEnvironment* env) {
	// Video blur
	return static_cast<AVSValue>(new VBlur(
		env,
		args[0].AsClip(), // clip
		args[1].AsFloat(4.0), // x_blur
		args[2].AsFloat(4.0), // y_blur
		args[3].AsString("clamp"), // x_edge
		args[4].AsString("clamp"), // y_edge
		args[5].AsInt(0x000000), // x_color
		args[6].AsInt(0x000000), // y_color
		args[7].AsInt(-1.0), // x_interval
		args[8].AsInt(-1.0), // y_interval
		args[9].AsString("gaussian"), // x_function
		args[10].AsString("gaussian"), // y_function
		args[11].AsFloat(1.0), // x_polynomial_power
		args[12].AsFloat(1.0), // y_polynomial_power
		args[13].AsFloat(0.0), // x_polynomial_intercept
		args[14].AsFloat(0.0), // y_polynomial_intercept
		args[15].AsBool(true) // gpu
	));
}
const char* VBlur :: registerClass(IScriptEnvironment* env) {
	/**
		clip.VBlur(...)

		Blur a clip with custom parameters.

		@param clip
			The clip to blur
		@param x_blur
			The radius (in pixels) to blur horizontally
			Default: 4
		@param y_blur
			The radius (in pixels) to blur vertically
			Default: 4
		@param x_edge
			The horizontal edge clamping method
			Values:
				"clamp",
				"wrap",
				"color"
			Default: "clamp"
		@param y_edge
			The vertical edge clamping method
			Values:
				"clamp",
				"wrap",
				"color"
			Default: "clamp"
		@param x_color
			The color to be used when using horizontal color edges
			Default: $000000
		@param y_color
			The color to be used when using vertical color edges
			Default: $000000
		@param x_interval
			The horizontal pixel sampling interval
			Default: 1.0
		@param y_interval
			The horizontal pixel sampling interval
			Default: 1.0
		@param x_function
			The function to use for horizontal blurring
			Values:
				"gaussian",
				"polynomial"
			Default: "gaussian"
		@param y_function
			The function to use for vertical blurring
			Values:
				"gaussian",
				"polynomial"
			Default: "gaussian"
		@param x_polynomial_power
			This value is only applicable when "x_function" is "polynomial"
			The function used for polynomials is the following:
				y = intercept + ((1.0 - x / radius) ^ (power)) * (1.0 - intercept)
			This value represents "power" in the equation
		@param y_polynomial_power
			This value is only applicable when "y_function" is "polynomial"
			The function used for polynomials is the following:
				y = intercept + ((1.0 - x / radius) ^ (power)) * (1.0 - intercept)
			This value represents "power" in the equation
		@param x_polynomial_intercept
			This value is only applicable when "x_function" is "polynomial"
			The function used for polynomials is the following:
				y = intercept + ((1.0 - x / radius) ^ (power)) * (1.0 - intercept)
			This value represents "intercept" in the equation
		@param y_polynomial_intercept
			This value is only applicable when "y_function" is "polynomial"
			The function used for polynomials is the following:
				y = intercept + ((1.0 - x / radius) ^ (power)) * (1.0 - intercept)
			This value represents "intercept" in the equation
		@param gpu
			Use the GPU to modify the frame
			Default: true
			@note Presently, this does not do anything when gpu=false

		@default
			clip.VBlur( \
				x_blur = 4, \
				y_blur = 4, \
				x_edge = "clamp", \
				y_edge = "clamp", \
				x_color = #000000, \
				y_color = #000000, \
				x_interval = 1.0, \
				y_interval = 1.0, \
				x_function = "gaussian", \
				y_function = "gaussian", \
				x_polynomial_power = 1.0, \
				y_polynomial_power = 1.0, \
				x_polynomial_intercept = 1.0, \
				y_polynomial_intercept = 1.0, \
				gpu = true \
			)
	*/
	env->AddFunction(
		"VBlur",
		"[clip]c[x_blur]f[y_blur]f[x_edge]s[y_edge]s[x_color]i[y_color]i[x_interval]f[y_interval]f"
		"[x_function]s[y_function]s[x_polynomial_power]f[y_polynomial_power]f[x_polynomial_intercept]f[y_polynomial_intercept]f"
		"[gpu]b",
		VBlur::create1,
		NULL
	);

	return "VBlur";
}



FUNCTION_END();
REGISTER(VBlur);


