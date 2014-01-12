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

#ifndef LITTLEPOLYGON_GO
#define LITTLEPOLYGON_GO

#include "littlepolygon_base.h"

// Generic GameObject (GO) system for entity-component style assets.  The basic idea
// is to model content as a database of GOs, each of which is assembled (by data) out of
// a finite set of built-in (or scripted) components.  This contrasts with a typical
// object-oriented architecture which makes different trade-ffs by utilizing inheritance.

// TL;DR - very simple, portable metadata.

struct GoContext;
typedef uint32_t GO;  // opaque game-object handle
typedef uint32_t CID; // opaque component-type handle

// Provide the plumping for components.  In principle, components are stored in 
// separate systems that update them in batches, rather than iterating through
// all the game objects and updating them logically with lots of inefficient
// interleaved context-switches.  

// Concrete Example 1: A "Body" component backed by Box2D.  Your asset-exporter
// can save a single readonly BodyDef record and the userData pointer can
// identify the actual Body instance.  Bodies are updated in b2World.Step(), not
// through the Go system

// Concrete Example 2: A "Scripted" component backed by a readonly lua metatable and
// and instance lua table.  All scripts in the vm are all updated in one "dispatchEvents"
// rather than individual calls, but discoverable throught the Go metadata.

struct GoComponent {
	CID cid;            // logical type
	GO go;              // logical go
	GoContext *context; // context for this component
	void *userData;     // active data associated with the component
};

// While components all have different specialized concrete behaviour, the share a
// basic interface for responding to GO messages:
//
//  INIT: initialize the component (due to order of init, not a good idea to depend
//        on sibling components here).  Components leave init logically "disabled"
//
//  ENABLE: the gameObject is enabled.  All siblings init'd at this point
//
//  DISABLE: the gameObject has been disabled (but not destroyed).  May be interleaved
//           with called to enable
//
//  DESTROY: actually tear-down the component and release resources.  The Go will be 
//           dealloced after all components are destroyed.
//
// All other nonnegative-messages are interpretted as "custom" messages.

enum GoMessage {
	GO_MESSAGE_INIT    = -1,
	GO_MESSAGE_ENABLE  = -2,
	GO_MESSAGE_DISABLE = -3,
	GO_MESSAGE_DESTROY = -4
};

typedef int (*GoMessageHandler)(
	GoComponent *component,       // the component receiving the message
	int message,             // the message identifier
	const void *args,             // the arguments to the message
	void *user                    // a user-supplied pointer
);

struct GoComponentDef {
	void *context;
	GoMessageHandler handler;
};

// Initialize a new GO context.  These are in principle serializable to ease the 
// implementation of "in-game editing" as well as syncable across networks for 
// "remote contexts" like in MMOs. Capacity is needed because the whole database
// is "block-allocated" to keep everything thread-local friendly.  No internal
// dynamic memory is used.
GoContext *createGoContext(size_t numComponents, GoComponentDef *componentBuf, size_t goCapacity=1024, size_t componentCapacity=1024);
void destroy(GoContext *context);

// Create a new GO.  This really just allocated an identifier and hashes the
// name for lookup purposes. 
// Explicit IDs are useful for deserialization and network syncronization.  Passing
// 0 will just generate a new one.  IDs may conflict, which will cause this method to
// fail.  Passing 1-N to a fresh context should work OK.
GO createGameObject(GoContext *context, const char* name=0, GO explicitId=0);

// Destroying a GO also nukes all the components that are logically attached to it.
void destroy(GoContext *context, GO go);

// Lookup a game object by name.  In the case of duplicate-names, simply returns the
// first one it finds.
GO find(GoContext *context, const char *name);

// GO parameters
bool goEnabled(GoContext *context, GO go);
const char* goName(GoContext *context, GO go);

// Attach a component to the given game object, initialized with the given data.  CID 
// corresponds to the index of the component message handler that the context was
// intialized with.
GoComponent *addComponent(GoContext *context, GO go, CID cid, const void *data=0);

// lookup a component by type.  Returns the first match.  For an exhaustive search
// (in the event of multiple components of the same type), use the component iterator
GoComponent *getComponent(GoContext *context, GO go, CID cid);

// Remove a component, destroying it completely.
void removeComponent(GoContext *context, GoComponent *component);

// ?? I feel kinda weird handling raw pointers like this... is there a compelling
//    concrete reason to only dispense opaque IDs to components?  Serialization?

// GOs support enabling and disabling, routed to it's components, for the purpose of
// implementing two common game idioms: GO pools and state-based content.  
void enable(GoContext *context, GO go);
void disable(GoContext *context, GO go);

// A generic message-sending function (a last resort if there isn't a better way to 
// handle data-flow, e.g. IoC dynamic event subscription + dispatch).
void sendMessage(GoContext *context, GO go, uint32_t messageId, void *params=0);

//------------------------------------------------------------------------------
// ITERATORS
//------------------------------------------------------------------------------

// Example useage:
//   for(GoIterator i(context); !i.finished(); i.next()) {
//     ...
//   }

struct GoIterator {
	void *internal;
	GO current;

	GoIterator(GoContext *context);
	inline bool finished() const { return internal == 0; }
	void next();

};

struct GoComponentIterator {
	GoComponent *current;

	GoComponentIterator(GoContext *context, GO go);
	inline bool finished() const { return current == 0; }
	void next();

};


#endif