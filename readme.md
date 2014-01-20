```
 __         __     ______   ______   __         ______    
/\ \       /\ \   /\__  _\ /\__  _\ /\ \       /\  ___\   
\ \ \____  \ \ \  \/_/\ \/ \/_/\ \/ \ \ \____  \ \  __\   
 \ \_____\  \ \_\    \ \_\    \ \_\  \ \_____\  \ \_____\ 
  \/_____/   \/_/     \/_/     \/_/   \/_____/   \/_____/ 
                                                          
 ______   ______     __         __  __     ______     ______     __   __    
/\  == \ /\  __ \   /\ \       /\ \_\ \   /\  ___\   /\  __ \   /\ "-.\ \   
\ \  _-/ \ \ \/\ \  \ \ \____  \ \____ \  \ \ \__ \  \ \ \/\ \  \ \ \-.  \  
 \ \_\    \ \_____\  \ \_____\  \/\_____\  \ \_____\  \ \_____\  \ \_\\"\_\ 
  \/_/     \/_____/   \/_____/   \/_____/   \/_____/   \/_____/   \/_/ \/_/ 
                                                                            
```

DESCRIPTION
-----------

A collection of useful little scripts for making native mobile and desktop games.  Features
are added as I need them for my personal work, but I'm always game for contributions.

Little Polygon is emphatically __not__ "middleware" or a "framework."  Modules do not maintain
any global state or manage control flow, and individual modules are decoupled from each other
and usable independently.  My goal is to add value, not express a particular architectural 
point of view.

Build Dependencies (python2x + pip)
* lxml - For parsing TMX Files
* Pillow - For image processing and compositting sprite atlasses
* pyyaml - For asset-script parsing
* psd_tools - For Photoshop import Support

Runtime Dependencies (native)
* sdl2 - platform abstraction
* sdl2_mixer - music and sound effects
* vectorial - SIMD math
* glew - desktop graphics feature discovery
* zlib - asset decompression

Modules
* littlepolygon_assets - a simple system for packaging and loading compressed game assets
* littlepolygon_bitset - fast bitvector with intrinsic optimizations
* littlepolygon_fk - portable forward-kinematic display tree
* littlepolygon_graphics - efficient spline and vector renderers
* littlepolygon_math - common geometry stuff
* littlepolygon_physics - bindings to between box2f and littlepolygon_fk
* littlepolygon_pools - a few common object-pool templates
* littlepolygon_sprites - batching sprite renderer

Misc Modules (very WIP)
* littlepolygon_go - an entity-component metadata system
* littlepolgyon_gocore - go bindings for littlepolygon modules
* littlepolygon_goedit - generic in-game editor based on go

CODING STANDARDS
----------------

Python scripts are organized into "tasks" - scripts that encapsulate a single method which
can be invoked directly (checking for `__main__`), or imported and composited with other
tasks in higher-level scripts.

Native code is written in C++ with a "C with Objects" mentality.  Each object is expressed
as a plain-old-data structure with just a few convenience getter methods.  Initialization and
non-idempotent methods are top-level functions which take the structure as a "context" argument.
I try to keep method names short by using argument overloading as an implicit namespace. This 
is a lot more flexible for controlling memory allocation, initialization order,  multithreading, 
binding to scripting environments, keeping private methods out of headers, and  allowing the use 
of completely opaque pointers that are just forward-declared.

WISHLIST
--------

* automatic lua binding (using python scripts, not C++ template madness)
* heirarchical go templates in asset export
* savedata (sqlite?)
* asynchronous sprite-batch (for multithreading)
* more import asset types (SVG, Flash)
* non-native export targets (e.g. unity, webgl)

LICENSE
-------

Little Polygon SDK
Copyright (C) 2014 Max Kaufmann (max@littlepolygon.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

(commercial license and support available upon request)

