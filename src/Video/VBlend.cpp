#include "VBlend.hpp"
#include <cassert>
FUNCTION_START();



// Clip blending
VBlend :: VBlend(
	IScriptEnvironment* env,
	PClip clip1,
	PClip clip2,

	InitData* r,
	InitData* g,
	InitData* b,
	InitData* a,

	unsigned int borderColor1,
	unsigned int borderColor2
) :
	IClip(),
	hasOffset(false)
{
	// Get info
	this->clip[0] = clip1;
	this->clip[1] = clip2;
	this->clipInfo[0] = this->clip[0]->GetVideoInfo();
	this->clipInfo[1] = this->clip[1]->GetVideoInfo();

	// Validation
	if (!this->clipInfo[0].IsRGB24() && !this->clipInfo[0].IsRGB32()) {
		env->ThrowError("VBlend: RGB24/RGB32 data only [1]");
	}
	if (!this->clipInfo[1].IsRGB24() && !this->clipInfo[1].IsRGB32()) {
		env->ThrowError("VBlend: RGB24/RGB32 data only [2]");
	}
	if (this->clipInfo[0].IsRGB24() != this->clipInfo[1].IsRGB24() || this->clipInfo[0].IsRGB32() != this->clipInfo[1].IsRGB32()) {
		env->ThrowError("VBlend: Clip format mismatch");
	}
	if (this->clipInfo[0].width != this->clipInfo[1].width || this->clipInfo[0].height != this->clipInfo[1].height) {
		env->ThrowError("VBlend: Clip dimension mismatch");
	}

	// Setup
	InitData* initDatas[4];
	initDatas[0] = r;
	initDatas[1] = g;
	initDatas[2] = b;
	initDatas[3] = a;

	// Parse init datas
	bool hasWord;
	int pos, id;
	std::string str, word;
	for (int i = 0; i < 4; ++i) {
		// Validate
		assert(initDatas[i] != NULL);
		if (initDatas[i]->factors[0] == NULL || initDatas[i]->factors[1] == NULL || initDatas[i]->equation == NULL) {
			env->ThrowError("VBlend: Incomplete data");
			break;
		}

		// Factors
		for (int j = 0; j < 2; ++j) {
			// Default
			this->components[i].factors[j] = FT_COMPONENT;
			this->components[i].factorValues[j] = 1.0;
			this->components[i].factorComponent[j] = i;
			this->components[i].factorSource[j] = j;
			this->components[i].factorInvert[j] = false;

			// Values
			pos = 0;
			VBlend::cStringToLower(initDatas[i]->factors[j], str);

			if (VBlend::getWord(str, word, pos)) {
				if (word == "value") {
					// Get the value
					if (VBlend::getWord(str, word, pos)) {
						this->components[i].factors[j] = FT_VALUE;
						this->components[i].factorValues[j] = atof(word.c_str());
						if (this->components[i].factorValues[j] < 0.0) this->components[i].factorValues[j] = 0.0;
						else if (this->components[i].factorValues[j] > 1.0) this->components[i].factorValues[j] = 1.0;
					}
				}
				else {
					// What type
					hasWord = true;
					if (word == "color") {
						this->components[i].factors[j] = FT_COMPONENT;
						this->components[i].factorComponent[j] = i;
					}
					else if (word == "alpha") {
						this->components[i].factors[j] = FT_COMPONENT;
						this->components[i].factorComponent[j] = 3;
					}
					else if (word == "red") {
						this->components[i].factors[j] = FT_COMPONENT;
						this->components[i].factorComponent[j] = 0;
					}
					else if (word == "green") {
						this->components[i].factors[j] = FT_COMPONENT;
						this->components[i].factorComponent[j] = 1;
					}
					else if (word == "blue") {
						this->components[i].factors[j] = FT_COMPONENT;
						this->components[i].factorComponent[j] = 2;
					}
					else {
						hasWord = false;
					}

					// Next
					if (hasWord && (hasWord = VBlend::getWord(str, word, pos))) {
						if (word == "rev" || word == "reverse" || word == "reversed" || word == "inv" || word == "inverse" || word == "inversed") {
							this->components[i].factorInvert[j] = true;

							hasWord = VBlend::getWord(str, word, pos);
						}

						if (hasWord) {
							// Get id
							id = atoi(word.c_str());
							if (id < 1) id = 1;
							else if (id > 2) id = 2;

							// Set id
							this->components[i].factorSource[j] = id - 1;
						}
					}
				}
			}
		}

		// Equation
		pos = 0;
		VBlend::cStringToLower(initDatas[i]->equation, str);
		VBlend::getWord(str, word, pos);
		if (word == "sub" || word == "subtract") this->components[i].equation = EQ_SUBTRACT;
		else if (word == "mult" || word == "multiply") this->components[i].equation = EQ_MULTIPLY;
		else if (word == "div" || word == "divide") this->components[i].equation = EQ_DIVIDE;
		else if (word == "max") this->components[i].equation = EQ_MAX;
		else if (word == "min") this->components[i].equation = EQ_MIN;
		else this->components[i].equation = EQ_ADD;

		if (VBlend::getWord(str, word, pos)) {
			this->components[i].equationReversed = (word == "rev" || word == "reverse" || word == "reversed" || word == "inv" || word == "inverse" || word == "inversed");
		}

		// Offsets
		this->components[i].offset[0][0] = initDatas[i]->offsets[0];
		this->components[i].offset[0][1] = initDatas[i]->offsets[1];
		this->components[i].offset[1][0] = initDatas[i]->offsets[2];
		this->components[i].offset[1][1] = initDatas[i]->offsets[3];
		if (!this->hasOffset) {
			for (int j = 0; j < 4; ++j) {
				this->hasOffset = (initDatas[i]->offsets[j] != 0);
				if (this->hasOffset) break;
			}
		}
	}

	// Border color
	this->borderColor[0][0] = (borderColor1 & 0xFF);
	this->borderColor[0][1] = ((borderColor1 >> 8) & 0xFF);
	this->borderColor[0][2] = ((borderColor1 >> 16) & 0xFF);
	this->borderColor[0][3] = 0xFF - ((borderColor1 >> 24) & 0xFF);
	this->borderColor[1][0] = (borderColor2 & 0xFF);
	this->borderColor[1][1] = ((borderColor2 >> 8) & 0xFF);
	this->borderColor[1][2] = ((borderColor2 >> 16) & 0xFF);
	this->borderColor[1][3] = 0xFF - ((borderColor2 >> 24) & 0xFF);
}

