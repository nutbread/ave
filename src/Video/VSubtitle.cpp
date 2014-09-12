// Custom subtitling
#if (PLUGIN_AVISYNTH_VERSION == 26) && !defined(_M_X64) && !defined(__amd64__)


#include "VSubtitle.hpp"
#include <cassert>
FUNCTION_START();



/******************************
 *******   Anti-alias    ******
 *****************************/


inline static HFONT LoadFont(const char name[], int size, bool bold, bool italic, int width=0, int angle=0)
{
  return CreateFont( size, width, angle, angle, bold ? FW_BOLD : FW_NORMAL,
                     italic, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE | DEFAULT_PITCH, name );
}

Antialiaser::Antialiaser(int width, int height, const char fontname[], int size,
   int _textcolor, int _halocolor, int font_width, int font_angle, bool _interlaced)
 : w(width), h(height), textcolor(_textcolor), halocolor(_halocolor), alpha_calcs(0),
   dirty(true), interlaced(_interlaced)
{
  struct {
    BITMAPINFOHEADER bih;
    RGBQUAD clr[2];
  } b;

  b.bih.biSize                    = sizeof(BITMAPINFOHEADER);
  b.bih.biWidth                   = width * 8 + 32;
  b.bih.biHeight                  = height * 8 + 32;
  b.bih.biBitCount                = 1;
  b.bih.biPlanes                  = 1;
  b.bih.biCompression             = BI_RGB;
  b.bih.biXPelsPerMeter   = 0;
  b.bih.biYPelsPerMeter   = 0;
  b.bih.biClrUsed                 = 2;
  b.bih.biClrImportant    = 2;
  b.clr[0].rgbBlue = b.clr[0].rgbGreen = b.clr[0].rgbRed = 0;
  b.clr[1].rgbBlue = b.clr[1].rgbGreen = b.clr[1].rgbRed = 255;

  hdcAntialias = CreateCompatibleDC(NULL);
  if (hdcAntialias) {
	hbmAntialias = CreateDIBSection
	  ( hdcAntialias,
		(BITMAPINFO *)&b,
		DIB_RGB_COLORS,
		&lpAntialiasBits,
		NULL,
		0 );
	if (hbmAntialias) {
	  hbmDefault = (HBITMAP)SelectObject(hdcAntialias, hbmAntialias);

	  HFONT newfont = LoadFont(fontname, size, true, false, font_width, font_angle);
	  hfontDefault = newfont ? (HFONT)SelectObject(hdcAntialias, newfont) : 0;

	  SetMapMode(hdcAntialias, MM_TEXT);
	  SetTextColor(hdcAntialias, 0xffffff);
	  SetBkColor(hdcAntialias, 0);

	  alpha_calcs = new unsigned short[width*height*4];

	  if (!alpha_calcs) FreeDC();
	}
  }
}


Antialiaser::~Antialiaser() {
  FreeDC();
  if (alpha_calcs) delete[] alpha_calcs;
}


HDC Antialiaser::GetDC() {
  dirty = true;
  return hdcAntialias;
}


void Antialiaser::FreeDC() {
  if (hdcAntialias) { // :FIXME: Interlocked
    if (hbmDefault) {
	  DeleteObject(SelectObject(hdcAntialias, hbmDefault));
	  hbmDefault = 0;
	}
    if (hfontDefault) {
	  DeleteObject(SelectObject(hdcAntialias, hfontDefault));
	  hfontDefault = 0;
	}
    DeleteDC(hdcAntialias);
    hdcAntialias = 0;
  }
}


void Antialiaser::Apply( const VideoInfo& vi, PVideoFrame* frame, int pitch)
{
  if (!alpha_calcs) return;

  if (vi.IsRGB32())
    ApplyRGB32((*frame)->GetWritePtr(), pitch);
  else if (vi.IsRGB24())
    ApplyRGB24((*frame)->GetWritePtr(), pitch);
  else if (vi.IsYUY2())
    ApplyYUY2((*frame)->GetWritePtr(), pitch);
  else if (vi.IsYV12())
    ApplyYV12((*frame)->GetWritePtr(), pitch,
              (*frame)->GetPitch(PLANAR_U),
			  (*frame)->GetWritePtr(PLANAR_U),
			  (*frame)->GetWritePtr(PLANAR_V) );
  else if (vi.IsY8())
    ApplyPlanar((*frame)->GetWritePtr(), pitch, 0, 0, 0, 0, 0);
  else if (vi.IsPlanar())
    ApplyPlanar((*frame)->GetWritePtr(), pitch,
              (*frame)->GetPitch(PLANAR_U),
			  (*frame)->GetWritePtr(PLANAR_U),
			  (*frame)->GetWritePtr(PLANAR_V),
			  vi.GetPlaneWidthSubsampling(PLANAR_U),
			  vi.GetPlaneHeightSubsampling(PLANAR_U));

}


