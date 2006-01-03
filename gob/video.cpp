/* ScummVM - Scumm Interpreter
 * Copyright (C) 2004 Ivan Dubrov
 * Copyright (C) 2004-2005 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Header$
 *
 */
#include "gob/gob.h"
#include "gob/global.h"
#include "gob/video.h"
#include "gob/dataio.h"

#include "gob/driver_vga.h"

namespace Gob {

/* NOT IMPLEMENTED */

Video::Video(GobEngine *vm) : _vm(vm) {
}

//XXX: Use this function to update the screen for now.
//     This should be moved to a better location later on.
void Video::waitRetrace(int16) {
	if (_vm->_global->pPrimarySurfDesc) {
		g_system->copyRectToScreen(_vm->_global->pPrimarySurfDesc->vidPtr, 320, 0, 0, 320, 200);
		g_system->updateScreen();
	}
}

char Video::initDriver(int16 vidMode) {
	warning("STUB: Video::initDriver");

	// FIXME: Finish all this stuff :)
	_videoDriver = new VGAVideoDriver();

	return 1;
}

void Video::freeDriver() {
	delete _videoDriver;
	warning("STUB: Video::freeDriver");
}

int32 Video::getRectSize(int16 width, int16 height, int16 flag, int16 mode) {
	int32 size;

	if ((mode & 0x7f) != 0x13)
		warning
		    ("Video::getRectSize: Video mode %d is not fully supported!",
		    mode & 0x7f);
	switch (mode & 0x7f) {
	case 5:
	case 7:
		size = ((int32)((width + 3) / 4)) * height * (flag + 1);
		break;
	case 0x13:
		size = (int32)width *height;
		break;
	case 0x14:
	case 0x15:
	case 0x16:
		size = ((int32)((width + 3) / 4)) * height * 4;
		break;
	default:
		size = ((int32)((width + 7) / 8)) * height * (flag + 4);
		break;
	}
	return size;
}

Video::SurfaceDesc *Video::initSurfDesc(int16 vidMode, int16 width, int16 height, int16 flags) {
	int8 flagsAnd2;
	byte *vidMem;
	int32 sprSize;
	int16 someFlags = 1;
	SurfaceDesc *descPtr;

	if (flags != PRIMARY_SURFACE)
		_vm->_global->sprAllocated++;

	if (flags & RETURN_PRIMARY)
		return _vm->_global->pPrimarySurfDesc;

	if (vidMode != 0x13)
		error("Video::initSurfDesc: Only VGA 0x13 mode is supported!");

	if ((flags & PRIMARY_SURFACE) == 0)
		vidMode += 0x80;

	if (flags & 2)
		flagsAnd2 = 1;
	else
		flagsAnd2 = 0;

	if (flags & PRIMARY_SURFACE) {
		vidMem = 0;
		_vm->_global->primaryWidth = width;
		_vm->_global->mouseMaxCol = width;
		_vm->_global->primaryHeight = height;
		_vm->_global->mouseMaxRow = height;
		sprSize = 0;

	} else {
		vidMem = 0;
		sprSize = Video::getRectSize(width, height, flagsAnd2, vidMode);
		if (flagsAnd2)
			someFlags += 0x80;
	}
	if (flags & PRIMARY_SURFACE) {
		descPtr = _vm->_global->pPrimarySurfDesc;
		vidMem = (byte *)malloc(320 * 200);
	} else {
		if (flags & DISABLE_SPR_ALLOC)
			descPtr = (SurfaceDesc *)malloc(sizeof(SurfaceDesc));
		else
			descPtr = (SurfaceDesc *)malloc(sizeof(SurfaceDesc) + sprSize);
	}
	if (descPtr == 0)
		return 0;

	descPtr->width = width;
	descPtr->height = height;
	descPtr->flag = someFlags;
	descPtr->vidMode = vidMode;
	if (vidMem == 0)
		vidMem = ((byte *)descPtr) + sizeof(SurfaceDesc);
	descPtr->vidPtr = vidMem;

	descPtr->reserved1 = 0;
	descPtr->reserved2 = 0;
	return descPtr;
}

void Video::freeSurfDesc(SurfaceDesc * surfDesc) {
	_vm->_global->sprAllocated--;
	if (surfDesc != _vm->_global->pPrimarySurfDesc)
		free(surfDesc);
	else
		free(surfDesc->vidPtr);
}

int16 Video::clampValue(int16 val, int16 max) {
	if (val >= max)
		val = max - 1;

	if (val < 0)
		val = 0;

	return val;
}

void Video::drawSprite(SurfaceDesc *source, SurfaceDesc *dest,
	    int16 left, int16 top, int16 right, int16 bottom, int16 x, int16 y, int16 transp) {
	int16 temp;
	int16 destRight;
	int16 destBottom;

	if (_vm->_global->doRangeClamp) {
		if (left > right) {
			temp = left;
			left = right;
			right = temp;
		}
		if (top > bottom) {
			temp = top;
			top = bottom;
			bottom = temp;
		}
		if (right < 0)
			return;
		if (bottom < 0)
			return;
		if (left >= source->width)
			return;
		if (top >= source->height)
			return;

		if (left < 0) {
			x -= left;
			left = 0;
		}
		if (top < 0) {
			y -= top;
			top = 0;
		}
		right = Video::clampValue(right, source->width);
		bottom = Video::clampValue(bottom, source->height);
		if (right - left >= source->width)
			right = left + source->width - 1;
		if (bottom - top >= source->height)
			bottom = top + source->height - 1;

		if (x < 0) {
			left -= x;
			x = 0;
		}
		if (y < 0) {
			top -= y;
			y = 0;
		}
		if (left > right)
			return;
		if (top > bottom)
			return;

		if (x >= dest->width)
			return;

		if (y >= dest->height)
			return;

		destRight = x + right - left;
		destBottom = y + bottom - top;
		if (destRight >= dest->width)
			right -= destRight - dest->width + 1;

		if (destBottom >= dest->height)
			bottom -= destBottom - dest->height + 1;
	}

	_videoDriver->drawSprite(source, dest, left, top, right, bottom, x, y, transp);
}

void Video::fillRect(SurfaceDesc *dest, int16 left, int16 top, int16 right, int16 bottom,
	    int16 color) {
	int16 temp;

	if (_vm->_global->doRangeClamp) {
		if (left > right) {
			temp = left;
			left = right;
			right = temp;
		}
		if (top > bottom) {
			temp = top;
			top = bottom;
			bottom = temp;
		}
		if (right < 0)
			return;
		if (bottom < 0)
			return;
		if (left >= dest->width)
			return;
		if (top >= dest->height)
			return;

		left = Video::clampValue(left, dest->width);
		top = Video::clampValue(top, dest->height);
		right = Video::clampValue(right, dest->width);
		bottom = Video::clampValue(bottom, dest->height);
	}

	_videoDriver->fillRect(dest, left, top, right, bottom, color);
}

void Video::drawLine(SurfaceDesc *dest, int16 x0, int16 y0, int16 x1, int16 y1, int16 color) {
	if (x0 == x1 || y0 == y1) {
		Video::fillRect(dest, x0, y0, x1, y1, color);
		return;
	}

	_videoDriver->drawLine(dest, x0, y0, x1, y1, color);
}

void Video::putPixel(int16 x, int16 y, int16 color, SurfaceDesc *dest) {
	if (x < 0 || y < 0 || x >= dest->width || y >= dest->height)
		return;

	_videoDriver->putPixel(x, y, color, dest);
}

void Video::drawLetter(unsigned char item, int16 x, int16 y, FontDesc *fontDesc, int16 color1,
	    int16 color2, int16 transp, SurfaceDesc *dest) {

	_videoDriver->drawLetter(item, x, y, fontDesc, color1, color2, transp, dest);
}

void Video::clearSurf(SurfaceDesc *dest) {
	Video::fillRect(dest, 0, 0, dest->width - 1, dest->height - 1, 0);
}

void Video::drawPackedSprite(byte *sprBuf, int16 width, int16 height, int16 x, int16 y,
	    int16 transp, SurfaceDesc *dest) {

	if (Video::spriteUncompressor(sprBuf, width, height, x, y, transp, dest))
		return;

	if ((dest->vidMode & 0x7f) != 0x13)
		error("Video::drawPackedSprite: Vide mode 0x%x is not fully supported!",
		    dest->vidMode & 0x7f);

	_videoDriver->drawPackedSprite(sprBuf, width, height, x, y, transp, dest);
}

void Video::setPalElem(int16 index, char red, char green, char blue, int16 unused,
	    int16 vidMode) {
	byte pal[4];

	_vm->_global->redPalette[index] = red;
	_vm->_global->greenPalette[index] = green;
	_vm->_global->bluePalette[index] = blue;

	if (vidMode != 0x13)
		error("Video::setPalElem: Video mode 0x%x is not supported!",
		    vidMode);

	pal[0] = (red << 2) | (red >> 4);
	pal[1] = (green << 2) | (green >> 4);
	pal[2] = (blue << 2) | (blue >> 4);
	pal[3] = 0;
	g_system->setPalette(pal, index, 1);
}

void Video::setPalette(PalDesc *palDesc) {
	int16 i;
	byte pal[1024];
	int16 numcolors;

	if (_vm->_global->videoMode != 0x13)
		error("Video::setPalette: Video mode 0x%x is not supported!",
		    _vm->_global->videoMode);

	if (_vm->_global->setAllPalette)
		numcolors = 256;
	else
		numcolors = 16;

	for (i = 0; i < numcolors; i++) {
		pal[i * 4 + 0] = (palDesc->vgaPal[i].red << 2) | (palDesc->vgaPal[i].red >> 4);
		pal[i * 4 + 1] = (palDesc->vgaPal[i].green << 2) | (palDesc->vgaPal[i].green >> 4);
		pal[i * 4 + 2] = (palDesc->vgaPal[i].blue << 2) | (palDesc->vgaPal[i].blue >> 4);
		pal[i * 4 + 3] = 0;
	}

	g_system->setPalette(pal, 0, numcolors);
}

void Video::setFullPalette(PalDesc *palDesc) {
	Color *colors;
	int16 i;
	byte pal[1024];

	if (_vm->_global->setAllPalette) {
		colors = palDesc->vgaPal;
		for (i = 0; i < 256; i++) {
			_vm->_global->redPalette[i] = colors[i].red;
			_vm->_global->greenPalette[i] = colors[i].green;
			_vm->_global->bluePalette[i] = colors[i].blue;
		}

		for (i = 0; i < 256; i++) {
			pal[i * 4 + 0] = (colors[i].red << 2) | (colors[i].red >> 4);
			pal[i * 4 + 1] = (colors[i].green << 2) | (colors[i].green >> 4);
			pal[i * 4 + 2] = (colors[i].blue << 2) | (colors[i].blue >> 4);
			pal[i * 4 + 3] = 0;
		}
		g_system->setPalette(pal, 0, 256);
	} else {
		Video::setPalette(palDesc);
	}
}

void Video::initPrimary(int16 mode) {
	int16 old;
	if (_vm->_global->curPrimaryDesc) {
		Video::freeSurfDesc(_vm->_global->curPrimaryDesc);
		Video::freeSurfDesc(_vm->_global->allocatedPrimary);

		_vm->_global->curPrimaryDesc = 0;
		_vm->_global->allocatedPrimary = 0;
	}
	if (mode != 0x13 && mode != 3 && mode != -1)
		error("Video::initPrimary: Video mode 0x%x is not supported!",
		    mode);

	if (_vm->_global->videoMode != 0x13)
		error("Video::initPrimary: Video mode 0x%x is not supported!",
		    mode);

	old = _vm->_global->oldMode;
	if (mode == -1)
		mode = 3;

	_vm->_global->oldMode = mode;
	if (mode != 3)
		Video::initDriver(mode);

	if (mode != 3) {
		Video::initSurfDesc(mode, 320, 200, PRIMARY_SURFACE);

		if (_vm->_global->dontSetPalette)
			return;

		Video::setFullPalette(_vm->_global->pPaletteDesc);
	}
}

char Video::spriteUncompressor(byte *sprBuf, int16 srcWidth, int16 srcHeight,
	    int16 x, int16 y, int16 transp, SurfaceDesc *destDesc) {
	SurfaceDesc sourceDesc;
	byte *memBuffer;
	byte *srcPtr;
	byte *destPtr;
	byte *linePtr;
	byte temp;
	uint16 sourceLeft;
	int16 curWidth;
	int16 curHeight;
	int16 offset;
	int16 counter2;
	uint16 cmdVar;
	int16 bufPos;
	int16 strLen;

	if (!destDesc)
		return 1;

	if ((destDesc->vidMode & 0x7f) != 0x13)
		error("Video::spriteUncompressor: Video mode 0x%x is not supported!",
		    destDesc->vidMode & 0x7f);

	if (sprBuf[0] != 1)
		return 0;

	if (sprBuf[1] != 2)
		return 0;

	if (sprBuf[2] == 2) {
		sourceDesc.width = srcWidth;
		sourceDesc.height = srcHeight;
		sourceDesc.vidMode = 0x93;
		sourceDesc.vidPtr = sprBuf + 3;
		Video::drawSprite(&sourceDesc, destDesc, 0, 0, srcWidth - 1,
		    srcHeight - 1, x, y, transp);
		return 1;
	} else {
		memBuffer = (byte *)malloc(4114);
		if (memBuffer == 0)
			return 0;

		srcPtr = sprBuf + 3;
		sourceLeft = READ_LE_UINT16(srcPtr);

		destPtr = destDesc->vidPtr + destDesc->width * y + x;

		curWidth = 0;
		curHeight = 0;

		linePtr = destPtr;
		srcPtr += 4;

		for (offset = 0; offset < 4078; offset++)
			memBuffer[offset] = 0x20;

		cmdVar = 0;
		bufPos = 4078;
		while (1) {
			cmdVar >>= 1;
			if ((cmdVar & 0x100) == 0) {
				cmdVar = *srcPtr | 0xff00;
				srcPtr++;
			}
			if ((cmdVar & 1) != 0) {
				temp = *srcPtr++;
				if (temp != 0 || transp == 0)
					*destPtr = temp;
				destPtr++;
				curWidth++;
				if (curWidth >= srcWidth) {
					curWidth = 0;
					linePtr += destDesc->width;
					destPtr = linePtr;
					curHeight++;
					if (curHeight >= srcHeight)
						break;
				}
				sourceLeft--;
				if (sourceLeft == 0)
					break;

				memBuffer[bufPos] = temp;
				bufPos++;
				bufPos %= 4096;
			} else {
				offset = *srcPtr;
				srcPtr++;
				offset |= (*srcPtr & 0xf0) << 4;
				strLen = (*srcPtr & 0x0f) + 3;
				srcPtr++;

				for (counter2 = 0; counter2 < strLen;
				    counter2++) {
					temp =
					    memBuffer[(offset +
						counter2) % 4096];
					if (temp != 0 || transp == 0)
						*destPtr = temp;
					destPtr++;

					curWidth++;
					if (curWidth >= srcWidth) {
						curWidth = 0;
						linePtr += destDesc->width;
						destPtr = linePtr;
						curHeight++;
						if (curHeight >= srcHeight) {
							free(memBuffer);
							return 1;
						}
					}
					sourceLeft--;
					if (sourceLeft == 0) {
						free(memBuffer);
						return 1;
					}
					memBuffer[bufPos] = temp;
					bufPos++;
					bufPos %= 4096;
				}
			}
		}
	}
	free(memBuffer);
	return 1;
}

void Video::setHandlers() { _vm->_global->setAllPalette = 1; }

}				// End of namespace Gob
