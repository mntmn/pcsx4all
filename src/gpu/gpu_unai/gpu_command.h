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
INLINE void gpuSetTexture(u16 tpage)
{
	//senquack - 64-bit fix (from Notaz)
	//long tp;
	//long tx, ty;
	u32 tp;
	u32 tx, ty;

	//senquack - From Notaz's '64-bit issues' commit, he changed the following,
	//           in pcsx_rearmed's gpu_unai, but there was no associated comment.
	//           This fix makes Unai's code match gpu_drhell's GPU code
	// https://github.com/notaz/pcsx_rearmed/commit/4144e9abc1fb8420e08e0a5ef48a9ceba7f26661
	//GPU_GP1 = (GPU_GP1 & ~0x7FF) | (tpage & 0x7FF);
	GPU_GP1 = (GPU_GP1 & ~0x1FF) | (tpage & 0x1FF);

	TextureWindow[0]&= ~TextureWindow[2];
	TextureWindow[1]&= ~TextureWindow[3];

	tp = (tpage >> 7) & 3;
	tx = (tpage & 0x0F) << 6;
	ty = (tpage & 0x10) << 4;

	//senquack - Added this line from same commit by Notaz as above comment mentions:
	//           As you can see from 'tx += ' line just below the added line, it
	//           would be shifting by undefined amount should tp equal a value
	//           more than 2, so I suppose this is a good check to do.
	if (tp == 3) tp = 2;

	tx += (TextureWindow[0] >> (2 - tp));
	ty += TextureWindow[1];
	
	BLEND_MODE  = (((tpage>>5)&0x3)     ) << 3;
	TEXT_MODE   = (((tpage>>7)&0x3) + 1 ) << 5; // +1 el cero no lo usamos

	TBA = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(tx, ty)];

}

///////////////////////////////////////////////////////////////////////////////
INLINE void gpuSetCLUT(u16 clut)
{
	CBA = &((u16*)GPU_FrameBuffer)[(clut & 0x7FFF) << 4];
}

#ifdef  ENABLE_GPU_NULL_SUPPORT
#define NULL_GPU() break
#else
#define NULL_GPU()
#endif

#ifdef  ENABLE_GPU_LOG_SUPPORT
#define DO_LOG(expr) printf expr
#else
#define DO_LOG(expr) {}
#endif

#define Blending (((PRIM&0x2)&&(blend))?(PRIM&0x2):0)
#define Blending_Mode (((PRIM&0x2)&&(blend))?BLEND_MODE:0)
#define Lighting (((~PRIM)&0x1)&&(light))