void Antialiaser::ApplyYV12(BYTE* buf, int pitch, int pitchUV, BYTE* bufU, BYTE* bufV) {
  if (dirty) {
    GetAlphaRect();
	xl &= -2; xr |= 1;
	yb &= -2; yt |= 1;
  }
  const int w4 = w*4;
  unsigned short* alpha = alpha_calcs + yb*w4;
  buf  += pitch*yb;
  bufU += (pitchUV*yb)>>1;
  bufV += (pitchUV*yb)>>1;

  for (int y=yb; y<=yt; y+=2) {
    for (int x=xl; x<=xr; x+=2) {
      const int x4 = x<<2;
      const int basealpha00 = alpha[x4+0];
      const int basealpha10 = alpha[x4+4];
      const int basealpha01 = alpha[x4+0+w4];
      const int basealpha11 = alpha[x4+4+w4];
      const int basealphaUV = basealpha00 + basealpha10 + basealpha01 + basealpha11;

      if (basealphaUV != 1024) {
        buf[x+0]       = (buf[x+0]       * basealpha00 + alpha[x4+3]   ) >> 8;
        buf[x+1]       = (buf[x+1]       * basealpha10 + alpha[x4+7]   ) >> 8;
        buf[x+0+pitch] = (buf[x+0+pitch] * basealpha01 + alpha[x4+3+w4]) >> 8;
        buf[x+1+pitch] = (buf[x+1+pitch] * basealpha11 + alpha[x4+7+w4]) >> 8;

        const int au  = alpha[x4+2] + alpha[x4+6] + alpha[x4+2+w4] + alpha[x4+6+w4];
        bufU[x>>1] = (bufU[x>>1] * basealphaUV + au) >> 10;

        const int av  = alpha[x4+1] + alpha[x4+5] + alpha[x4+1+w4] + alpha[x4+5+w4];
        bufV[x>>1] = (bufV[x>>1] * basealphaUV + av) >> 10;
      }
    }
    buf += pitch<<1;
    bufU += pitchUV;
    bufV += pitchUV;
    alpha += w<<3;
  }
}


void Antialiaser::ApplyPlanar(BYTE* buf, int pitch, int pitchUV, BYTE* bufU, BYTE* bufV, int shiftX, int shiftY) {
  const int stepX = 1<<shiftX;
  const int stepY = 1<<shiftY;

  if (dirty) {
    GetAlphaRect();
    xl &= -stepX; xr |= stepX-1;
    yb &= -stepY; yt |= stepY-1;
  }
  const int w4 = w*4;
  unsigned short* alpha = alpha_calcs + yb*w4;
  buf += pitch*yb;

  // Apply Y
  {for (int y=yb; y<=yt; y+=1) {
    for (int x=xl; x<=xr; x+=1) {
      const int x4 = x<<2;
      const int basealpha = alpha[x4+0];

      if (basealpha != 256) {
        buf[x] = (buf[x] * basealpha + alpha[x4+3]) >> 8;
      }
    }
    buf += pitch;
    alpha += w4;
  }}

  if (!bufU) return;

  // This will not be fast, but it will be generic.
  const int skipThresh = 256 << (shiftX+shiftY);
  const int shifter = 8+shiftX+shiftY;
  const int UVw4 = w<<(2+shiftY);
  const int xlshiftX = xl>>shiftX;

  alpha = alpha_calcs + yb*w4;
  bufU += (pitchUV*yb)>>shiftY;
  bufV += (pitchUV*yb)>>shiftY;

  {for (int y=yb; y<=yt; y+=stepY) {
    for (int x=xl, xs=xlshiftX; x<=xr; x+=stepX, xs+=1) {
      unsigned short* UValpha = alpha + x*4;
      int basealphaUV = 0;
      int au = 0;
      int av = 0;
      for (int i = 0; i<stepY; i++) {
        for (int j = 0; j<stepX; j++) {
          basealphaUV += UValpha[0 + j*4];
          av          += UValpha[1 + j*4];
          au          += UValpha[2 + j*4];
        }
        UValpha += w4;
      }
      if (basealphaUV != skipThresh) {
        bufU[xs] = (bufU[xs] * basealphaUV + au) >> shifter;
        bufV[xs] = (bufV[xs] * basealphaUV + av) >> shifter;
      }
    }// end for x
    bufU  += pitchUV;
    bufV  += pitchUV;
    alpha += UVw4;
  }}//end for y
}


