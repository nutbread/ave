#ifndef ___PLUGIN_EXT_HELPERS_H
#define ___PLUGIN_EXT_HELPERS_H



// Math helpers
class Math {
private:
	Math();
	~Math();

public:
	template <class T> static inline T absoluteValue(T value) {
		if (value < 0) return -value;
		return value;
	}
	template <class T> static inline T log2(T n) {
		return log(n) / log(static_cast<T>(2));
	}

};



// Color class
class Color {
private:
	Color();
	~Color();

public:
	static const int MAX_HUE;

	static void convertHSVtoRGB(int hue, int sat, int val, int& r, int& g, int& b);
	static void convertRGBtoHSV(int r, int g, int b, int& hue, int& sat, int& val, int maxColor = 255, int maxHue = Color::MAX_HUE, int extraBitPrecision = 0);
	static void convertRGBtoHSV(int r, int g, int b, unsigned int& hue, unsigned int& sat, unsigned int& val, int maxColor = 255, int maxHue = Color::MAX_HUE, int extraBitPrecision = 0);

};



// Random class
class Random {
private:
	long long seedValue;

	int next(int bits);

public:
	Random();
	Random(long long seed);
	~Random();

	void seed(long long seed);

	float nextFloat();
	double nextDouble();
	int nextInt();
	int nextInt(int maximum);
	bool nextBool();

};



#endif