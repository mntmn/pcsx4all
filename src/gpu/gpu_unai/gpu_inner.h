/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Unai                                               *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
***************************************************************************/

///////////////////////////////////////////////////////////////////////////////
//  Inner loop driver instanciation file

///////////////////////////////////////////////////////////////////////////////
//  Option Masks
#define   L ((CF>>0)&1)
#define   B ((CF>>1)&1)
#define   M ((CF>>2)&1)
#define  BM ((CF>>3)&3)
#define  TM ((CF>>5)&3)
#define   G ((CF>>7)&1)

#define  MB ((CF>>8)&1)

#define  BI ((CF>>9)&1)

#ifdef __arm__
#ifndef ENABLE_GPU_ARMV7
/* ARMv5 */
#include "gpu_inner_blend_arm5.h"
#else
/* ARMv7 optimized */
#include "gpu_inner_blend_arm7.h"
#endif
#else
#include "gpu_inner_blend.h"
#endif

#include "gpu_inner_light.h"

///////////////////////////////////////////////////////////////////////////////
//  GPU Pixel operations generator
template<const int CF>
INLINE void gpuPixelFn(u16 *pixel,const u16 data)
{
	if ((!M)&&(!B))
	{
		if(MB) { *pixel = data | 0x8000; }
		else   { *pixel = data; }
	}
	else if ((M)&&(!B))
	{
		if (!(*pixel&0x8000))
		{
			if(MB) { *pixel = data | 0x8000; }
			else   { *pixel = data; }
		}
	}
	else
	{
		u16 uDst = *pixel;
		if(M) { if (uDst&0x8000) return; }
		u16 uSrc = data;
		u32 uMsk; if (BM==0) uMsk=0x7BDE;
		if (BM==0) gpuBlending00(uSrc, uDst);
		if (BM==1) gpuBlending01(uSrc, uDst);
		if (BM==2) gpuBlending02(uSrc, uDst);
		if (BM==3) gpuBlending03(uSrc, uDst);

		if(MB) { *pixel = uSrc | 0x8000; }
		else   { *pixel = uSrc; }
	}
}

INLINE void PixelNULL(u16 *pixel,const u16 data)
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"PixelNULL()\n");
	#endif
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Pixel drawing drivers, for lines (only blending)
typedef void (*PD)(u16 *pixel,const u16 data);
const PD  gpuPixelDrivers[32] =   //  We only generate pixel op for MASKING/BLEND_ENABLE/BLEND_MODE
{ 
	gpuPixelFn<0x00<<1>,gpuPixelFn<0x01<<1>,gpuPixelFn<0x02<<1>,gpuPixelFn<0x03<<1>,  
	PixelNULL,gpuPixelFn<0x05<<1>,PixelNULL,gpuPixelFn<0x07<<1>,
	PixelNULL,gpuPixelFn<0x09<<1>,PixelNULL,gpuPixelFn<0x0B<<1>,
	PixelNULL,gpuPixelFn<0x0D<<1>,PixelNULL,gpuPixelFn<0x0F<<1>,

	gpuPixelFn<(0x00<<1)|256>,gpuPixelFn<(0x01<<1)|256>,gpuPixelFn<(0x02<<1)|256>,gpuPixelFn<(0x03<<1)|256>,  
	PixelNULL,gpuPixelFn<(0x05<<1)|256>,PixelNULL,gpuPixelFn<(0x07<<1)|256>,
	PixelNULL,gpuPixelFn<(0x09<<1)|256>,PixelNULL,gpuPixelFn<(0x0B<<1)|256>,
	PixelNULL,gpuPixelFn<(0x0D<<1)|256>,PixelNULL,gpuPixelFn<(0x0F<<1)|256>
};

///////////////////////////////////////////////////////////////////////////////
//  GPU Tiles innerloops generator

template<const int CF>
INLINE void  gpuTileSpanFn(u16 *pDst, u32 count, u16 data)
{
	if ((!M)&&(!B))
	{
		if (MB) { data = data | 0x8000; }
		do { *pDst++ = data; } while (--count);
	}
	else if ((M)&&(!B))
	{
		if (MB) { data = data | 0x8000; }
		do { if (!(*pDst&0x8000)) { *pDst = data; } pDst++; } while (--count);
	}
	else
	{
		u16 uSrc;
		u16 uDst;
		u32 uMsk; if (BM==0) uMsk=0x7BDE;
		do
		{
			//  MASKING
			uDst = *pDst;
			if(M) { if (uDst&0x8000) goto endtile;  }
			uSrc = data;

			//  BLEND
			if (BM==0) gpuBlending00(uSrc, uDst);
			if (BM==1) gpuBlending01(uSrc, uDst);
			if (BM==2) gpuBlending02(uSrc, uDst);
			if (BM==3) gpuBlending03(uSrc, uDst);

			if (MB) { *pDst = uSrc | 0x8000; }
			else    { *pDst = uSrc; }

			//senquack - Did not apply "Silent Hill" mask-bit fix to here.
			// It is hard to tell from scarce documentation available and
			//  lack of comments in code, but I believe the tile-span
			//  functions here should not bother to preserve any source MSB,
			//  as they are not drawing from a texture.

			endtile: pDst++;
		}
		while (--count);
	}
}

INLINE void TileNULL(u16 *pDst, u32 count, u16 data)
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"TileNULL()\n");
	#endif
}