void Antialiaser::ApplyYUY2(BYTE* buf, int pitch) {
  if (dirty) {
    GetAlphaRect();
	xl &= -2; xr |= 1;
  }
  unsigned short* alpha = alpha_calcs + yb*w*4;
  buf += pitch*yb;

  for (int y=yb; y<=yt; ++y) {
    for (int x=xl; x<=xr; x+=2) {
      const int basealpha0  = alpha[x*4+0];
      const int basealpha1  = alpha[x*4+4];
      const int basealphaUV = basealpha0 + basealpha1;

      if (basealphaUV != 512) {
        buf[x*2+0] = (buf[x*2+0] * basealpha0 + alpha[x*4+3]) >> 8;
        buf[x*2+2] = (buf[x*2+2] * basealpha1 + alpha[x*4+7]) >> 8;

        const int au  = alpha[x*4+2] + alpha[x*4+6];
        buf[x*2+1] = (buf[x*2+1] * basealphaUV + au) >> 9;

        const int av  = alpha[x*4+1] + alpha[x*4+5];
        buf[x*2+3] = (buf[x*2+3] * basealphaUV + av) >> 9;
      }
    }
    buf += pitch;
    alpha += w*4;
  }
}


void Antialiaser::ApplyRGB24(BYTE* buf, int pitch) {
  if (dirty) GetAlphaRect();
  unsigned short* alpha = alpha_calcs + yb*w*4;
  buf  += pitch*(h-yb-1);

  for (int y=yb; y<=yt; ++y) {
    for (int x=xl; x<=xr; ++x) {
      const int basealpha = alpha[x*4+0];
      if (basealpha != 256) {
        buf[x*3+0] = (buf[x*3+0] * basealpha + alpha[x*4+1]) >> 8;
        buf[x*3+1] = (buf[x*3+1] * basealpha + alpha[x*4+2]) >> 8;
        buf[x*3+2] = (buf[x*3+2] * basealpha + alpha[x*4+3]) >> 8;
      }
    }
    buf -= pitch;
    alpha += w*4;
  }
}


void Antialiaser::ApplyRGB32(BYTE* buf, int pitch) {
  if (dirty) GetAlphaRect();
  unsigned short* alpha = alpha_calcs + yb*w*4;
  buf  += pitch*(h-yb-1);

  for (int y=yb; y<=yt; ++y) {
    for (int x=xl; x<=xr; ++x) {
      const int basealpha = alpha[x*4+0];
      if (basealpha != 256) {
        buf[x*4+0] = (buf[x*4+0] * basealpha + alpha[x*4+1]) >> 8;
        buf[x*4+1] = (buf[x*4+1] * basealpha + alpha[x*4+2]) >> 8;
        buf[x*4+2] = (buf[x*4+2] * basealpha + alpha[x*4+3]) >> 8;
      }
    }
    buf -= pitch;
    alpha += w*4;
  }
}


