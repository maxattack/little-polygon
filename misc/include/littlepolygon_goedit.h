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

struct EditSkin {
	FontAsset *font;
	ImageAsset *icons;    // diff. frames for icons
	ImageAsset *palette;  // diff. frames for solid color swatches
};

// Create a new editor.  Internally it creates some special editting components
// that it will use to annotate various game objects with UI status, undo stacks, etc.
GoEdit *createEditor(GoContext *goContext);

// Release the editor and all it's internal components.
void destroy(GoEdit *edit);

// Call this method when editting is enabled (app-defined).  Will return true if it
// handled the event with it's own internal UI.
bool handleEvents(GoEdit *edit, SDL_Event *event);

// Call this method when editting is enabled (app-defined) to draw the UI (right now it just
// draws the FK heirarchy as a tree-widget on the left quarter of the screen)
void draw(GoEdit *edit, const EditSkin *skin, SpritePlotter *plotter);
