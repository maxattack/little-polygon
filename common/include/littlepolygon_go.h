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
#include "littlepolygon_utils.h"

// Generic GameObject (GO) system for entity-component style assets.  The basic idea
// is to model content as a database of GOs, each of which is assembled (by data) out of
// a finite set of built-in (or scripted) components.  This contrasts with a typical
// object-oriented architecture which makes different trade-ffs by utilizing inheritance.

// TL;DR - very simple, portable metadata.

struct GoContext;
typedef uint32_t GO;  // opaque game-object handle
typedef uint32_t CID; // opaque component-type handle

// Initialize a new GO context.  These are in principle serializable to ease the 
// implementation of "in-game editing"
GoContext *createGoContext();
void destroy(GoContext *context);

// Create a new GO.  This really just allocated an identifier and hashes the
// name for lookup purposes. While in principle GOs are just database "primary keys,"
// in practice I find it useful to associate a position with each GO for use in 
// level-editting.
GO createGameObject(GoContext *context, const char* name);

// Destroying a GO also nukes all the components that are logically attached to it.
void destroy(GoContext *context, GO go);

// Enumerating GOs (for editting and debug purposes, mainly)
int numGameObjects(GoContext *context);
int listGameObjects(GoContext *context, int capacity, GO *resultBuf);

// Lookup a game object by name.  In the case of duplicate-names, simply returns the
// first one it finds.
GO find(GoContext *context, const char *name);

// GOs support enabling and disabling, routed to it's components, for the purpose of
// implementing two common game idioms: GO pools and state-based content.  There's also
// a generic message-sending function (a last resort if there isn't a better way to 
// handle data-flow).
void enable(GoContext *context, GO go);
void disable(GoContext *context, GO go);
void sendMessage(GoContext *context, GO go, uint32_t messageId, void *params);

// GO parameters
const char* goName(GoContext *context, GO go);
int goComponentCount(GoContext *context, GO go);
vec2 goPosition(GoContext *context, GO go);
void setPosition(GoContext *context, GO go);
// TODO: GO parent-child relationships?

// Provide the plumping for components.  In principle, components are stored in 
// separate systems that update them in batches, rather than iterating through
// all the game objects and updating them logically with lots of inefficient
// interleaved context-switches.  This GoComponent record simply identifies the 
// type of the handle to the GoContext, and provides a simple interface for 
// separating read-only data (which allows property deduplication across instances)
// and dynamic "user data."  This separation also helps for "reloading" content.

// Concrete Example 1: A "Body" component backed by Box2D.  Your asset-exporter
// can save a single readonly b2BodyDef record and the userData pointer can
// identify the actual b2Body instance.  Bodies are updated in b2World.Step(), not
// through the Go system

// Concrete Example 2: A "Scripted" component backed by a readonly lua metatable and
// and instance lua table.  All scripts in the vm are all updated in one "dispatchEvents"
// rather than individual calls, but discoverable throught the Go metadata.

struct GoComponent {
	CID cid;          // logical type
	GO go;            // logical go

	const void *data; // initialization data (copy-on-write semantics)
	void *userData;   // active data associated with the component
};

// While components all have different specialized concrete behaviour, the share a
// basic interface for responding to GO messages:
//  - INIT: initialize the component (due to order of init, not a good idea to depend
//          on sibling components here).  Components leave init logically "disabled"
//  - ENABLE: the gameObject is enabled.  All siblings init'd at this point
//  - DISABLE: the gameObject has been disabled (but not destroyed).  May be interleaved
//             with called to enable
//  - DESTROY: actually tear-down the component and release resources.  The Go will be 
//             dealloced after all components are destroyed.
// All other nonnegative-messages are interpretted as "custom" messages.

enum GoMessage {
	GO_MESSAGE_INIT    = -1,
	GO_MESSAGE_ENABLE  = -2,
	GO_MESSAGE_DISABLE = -3,
	GO_MESSAGE_DESTROY = -4
};

typedef int (*GoMessageHandler)(GoComponent *component, uint32_t message, void *params);

// Main component interface
int registerComponent(GoContext *context, CID cid, GoMessageHandler handler);
GoComponent *addComponent(GoContext *context, GO go, CID cid, const void *data);
GoComponent *getComponent(GoContext *context, GO go, CID cid);
GoComponent *removeComponent(GoContext *context, GO go, CID cid);

// Listing components
struct GoComponentIterator {
	GoComponent *current;

	GoComponentIterator(GoContext *context, GO go);
	inline bool finished() const { return current == 0; }
	void next();
};