PVideoFrame VBlend :: getFrameWithoutOffset(int n, IScriptEnvironment* env) {
	// Destination
	PVideoFrame dest = env->NewVideoFrame(this->clipInfo[0]);
	unsigned char* destPtr = dest->GetWritePtr();

	// Get main source
	PVideoFrame source[2];
	const unsigned char* sourcePtr[2];

	source[0] = this->clip[0]->GetFrame(n, env);
	source[1] = this->clip[1]->GetFrame(n, env);
	sourcePtr[0] = source[0]->GetReadPtr();
	sourcePtr[1] = source[1]->GetReadPtr();

	union {
		int rgba[4];
	} color[2];
	int modifyColor[4];

	// Loop values
	int x, y, c;
	int width = this->clipInfo[0].width;
	int height = this->clipInfo[0].height;
	bool hasAlpha = this->clipInfo[0].IsRGB32();
	int components = 3 + hasAlpha;

	// Copy from main source
	for (y = 0; y < height; ++y) {
		// Loop over line
		for (x = 0; x < width; ++x) {
			// Get old
			for (c = 0; c < 3; ++c) {
				color[0].rgba[2 - c] = *(sourcePtr[0] + c);
				color[1].rgba[2 - c] = *(sourcePtr[1] + c);
			}
			if (hasAlpha) {
				color[0].rgba[3] = *(sourcePtr[0] + 3);
				color[1].rgba[3] = *(sourcePtr[1] + 3);
			}

			// Modify
			modifyColor[2] = this->modify(0, color[0].rgba, color[1].rgba);
			modifyColor[1] = this->modify(1, color[0].rgba, color[1].rgba);
			modifyColor[0] = this->modify(2, color[0].rgba, color[1].rgba);
			if (hasAlpha) {
				modifyColor[3] = this->modify(3, color[0].rgba, color[1].rgba);
			}

			// Set new
			for (c = 0; c < components; ++c) {
				*(destPtr + c) = modifyColor[c];
			}

			// Continue
			destPtr += components;
			sourcePtr[0] += components;
			sourcePtr[1] += components;
		}
	}

	// Done
	return dest;
}
PVideoFrame VBlend :: getFrameWithOffset(int n, IScriptEnvironment* env) {
	// TODO
	// Destination
	PVideoFrame dest = env->NewVideoFrame(this->clipInfo[0]);
	unsigned char* destPtr = dest->GetWritePtr();

	// Get main source
	PVideoFrame source[2];
	const unsigned char* sourcePtr[2];

	source[0] = this->clip[0]->GetFrame(n, env);
	source[1] = this->clip[1]->GetFrame(n, env);
	sourcePtr[0] = source[0]->GetReadPtr();
	sourcePtr[1] = source[1]->GetReadPtr();

	union {
		int rgba[4];
	} color[2];
	int modifyColor[4];

	// Loop values
	int x, y, c;
	int width = this->clipInfo[0].width;
	int height = this->clipInfo[0].height;
	bool hasAlpha = this->clipInfo[0].IsRGB32();
	int components = 3 + hasAlpha;

	// Copy from main source
	for (y = 0; y < height; ++y) {
		// Loop over line
		for (x = 0; x < width; ++x) {
			// Get old
			for (c = 0; c < 3; ++c) {
				color[0].rgba[2 - c] = *(sourcePtr[0] + c);
				color[1].rgba[2 - c] = *(sourcePtr[1] + c);
			}
			if (hasAlpha) {
				color[0].rgba[3] = *(sourcePtr[0] + 3);
				color[1].rgba[3] = *(sourcePtr[1] + 3);
			}

			// Modify
			modifyColor[2] = this->modify(0, color[0].rgba, color[1].rgba);
			modifyColor[1] = this->modify(1, color[0].rgba, color[1].rgba);
			modifyColor[0] = this->modify(2, color[0].rgba, color[1].rgba);
			if (hasAlpha) {
				modifyColor[3] = this->modify(3, color[0].rgba, color[1].rgba);
			}

			// Set new
			for (c = 0; c < components; ++c) {
				*(destPtr + c) = modifyColor[c];
			}

			// Continue
			destPtr += components;
			sourcePtr[0] += components;
			sourcePtr[1] += components;
		}
	}

	// Done
	return dest;
}