void Antialiaser::GetAlphaRect() {

  dirty = false;

  static BYTE bitcnt[256],    // bit count
              bitexl[256],    // expand to left bit
              bitexr[256];    // expand to right bit
  static bool fInited = false;
  static unsigned short gamma[129]; // Gamma lookups

  if (!fInited) {
    fInited = true;

    const double scale = 516*64/sqrt(128.0);
    {for(int i=0; i<=128; i++)
      gamma[i]=unsigned short(sqrt((double)i) * scale + 0.5); // Gamma = 2.0
    }

	{for(int i=0; i<256; i++) {
      BYTE b=0, l=0, r=0;

      if (i&  1) { b=1; l|=0x01; r|=0xFF; }
      if (i&  2) { ++b; l|=0x03; r|=0xFE; }
      if (i&  4) { ++b; l|=0x07; r|=0xFC; }
      if (i&  8) { ++b; l|=0x0F; r|=0xF8; }
      if (i& 16) { ++b; l|=0x1F; r|=0xF0; }
      if (i& 32) { ++b; l|=0x3F; r|=0xE0; }
      if (i& 64) { ++b; l|=0x7F; r|=0xC0; }
      if (i&128) { ++b; l|=0xFF; r|=0x80; }

      bitcnt[i] = b;
      bitexl[i] = l;
      bitexr[i] = r;
    }}
  }

  const int RYtext = ((textcolor>>16)&255), GUtext = ((textcolor>>8)&255), BVtext = (textcolor&255);
  const int RYhalo = ((halocolor>>16)&255), GUhalo = ((halocolor>>8)&255), BVhalo = (halocolor&255);

  // Scaled Alpha
  const int Atext = 255 - ((textcolor >> 24) & 0xFF);
  const int Ahalo = 255 - ((halocolor >> 24) & 0xFF);

  const int srcpitch = (w+4+3) & -4;

  xl=0;
  xr=w+1;
  yt=-1;
  yb=h;

  unsigned short* dest = alpha_calcs;
  for (int y=0; y<h; ++y) {
    BYTE* src = (BYTE*)lpAntialiasBits + ((h-y-1)*8 + 20) * srcpitch + 2;
    int wt = w;
    do {
      int i;

/*      BYTE tmp = 0;
      for (i=0; i<8; i++) {
        tmp |= src[srcpitch*i];
        tmp |= src[srcpitch*i-1];
        tmp |= src[srcpitch*i+1];
        tmp |= src[srcpitch*(-8+i)];
        tmp |= src[srcpitch*(-8+i)-1];
        tmp |= src[srcpitch*(-8+i)+1];
        tmp |= src[srcpitch*(8+i)];
        tmp |= src[srcpitch*(8+i)-1];
        tmp |= src[srcpitch*(8+i)+1];
      }
*/
      DWORD tmp = interlaced;
      __asm {           // test if the whole area isn't just plain black
        mov edx, srcpitch
        mov esi, src
        mov ecx, edx
        dec esi
        shl ecx, 3
        sub esi, ecx  ; src - 8*pitch - 1
        cmp tmp,-1
        jnz do32

        lea edi,[esi+edx*2]
        xor eax,eax
        xor ecx,ecx
        jmp do24
do32:
        sar ecx, 1
        sub esi, ecx  ; src - 12*pitch - 1
        lea edi,[esi+edx*2]

        mov eax, [esi]  ; repeat 32 times
        mov ecx, [esi+edx]
        lea esi,[esi+edx*4]
        or eax, [edi]
        or ecx, [edi+edx]
        lea edi,[edi+edx*4]
        or eax, [esi]
        or ecx, [esi+edx]
        lea esi,[esi+edx*4]
        or eax, [edi]
        or ecx, [edi+edx]
        lea edi,[edi+edx*4]
do24:
        or eax, [esi]  ; repeat 24 times
        or ecx, [esi+edx]
        lea esi,[esi+edx*4]
        or eax, [edi]
        or ecx, [edi+edx]
        lea edi,[edi+edx*4]
        or eax, [esi]
        or ecx, [esi+edx]
        lea esi,[esi+edx*4]
        or eax, [edi]
        or ecx, [edi+edx]
        lea edi,[edi+edx*4]
        or eax, [esi]
        or ecx, [esi+edx]
        lea esi,[esi+edx*4]
        or eax, [edi]
        or ecx, [edi+edx]
        lea edi,[edi+edx*4]
        or eax, [esi]
        or ecx, [esi+edx]
        lea esi,[esi+edx*4]
        or eax, [edi]
        or ecx, [edi+edx]
        lea edi,[edi+edx*4]
        or eax, [esi]
        or ecx, [esi+edx]
        lea esi,[esi+edx*4]
        or eax, [edi]
        or ecx, [edi+edx]
        lea edi,[edi+edx*4]
        or eax, [esi]
        or ecx, [esi+edx]
        or eax, [edi]
        or ecx, [edi+edx]

        or eax, ecx
        and eax, 0x00ffffff
        mov tmp, eax
      }

      if (tmp != 0) {     // quick exit in a common case
		if (wt >= xl) xl=wt;
		if (wt <= xr) xr=wt;
		if (y  >= yt) yt=y;
		if (y  <= yb) yb=y;

        int alpha1, alpha2;

        alpha1 = alpha2 = 0;

		if (interlaced) {
		  BYTE topmask=0, cenmask=0, botmask=0;
		  BYTE hmasks[16], mask;

		  for(i=-4; i<12; i++) {// For interlaced include extra half cells above and below
			mask = src[srcpitch*i];
			// How many lit pixels in the centre cell?
			alpha1 += bitcnt[mask];
			// turn on all halo bits if cell has any lit pixels
			mask = - !! mask;
			// Check left and right neighbours, extend the halo
			// mask 8 pixels in from the nearest lit pixels.
			mask |= bitexr[src[srcpitch*i-1]];
			mask |= bitexl[src[srcpitch*i+1]];
			hmasks[i+4] = mask;
		  }

		  // Extend halo vertically to 8x8 blocks
		  for(i=-4; i<4;  i++) topmask |= hmasks[i+4];
		  for(i=0;  i<8;  i++) cenmask |= hmasks[i+4];
		  for(i=4;  i<12; i++) botmask |= hmasks[i+4];
		  // Check the 3x1.5 cells above
		  for(mask = topmask, i=-4; i<4; i++) {
			mask |= bitexr[ src[srcpitch*(i+8)-1] ];
			mask |=    - !! src[srcpitch*(i+8)  ];
			mask |= bitexl[ src[srcpitch*(i+8)+1] ];
			hmasks[i+4] |= mask;
		  }
		  for(mask = cenmask, i=0; i<8; i++) {
			mask |= bitexr[ src[srcpitch*(i+8)-1] ];
			mask |=    - !! src[srcpitch*(i+8)  ];
			mask |= bitexl[ src[srcpitch*(i+8)+1] ];
			hmasks[i+4] |= mask;
		  }
		  for(mask = botmask, i=4; i<12; i++) {
			mask |= bitexr[ src[srcpitch*(i+8)-1] ];
			mask |=    - !! src[srcpitch*(i+8)  ];
			mask |= bitexl[ src[srcpitch*(i+8)+1] ];
			hmasks[i+4] |= mask;
		  }
		  // Check the 3x1.5 cells below
		  for(mask = botmask, i=11; i>=4; i--) {
			mask |= bitexr[ src[srcpitch*(i-8)-1] ];
			mask |=    - !! src[srcpitch*(i-8)  ];
			mask |= bitexl[ src[srcpitch*(i-8)+1] ];
			hmasks[i+4] |= mask;
		  }
		  for(mask = cenmask,i=7; i>=0; i--) {
			mask |= bitexr[ src[srcpitch*(i-8)-1] ];
			mask |=    - !! src[srcpitch*(i-8)  ];
			mask |= bitexl[ src[srcpitch*(i-8)+1] ];
			hmasks[i+4] |= mask;
		  }
		  for(mask = topmask, i=3; i>=-4; i--) {
			mask |= bitexr[ src[srcpitch*(i-8)-1] ];
			mask |=    - !! src[srcpitch*(i-8)  ];
			mask |= bitexl[ src[srcpitch*(i-8)+1] ];
			hmasks[i+4] |= mask;
		  }
		  // count the halo pixels
		  for(i=0; i<16; i++)
			alpha2 += bitcnt[hmasks[i]];
		}
		else {
		  // How many lit pixels in the centre cell?
		  for(i=0; i<8; i++)
			alpha1 += bitcnt[src[srcpitch*i]];
		  alpha1 *=2;

		  if (alpha1) {
			// If we have any lit pixels we fully occupy the cell.
			alpha2 = 128;
		  }
		  else {
			// No lit pixels here so build the halo mask from the neighbours
			BYTE cenmask = 0;

			// Check left and right neighbours, extend the halo
			// mask 8 pixels in from the nearest lit pixels.
			for(i=0; i<8; i++) {
			  cenmask |= bitexr[src[srcpitch*i-1]];
			  cenmask |= bitexl[src[srcpitch*i+1]];
			}

			if (cenmask == 0xFF) {
			  // If we have hard adjacent lit pixels we fully occupy this cell.
			  alpha2 = 128;
			}
			else {
			  BYTE hmasks[8], mask;

			  mask = cenmask;
			  for(i=0; i<8; i++) {
				// Check the 3 cells above
				mask |= bitexr[ src[srcpitch*(i+8)-1] ];
				mask |=    - !! src[srcpitch*(i+8)  ];
				mask |= bitexl[ src[srcpitch*(i+8)+1] ];
				hmasks[i] = mask;
			  }

			  mask = cenmask;
			  for(i=7; i>=0; i--) {
				// Check the 3 cells below
				mask |= bitexr[ src[srcpitch*(i-8)-1] ];
				mask |=    - !! src[srcpitch*(i-8)  ];
				mask |= bitexl[ src[srcpitch*(i-8)+1] ];

				alpha2 += bitcnt[hmasks[i] | mask];
			  }
			  alpha2 *=2;
			}
		  }
		}
		alpha2  = gamma[alpha2];
		alpha1  = gamma[alpha1];

		alpha2 -= alpha1;
		alpha2 *= Ahalo;
		alpha1 *= Atext;
        // Pre calulate table for quick use  --  Pc = (Pc * dest[0] + dest[c]) >> 8;

		dest[0] = (64*516*255 - alpha1 -          alpha2)>>15;
		dest[1] = (    BVtext * alpha1 + BVhalo * alpha2)>>15;
		dest[2] = (    GUtext * alpha1 + GUhalo * alpha2)>>15;
		dest[3] = (    RYtext * alpha1 + RYhalo * alpha2)>>15;
      }
	  else {
		dest[0] = 256;
		dest[1] = 0;
		dest[2] = 0;
		dest[3] = 0;
      }

      dest += 4;
      ++src;
    } while(--wt);
  }

  xl=w-xl;
  xr=w-xr;
}










