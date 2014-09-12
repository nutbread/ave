#include "Helpers.hpp"
#include <cmath>
#include <cassert>
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>



// Color
const int Color::MAX_HUE = 256 * 6;
void Color :: convertHSVtoRGB(int hue, int sat, int val, int& r, int& g, int& b) {
	assert(hue >= 0);
	assert(hue < Color::MAX_HUE);
	assert(sat >= 0);
	assert(sat < 256);
	assert(val >= 0);
	assert(val < 256);

	unsigned char vMin = val - static_cast<unsigned char>((val / 255.0 * sat / 255.0) * 255);
	unsigned char vMax = val;
	unsigned char vSpan = vMax - vMin;

	if (hue < 256 * 1) {
		r = vMax;
		g = vMin + ((hue - 0) * vSpan) / 255;
		b = vMin;
	}
	else if (hue < 256 * 2) {
		r = vMax - ((hue - 256) * vSpan) / 255;
		g = vMax;
		b = vMin;
	}
	else if (hue < 256 * 3) {
		r = vMin;
		g = vMax;
		b = vMin + ((hue - 256 * 2) * vSpan) / 255;
	}
	else if (hue < 256 * 4) {
		r = vMin;
		g = vMax - ((hue - 256 * 3) * vSpan) / 255;
		b = vMax;
	}
	else if (hue < 256 * 5) {
		r = vMin + ((hue - 256 * 4) * vSpan) / 255;
		g = vMin;
		b = vMax;
	}
	else {
		r = vMax;
		g = vMin;
		b = vMax - ((hue - 256 * 5) * vSpan) / 255;
	}
}
void Color :: convertRGBtoHSV(int r, int g, int b, int& hue, int& saturation, int& value, int maxColor, int maxHue, int extraBitPrecision) {
	assert(maxColor > 0);
	assert(maxHue > 0);
	assert(extraBitPrecision >= 0);
	assert(r >= 0);
	assert(g >= 0);
	assert(b >= 0);
	assert(r <= maxColor);
	assert(g <= maxColor);
	assert(b <= maxColor);

	// After color comparisons are done, r is treated as "cDiff" and g as "cFactor"

	// Calculate which colors are the min/max; gets value, part of saturation
	if (r > g) {
		// r > g
		if (r > b) {
			// r > (g,b)
			value = r;
			if (g > b) {
				// r > g > b
				saturation = value - b;
			}
			else {
				// r > b > g
				saturation = value - g;
			}
			r = g - b;
			g = 6;
		}
		else {
			// b > r > g
			value = b;
			saturation = value - g;
			r = r - g;
			g = 4;
		}
	}
	else {
		// g > r
		if (g > b) {
			// g > (b,r)
			value = g;
			if (b > r) {
				// g > b > r
				saturation = value - r;
			}
			else {
				// g > r > b
				saturation = value - b;
			}
			r = b - r;
			g = 2;
		}
		else {
			// b > g > r
			value = b;
			saturation = value - r;
			r = r - g;
			g = 4;
		}
	}

	// Calculate hue and saturation
	if (value == 0) {
		assert(saturation == 0);
		hue = 0;
	}
	else {
		value <<= extraBitPrecision;

		if (saturation == 0) {
			hue = 0;
		}
		else {
			hue = (maxHue * (g * saturation + r)) / (6 * saturation);
			if (hue >= maxHue) hue -= maxHue;

			saturation = ((maxColor << extraBitPrecision) * saturation) / value;
		}
	}

	assert(hue >= 0);
	assert(hue < maxHue);
}
void Color :: convertRGBtoHSV(int r, int g, int b, unsigned int& hue, unsigned int& saturation, unsigned int& value, int maxColor, int maxHue, int extraBitPrecision) {
	int temp[3];
	Color::convertRGBtoHSV(r, g, b, temp[0], temp[1], temp[2], maxColor, maxHue, extraBitPrecision);
	hue = temp[0];
	saturation = temp[1];
	value = temp[2];
}



// Random
Random :: Random() :
	seedValue(0)
{
	this->seed(GetTickCount());
}
Random :: Random(long long seed) :
	seedValue(0)
{
	this->seed(seed);
}
Random :: ~Random() {
}
int Random :: next(int bits) {
	assert(bits >= 0);
	assert(bits <= 32);

	this->seedValue = (this->seedValue * 25214903917LL + 11LL) & 0x00000000FFFFFFFFLL;

    return static_cast<int>(static_cast<unsigned long long>(this->seedValue) >> (32 - bits));
}
void Random :: seed(long long seed) {
	this->seedValue = (seed ^ 0x00000000DEECEF00LL) & 0x00000000FFFFFFFFLL;
}

int Random :: nextInt(int maximum) {
	assert(maximum > 0);

	if ((maximum & -maximum) == maximum) {
		// Power of 2
		return static_cast<int>((maximum * static_cast<long long int>(next(31))) >> 31);
		//return static_cast<int>(this->next(31) & (maximum - 1));
	}
	int i, j;
	do {
		i = this->next(31);
		j = i % maximum;
	}
	while (i - j + maximum < 1);
	return j;
}
float Random :: nextFloat() {
	return (this->next(24) / 16777216.0f);
}
double Random :: nextDouble() {
	return (((static_cast<long long>(next(26)) << 27) + next(27)) / 9007199254740992.0);
}
int Random :: nextInt() {
	return this->next(32);
}
bool Random :: nextBool() {
	return (this->next(1) > 0);
}