void gpuSendPacketFunction(const int PRIM)
{
	//printf("0x%x\n",PRIM);

	switch (PRIM)
	{
		case 0x02:
			NULL_GPU();
			gpuClearImage();    //  prim handles updateLace && skip
			fb_dirty = true;
			DO_LOG(("gpuClearImage(0x%x)\n",PRIM));
			break;
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23:
			if (!skipGPU)
			{
				NULL_GPU();
				gpuDrawF3(gpuPolySpanDrivers [(blit_mask?512:0) | Blending_Mode | Masking | Blending | PixelMSB]);
				fb_dirty = true;
				DO_LOG(("gpuDrawF3(0x%x)\n",PRIM));
			}
			break;
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
			if (!skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (PacketBuffer.U4[4] >> 16);
				if ((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F))
					gpuDrawFT3(gpuPolySpanDrivers [(blit_mask?512:0) | Blending_Mode | TEXT_MODE | Masking | Blending | PixelMSB]);
				else
					gpuDrawFT3(gpuPolySpanDrivers [(blit_mask?512:0) | Blending_Mode | TEXT_MODE | Masking | Blending | Lighting | PixelMSB]);
				fb_dirty = true;
				DO_LOG(("gpuDrawFT3(0x%x)\n",PRIM));
			}
			break;
		case 0x28:
		case 0x29:
		case 0x2A:
		case 0x2B:
			if (!skipGPU)
			{
				NULL_GPU();
				const PP gpuPolySpanDriver  = gpuPolySpanDrivers [(blit_mask?512:0) | Blending_Mode | Masking | Blending | PixelMSB];
				//--PacketBuffer.S2[6];
				gpuDrawF3(gpuPolySpanDriver);
				PacketBuffer.U4[1] = PacketBuffer.U4[4];
				//--PacketBuffer.S2[2];
				gpuDrawF3(gpuPolySpanDriver);
				fb_dirty = true;
				DO_LOG(("gpuDrawF4(0x%x)\n",PRIM));
			}
			break;
		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F:
			if (!skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (PacketBuffer.U4[4] >> 16);
				PP gpuPolySpanDriver;
				if ((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F))
					gpuPolySpanDriver = gpuPolySpanDrivers [(blit_mask?512:0) | Blending_Mode | TEXT_MODE | Masking | Blending | PixelMSB];
				else
					gpuPolySpanDriver = gpuPolySpanDrivers [(blit_mask?512:0) | Blending_Mode | TEXT_MODE | Masking | Blending | Lighting | PixelMSB];
				//--PacketBuffer.S2[6];
				gpuDrawFT3(gpuPolySpanDriver);
				PacketBuffer.U4[1] = PacketBuffer.U4[7];
				PacketBuffer.U4[2] = PacketBuffer.U4[8];
				//--PacketBuffer.S2[2];
				gpuDrawFT3(gpuPolySpanDriver);
				fb_dirty = true;
				DO_LOG(("gpuDrawFT4(0x%x)\n",PRIM));
			}
			break;
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
			if (!skipGPU)
			{
				NULL_GPU();
				gpuDrawG3(gpuPolySpanDrivers [(blit_mask?512:0) | Blending_Mode | Masking | Blending | 129 | PixelMSB]);
				fb_dirty = true;
				DO_LOG(("gpuDrawG3(0x%x)\n",PRIM));
			}
			break;
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			if (!skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (PacketBuffer.U4[5] >> 16);
				gpuDrawGT3(gpuPolySpanDrivers [(blit_mask?512:0) | Blending_Mode | TEXT_MODE | Masking | Blending | ((Lighting)?129:0) | PixelMSB]);
				fb_dirty = true;
				DO_LOG(("gpuDrawGT3(0x%x)\n",PRIM));
			}
			break;
		case 0x38:
		case 0x39:
		case 0x3A:
		case 0x3B:
			if (!skipGPU)
			{
				NULL_GPU();
				const PP gpuPolySpanDriver  = gpuPolySpanDrivers [(blit_mask?512:0) | Blending_Mode | Masking | Blending | 129 | PixelMSB];
				//--PacketBuffer.S2[6];
				gpuDrawG3(gpuPolySpanDriver);
				PacketBuffer.U4[0] = PacketBuffer.U4[6];
				PacketBuffer.U4[1] = PacketBuffer.U4[7];
				//--PacketBuffer.S2[2];
				gpuDrawG3(gpuPolySpanDriver);
				fb_dirty = true;
				DO_LOG(("gpuDrawG4(0x%x)\n",PRIM));
			}
			break;
		case 0x3C:
		case 0x3D:
		case 0x3E:
		case 0x3F:
			if (!skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (PacketBuffer.U4[5] >> 16);
				const PP gpuPolySpanDriver  = gpuPolySpanDrivers [(blit_mask?512:0) | Blending_Mode | TEXT_MODE | Masking | Blending | ((Lighting)?129:0) | PixelMSB];
				//--PacketBuffer.S2[6];
				gpuDrawGT3(gpuPolySpanDriver);
				PacketBuffer.U4[0] = PacketBuffer.U4[9];
				PacketBuffer.U4[1] = PacketBuffer.U4[10];
				PacketBuffer.U4[2] = PacketBuffer.U4[11];
				//--PacketBuffer.S2[2];
				gpuDrawGT3(gpuPolySpanDriver);
				fb_dirty = true;
				DO_LOG(("gpuDrawGT4(0x%x)\n",PRIM));
			}
			break;
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
			if (!skipGPU)
			{
				NULL_GPU();
				gpuDrawLF(gpuPixelDrivers [ (Blending_Mode | Masking | Blending | (PixelMSB>>3)) >> 1]);
				fb_dirty = true;
				DO_LOG(("gpuDrawLF(0x%x)\n",PRIM));
			}
			break;
		case 0x48:
		case 0x49:
		case 0x4A:
		case 0x4B:
		case 0x4C:
		case 0x4D:
		case 0x4E:
		case 0x4F:
			if (!skipGPU)
			{
				NULL_GPU();
				gpuDrawLF(gpuPixelDrivers [ (Blending_Mode | Masking | Blending | (PixelMSB>>3)) >> 1]);
				fb_dirty = true;
				DO_LOG(("gpuDrawLF(0x%x)\n",PRIM));
			}
			if ((PacketBuffer.U4[3] & 0xF000F000) != 0x50005000)
			{
				PacketBuffer.U4[1] = PacketBuffer.U4[2];
				PacketBuffer.U4[2] = PacketBuffer.U4[3];
				PacketCount = 1;
				PacketIndex = 3;
			}
			break;
		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53:
			if (!skipGPU)
			{
				NULL_GPU();
				gpuDrawLG(gpuPixelDrivers [ (Blending_Mode | Masking | Blending | (PixelMSB>>3)) >> 1]);
				fb_dirty = true;
				DO_LOG(("gpuDrawLG(0x%x)\n",PRIM));
			}
			break;
		case 0x58:
		case 0x59:
		case 0x5A:
		case 0x5B:
		case 0x5C:
		case 0x5D:
		case 0x5E:
		case 0x5F:
			if (!skipGPU)
			{
				NULL_GPU();
				gpuDrawLG(gpuPixelDrivers [ (Blending_Mode | Masking | Blending | (PixelMSB>>3)) >> 1]);
				fb_dirty = true;
				DO_LOG(("gpuDrawLG(0x%x)\n",PRIM));
			}
			if ((PacketBuffer.U4[4] & 0xF000F000) != 0x50005000)
			{
				PacketBuffer.U1[3 + (2 * 4)] = PacketBuffer.U1[3 + (0 * 4)];
				PacketBuffer.U4[0] = PacketBuffer.U4[2];
				PacketBuffer.U4[1] = PacketBuffer.U4[3];
				PacketBuffer.U4[2] = PacketBuffer.U4[4];
				PacketCount = 2;
				PacketIndex = 3;
			}
			break;
		case 0x60:
		case 0x61:
		case 0x62:
		case 0x63:
			if (!skipGPU)
			{
				NULL_GPU();
				gpuDrawT(gpuTileSpanDrivers [Blending_Mode | Masking | Blending | (PixelMSB>>3)]);
				fb_dirty = true;
				DO_LOG(("gpuDrawT(0x%x)\n",PRIM));
			}
			break;
		case 0x64:
		case 0x65:
		case 0x66:
		case 0x67:
			if (!skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (GPU_GP1);
				if ((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F))
					gpuDrawS(gpuSpriteSpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | (PixelMSB>>1)]);
				else
					gpuDrawS(gpuSpriteSpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | Lighting | (PixelMSB>>1)]);
				fb_dirty = true;
				DO_LOG(("gpuDrawS(0x%x)\n",PRIM));
			}
			break;
		case 0x68:
		case 0x69:
		case 0x6A:
		case 0x6B:
			if (!skipGPU)
			{
				NULL_GPU();
				PacketBuffer.U4[2] = 0x00010001;
				gpuDrawT(gpuTileSpanDrivers [Blending_Mode | Masking | Blending | (PixelMSB>>3)]);
				fb_dirty = true;
				DO_LOG(("gpuDrawT(0x%x)\n",PRIM));
			}
			break;
		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
			if (!skipGPU)
			{
				NULL_GPU();
				PacketBuffer.U4[2] = 0x00080008;
				gpuDrawT(gpuTileSpanDrivers [Blending_Mode | Masking | Blending | (PixelMSB>>3)]);
				fb_dirty = true;
				DO_LOG(("gpuDrawT(0x%x)\n",PRIM));
			}
			break;
		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77:
			if (!skipGPU)
			{
				NULL_GPU();
				PacketBuffer.U4[3] = 0x00080008;
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (GPU_GP1);
				if ((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F))
					gpuDrawS(gpuSpriteSpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | (PixelMSB>>1)]);
				else
					gpuDrawS(gpuSpriteSpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | Lighting | (PixelMSB>>1)]);
				fb_dirty = true;
				DO_LOG(("gpuDrawS(0x%x)\n",PRIM));
			}
			break;
		case 0x78:
		case 0x79:
		case 0x7A:
		case 0x7B:
			if (!skipGPU)
			{
				NULL_GPU();
				PacketBuffer.U4[2] = 0x00100010;
				gpuDrawT(gpuTileSpanDrivers [Blending_Mode | Masking | Blending | (PixelMSB>>3)]);
				fb_dirty = true;
				DO_LOG(("gpuDrawT(0x%x)\n",PRIM));
			}
			break;
		case 0x7C:
		case 0x7D:
			#ifdef __arm__
			/* Notaz 4bit sprites optimization */
			if ((!skipGPU) && (!(GPU_GP1&0x180)) && (!(Masking|PixelMSB)))
			{
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (GPU_GP1);
				gpuDrawS16();
				fb_dirty = true;
				break;
			}
			#endif
		case 0x7E:
		case 0x7F:
			if (!skipGPU)
			{
				NULL_GPU();
				PacketBuffer.U4[3] = 0x00100010;
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (GPU_GP1);
				if ((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F))
					gpuDrawS(gpuSpriteSpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | (PixelMSB>>1)]);
				else
					gpuDrawS(gpuSpriteSpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | Lighting | (PixelMSB>>1)]);
				fb_dirty = true;
				DO_LOG(("gpuDrawS(0x%x)\n",PRIM));
			}
			break;
		case 0x80:          //  vid -> vid
			gpuMoveImage();   //  prim handles updateLace && skip
			if ((!skipCount) && (DisplayArea[3] == 480)) // Tekken 3 hack
			{
				if (!skipGPU) fb_dirty = true;
			}
			else
			{
				fb_dirty = true;
			}
			DO_LOG(("gpuMoveImage(0x%x)\n",PRIM));
			break;
		case 0xA0:          //  sys ->vid
			gpuLoadImage();   //  prim handles updateLace && skip
			DO_LOG(("gpuLoadImage(0x%x)\n",PRIM));
			break;
		case 0xC0:          //  vid -> sys
			gpuStoreImage();  //  prim handles updateLace && skip
			DO_LOG(("gpuStoreImage(0x%x)\n",PRIM));
			break;
		case 0xE1:
			{
				const u32 temp = PacketBuffer.U4[0];
				GPU_GP1 = (GPU_GP1 & ~0x000007FF) | (temp & 0x000007FF);
				gpuSetTexture(temp);
				DO_LOG(("gpuSetTexture(0x%x)\n",PRIM));
			}
			break;
		case 0xE2:	  
			{
				static const u8  TextureMask[32] = {
					255, 7, 15, 7, 31, 7, 15, 7, 63, 7, 15, 7, 31, 7, 15, 7,	//
					127, 7, 15, 7, 31, 7, 15, 7, 63, 7, 15, 7, 31, 7, 15, 7	  //
				};
				const u32 temp = PacketBuffer.U4[0];
				tw=temp&0xFFFFF;
				TextureWindow[0] = ((temp >> 10) & 0x1F) << 3;
				TextureWindow[1] = ((temp >> 15) & 0x1F) << 3;
				TextureWindow[2] = TextureMask[(temp >> 0) & 0x1F];
				TextureWindow[3] = TextureMask[(temp >> 5) & 0x1F];
				//senquack - new vars must be updated whenever texture window is changed:
				u4_msk = i2x(TextureWindow[2]) | ((1 << FIXED_BITS) - 1);
				v4_msk = i2x(TextureWindow[3]) | ((1 << FIXED_BITS) - 1);
				gpuSetTexture(GPU_GP1);
				DO_LOG(("TextureWindow(0x%x)\n",PRIM));
			}
			break;
		case 0xE3:
			{
				const u32 temp = PacketBuffer.U4[0];
				DrawingArea[0] = temp         & 0x3FF;
				DrawingArea[1] = (temp >> 10) & 0x3FF;
				DO_LOG(("DrawingArea_Pos(0x%x)\n",PRIM));
			}
			break;
		case 0xE4:
			{
				const u32 temp = PacketBuffer.U4[0];
				DrawingArea[2] = (temp         & 0x3FF) + 1;
				DrawingArea[3] = ((temp >> 10) & 0x3FF) + 1;
				DO_LOG(("DrawingArea_Size(0x%x)\n",PRIM));
			}
			break;
		case 0xE5:
			{
				const u32 temp = PacketBuffer.U4[0];
				//senquack - 64-bit fix from Notaz:
				//DrawingOffset[0] = ((long)temp<<(32-11))>>(32-11);
				//DrawingOffset[1] = ((long)temp<<(32-22))>>(32-11);
				DrawingOffset[0] = ((s32)temp<<(32-11))>>(32-11);
				DrawingOffset[1] = ((s32)temp<<(32-22))>>(32-11);
				DO_LOG(("DrawingOffset(0x%x)\n",PRIM));
			}
			break;
		case 0xE6:
			{
				const u32 temp = PacketBuffer.U4[0];
				//GPU_GP1 = (GPU_GP1 & ~0x00001800) | ((temp&3) << 11);
				Masking = (temp & 0x2) <<  1;
				PixelMSB =(temp & 0x1) <<  8;
				DO_LOG(("SetMask(0x%x)\n",PRIM));
			}
			break;
	}
}