/***********************************
 *******   VSubtitle Filter    ******
 **********************************/

class PixelClip {
  enum { buffer=320 };
  BYTE clip[256+buffer*2];
public:
  PixelClip() {
    memset(clip, 0, buffer);
    for (int i=0; i<256; ++i) clip[i+buffer] = (BYTE)i;
    memset(clip+buffer+256, 255, buffer);
  }
  BYTE operator()(int i) { return clip[i+buffer]; }
};

static __inline BYTE ScaledPixelClip(int i) {
  return PixelClip()((i+32768) >> 16);
}

inline int RGB2YUV(int rgb)
{
  const int cyb = int(0.114*219/255*65536+0.5);
  const int cyg = int(0.587*219/255*65536+0.5);
  const int cyr = int(0.299*219/255*65536+0.5);

  // y can't overflow
  int y = (cyb*(rgb&255) + cyg*((rgb>>8)&255) + cyr*((rgb>>16)&255) + 0x108000) >> 16;
  int scaled_y = (y - 16) * int(255.0/219.0*65536+0.5);
  int b_y = ((rgb&255) << 16) - scaled_y;
  int u = ScaledPixelClip((b_y >> 10) * int(1/2.018*1024+0.5) + 0x800000);
  int r_y = (rgb & 0xFF0000) - scaled_y;
  int v = ScaledPixelClip((r_y >> 10) * int(1/1.596*1024+0.5) + 0x800000);
  return ((y*256+u)*256+v) | (rgb & 0xff000000);
}