PVideoFrame __stdcall VBlend :: GetFrame(int n, IScriptEnvironment* env) {
	return this->hasOffset ? this->getFrameWithOffset(n, env) : this->getFrameWithoutOffset(n, env);
}
void __stdcall VBlend :: GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
	this->clip[0]->GetAudio(buf, start, count, env);
}
const VideoInfo& __stdcall VBlend :: GetVideoInfo() {
	return this->clipInfo[0];
}
bool __stdcall VBlend :: GetParity(int n) {
	return this->clip[0]->GetParity(n);
}
CacheHintsReturnType __stdcall VBlend :: SetCacheHints(int cachehints, int frame_range) {
	// Do not pass cache requests upwards, only to the next filter
	return CacheHintsReturn(0);
}

void VBlend :: cStringToLower(const char* cstr, std::string& target) {
	assert(cstr != NULL);

	target = cstr;
	for (size_t i = 0; i < target.length(); ++i) {
		if (target[i] >= 'A' && target[i] <= 'Z') {
			target[i] -= 'A' - 'a';
		}
	}
}
bool VBlend :: getWord(const std::string& str, std::string& word, int& pos) {
	word = "";

	while (pos < str.length()) {
		if (str[pos] > ' ') {
			word += str[pos];
		}
		else if (word.length() > 0) {
			return true;
		}

		++pos;
	}

	return (word.length() > 0);
}
bool VBlend :: isStringInt(const std::string& str) {
	for (int i = 0; i < str.length(); ++i) {
		if (str[i] < '0' || str[i] > '9') return false;
	}

	return true;
}
int VBlend :: sign(double value) {
	return (value > 0.0 ? 1 : (value < 0.0 ? -1 : 0));
}
int VBlend :: modify(int c, int* source1Values, int* source2Values) {
	EquationType eqn = this->components[c].equation;
	bool eqnid = this->components[c].equationReversed;
	double vs[2];
	int* sources[2];

	// Get the original value
	sources[0] = source1Values;
	sources[1] = source2Values;

	// Apply factors
	for (int i = 0; i < 2; ++i) {
		vs[eqnid] = sources[i][c] / 255.0;

		if (this->components[c].factors[i] == FT_COMPONENT) {
			vs[eqnid] *= sources[this->components[c].factorSource[i]][this->components[c].factorComponent[i]] / 255.0;

			if (this->components[c].factorInvert[i]) vs[eqnid] = 1.0 - vs[eqnid];
		}
		else { // if (this->components[c].factors[i] == FT_VALUE) {
			vs[eqnid] *= this->components[c].factorValues[i];
		}

		eqnid = !eqnid;
	}

	// Equation
	int rValue = 0;
	if (eqn == EQ_ADD) {
		rValue = static_cast<int>((vs[0] + vs[1]) * 255);
	}
	else if (eqn == EQ_SUBTRACT) {
		rValue = static_cast<int>((vs[0] - vs[1]) * 255);
	}
	else if (eqn == EQ_MULTIPLY) {
		rValue = static_cast<int>((vs[0] * vs[1]) * 255);
	}
	else if (eqn == EQ_DIVIDE) {
		rValue = vs[1] == 0.0 ? (VBlend::sign(vs[1])) : static_cast<int>((vs[0] / vs[1]) * 255);
	}
	else if (eqn == EQ_MAX) {
		rValue = static_cast<int>(max(vs[0], vs[1]) * 255);
	}
	else { // if (eqn == EQ_MIN) {
		rValue = static_cast<int>(min(vs[0], vs[1]) * 255);
	}

	// Bound
	if (rValue < 0) rValue = 0;
	else if (rValue > 255) rValue = 255;

	// Return
	return rValue;
}