///////////////////////////////////////////////////////////////////////////////
//  Tiles innerloops driver
typedef void (*PT)(u16 *pDst, u32 count, u16 data);
const PT gpuTileSpanDrivers[64] = 
{
	gpuTileSpanFn<0x00>,TileNULL,gpuTileSpanFn<0x02>,TileNULL,  gpuTileSpanFn<0x04>,TileNULL,gpuTileSpanFn<0x06>,TileNULL,  TileNULL,TileNULL,gpuTileSpanFn<0x0A>,TileNULL,  TileNULL,TileNULL,gpuTileSpanFn<0x0E>,TileNULL,
	TileNULL,TileNULL,gpuTileSpanFn<0x12>,TileNULL,  TileNULL,TileNULL,gpuTileSpanFn<0x16>,TileNULL,  TileNULL,TileNULL,gpuTileSpanFn<0x1A>,TileNULL,  TileNULL,TileNULL,gpuTileSpanFn<0x1E>,TileNULL,

	gpuTileSpanFn<0x100>,TileNULL,gpuTileSpanFn<0x102>,TileNULL,  gpuTileSpanFn<0x104>,TileNULL,gpuTileSpanFn<0x106>,TileNULL,  TileNULL,TileNULL,gpuTileSpanFn<0x10A>,TileNULL,  TileNULL,TileNULL,gpuTileSpanFn<0x10E>,TileNULL,
	TileNULL,TileNULL,gpuTileSpanFn<0x112>,TileNULL,  TileNULL,TileNULL,gpuTileSpanFn<0x116>,TileNULL,  TileNULL,TileNULL,gpuTileSpanFn<0x11A>,TileNULL,  TileNULL,TileNULL,gpuTileSpanFn<0x11E>,TileNULL,
};

///////////////////////////////////////////////////////////////////////////////
//  GPU Sprites innerloops generator

template<const int CF>
INLINE void  gpuSpriteSpanFn(u16 *pDst, u32 count, u32 u0, const u32 mask)
{
	u16 uSrc;
	u16 uDst;
	const u16* pTxt = TBA+(u0&~0x1ff); u0=u0&0x1ff;
	const u16 *_CBA; if(TM!=3) _CBA=CBA;
	u32 lCol; if(L)  { lCol = ((u32)(b4<< 2)&(0x03ff)) | ((u32)(g4<<13)&(0x07ff<<10)) | ((u32)(r4<<24)&(0x07ff<<21));  }
	u8 rgb; if (TM==1) rgb = ((u8*)pTxt)[u0>>1];
	u32 uMsk; if ((B)&&(BM==0)) uMsk=0x7BDE;

	//senquack - 'Silent Hill' white rectangles around characters fix:
	// MSB of pixel from source texture was not preserved across calls to
	// gpuBlendingXX() macros.. we must save it if calling them:
	// NOTE: it was gpuPolySpanFn() that cause the Silent Hill glitches, but
	// from reading the few PSX HW docs available, MSB should be transferred
	// whenever a texture is being read.
	u16 srcMSB;

	do
	{
		//  MASKING
		if(M)   { uDst = *pDst;   if (uDst&0x8000) { u0=(u0+1)&mask; goto endsprite; }  }

		//  TEXTURE MAPPING
		if (TM==1) { if (!(u0&1)) rgb = ((u8*)pTxt)[u0>>1]; uSrc = _CBA[(rgb>>((u0&1)<<2))&0xf]; u0=(u0+1)&mask; }
		if (TM==2) { uSrc = _CBA[((u8*)pTxt)[u0]]; u0=(u0+1)&mask; }
		if (TM==3) { uSrc = pTxt[u0]; u0=(u0+1)&mask; }
		if (!uSrc) goto endsprite;

		//senquack - save source MSB:
		if (!MB) { srcMSB = uSrc & 0x8000; }
		
		//  BLEND
		if(B)
		{
			if(uSrc&0x8000)
			{
				//  LIGHTING CALCULATIONS
				if(L)  { gpuLightingTXT(uSrc, lCol);   }

				if(!M)    { uDst = *pDst; }
				if (BM==0) gpuBlending00(uSrc, uDst);
				if (BM==1) gpuBlending01(uSrc, uDst);
				if (BM==2) gpuBlending02(uSrc, uDst);
				if (BM==3) gpuBlending03(uSrc, uDst);
			}
			else
			{
				//  LIGHTING CALCULATIONS
				if(L)  { gpuLightingTXT(uSrc, lCol); }
			}
		}
		else
		{
			//  LIGHTING CALCULATIONS
			if(L)  { gpuLightingTXT(uSrc, lCol);   } else
			{ if(!MB) uSrc&= 0x7fff;               }
		}

		//senquack - 'Silent Hill' fix: MSB of pixel from source texture
		// was not preserved by calls to gpuBlendingXX() macros.. Now it
		// is saved in srcMSB before calling them:
		if (MB) { *pDst = uSrc | 0x8000; }
		else    { *pDst = uSrc | srcMSB; }

		endsprite: pDst++;
	}
	while (--count);
}

