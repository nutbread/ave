// Custom subtitling
#ifndef ___PLUGIN_VBLUR_H
#define ___PLUGIN_VBLUR_H
#if (PLUGIN_AVISYNTH_VERSION == 26) && !defined(_M_X64) && !defined(__amd64__)



#include "../Header.hpp"
FUNCTION_START();



// Helper class to anti-alias text
class Antialiaser {
public:
	Antialiaser(
		int width,
		int height,
		const char fontname[],
		int size,
		int textcolor,
		int halocolor,
		int font_width=0,
		int font_angle=0,
		bool _interlaced=false
	);
	~Antialiaser();
	HDC GetDC();
	void FreeDC();

	void Apply(const VideoInfo& vi, PVideoFrame* frame, int pitch);

private:
	void ApplyYV12(BYTE* buf, int pitch, int UVpitch,BYTE* bufV,BYTE* bufU);
	void ApplyPlanar(BYTE* buf, int pitch, int UVpitch,BYTE* bufV,BYTE* bufU, int shiftX, int shiftY);
	void ApplyYUY2(BYTE* buf, int pitch);
	void ApplyRGB24(BYTE* buf, int pitch);
	void ApplyRGB32(BYTE* buf, int pitch);

	const int w, h;
	HDC hdcAntialias;
	HBITMAP hbmAntialias;
	void* lpAntialiasBits;
	HFONT hfontDefault;
	HBITMAP hbmDefault;
	unsigned short* alpha_calcs;
	bool dirty, interlaced;
	const int textcolor, halocolor;
	int xl, yt, xr, yb; // sub-rectangle containing live text

	void GetAlphaRect();
};



// Subtitle creation class
class VSubtitle :
	public GenericVideoFilter
{
private:
	const int x, y, firstframe, lastframe, size, lsp, font_width, font_angle;
	const bool multiline, interlaced;
	const int textcolor, halocolor, align, spc;
	const char* const fontname;
	const char* const text;
	Antialiaser* antialiaser;

	bool needsFormat;
	size_t formatSpace;
	char* formatBuffer;//_snprintf (NULL,0,"","");


	void InitAntialiaser(IScriptEnvironment* env, int frame);

	VSubtitle(
		PClip _child,
		const char _text[],
		int _x,
		int _y,
		int _firstframe,
		int _lastframe,
		const char _fontname[],
		int _size,
		int _textcolor,
		int _halocolor,
		int _align,
		int _spc,
		bool _multiline,
		int _lsp,
		int _font_width,
		int _font_angle,
		bool _interlaced,
		int _formatSpace
	);
	~VSubtitle();

	static AVSValue __cdecl create1(AVSValue args, void*, IScriptEnvironment* env);

public:
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

	static const char* registerClass(IScriptEnvironment* env);

};



FUNCTION_END();



#endif
#endif

