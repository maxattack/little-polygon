// Little Polygon SDK
// Copyright (C) 2013 Max Kaufmann
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <littlepolygon_gocore.h>

struct GoEdit;
class GoEditorRer;

struct EditSkin {
	FontAsset *font;
	ImageAsset *icons;    // diff. frames for icons
	ImageAsset *palette;  // diff. frames for solid color swatches
};

// Create a new editor.  Internally it creates some special editting components
// that it will use to annotate various game objects with UI status, undo stacks, etc.
GoEdit *createEditor(GoContextRef goContext);

class GoEditorRef {
private:
	GoEdit *edit;

public:
	GoEditorRef() {}
	GoEditorRef(GoEdit *aEdit) : edit(aEdit) {}

	operator GoEdit*() { return edit; }
	operator bool() const { return edit; }

	void destroy();

	// Call this function if in-game editting is active.
	//   true - the editor UI handled the event
	//   false - the editor UI did not handle this event
	bool handleEvents(GoEdit *edit, SDL_Event *event);

	// Draw the UI editor with the given skin.  By default, shows
	// the property-tree for the scene.
	void draw(const EditSkin& skin, SpritePlotterRef sprites);
};