INLINE void SpriteNULL(u16 *pDst, u32 count, u32 u0, const u32 mask)
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"SpriteNULL()\n");
	#endif
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Sprite innerloops driver
typedef void (*PS)(u16 *pDst, u32 count, u32 u0, const u32 mask);
const PS gpuSpriteSpanDrivers[256] = 
{
	SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,
	SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,
	gpuSpriteSpanFn<0x20>,gpuSpriteSpanFn<0x21>,gpuSpriteSpanFn<0x22>,gpuSpriteSpanFn<0x23>,  gpuSpriteSpanFn<0x24>,gpuSpriteSpanFn<0x25>,gpuSpriteSpanFn<0x26>,gpuSpriteSpanFn<0x27>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x2A>,gpuSpriteSpanFn<0x2B>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x2E>,gpuSpriteSpanFn<0x2F>,
	SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x32>,gpuSpriteSpanFn<0x33>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x36>,gpuSpriteSpanFn<0x37>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x3A>,gpuSpriteSpanFn<0x3B>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x3E>,gpuSpriteSpanFn<0x3F>,
	gpuSpriteSpanFn<0x40>,gpuSpriteSpanFn<0x41>,gpuSpriteSpanFn<0x42>,gpuSpriteSpanFn<0x43>,  gpuSpriteSpanFn<0x44>,gpuSpriteSpanFn<0x45>,gpuSpriteSpanFn<0x46>,gpuSpriteSpanFn<0x47>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x4A>,gpuSpriteSpanFn<0x4B>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x4E>,gpuSpriteSpanFn<0x4F>,
	SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x52>,gpuSpriteSpanFn<0x53>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x56>,gpuSpriteSpanFn<0x57>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x5A>,gpuSpriteSpanFn<0x5B>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x5E>,gpuSpriteSpanFn<0x5F>,
	gpuSpriteSpanFn<0x60>,gpuSpriteSpanFn<0x61>,gpuSpriteSpanFn<0x62>,gpuSpriteSpanFn<0x63>,  gpuSpriteSpanFn<0x64>,gpuSpriteSpanFn<0x65>,gpuSpriteSpanFn<0x66>,gpuSpriteSpanFn<0x67>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x6A>,gpuSpriteSpanFn<0x6B>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x6E>,gpuSpriteSpanFn<0x6F>,
	SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x72>,gpuSpriteSpanFn<0x73>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x76>,gpuSpriteSpanFn<0x77>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x7A>,gpuSpriteSpanFn<0x7B>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x7E>,gpuSpriteSpanFn<0x7F>,

	SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,
	SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,SpriteNULL,
	gpuSpriteSpanFn<0x120>,gpuSpriteSpanFn<0x121>,gpuSpriteSpanFn<0x122>,gpuSpriteSpanFn<0x123>,  gpuSpriteSpanFn<0x124>,gpuSpriteSpanFn<0x125>,gpuSpriteSpanFn<0x126>,gpuSpriteSpanFn<0x127>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x12A>,gpuSpriteSpanFn<0x12B>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x12E>,gpuSpriteSpanFn<0x12F>,
	SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x132>,gpuSpriteSpanFn<0x133>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x136>,gpuSpriteSpanFn<0x137>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x13A>,gpuSpriteSpanFn<0x13B>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x13E>,gpuSpriteSpanFn<0x13F>,
	gpuSpriteSpanFn<0x140>,gpuSpriteSpanFn<0x141>,gpuSpriteSpanFn<0x142>,gpuSpriteSpanFn<0x143>,  gpuSpriteSpanFn<0x144>,gpuSpriteSpanFn<0x145>,gpuSpriteSpanFn<0x146>,gpuSpriteSpanFn<0x147>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x14A>,gpuSpriteSpanFn<0x14B>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x14E>,gpuSpriteSpanFn<0x14F>,
	SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x152>,gpuSpriteSpanFn<0x153>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x156>,gpuSpriteSpanFn<0x157>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x15A>,gpuSpriteSpanFn<0x15B>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x15E>,gpuSpriteSpanFn<0x15F>,
	gpuSpriteSpanFn<0x160>,gpuSpriteSpanFn<0x161>,gpuSpriteSpanFn<0x162>,gpuSpriteSpanFn<0x163>,  gpuSpriteSpanFn<0x164>,gpuSpriteSpanFn<0x165>,gpuSpriteSpanFn<0x166>,gpuSpriteSpanFn<0x167>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x16A>,gpuSpriteSpanFn<0x16B>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x16E>,gpuSpriteSpanFn<0x16F>,
	SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x172>,gpuSpriteSpanFn<0x173>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x176>,gpuSpriteSpanFn<0x177>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x17A>,gpuSpriteSpanFn<0x17B>,  SpriteNULL,SpriteNULL,gpuSpriteSpanFn<0x17E>,gpuSpriteSpanFn<0x17F>
};

///////////////////////////////////////////////////////////////////////////////
//  GPU Polygon innerloops generator