VSubtitle::VSubtitle( PClip _child, const char _text[], int _x, int _y, int _firstframe,
                    int _lastframe, const char _fontname[], int _size, int _textcolor,
                    int _halocolor, int _align, int _spc, bool _multiline, int _lsp,
					int _font_width, int _font_angle, bool _interlaced,
					int _formatSpace)
 : GenericVideoFilter(_child), antialiaser(0), text(_text), x(_x), y(_y),
   firstframe(_firstframe), lastframe(_lastframe), fontname(_fontname), size(_size),
   textcolor(vi.IsYUV() ? RGB2YUV(_textcolor) : _textcolor),
   halocolor(vi.IsYUV() ? RGB2YUV(_halocolor) : _halocolor),
   align(_align), spc(_spc), multiline(_multiline), lsp(_lsp),
   font_width(_font_width), font_angle(_font_angle), interlaced(_interlaced),

  needsFormat(false),
  formatSpace(0),
  formatBuffer(NULL)
{
	int i = 0;
	while (text[i] != '\0') {
		if (text[i] == '%') this->needsFormat = true;
		++i;
	}

	if (this->needsFormat) {
		this->formatSpace = i;
		if (_formatSpace > this->formatSpace) this->formatSpace = _formatSpace;
		this->formatBuffer = new char[this->formatSpace + 1];
		this->formatBuffer[this->formatSpace] = '\0';
	}
}



