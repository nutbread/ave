#include "VGraphicsBase.hpp"
#include "../Graphics.hpp"
#include <cassert>
FUNCTION_START();



// Graphics base class
VGraphicsBase :: VGraphicsBase(IScriptEnvironment* env, bool useGPU) :
	IClip(),
	transformData(),
	graphicsSetup(false),
	useGPU(false),
	graphics(NULL),
	createFunction(NULL),
	createData(NULL)
{
	if (useGPU) {
		Graphics::init();
		this->useGPU = Graphics::okay();
	}

	this->transformData.self = this;
}
VGraphicsBase :: ~VGraphicsBase() {
	// Delete graphics
	if (this->graphics != NULL) {
		InitData* id = new InitData();
		id->self = this;
		id->errorMessage = NULL;

		Graphics::execute(&VGraphicsBase::graphicsDeinitFunction, id);

		delete id;
	}
}

PVideoFrame __stdcall VGraphicsBase :: GetFrame(int n, IScriptEnvironment* env) {
	if (this->useGPU) {
		// GPU
		if (!this->graphicsSetup) {
			// Setup
			std::stringstream errorMessage;

			// Required setup
			this->graphicsSetup = true;

			// Custom setup
			InitData* id = new InitData();
			id->self = this;
			id->errorMessage = &errorMessage;

			bool error = Graphics::execute(&VGraphicsBase::graphicsInitFunction, id);

			delete id;

			if (error) {
				// Error
				env->ThrowError(this->graphics->getError().str().c_str());
				this->useGPU = false;
				return NULL;
			}
		}

		// Run frame transform
		this->transformData.n = n;
		this->transformData.env = env;
		Graphics::execute(&VGraphicsBase::graphicsTransformFunction, &this->transformData);
		return this->transformData.returnValue;
	}
	else {
		// CPU
		return this->transformFrame(n, env);
	}
}
int VGraphicsBase :: graphicsInitFunction(void* data) {
	assert(static_cast<InitData*>(data)->self->createFunction != NULL);

	// Create
	VGraphicsBase::GraphicsData* gd = (static_cast<InitData*>(data)->self->createFunction)(static_cast<InitData*>(data)->self->createData);
	static_cast<InitData*>(data)->self->graphics = gd;

	return gd->hasError();
}
int VGraphicsBase :: graphicsDeinitFunction(void* data) {
	delete static_cast<InitData*>(data)->self->graphics;

	return 0;
}
int VGraphicsBase :: graphicsTransformFunction(void* data) {
	// Call function
	static_cast<TransformData*>(data)->returnValue = static_cast<TransformData*>(data)->self->transformFrameGPU(
		static_cast<TransformData*>(data)->n,
		static_cast<TransformData*>(data)->env
	);

	// Done
	return 0;
}

VGraphicsBase::GraphicsData :: GraphicsData(void* data) :
	encounteredError(false),
	errorMessage()
{
}
VGraphicsBase::GraphicsData :: ~GraphicsData() {
}
std::stringstream& VGraphicsBase::GraphicsData :: error() {
	// Set error
	this->encounteredError = true;
	// Return error
	return this->errorMessage;
}
bool VGraphicsBase::GraphicsData :: hasError() const {
	return this->encounteredError;
}
const std::stringstream& VGraphicsBase::GraphicsData :: getError() const {
	return this->errorMessage;
}



FUNCTION_END();





