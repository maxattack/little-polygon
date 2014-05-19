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
* zlib - asset decompression

Modules

* littlepolygon/math - common geometry stuff
* littlepolygon/bitset - efficient bitvector with intrinsic optimizations
* littlepolygon/collections - nondynamic collection templates
* littlepolygon/pools - a few common object-pool templates
* littlepolygon/events - functors, delegates, and timers
* littlepolygon/assets - a simple system for packaging and loading compressed game assets
* littlepolygon/graphics - opengl utilities
* littlepolygon/sprites - batching sprite renderer
* littlepolygon/splines - efficient cubic spline drawing
* littlepolygon/trail - efficient trail renderer
* littlepolygon/context - convenient global context setup and teardown

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

