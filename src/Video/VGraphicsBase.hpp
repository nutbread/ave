#ifndef ___PLUGIN_VGRAPHICSBASE_H
#define ___PLUGIN_VGRAPHICSBASE_H



#include "../Header.hpp"
#include <sstream>
#include <cassert>
FUNCTION_START();



// OpenGL graphics class
class VGraphicsBase :
	public IClip
{
protected:
	class GraphicsData {
		friend class VGraphicsBase;

	private:
		bool encounteredError;
		std::stringstream errorMessage;

		typedef GraphicsData* (*CreateFunction)(void*);

		template <class VGType> static GraphicsData* create(void* data) {
			return static_cast<GraphicsData*>(new VGType(data));
		}

	protected:
		GraphicsData(void* data);
		virtual ~GraphicsData();

		std::stringstream& error();

	public:
		bool hasError() const;
		const std::stringstream& getError() const;

	};

private:
	struct InitData {
		VGraphicsBase* self;
		std::stringstream* errorMessage;
	};
	struct TransformData {
		VGraphicsBase* self;
		PVideoFrame returnValue;
		int n;
		IScriptEnvironment* env;
	};

	TransformData transformData;
	bool graphicsSetup;
	bool useGPU;
	GraphicsData::CreateFunction createFunction;
	void* createData;

	static int graphicsInitFunction(void* data);
	static int graphicsDeinitFunction(void* data);
	static int graphicsTransformFunction(void* data);

protected:
	GraphicsData* graphics;

	virtual PVideoFrame transformFrame(int n, IScriptEnvironment* env) = 0;
	virtual PVideoFrame transformFrameGPU(int n, IScriptEnvironment* env) = 0;

	template <class VGType> void setupInit(void* createData) {
		assert(this->createFunction == NULL);

		this->createFunction = GraphicsData::create<VGType>;
		this->createData = createData;
	}

public:
	VGraphicsBase(IScriptEnvironment* env, bool useGPU);
	virtual ~VGraphicsBase();

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

};



FUNCTION_END();



#endif

