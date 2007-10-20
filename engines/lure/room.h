/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
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
 * $URL$
 * $Id$
 *
 */

#ifndef LURE_ROOM_H
#define LURE_ROOM_H


#include "common/scummsys.h"
#include "lure/disk.h"
#include "lure/res.h"
#include "lure/memory.h"
#include "lure/surface.h"
#include "lure/screen.h"
#include "lure/hotspots.h"

namespace Lure {

#define RECT_SIZE 32
#define NUM_HORIZ_RECTS 10
#define NUM_VERT_RECTS 6
#define FULL_HORIZ_RECTS 18
#define FULL_VERT_RECTS 14
#define NUM_EDGE_RECTS 4
#define GRID_SIZE (FULL_VERT_RECTS * FULL_HORIZ_RECTS)

class RoomLayer: public Surface {
private:
	bool _cells[FULL_VERT_RECTS][FULL_HORIZ_RECTS];
public:
	RoomLayer(uint16 screenId, bool backgroundLayer);
	bool isOccupied(byte cellX, byte cellY) {
		return _cells[cellY][cellX];
	}
};

enum CursorState {CS_NONE, CS_ACTION, CS_SEQUENCE, CS_TALKING, CS_BUMPED};

class Room {
private:
	RoomData *_roomData;
	Screen &_screen;
	uint16 _roomNumber;
	uint16 _descId;
	uint16 _hotspotId;
	uint16 _hotspotNameId;
	uint16 _destRoomNumber;
	bool _isExit;
	char _hotspotName[MAX_HOTSPOT_NAME_SIZE + MAX_ACTION_NAME_SIZE];
	char _statusLine[MAX_DESC_SIZE];
	HotspotData *_hotspot;
	bool _showInfo;
	uint8 _numLayers;
	RoomLayer *_layers[MAX_NUM_LAYERS];
	TalkDialog *_talkDialog;
	int16 _talkDialogX, _talkDialogY;
	CursorState _cursorState;

	void checkRoomHotspots();
	CursorType checkRoomExits();
	void loadRoomHotspots();
	void addAnimation(Hotspot &h);
	void addLayers(Hotspot &h);
	void addCell(int16 xp, int16 yp, int layerNum);
	void blockMerge();
public:
	RoomPathsDecompressedData tempLayer;
	Room();
	~Room();
	static Room &getReference();

	void update();
	void nextFrame();
	void checkCursor();
	uint16 roomNumber() { return _roomNumber; }
	void setRoomNumber(uint16 newRoomNumber, bool showOverlay = false);
	void leaveRoom();
	uint8 numLayers() { return _numLayers; }
	uint16 hotspotId() { return _hotspotId; }
	uint16 destRoomNumber() { return _destRoomNumber; }
	uint16 isExit() { return _isExit; }
	uint32 hotspotActions() { return _hotspot->actions & 0x10ffffff; }
	uint8 hotspotFlags() { return (_hotspot->actions >> 24) & 0xfe; }
	HotspotData &hotspot() { return *_hotspot; }
	uint16 descId() { return _descId; }
	bool showInfo() { return _showInfo; }
	CursorState cursorState() { return _cursorState; }
	void setShowInfo(bool value) { _showInfo = value; }
	void setTalkDialog(uint16 srcCharacterId, uint16 destCharacterId, uint16 usedId, uint16 stringId);
	void setCursorState(CursorState state) { _cursorState = state; }
	bool isDialogActive() { return _talkDialog != NULL; }
	bool checkInTalkDialog();
	char *statusLine() { return _statusLine; }
	void saveToStream(Common::WriteStream *stream);
	void loadFromStream(Common::ReadStream *stream);
};

} // end of namespace Lure

#endif