VSubtitle::~VSubtitle(void)
{
  delete antialiaser;
  delete [] this->formatBuffer;
}



PVideoFrame VSubtitle::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame frame = child->GetFrame(n, env);

  if (n >= firstframe && n <= lastframe) {
    env->MakeWritable(&frame);
    if (!antialiaser) // :FIXME: CriticalSection
	  InitAntialiaser(env, n);
    if (antialiaser) {
	  antialiaser->Apply(vi, &frame, frame->GetPitch());
	  // Release all the windows drawing stuff
	  // and just keep the alpha calcs
	  antialiaser->FreeDC();
	}
  }
  // if we get far enough away from the frames we're supposed to
  // VSubtitle, then junk the buffered drawing information
  if (this->needsFormat || (antialiaser && (n < firstframe-10 || n > lastframe+10 || n == vi.num_frames-1))) {
	delete antialiaser;
	antialiaser = 0; // :FIXME: CriticalSection
  }

  return frame;
}

AVSValue __cdecl VSubtitle::create1(AVSValue args, void*, IScriptEnvironment* env) {
    PClip clip = args[0].AsClip();
    const char* text = args[1].AsString();
    const int first_frame = args[4].AsInt(0);
    const int last_frame = args[5].AsInt(clip->GetVideoInfo().num_frames-1);
    const char* font = args[6].AsString("Arial");
    const int size = int(args[7].AsFloat(18)*8+0.5);
    const int text_color = args[8].AsInt(0xFFFF00);
    const int halo_color = args[9].AsInt(0);
    const int align = args[10].AsInt(args[2].AsFloat(0)==-1?2:7);
    const int spc = args[11].AsInt(0);
    const bool multiline = args[12].Defined();
    const int lsp = args[12].AsInt(0);
	const int font_width = int(args[13].AsFloat(0)*8+0.5);
	const int font_angle = int(args[14].AsFloat(0)*10+0.5);
	const bool interlaced = args[15].AsBool(false);
	const int format_space = args[16].AsInt(0);

    if ((align < 1) || (align > 9))
     env->ThrowError("VSubtitle: Align values are 1 - 9 mapped to your numeric pad");

    int defx, defy;
    switch (align) {
	 case 1: case 4: case 7: defx = 8; break;
     case 2: case 5: case 8: defx = -1; break;
     case 3: case 6: case 9: defx = clip->GetVideoInfo().width-8; break;
     default: defx = 8; break; }
    switch (align) {
     case 1: case 2: case 3: defy = clip->GetVideoInfo().height-2; break;
     case 4: case 5: case 6: defy = -1; break;
	 case 7: case 8: case 9: defy = 0; break;
     default: defy = (size+4)/8; break; }

    const int x = int(args[2].AsDblDef(defx)*8+0.5);
    const int y = int(args[3].AsDblDef(defy)*8+0.5);


    return new VSubtitle(clip, text, x, y, first_frame, last_frame, font, size, text_color,
	                    halo_color, align, spc, multiline, lsp, font_width, font_angle, interlaced,
						format_space);
}



