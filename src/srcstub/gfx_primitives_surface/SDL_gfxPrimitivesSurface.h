/* 

SDL_gfxPrimitives.h: graphics primitives for SDL

Copyright (C) 2001-2012  Andreas Schiffler

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.

Andreas Schiffler -- aschiffler at ferzkopp dot net

*/

/* 

This is a modified version of SDL_gfxPrimitives from SDL_GFX for SDL 1.XX to make it work
with SDL2, its basically just a copy of the original sources with some things stipped out
modified by joyrider3774 aka Willems Davy

*/

#ifndef _SDL_gfxPrimitivesSurface_h
#define _SDL_gfxPrimitivesSurface_h

#include <math.h>
#ifndef M_PI
#define M_PI	3.1415926535897932384626433832795
#endif

#include "SDL.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif



	/* ---- Function Prototypes */

#ifdef _MSC_VER
#  if defined(DLL_EXPORT) && !defined(LIBSDL_GFX_DLL_IMPORT)
#    define SDL_GFXPRIMITIVES_SCOPE __declspec(dllexport)
#  else
#    ifdef LIBSDL_GFX_DLL_IMPORT
#      define SDL_GFXPRIMITIVES_SCOPE __declspec(dllimport)
#    endif
#  endif
#endif
#ifndef SDL_GFXPRIMITIVES_SCOPE
#  define SDL_GFXPRIMITIVES_SCOPE extern
#endif

	/* Note: all ___Color routines expect the color to be in format 0xRRGGBBAA */

	/* Pixel */

	SDL_GFXPRIMITIVES_SCOPE int pixelColorSurface(SDL_Surface * dst, Sint16 x, Sint16 y, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int pixelRGBASurface(SDL_Surface * dst, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Horizontal line */

	SDL_GFXPRIMITIVES_SCOPE int hlineColorSurface(SDL_Surface * dst, Sint16 x1, Sint16 x2, Sint16 y, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int hlineRGBASurface(SDL_Surface * dst, Sint16 x1, Sint16 x2, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Vertical line */

	SDL_GFXPRIMITIVES_SCOPE int vlineColorSurface(SDL_Surface * dst, Sint16 x, Sint16 y1, Sint16 y2, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int vlineRGBASurace(SDL_Surface * dst, Sint16 x, Sint16 y1, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Rectangle */

	SDL_GFXPRIMITIVES_SCOPE int rectangleColorSurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int rectangleRGBASurface(SDL_Surface * dst, Sint16 x1, Sint16 y1,
		Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Rounded-Corner Rectangle */

	SDL_GFXPRIMITIVES_SCOPE int roundedRectangleColorSurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 rad, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int roundedRectangleRGBASurface(SDL_Surface * dst, Sint16 x1, Sint16 y1,
		Sint16 x2, Sint16 y2, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Filled rectangle (Box) */

	SDL_GFXPRIMITIVES_SCOPE int boxColorSurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int boxRGBASurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2,
		Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Rounded-Corner Filled rectangle (Box) */

	SDL_GFXPRIMITIVES_SCOPE int roundedBoxColorSurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 rad, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int roundedBoxRGBASurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2,
		Sint16 y2, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Line */

	SDL_GFXPRIMITIVES_SCOPE int lineColorSurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int lineRGBASurface(SDL_Surface * dst, Sint16 x1, Sint16 y1,
		Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* AA Line */

	SDL_GFXPRIMITIVES_SCOPE int aalineColorSurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int aalineRGBASurface(SDL_Surface * dst, Sint16 x1, Sint16 y1,
		Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Thick Line */
	SDL_GFXPRIMITIVES_SCOPE int thickLineColorSurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, 
		Uint8 width, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int thickLineRGBASurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, 
		Uint8 width, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Circle */

	SDL_GFXPRIMITIVES_SCOPE int circleColorSurface(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int circleRGBASurface(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Arc */

	SDL_GFXPRIMITIVES_SCOPE int arcColorSurface(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int arcRGBASurface(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, 
		Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* AA Circle */

	SDL_GFXPRIMITIVES_SCOPE int aacircleColorSurface(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int aacircleRGBASurface(SDL_Surface * dst, Sint16 x, Sint16 y,
		Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Filled Circle */

	SDL_GFXPRIMITIVES_SCOPE int filledCircleColorSurface(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 r, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int filledCircleRGBASurface(SDL_Surface * dst, Sint16 x, Sint16 y,
		Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Ellipse */

	SDL_GFXPRIMITIVES_SCOPE int ellipseColorSurace(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int ellipseRGBASurface(SDL_Surface * dst, Sint16 x, Sint16 y,
		Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* AA Ellipse */

	SDL_GFXPRIMITIVES_SCOPE int aaellipseColorSurface(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int aaellipseRGBASurface(SDL_Surface * dst, Sint16 x, Sint16 y,
		Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Filled Ellipse */

	SDL_GFXPRIMITIVES_SCOPE int filledEllipseColorSurface(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int filledEllipseRGBASurface(SDL_Surface * dst, Sint16 x, Sint16 y,
		Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Pie */

	SDL_GFXPRIMITIVES_SCOPE int pieColorSurface(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad,
		Sint16 start, Sint16 end, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int pieRGBASurface(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad,
		Sint16 start, Sint16 end, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Filled Pie */

	SDL_GFXPRIMITIVES_SCOPE int filledPieColorSurface(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad,
		Sint16 start, Sint16 end, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int filledPieRGBASurface(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad,
		Sint16 start, Sint16 end, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Trigon */

	SDL_GFXPRIMITIVES_SCOPE int trigonColorSurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int trigonRGBASurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
		Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* AA-Trigon */

	SDL_GFXPRIMITIVES_SCOPE int aatrigonColorSurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int aatrigonRGBASurface(SDL_Surface * dst,  Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
		Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Filled Trigon */

	SDL_GFXPRIMITIVES_SCOPE int filledTrigonColorSurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int filledTrigonRGBASurface(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
		Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Polygon */

	SDL_GFXPRIMITIVES_SCOPE int polygonColorSurface(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int polygonRGBASurface(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy,
		int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* AA-Polygon */

	SDL_GFXPRIMITIVES_SCOPE int aapolygonColorSurface(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int aapolygonRGBASurface(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy,
		int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	/* Filled Polygon */

	SDL_GFXPRIMITIVES_SCOPE int filledPolygonColorSurface(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int filledPolygonRGBASurface(SDL_Surface * dst, const Sint16 * vx,
		const Sint16 * vy, int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	SDL_GFXPRIMITIVES_SCOPE int texturedPolygonSurface(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, SDL_Surface * texture,int texture_dx,int texture_dy);

	/* (Note: These MT versions are required for multi-threaded operation.) */

	SDL_GFXPRIMITIVES_SCOPE int filledPolygonColorMTSurface(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, Uint32 color, int **polyInts, int *polyAllocated);
	SDL_GFXPRIMITIVES_SCOPE int filledPolygonRGBAMTSurface(SDL_Surface * dst, const Sint16 * vx,
		const Sint16 * vy, int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a,
		int **polyInts, int *polyAllocated);
	SDL_GFXPRIMITIVES_SCOPE int texturedPolygonMTSurface(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, SDL_Surface * texture,int texture_dx,int texture_dy, int **polyInts, int *polyAllocated);

	/* Bezier */

	SDL_GFXPRIMITIVES_SCOPE int bezierColorSurface(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, int s, Uint32 color);
	SDL_GFXPRIMITIVES_SCOPE int bezierRGBASurface(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy,
		int n, int s, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	
	/* Draw / fill an ellipse arc (Playdate-compatible angle convention) */
	SDL_GFXPRIMITIVES_SCOPE int drawEllipseRGBASurface(SDL_Surface * dst, Sint16 rectX, Sint16 rectY, Sint16 width, Sint16 height, Sint16 lineWidth, float startAngle, float endAngle, Uint8 r, Uint8 g, Uint8 b, Uint8 a, int filled);

	/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif				/* _SDL_gfxPrimitives_h */
