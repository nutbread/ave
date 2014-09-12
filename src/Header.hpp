// Version must be defined
#ifndef PLUGIN_AVISYNTH_VERSION
#error PLUGIN_AVISYNTH_VERSION not defined
#endif



// Main header
#ifndef ___PLUGIN_HEADER_H
#define ___PLUGIN_HEADER_H



#define WIN32_LEAN_AND_MEAN 1
#define _USE_MATH_DEFINES 1
#include <string>
#include <vector>
#include <algorithm>
#include <windows.h>



// Registration management class
template <class Environment> class Registration {
public:
	typedef const char* (*RegisterFunction)(Environment* env);

private:
	static Registration* first;
	static Registration** last;

	Registration* next;
	RegisterFunction fcn;
	std::string fcnName;

private:
	~Registration() {
	}

	static bool fcnNameSortCompare(std::string* x, std::string* y) {
		return (x->compare(*y) < 0);
	}

public:
	Registration(RegisterFunction fcn, const char* fcnName) :
		next(NULL),
		fcn(fcn)
	{
		// Update list pointers
		*(Registration<Environment>::last) = this;
		Registration<Environment>::last = &this->next;

		// Function name with any whitespace removed
		this->fcnName = fcnName;
		size_t i = 0;
		size_t j = this->fcnName.length();
		while (fcnName[i] < ' ' && ++i < j);
		while (--j >= i && fcnName[j] < ' ');

		// Only substr it if necessary
		++j;
		if (i > 0 || j < this->fcnName.length()) {
			this->fcnName = this->fcnName.substr(i, j - i);
		}
	}

	static void setup(Environment* const env, std::stringstream* const pluginList, const char* const separator) {
		// Setup name list
		Registration<Environment>* reg = Registration<Environment>::first;
		Registration<Environment>* regNext;
		std::vector<std::string*> regNameList;
		std::string* fcnName;

		while (reg != NULL) {
			// Call function
			fcnName = new std::string((*reg->fcn)(env));

			// Add to plugin list name
			regNameList.push_back(fcnName);

			// Delete
			regNext = reg->next;
			delete reg;
			reg = regNext;
		}

		// Sort
		std::sort(regNameList.begin(), regNameList.end(), &Registration<Environment>::fcnNameSortCompare);

		// Update name list
		bool first = true;
		for (std::vector<std::string*>::iterator it = regNameList.begin(); it != regNameList.end(); ++it) {
			if (first) {
				first = false;
			}
			else {
				*pluginList << separator;
			}

			*pluginList << *(*it);
			delete (*it);
		}
	}

};
template <class Environment> Registration<Environment>* Registration<Environment>::first = NULL;
template <class Environment> Registration<Environment>** Registration<Environment>::last = &Registration<Environment>::first;



template <class RegisterClass> class RegisteredClass {
public:
	static void* reg;

private:
	RegisteredClass();
	~RegisteredClass();

};



#endif



// Version differences
#if (PLUGIN_AVISYNTH_VERSION == 25)

#	ifndef ___PLUGIN_AVISYNTH_25_H
#	define ___PLUGIN_AVISYNTH_25_H

		// Undefine
#		undef CacheHintsReturnType
#		undef CacheHintsReturn
#		undef FUNCTION_START
#		undef FUNCTION_END
#		undef REGISTER

		// 2.5x
#		define CacheHintsReturnType void
#		define CacheHintsReturn(value)

#		define FUNCTION_START() namespace AviSynthV25 {
#		define FUNCTION_END() };

#		define REGISTER(cls) void* ::RegisteredClass<::AviSynthV25::cls>::reg = static_cast<void*>(new ::Registration<::AviSynthV25::IScriptEnvironment>(&::AviSynthV25::cls::registerClass, #cls));

		FUNCTION_START();
#		undef __AVISYNTH_H__
#		include "Headers/avisynth25.h"
		FUNCTION_END();

#	endif

#else

#	ifndef ___PLUGIN_AVISYNTH_26_H
#	define ___PLUGIN_AVISYNTH_26_H

		// Undefine
#		undef CacheHintsReturnType
#		undef CacheHintsReturn
#		undef FUNCTION_START
#		undef FUNCTION_END
#		undef REGISTER

		// 2.6x
#		define CacheHintsReturnType int
#		define CacheHintsReturn(value) value

#		define FUNCTION_START() namespace AviSynthV26 {
#		define FUNCTION_END() };

#		define REGISTER(cls) void* ::RegisteredClass<::AviSynthV26::cls>::reg = static_cast<void*>(new ::Registration<::AviSynthV26::IScriptEnvironment>(&::AviSynthV26::cls::registerClass, #cls));

		FUNCTION_START();
#		undef __AVISYNTH_H__
#		include "Headers/avisynth26.h"
		extern const AVS_Linkage* AVS_linkage;
		FUNCTION_END();

#	endif

#endif