void VSubtitle::InitAntialiaser(IScriptEnvironment* env, int frame)
{
  antialiaser = new Antialiaser(vi.width, vi.height, fontname, size, textcolor, halocolor,
                                font_width, font_angle, interlaced);

	const char* the_text = this->text;
	if (this->needsFormat) {
		_snprintf(this->formatBuffer, this->formatSpace, this->text, frame);

		the_text = this->formatBuffer;
	}

  int real_x = x;
  int real_y = y;
  unsigned int al = 0;
  char *_text = 0;

  HDC hdcAntialias = antialiaser->GetDC();
  if (!hdcAntialias) goto GDIError;

  switch (align) // This spec where [X, Y] is relative to the text (inverted logic)
  { case 1: al = TA_BOTTOM   | TA_LEFT; break;		// .----
    case 2: al = TA_BOTTOM   | TA_CENTER; break;	// --.--
    case 3: al = TA_BOTTOM   | TA_RIGHT; break;		// ----.
    case 4: al = TA_BASELINE | TA_LEFT; break;		// .____
    case 5: al = TA_BASELINE | TA_CENTER; break;	// __.__
    case 6: al = TA_BASELINE | TA_RIGHT; break;		// ____.
    case 7: al = TA_TOP      | TA_LEFT; break;		// `----
    case 8: al = TA_TOP      | TA_CENTER; break;	// --`--
    case 9: al = TA_TOP      | TA_RIGHT; break;		// ----`
    default: al= TA_BASELINE | TA_LEFT; break;		// .____
  }
  if (SetTextCharacterExtra(hdcAntialias, spc) == 0x80000000) goto GDIError;
  if (SetTextAlign(hdcAntialias, al) == GDI_ERROR) goto GDIError;

  if (x==-7) real_x = (vi.width>>1)*8;
  if (y==-7) real_y = (vi.height>>1)*8;

  if (!multiline) {
	if (!TextOut(hdcAntialias, real_x+16, real_y+16, the_text, strlen(the_text))) goto GDIError;
  }
  else {
	// multiline patch -- tateu
	char *pdest, *psrc;
	int result, y_inc = real_y+16;
	char search[] = "\\n";
	psrc = _text = _strdup(the_text); // don't mangle the string constant -- Gavino
	if (!_text) goto GDIError;
	int length = strlen(psrc);

	do {
	  pdest = strstr(psrc, search);
	  while (pdest != NULL && pdest != psrc && *(pdest-1)=='\\') { // \n escape -- foxyshadis
		for (int i=pdest-psrc; i>0; i--) psrc[i] = psrc[i-1];
		psrc++;
		--length;
		pdest = strstr(pdest+1, search);
	  }
	  result = pdest == NULL ? length : pdest - psrc;
	  if (!TextOut(hdcAntialias, real_x+16, y_inc, psrc, result)) goto GDIError;
	  y_inc += size + lsp;
	  psrc = pdest + 2;
	  length -= result + 2;
	} while (pdest != NULL && length > 0);
	free(_text);
	_text = 0;
  }
  if (!GdiFlush()) goto GDIError;
  return;

GDIError:
  delete antialiaser;
  antialiaser = 0;
  if (_text) free(_text);

  env->ThrowError("VSubtitle: GDI or Insufficient Memory Error");
}



inline int CalcFontSize(int w, int h)
{
  enum { minFS=8, FS=128, minH=224, minW=388 };

  const int ws = (w < minW) ? (FS*w)/minW : FS;
  const int hs = (h < minH) ? (FS*h)/minH : FS;
  const int fs = (ws < hs) ? ws : hs;
  return ( (fs < minFS) ? minFS : fs );
}



const char* VSubtitle :: registerClass(IScriptEnvironment* env) {
	/**
		Identical to Subtitle with the exception that it is formatted using
		snprintf(new_text, max(format_space, old_text.length), old_text, frame_number)
		if any % characters are located in the text. Otherwise it's the same
	*/
	env->AddFunction(
		"VSubtitle",
		"cs[x]f[y]f[first_frame]i[last_frame]i[font]s[size]f[text_color]i[halo_color]i"
		"[align]i[spc]i[lsp]i[font_width]f[font_angle]f[interlaced]b"
		"[format_space]i",
		VSubtitle::create1,
		NULL
	);
	return "VSubtitle";
}



FUNCTION_END();
REGISTER(VSubtitle);



#endif