AVSValue __cdecl VBlend :: create1(AVSValue args, void* user_data, IScriptEnvironment* env) {
	VBlend::InitData iData[5];
	bool isDefined[5][7];

	// Read arguments
	int offset = 2, pos;
	for (int i = 0; i < 5; ++i) {
		pos = 0;

		// Factors
		for (int j = 0; j < 2; ++j) {
			iData[i].factors[j] = (isDefined[i][pos] = args[offset + pos].Defined()) ? args[offset + pos].AsString("") : NULL;

			if (!isDefined[0][pos] && i > 0 && (isDefined[0][pos] = isDefined[i][pos])) {
				iData[0].factors[j] = iData[i].factors[j];
			}

			++pos;
		}

		// Equation
		iData[i].equation = (isDefined[i][pos] = args[offset + pos].Defined()) ? args[offset + pos].AsString("") : NULL;

		if (!isDefined[0][pos] && i > 0 && (isDefined[0][pos] = isDefined[i][pos])) {
			iData[0].equation = iData[i].equation;
		}

		++pos;

		// Offsets
		for (int j = 0; j < 4; ++j) {
			iData[i].offsets[j] = (isDefined[i][pos] = args[offset + pos].Defined()) ? args[offset + pos].AsInt(0) : 0;

			if (!isDefined[0][pos] && i > 0 && (isDefined[0][pos] = isDefined[i][pos])) {
				iData[0].offsets[j] = iData[i].offsets[j];
			}

			++pos;
		}

		// Next
		offset += pos;
	}

	// Propagate default into unfilled values
	for (int i = 1; i < 5; ++i) {
		pos = 0;

		// Factors
		for (int j = 0; j < 2; ++j) {
			if (!isDefined[i][pos]) {
				iData[i].factors[j] = iData[0].factors[j];
			}

			++pos;
		}

		// Equation
		if (!isDefined[i][pos]) {
			iData[i].equation = iData[0].equation;
		}

		++pos;

		// Factors
		for (int j = 0; j < 2; ++j) {
			if (!isDefined[i][pos] && isDefined[0][pos]) {
				iData[i].offsets[j] = iData[0].offsets[j];
			}

			++pos;
		}
	}

	// VChannel
	return static_cast<AVSValue>(new VBlend(
		env,
		args[0].AsClip(), // source1
		args[1].AsClip(), // source2

		&iData[1],
		&iData[2],
		&iData[3],
		&iData[4],

		args[offset].AsInt(0xFF000000),
		args[offset + 1].AsInt(0xFF000000)
	));
}
const char* VBlend :: registerClass(IScriptEnvironment* env) {
	/**
		clip.VBlend(...)

		Blend two clips together using a custom blending function

		@param source1
			The first clip to modify
		@param source2
			The second clip to modify
		@param factor1
			The factor to multiply source1's pixels by
			The total equation winds up being:
				(source1.pixel_component * factor1) [equation] (source2.pixel_component * factor2)
				with values being in the range [0,1] instead of [0,255]
			Available usages are:
				"value 0.5"
					Multiply it by a fixed value (Range: [0,1])
				"color [inv] (src)"
					Multiply the component by the same component of source(src)
					(src) can be 1 or 2
					[inv] is optional, and if specified, (1.0 - source(src)[component]) is used
				"red [inv] (src)"
				"green [inv] (src)"
				"blue [inv] (src)"
				"alpha [inv] (src)"
					Same idea as the "color" version, except the component is fixed to R, G, B, or A
		@param factor2
			Same idea as factor1, except applied to source2
		@param equation
			The equation to use to modify the two clips
			Available equations are:
				"add"
				"subtract"
				"multiply"
				"divide"
				"max"
				"min"
		@param x_offset1
			The number of pixels to offset the first source clip by (horizontally)
		@param y_offset1
			The number of pixels to offset the first source clip by (vertically)
		@param x_offset2
			The number of pixels to offset the secondsource clip by (horizontally)
		@param y_offset2
			The number of pixels to offset the second source clip by (vertically)
		@note
			The same applies for the _r, _g, _b, _a variations, except they apply for a specific component
		@param border_color1
			The color to be used if there is no pixel at a location in source1 (due to offsets)
		@param border_color2
			The color to be used if there is no pixel at a location in source2 (due to offsets)
		@return
			The two clips blended together

		@default
			Not applicable, always define the necessary variables
	*/
	env->AddFunction(
		"VBlend",
		"[source1]c[source2]c"
		"[factor1]s[factor2]s[equation]s[x_offset1]i[y_offset1]i[x_offset2]i[y_offset2]i"
		"[factor1_r]s[factor2_r]s[equation_r]s[x_offset1_r]i[y_offset1_r]i[x_offset2_r]i[y_offset2_r]i"
		"[factor1_g]s[factor2_g]s[equation_g]s[x_offset1_g]i[y_offset1_g]i[x_offset2_g]i[y_offset2_g]i"
		"[factor1_b]s[factor2_b]s[equation_b]s[x_offset1_b]i[y_offset1_b]i[x_offset2_b]i[y_offset2_b]i"
		"[factor1_a]s[factor2_a]s[equation_a]s[x_offset1_a]i[y_offset1_a]i[x_offset2_a]i[y_offset2_a]i"
		"[border_color1]i[border_color2]i",
		VBlend::create1,
		NULL
	);

	return "VBlend";
}



FUNCTION_END();
REGISTER(VBlend);