//senquack - Original UNAI code, has been replaced with newer function below it.
//           (kept original here for future reference)
#if 0
template<const int CF>
INLINE void  gpuPolySpanFn(u16 *pDst, u32 count)
{
	if (!TM)
	{	
		// NO TEXTURE
		if (!G)
		{
			// NO GOURAUD
			u16 data;
			if (L) { u32 lCol=((u32)(b4<< 2)&(0x03ff)) | ((u32)(g4<<13)&(0x07ff<<10)) | ((u32)(r4<<24)&(0x07ff<<21)); gpuLightingRGB(data,lCol); }
			else data=PixelData;
			if ((!M)&&(!B))
			{
				if (MB) { data = data | 0x8000; }
				do { *pDst++ = data; } while (--count);
			}
			else if ((M)&&(!B))
			{
				if (MB) { data = data | 0x8000; }
				do { if (!(*pDst&0x8000)) { *pDst = data; } pDst++; } while (--count);
			}
			else
			{
				u16 uSrc;
				u16 uDst;
				u32 uMsk; if (BM==0) uMsk=0x7BDE;
				u32 bMsk; if (BI) bMsk=blit_mask;
				do
				{
					// blit-mask
					if (BI) { if((bMsk>>((((u32)pDst)>>1)&7))&1) goto endtile; }
					//  masking
					uDst = *pDst;
					if(M) { if (uDst&0x8000) goto endtile;  }
					uSrc = data;
					//  blend
					if (BM==0) gpuBlending00(uSrc, uDst);
					if (BM==1) gpuBlending01(uSrc, uDst);
					if (BM==2) gpuBlending02(uSrc, uDst);
					if (BM==3) gpuBlending03(uSrc, uDst);
					if (MB) { *pDst = uSrc | 0x8000; }
					else    { *pDst = uSrc; }
					endtile: pDst++;
				}
				while (--count);
			}
		}
		else
		{
			// GOURAUD
			u16 uDst;
			u16 uSrc;
			u32 linc=lInc;
			u32 lCol=((u32)(b4>>14)&(0x03ff)) | ((u32)(g4>>3)&(0x07ff<<10)) | ((u32)(r4<<8)&(0x07ff<<21));
			u32 uMsk; if ((B)&&(BM==0)) uMsk=0x7BDE;
			u32 bMsk; if (BI) bMsk=blit_mask;
			do
			{
				// blit-mask
				if (BI) { if((bMsk>>((((u32)pDst)>>1)&7))&1) goto endgou; }
				//  masking
				if(M) { uDst = *pDst;  if (uDst&0x8000) goto endgou;  }
				//  blend
				if(B)
				{
					//  light
					gpuLightingRGB(uSrc,lCol);
					if(!M)    { uDst = *pDst; }
					if (BM==0) gpuBlending00(uSrc, uDst);
					if (BM==1) gpuBlending01(uSrc, uDst);
					if (BM==2) gpuBlending02(uSrc, uDst);
					if (BM==3) gpuBlending03(uSrc, uDst);
				}
				else
				{
					//  light
					gpuLightingRGB(uSrc,lCol);
				}
				if (MB) { *pDst = uSrc | 0x8000; }
				else    { *pDst = uSrc; }
				endgou: pDst++; lCol=(lCol+linc);
			}
			while (--count);
		}
	}
	else
	{
		// TEXTURE
		u16 uDst;
		u16 uSrc;
		u32 linc; if (L&&G) linc=lInc;
		u32 tinc=tInc;
		u32 tmsk=tMsk;
		u32 tCor = ((u32)( u4<<7)&0x7fff0000) | ((u32)( v4>>9)&0x00007fff); tCor&= tmsk;
		const u16* _TBA=TBA;
		const u16* _CBA; if (TM!=3) _CBA=CBA;
		u32 lCol;
		if(L && !G) { lCol = ((u32)(b4<< 2)&(0x03ff)) | ((u32)(g4<<13)&(0x07ff<<10)) | ((u32)(r4<<24)&(0x07ff<<21)); }
		else if(L && G) { lCol = ((u32)(b4>>14)&(0x03ff)) | ((u32)(g4>>3)&(0x07ff<<10)) | ((u32)(r4<<8)&(0x07ff<<21)); 	}
		u32 uMsk; if ((B)&&(BM==0)) uMsk=0x7BDE;
		u32 bMsk; if (BI) bMsk=blit_mask;
		do
		{
			// blit-mask
			if (BI) { if((bMsk>>((((u32)pDst)>>1)&7))&1) goto endpoly; }
			//  masking
			if(M) { uDst = *pDst;  if (uDst&0x8000) goto endpoly;  }
			//  texture
			if (TM==1) { u32 tu=(tCor>>23); u32 tv=(tCor<<4)&(0xff<<11); u8 rgb=((u8*)_TBA)[tv+(tu>>1)]; uSrc=_CBA[(rgb>>((tu&1)<<2))&0xf]; if(!uSrc) goto endpoly; }
			if (TM==2) { uSrc = _CBA[(((u8*)_TBA)[(tCor>>23)+((tCor<<4)&(0xff<<11))])]; if(!uSrc)  goto endpoly; }
			if (TM==3) { uSrc = _TBA[(tCor>>23)+((tCor<<3)&(0xff<<10))]; if(!uSrc)  goto endpoly; }
			//  blend
			if(B)
			{
				if (uSrc&0x8000)
				{
					//  light
					if(L) gpuLightingTXT(uSrc, lCol);
					if(!M)    { uDst = *pDst; }
					if (BM==0) gpuBlending00(uSrc, uDst);
					if (BM==1) gpuBlending01(uSrc, uDst);
					if (BM==2) gpuBlending02(uSrc, uDst);
					if (BM==3) gpuBlending03(uSrc, uDst);
				}
				else
				{
					// light
					if(L) gpuLightingTXT(uSrc, lCol);
				}
			}
			else
			{
				//  light
				if(L)  { gpuLightingTXT(uSrc, lCol); } else if(!MB) { uSrc&= 0x7fff; }
			}
			if (MB) { *pDst = uSrc | 0x8000; }
			else    { *pDst = uSrc; }
			endpoly: pDst++;
			tCor=(tCor+tinc)&tmsk;
			if (L&&G) lCol=(lCol+linc);
		}
		while (--count);
	}
}
#endif // ^^^ DISABLED ORIGINAL VERSION ^^^

