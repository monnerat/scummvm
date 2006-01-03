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
#ifndef GOB_UTIL_H
#define GOB_UTIL_H

#include "gob/video.h"

namespace Gob {

#define KEYBUFSIZE 16

class Util {
public:
	struct ListNode;
	typedef struct ListNode {
		void *pData;
		struct ListNode *pNext;
		struct ListNode *pPrev;
		ListNode() : pData(0), pNext(0), pPrev(0) {}
	} ListNode;

	typedef struct List {
		ListNode *pHead;
		ListNode *pTail;
		List() : pHead(0), pTail(0) {}
	} List;

	void initInput(void);
	void processInput(void);
	void waitKey(void);
	int16 getKey(void);
	int16 checkKey(void);
	int16 getRandom(int16 max);
	void getMouseState(int16 *pX, int16 *pY, int16 *pButtons);
	void setMousePos(int16 x, int16 y);
	void longDelay(uint16 msecs);
	void delay(uint16 msecs);
	void beep(int16 freq);
	uint32 getTimeKey(void);
	void waitMouseUp(void);
	void waitMouseDown(void);

	void keyboard_init(void);
	void keyboard_release(void);

	void clearPalette(void);

	void asm_setPaletteBlock(char *tmpPalBuffer, int16 start, int16 end);

	void vid_waitRetrace(int16 mode);

	Video::FontDesc *loadFont(const char *path);
	void freeFont(Video::FontDesc * fontDesc);
	void insertStr(const char *str1, char *str2, int16 pos);
	void cutFromStr(char *str, int16 from, int16 cutlen);
	int16 strstr(const char *str1, char *str2);
	void waitEndFrame();
	void setFrameRate(int16 rate);

	void listInsertBack(List * list, void *data);
	void listInsertFront(List * list, void *data);
	void listDropFront(List * list);
	void deleteList(List * list);
	void prepareStr(char *str);
	void waitMouseRelease(char drawMouse);

	static const char trStr1[];
	static const char trStr2[];
	static const char trStr3[];
	Util(GobEngine *vm);

protected:
	int16 _mouseX, _mouseY, _mouseButtons;
	int16 _keyBuffer[KEYBUFSIZE], _keyBufferHead, _keyBufferTail;
	GobEngine *_vm;

	void addKeyToBuffer(int16 key);
	bool keyBufferEmpty();
	bool getKeyFromBuffer(int16& key);
	int16 translateKey(int16 key);
	int16 calcDelayTime();
	void checkJoystick();
};

}				// End of namespace Gob

#endif