//senquack - Newer version of above with following changes:
//           * Adapted to work with new poly routings in gpu_raster_polygon.h
//             adapted from DrHell GPU. They are less glitchy and use 22.10
//             fixed-point instead of original UNAI's 16.16.
//           * Texture coordinates are no longer packed together into one
//             unsigned int. This seems to lose too much accuracy (they each
//             end up being only 8.7 fixed-point that way) and pixel-droupouts
//             were noticeable both with original code and current DrHell
//             adaptations. An example would be the sky in NFS3. Now, they are
//             stored in separate ints, using separate masks.
//           * Function is no longer INLINE, as it was always called
//             through a function pointer.
//           * Function now ensures the mask bit of source texture is preserved
//             across calls to blending functions (Silent Hill rectangles fix)
template<const int CF>
void gpuPolySpanFn(u16 *pDst, u32 count)
{
	if (!TM)
	{	
		// NO TEXTURE
		if (!G)
		{
			// NO GOURAUD
			u16 data;

			//senquack - NOTE: if no Gouraud shading is used, b4,g4,r4 contain integers,
			//           not fixed-point (this is behavior of original code)
			if (L) { u32 lCol=((u32)(b4<< 2)&(0x03ff)) | ((u32)(g4<<13)&(0x07ff<<10)) | ((u32)(r4<<24)&(0x07ff<<21)); gpuLightingRGB(data,lCol); }
			else data=PixelData;
			if ((!M)&&(!B))
			{
				if (MB) { data = data | 0x8000; }
				do { *pDst++ = data; } while (--count);
			}
			else if ((M)&&(!B))
			{
				if (MB) { data = data | 0x8000; }
				do { if (!(*pDst&0x8000)) { *pDst = data; } pDst++; } while (--count);
			}
			else
			{
				u16 uSrc;
				u16 uDst;
				u32 uMsk; if (BM==0) uMsk=0x7BDE;
				u32 bMsk; if (BI) bMsk=blit_mask;
				do
				{
					// blit-mask
					if (BI) { if((bMsk>>((((u32)pDst)>>1)&7))&1) goto endtile; }
					//  masking
					uDst = *pDst;
					if(M) { if (uDst&0x8000) goto endtile;  }
					uSrc = data;
					//  blend
					if (BM==0) gpuBlending00(uSrc, uDst);
					if (BM==1) gpuBlending01(uSrc, uDst);
					if (BM==2) gpuBlending02(uSrc, uDst);
					if (BM==3) gpuBlending03(uSrc, uDst);

					if (MB) { *pDst = uSrc | 0x8000; }
					else    { *pDst = uSrc; }
					endtile: pDst++;
				}
				while (--count);
			}
		}
		else
		{
			// GOURAUD
			u16 uDst;
			u16 uSrc;
			u32 linc=lInc;

			//senquack - DRHELL poly code uses 22.10 fixed point, while UNAI used 16.16:
			//u32 lCol=((u32)(b4>>14)&(0x03ff)) | ((u32)(g4>>3)&(0x07ff<<10)) | ((u32)(r4<<8)&(0x07ff<<21));
			u32 lCol=((u32)(b4>>8)&(0x03ff)) | ((u32)(g4<<3)&(0x07ff<<10)) | ((u32)(r4<<14)&(0x07ff<<21));

			u32 uMsk; if ((B)&&(BM==0)) uMsk=0x7BDE;
			u32 bMsk; if (BI) bMsk=blit_mask;
			do
			{
				// blit-mask
				if (BI) { if((bMsk>>((((u32)pDst)>>1)&7))&1) goto endgou; }
				//  masking
				if(M) { uDst = *pDst;  if (uDst&0x8000) goto endgou;  }
				//  blend
				if(B)
				{
					//  light
					gpuLightingRGB(uSrc,lCol);
					if(!M)    { uDst = *pDst; }
					if (BM==0) gpuBlending00(uSrc, uDst);
					if (BM==1) gpuBlending01(uSrc, uDst);
					if (BM==2) gpuBlending02(uSrc, uDst);
					if (BM==3) gpuBlending03(uSrc, uDst);
				}
				else
				{
					//  light
					gpuLightingRGB(uSrc,lCol);
				}

				if (MB) { *pDst = uSrc | 0x8000; }
				else    { *pDst = uSrc; }

				endgou: pDst++; lCol=(lCol+linc);
			}
			while (--count);
		}
	}
	else
	{
		//senquack - 'Silent Hill' white rectangles around characters fix:
		// MSB of pixel from source texture was not preserved across calls to
		// gpuBlendingXX() macros.. we must save it if calling them:
		u16 srcMSB;

		// TEXTURE
		u16 uDst;
		u16 uSrc;
		u32 linc; if (L&&G) linc=lInc;

		//senquack - note: original UNAI code had u4/v4 packed into one
		// 32-bit unsigned int, but this proved to lose too much accuracy
		// (pixel drouputs noticeable in NFS3 sky), so now are separate vars.
		// u4_msk/v4_msk are new global vars (u4 and v4 were already globals)
		// TODO: find better way of passing parameters like u4/v4/r4 etc
		//       into this function than original pass-through-globals method.
		u32 l_u4_msk = u4_msk;      u32 l_v4_msk = v4_msk;
		u32 l_u4 = u4 & l_u4_msk;   u32 l_v4 = v4 & l_v4_msk;
		u32 l_du4 = du4;            u32 l_dv4 = dv4;

		const u16* _TBA=TBA;
		const u16* _CBA; if (TM!=3) _CBA=CBA;
		u32 lCol;

		//senquack - After adapting DrHell poly code to replace Unai's glitchy
		// routines, it was necessary to adjust for the differences in fixed
		// point: DrHell is 22.10, Unai used 16.16. In the following two lines,
		// the top line expected b4,g4,r4 to be integers, as it is not using
		// Gouraud shading, so it needed no adjustment. The bottom line did
		// need adjustment for fixed point changes.
		if(L && !G) { lCol = ((u32)(b4<< 2)&(0x03ff)) | ((u32)(g4<<13)&(0x07ff<<10)) | ((u32)(r4<<24)&(0x07ff<<21)); }
		else if(L && G) { lCol = ((u32)(b4>>8)&(0x03ff)) | ((u32)(g4<<3)&(0x07ff<<10)) | ((u32)(r4<<14)&(0x07ff<<21)); 	}

		u32 uMsk; if ((B)&&(BM==0)) uMsk=0x7BDE;
		u32 bMsk; if (BI) bMsk=blit_mask;

		do
		{
			// blit-mask
			if (BI) { if((bMsk>>((((u32)pDst)>>1)&7))&1) goto endpoly; }
			//  masking
			if(M) { uDst = *pDst;  if (uDst&0x8000) goto endpoly;  }

			//senquack - adapted to work with new 22.10 fixed point routines:
			//           (UNAI originally used 16.16)
			if (TM==1) { 
				u32 tu=(l_u4>>10);
				u32 tv=(l_v4<<1)&(0xff<<11);
				u8 rgb=((u8*)_TBA)[tv+(tu>>1)];
				uSrc=_CBA[(rgb>>((tu&1)<<2))&0xf];
				if(!uSrc) goto endpoly;
			}
			if (TM==2) {
				uSrc = _CBA[(((u8*)_TBA)[(l_u4>>10)+((l_v4<<1)&(0xff<<11))])];
				if(!uSrc)  goto endpoly;
			}
			if (TM==3) {
				uSrc = _TBA[(l_u4>>10)+((l_v4)&(0xff<<10))];
				if(!uSrc)  goto endpoly;
			}

			//senquack - Silent Hill fix
			if (!MB) { srcMSB = uSrc & 0x8000; }

			//  blend
			if(B)
			{
				if (uSrc&0x8000)
				{
					//  light
					if(L) gpuLightingTXT(uSrc, lCol);
					if(!M)    { uDst = *pDst; }
					if (BM==0) gpuBlending00(uSrc, uDst);
					if (BM==1) gpuBlending01(uSrc, uDst);
					if (BM==2) gpuBlending02(uSrc, uDst);
					if (BM==3) gpuBlending03(uSrc, uDst);
				}
				else
				{
					// light
					if(L) gpuLightingTXT(uSrc, lCol);
				}
			}
			else
			{
				//  light

				//senquack - While fixing Silent Hill white-rectangles bug, I
				// noticed uSrc was being masked unnecessarily here:
				//if(L)  { gpuLightingTXT(uSrc, lCol); } else if(!MB) { uSrc&= 0x7fff; }
				if(L)  { gpuLightingTXT(uSrc, lCol); }
			}

			//senquack - 'Silent Hill' fix: MSB of pixel from source texture
			// was not preserved by calls to gpuBlendingXX() macros.. Now it
			// is saved in srcMSB before calling them:
			//if (MB) { *pDst = uSrc | 0x8000; }
			//else    { *pDst = uSrc; }
			if (MB) { *pDst = uSrc | 0x8000; }
			else    { *pDst = uSrc | srcMSB; }

			endpoly: pDst++;

			l_u4 = (l_u4 + l_du4) & l_u4_msk;
			l_v4 = (l_v4 + l_dv4) & l_v4_msk;

			if (L&&G) lCol=(lCol+linc);
		}
		while (--count);
	}
}

INLINE void PolyNULL(u16 *pDst, u32 count)
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"PolyNULL()\n");
	#endif
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Polygon innerloops driver
typedef void (*PP)(u16 *pDst, u32 count);
const PP gpuPolySpanDrivers[1024] =
{
	gpuPolySpanFn<0x00>,gpuPolySpanFn<0x01>,gpuPolySpanFn<0x02>,gpuPolySpanFn<0x03>,  gpuPolySpanFn<0x04>,gpuPolySpanFn<0x05>,gpuPolySpanFn<0x06>,gpuPolySpanFn<0x07>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x0A>,gpuPolySpanFn<0x0B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x0E>,gpuPolySpanFn<0x0F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<0x12>,gpuPolySpanFn<0x13>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x16>,gpuPolySpanFn<0x17>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x1A>,gpuPolySpanFn<0x1B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x1E>,gpuPolySpanFn<0x1F>,
	gpuPolySpanFn<0x20>,gpuPolySpanFn<0x21>,gpuPolySpanFn<0x22>,gpuPolySpanFn<0x23>,  gpuPolySpanFn<0x24>,gpuPolySpanFn<0x25>,gpuPolySpanFn<0x26>,gpuPolySpanFn<0x27>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x2A>,gpuPolySpanFn<0x2B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x2E>,gpuPolySpanFn<0x2F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<0x32>,gpuPolySpanFn<0x33>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x36>,gpuPolySpanFn<0x37>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x3A>,gpuPolySpanFn<0x3B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x3E>,gpuPolySpanFn<0x3F>,
	gpuPolySpanFn<0x40>,gpuPolySpanFn<0x41>,gpuPolySpanFn<0x42>,gpuPolySpanFn<0x43>,  gpuPolySpanFn<0x44>,gpuPolySpanFn<0x45>,gpuPolySpanFn<0x46>,gpuPolySpanFn<0x47>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x4A>,gpuPolySpanFn<0x4B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x4E>,gpuPolySpanFn<0x4F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<0x52>,gpuPolySpanFn<0x53>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x56>,gpuPolySpanFn<0x57>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x5A>,gpuPolySpanFn<0x5B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x5E>,gpuPolySpanFn<0x5F>,
	gpuPolySpanFn<0x60>,gpuPolySpanFn<0x61>,gpuPolySpanFn<0x62>,gpuPolySpanFn<0x63>,  gpuPolySpanFn<0x64>,gpuPolySpanFn<0x65>,gpuPolySpanFn<0x66>,gpuPolySpanFn<0x67>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x6A>,gpuPolySpanFn<0x6B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x6E>,gpuPolySpanFn<0x6F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<0x72>,gpuPolySpanFn<0x73>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x76>,gpuPolySpanFn<0x77>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x7A>,gpuPolySpanFn<0x7B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x7E>,gpuPolySpanFn<0x7F>,

	PolyNULL,gpuPolySpanFn<0x81>,PolyNULL,gpuPolySpanFn<0x83>,  PolyNULL,gpuPolySpanFn<0x85>,PolyNULL,gpuPolySpanFn<0x87>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x8B>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x8F>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x93>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x97>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x9B>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x9F>,
	PolyNULL,gpuPolySpanFn<0xa1>,PolyNULL,gpuPolySpanFn<0xa3>,  PolyNULL,gpuPolySpanFn<0xa5>,PolyNULL,gpuPolySpanFn<0xa7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xaB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xaF>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xb3>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xb7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xbB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xbF>,
	PolyNULL,gpuPolySpanFn<0xc1>,PolyNULL,gpuPolySpanFn<0xc3>,  PolyNULL,gpuPolySpanFn<0xc5>,PolyNULL,gpuPolySpanFn<0xc7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xcB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xcF>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xd3>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xd7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xdB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xdF>,
	PolyNULL,gpuPolySpanFn<0xe1>,PolyNULL,gpuPolySpanFn<0xe3>,  PolyNULL,gpuPolySpanFn<0xe5>,PolyNULL,gpuPolySpanFn<0xe7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xeB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xeF>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xf3>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xf7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xfB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0xfF>,

	gpuPolySpanFn<0x100>,gpuPolySpanFn<0x101>,gpuPolySpanFn<0x102>,gpuPolySpanFn<0x103>,  gpuPolySpanFn<0x104>,gpuPolySpanFn<0x105>,gpuPolySpanFn<0x106>,gpuPolySpanFn<0x107>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x10A>,gpuPolySpanFn<0x10B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x10E>,gpuPolySpanFn<0x10F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<0x112>,gpuPolySpanFn<0x113>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x116>,gpuPolySpanFn<0x117>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x11A>,gpuPolySpanFn<0x11B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x11E>,gpuPolySpanFn<0x11F>,
	gpuPolySpanFn<0x120>,gpuPolySpanFn<0x121>,gpuPolySpanFn<0x122>,gpuPolySpanFn<0x123>,  gpuPolySpanFn<0x124>,gpuPolySpanFn<0x125>,gpuPolySpanFn<0x126>,gpuPolySpanFn<0x127>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x12A>,gpuPolySpanFn<0x12B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x12E>,gpuPolySpanFn<0x12F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<0x132>,gpuPolySpanFn<0x133>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x136>,gpuPolySpanFn<0x137>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x13A>,gpuPolySpanFn<0x13B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x13E>,gpuPolySpanFn<0x13F>,
	gpuPolySpanFn<0x140>,gpuPolySpanFn<0x141>,gpuPolySpanFn<0x142>,gpuPolySpanFn<0x143>,  gpuPolySpanFn<0x144>,gpuPolySpanFn<0x145>,gpuPolySpanFn<0x146>,gpuPolySpanFn<0x147>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x14A>,gpuPolySpanFn<0x14B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x14E>,gpuPolySpanFn<0x14F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<0x152>,gpuPolySpanFn<0x153>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x156>,gpuPolySpanFn<0x157>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x15A>,gpuPolySpanFn<0x15B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x15E>,gpuPolySpanFn<0x15F>,
	gpuPolySpanFn<0x160>,gpuPolySpanFn<0x161>,gpuPolySpanFn<0x162>,gpuPolySpanFn<0x163>,  gpuPolySpanFn<0x164>,gpuPolySpanFn<0x165>,gpuPolySpanFn<0x166>,gpuPolySpanFn<0x167>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x16A>,gpuPolySpanFn<0x16B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x16E>,gpuPolySpanFn<0x16F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<0x172>,gpuPolySpanFn<0x173>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x176>,gpuPolySpanFn<0x177>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x17A>,gpuPolySpanFn<0x17B>,  PolyNULL,PolyNULL,gpuPolySpanFn<0x17E>,gpuPolySpanFn<0x17F>,
                                                                                                                                                                                                                                                                                                                                                                                      
	PolyNULL,gpuPolySpanFn<0x181>,PolyNULL,gpuPolySpanFn<0x183>,  PolyNULL,gpuPolySpanFn<0x185>,PolyNULL,gpuPolySpanFn<0x187>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x18B>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x18F>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x193>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x197>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x19B>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x19F>,
	PolyNULL,gpuPolySpanFn<0x1a1>,PolyNULL,gpuPolySpanFn<0x1a3>,  PolyNULL,gpuPolySpanFn<0x1a5>,PolyNULL,gpuPolySpanFn<0x1a7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1aB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1aF>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1b3>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1b7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1bB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1bF>,
	PolyNULL,gpuPolySpanFn<0x1c1>,PolyNULL,gpuPolySpanFn<0x1c3>,  PolyNULL,gpuPolySpanFn<0x1c5>,PolyNULL,gpuPolySpanFn<0x1c7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1cB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1cF>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1d3>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1d7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1dB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1dF>,
	PolyNULL,gpuPolySpanFn<0x1e1>,PolyNULL,gpuPolySpanFn<0x1e3>,  PolyNULL,gpuPolySpanFn<0x1e5>,PolyNULL,gpuPolySpanFn<0x1e7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1eB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1eF>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1f3>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1f7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1fB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<0x1fF>,

	gpuPolySpanFn<512|0x00>,gpuPolySpanFn<512|0x01>,gpuPolySpanFn<512|0x02>,gpuPolySpanFn<512|0x03>,  gpuPolySpanFn<512|0x04>,gpuPolySpanFn<512|0x05>,gpuPolySpanFn<512|0x06>,gpuPolySpanFn<512|0x07>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x0A>,gpuPolySpanFn<512|0x0B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x0E>,gpuPolySpanFn<512|0x0F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<512|0x12>,gpuPolySpanFn<512|0x13>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x16>,gpuPolySpanFn<512|0x17>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1A>,gpuPolySpanFn<512|0x1B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1E>,gpuPolySpanFn<512|0x1F>,
	gpuPolySpanFn<512|0x20>,gpuPolySpanFn<512|0x21>,gpuPolySpanFn<512|0x22>,gpuPolySpanFn<512|0x23>,  gpuPolySpanFn<512|0x24>,gpuPolySpanFn<512|0x25>,gpuPolySpanFn<512|0x26>,gpuPolySpanFn<512|0x27>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x2A>,gpuPolySpanFn<512|0x2B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x2E>,gpuPolySpanFn<512|0x2F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<512|0x32>,gpuPolySpanFn<512|0x33>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x36>,gpuPolySpanFn<512|0x37>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x3A>,gpuPolySpanFn<512|0x3B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x3E>,gpuPolySpanFn<512|0x3F>,
	gpuPolySpanFn<512|0x40>,gpuPolySpanFn<512|0x41>,gpuPolySpanFn<512|0x42>,gpuPolySpanFn<512|0x43>,  gpuPolySpanFn<512|0x44>,gpuPolySpanFn<512|0x45>,gpuPolySpanFn<512|0x46>,gpuPolySpanFn<512|0x47>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x4A>,gpuPolySpanFn<512|0x4B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x4E>,gpuPolySpanFn<512|0x4F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<512|0x52>,gpuPolySpanFn<512|0x53>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x56>,gpuPolySpanFn<512|0x57>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x5A>,gpuPolySpanFn<512|0x5B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x5E>,gpuPolySpanFn<512|0x5F>,
	gpuPolySpanFn<512|0x60>,gpuPolySpanFn<512|0x61>,gpuPolySpanFn<512|0x62>,gpuPolySpanFn<512|0x63>,  gpuPolySpanFn<512|0x64>,gpuPolySpanFn<512|0x65>,gpuPolySpanFn<512|0x66>,gpuPolySpanFn<512|0x67>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x6A>,gpuPolySpanFn<512|0x6B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x6E>,gpuPolySpanFn<512|0x6F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<512|0x72>,gpuPolySpanFn<512|0x73>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x76>,gpuPolySpanFn<512|0x77>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x7A>,gpuPolySpanFn<512|0x7B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x7E>,gpuPolySpanFn<512|0x7F>,

	PolyNULL,gpuPolySpanFn<512|0x81>,PolyNULL,gpuPolySpanFn<512|0x83>,  PolyNULL,gpuPolySpanFn<512|0x85>,PolyNULL,gpuPolySpanFn<512|0x87>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x8B>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x8F>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x93>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x97>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x9B>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x9F>,
	PolyNULL,gpuPolySpanFn<512|0xa1>,PolyNULL,gpuPolySpanFn<512|0xa3>,  PolyNULL,gpuPolySpanFn<512|0xa5>,PolyNULL,gpuPolySpanFn<512|0xa7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xaB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xaF>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xb3>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xb7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xbB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xbF>,
	PolyNULL,gpuPolySpanFn<512|0xc1>,PolyNULL,gpuPolySpanFn<512|0xc3>,  PolyNULL,gpuPolySpanFn<512|0xc5>,PolyNULL,gpuPolySpanFn<512|0xc7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xcB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xcF>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xd3>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xd7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xdB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xdF>,
	PolyNULL,gpuPolySpanFn<512|0xe1>,PolyNULL,gpuPolySpanFn<512|0xe3>,  PolyNULL,gpuPolySpanFn<512|0xe5>,PolyNULL,gpuPolySpanFn<512|0xe7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xeB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xeF>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xf3>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xf7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xfB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0xfF>,

	gpuPolySpanFn<512|0x100>,gpuPolySpanFn<512|0x101>,gpuPolySpanFn<512|0x102>,gpuPolySpanFn<512|0x103>,  gpuPolySpanFn<512|0x104>,gpuPolySpanFn<512|0x105>,gpuPolySpanFn<512|0x106>,gpuPolySpanFn<512|0x107>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x10A>,gpuPolySpanFn<512|0x10B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x10E>,gpuPolySpanFn<512|0x10F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<512|0x112>,gpuPolySpanFn<512|0x113>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x116>,gpuPolySpanFn<512|0x117>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x11A>,gpuPolySpanFn<512|0x11B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x11E>,gpuPolySpanFn<512|0x11F>,
	gpuPolySpanFn<512|0x120>,gpuPolySpanFn<512|0x121>,gpuPolySpanFn<512|0x122>,gpuPolySpanFn<512|0x123>,  gpuPolySpanFn<512|0x124>,gpuPolySpanFn<512|0x125>,gpuPolySpanFn<512|0x126>,gpuPolySpanFn<512|0x127>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x12A>,gpuPolySpanFn<512|0x12B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x12E>,gpuPolySpanFn<512|0x12F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<512|0x132>,gpuPolySpanFn<512|0x133>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x136>,gpuPolySpanFn<512|0x137>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x13A>,gpuPolySpanFn<512|0x13B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x13E>,gpuPolySpanFn<512|0x13F>,
	gpuPolySpanFn<512|0x140>,gpuPolySpanFn<512|0x141>,gpuPolySpanFn<512|0x142>,gpuPolySpanFn<512|0x143>,  gpuPolySpanFn<512|0x144>,gpuPolySpanFn<512|0x145>,gpuPolySpanFn<512|0x146>,gpuPolySpanFn<512|0x147>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x14A>,gpuPolySpanFn<512|0x14B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x14E>,gpuPolySpanFn<512|0x14F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<512|0x152>,gpuPolySpanFn<512|0x153>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x156>,gpuPolySpanFn<512|0x157>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x15A>,gpuPolySpanFn<512|0x15B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x15E>,gpuPolySpanFn<512|0x15F>,
	gpuPolySpanFn<512|0x160>,gpuPolySpanFn<512|0x161>,gpuPolySpanFn<512|0x162>,gpuPolySpanFn<512|0x163>,  gpuPolySpanFn<512|0x164>,gpuPolySpanFn<512|0x165>,gpuPolySpanFn<512|0x166>,gpuPolySpanFn<512|0x167>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x16A>,gpuPolySpanFn<512|0x16B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x16E>,gpuPolySpanFn<512|0x16F>,
	PolyNULL,PolyNULL,gpuPolySpanFn<512|0x172>,gpuPolySpanFn<512|0x173>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x176>,gpuPolySpanFn<512|0x177>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x17A>,gpuPolySpanFn<512|0x17B>,  PolyNULL,PolyNULL,gpuPolySpanFn<512|0x17E>,gpuPolySpanFn<512|0x17F>,
                                                                                                                                                                                                                                                                                                                                                                                      
	PolyNULL,gpuPolySpanFn<512|0x181>,PolyNULL,gpuPolySpanFn<512|0x183>,  PolyNULL,gpuPolySpanFn<512|0x185>,PolyNULL,gpuPolySpanFn<512|0x187>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x18B>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x18F>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x193>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x197>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x19B>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x19F>,
	PolyNULL,gpuPolySpanFn<512|0x1a1>,PolyNULL,gpuPolySpanFn<512|0x1a3>,  PolyNULL,gpuPolySpanFn<512|0x1a5>,PolyNULL,gpuPolySpanFn<512|0x1a7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1aB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1aF>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1b3>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1b7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1bB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1bF>,
	PolyNULL,gpuPolySpanFn<512|0x1c1>,PolyNULL,gpuPolySpanFn<512|0x1c3>,  PolyNULL,gpuPolySpanFn<512|0x1c5>,PolyNULL,gpuPolySpanFn<512|0x1c7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1cB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1cF>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1d3>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1d7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1dB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1dF>,
	PolyNULL,gpuPolySpanFn<512|0x1e1>,PolyNULL,gpuPolySpanFn<512|0x1e3>,  PolyNULL,gpuPolySpanFn<512|0x1e5>,PolyNULL,gpuPolySpanFn<512|0x1e7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1eB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1eF>,
	PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1f3>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1f7>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1fB>,  PolyNULL,PolyNULL,PolyNULL,gpuPolySpanFn<512|0x1fF>
};
