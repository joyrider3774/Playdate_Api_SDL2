#include <memory>
#include <string.h>
#include <dirent.h>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <SDL_rect.h>
#include <SDL_blendmode.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_rotate.h>
#include <SDL_stdinc.h>
#include <SDL2_rotozoom.h>
#include <SDL_ttf.h>
#include <sys/stat.h>
#include "gfx_primitives_surface/SDL_gfxPrimitivesSurface.h"
#include "sdl_rotate/SDL_rotate.h"
#include "pd_api/pd_api_gfx.h"
#include "gamestubcallbacks.h"
#include "gamestub.h"
#include "pd_menu.h"
#include "defines.h"
#include "debug.h"


const char loaderror[] = "Failed loading!";
static uint8_t pd_gfx_framebuffer[LCD_ROWSIZE * LCD_ROWS];
static uint8_t pd_gfx_display_framebuffer[LCD_ROWSIZE * LCD_ROWS]; // last completed frame
static bool pd_gfx_framebuffer_valid = false;
static bool pd_gfx_framebuffer_written = false; // set by markUpdatedRows, cleared each frame
static bool pd_gfx_framebuffer_got_frame = false; // set by getFrame() this frame, cleared each frame
static LCDBitmap*   pd_gfx_api_layer = NULL;      // persistent layer for normal API draws over framebuffer
static bool pd_gfx_layer_has_content = false;     // true when layer has something to composite
bool pd_gfx_in_update_callback = false; // set by gamestub around the update callback call
static bool pd_gfx_layer_active = false; // set when getFrame() called during update, not reset by clear()
static bool pd_gfx_rows_pushed = false; // true if markUpdatedRows was called this frame

struct LCDBitmap;

struct LCDBitmap {
    SDL_Surface* Tex;
	LCDBitmap* Mask;
	SDL_Surface* MaskedTex;
    /** When not `NULL`, indicates that this bitmap is a mask owned by the parent bitmap. Every draw/read operation will be made on the parent. */
    LCDBitmap* Parent;
	uint8_t *BitmapDataMask;
	uint8_t *BitmapDataData;
    uint32_t BitmapDataMaskChecksum;
    uint32_t BitmapDataDataChecksum;
	bool MaskDirty;
	bool BitmapDirty;
    bool TableBitmap;
    bool Opaque;
    int w;
    int h;
};

struct LCDBitmapTable {
    int count;
    int w;
    int h;
	int across;
    LCDBitmap** bitmaps;
};

struct LCDFontGlyphData {
    SDL_Surface* surface;  // white pixels on clear background
    int advance;           // advance width in pixels
};

struct LCDFont {
    TTF_Font* font;        // non-null for TTF fonts, null for native .fnt fonts
    bool isFntFont;        // true = native .fnt bitmap font
    int cellW;             // cell width in pixels
    int cellH;             // cell height in pixels (= font height)
    int fntTracking;       // default tracking from .fnt file
    std::unordered_map<uint32_t, LCDFontGlyphData> glyphs;  // codepoint -> glyph
    std::unordered_map<uint64_t, int> kerning;               // (prev<<32|cur) -> advance adjustment
};

struct LCDFontData {

};

struct LCDFontPage {

};

struct LCDFontGlyph {

};

struct LCDVideoPlayer {

};

class GfxContext;
class GfxContext {
    public:
        LCDBitmapDrawMode BitmapDrawMode;
        LCDSolidColor BackgroundColor;
        LCDBitmap* stencil;
        int drawoffsetx;
        int drawoffsety;
        SDL_Rect cliprect;
        LCDLineCapStyle linecapstyle;
        LCDFont* font;
        int tracking;
        int leading;
        LCDBitmap* DrawTarget;
        void Assign(GfxContext* Context);
};

void GfxContext::Assign(GfxContext* Context)
{
    this->BitmapDrawMode = Context->BitmapDrawMode;
    this->BackgroundColor = Context->BackgroundColor;
    this->stencil = Context->stencil;
    this->drawoffsetx = Context->drawoffsetx;
    this->drawoffsety = Context->drawoffsety;
    this->cliprect = Context->cliprect;
    this->linecapstyle = Context->linecapstyle;
    this->font = Context->font;
    this->tracking = Context->tracking;
    this->leading = Context->leading;
    this->DrawTarget = Context->DrawTarget;
}


class FontListEntry {
    public:
        char* path;
        LCDFont *font;
};

std::vector<FontListEntry*> fontlist;
std::vector<GfxContext*> gfxstack;

GfxContext* _pd_api_gfx_CurrentGfxContext;
LCDBitmap* _pd_api_gfx_Playdate_Screen;
LCDFont* _pd_api_gfx_Default_Font = NULL;
LCDFont* _pd_api_gfx_FPS_Font = NULL;

// Determine pixel at x, y is black or white.
#define pd_api_gfx_samplepixel(data, x, y, rowbytes) (((data[(y)*rowbytes+(x)/8] & (1 << (uint8_t)(7 - ((x) % 8)))) != 0) ? kColorWhite : kColorBlack)

// Set the pixel at x, y to black.
#define pd_api_gfx_setpixel(data, x, y, rowbytes) (data[(y)*rowbytes+(x)/8] &= ~(1 << (uint8_t)(7 - ((x) % 8))))

// Set the pixel at x, y to white.
#define pd_api_gfx_clearpixel(data, x, y, rowbytes) (data[(y)*rowbytes+(x)/8] |= (1 << (uint8_t)(7 - ((x) % 8))))

// Set the pixel at x, y to the specified color.
#define pd_api_gfx_drawpixel(data, x, y, rowbytes, color) (((color) == kColorBlack) ? pd_api_gfx_setpixel((data), (x), (y), (rowbytes)) : pd_api_gfx_clearpixel((data), (x), (y), (rowbytes)))

void pd_api_gfx_freeBitmap(LCDBitmap* Bitmap);

inline int make_even(int n)
{
    return n - n % 2;
}

static int saveddrawoffsetx = 0;
static int saveddrawoffsety = 0;

void pd_api_gfx_pushDrawOffset()
{
	saveddrawoffsetx = _pd_api_gfx_CurrentGfxContext->drawoffsetx;
	saveddrawoffsety = _pd_api_gfx_CurrentGfxContext->drawoffsety; 
}

void pd_api_gfx_popDrawOffset()
{
	_pd_api_gfx_CurrentGfxContext->drawoffsetx = saveddrawoffsetx;
	_pd_api_gfx_CurrentGfxContext->drawoffsety = saveddrawoffsety;
}


bool pd_api_gfx_MakeSurfaceBlackAndWhite(SDL_Surface *Img)
{
	bool opaque = true;
	if (Img)
	{
		bool unlocked = true;
		if (SDL_MUSTLOCK(Img))
			unlocked = SDL_LockSurface(Img);
		if(unlocked)
		{
			Uint32 white = SDL_MapRGBA(Img->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
			Uint32 black = SDL_MapRGBA(Img->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
			Uint32 transparant = SDL_MapRGBA(Img->format, 0, 0, 0, 0);
			Uint8 r,g,b,a;
			float lum;
			//how does pdc.exe calculate this when converting images to pdi ??
			for (int y = 0; y < Img->h; y++)
			{
				for(int x = 0; x< Img->w; x++)
				{
					Uint32 *p = (Uint32*)((Uint8 *)Img->pixels + ((y) * Img->pitch) + (x * Img->format->BytesPerPixel));
					if(*p == transparant)
					{
						opaque = false;
						continue;
					}
					// Convert the pixel value to grayscale i.e. intensity
					SDL_GetRGBA(*p, Img->format, &r, &g, &b, &a);
					lum = 0.212671f  *r + 0.715160f  * g + 0.072169f  *b;
					//everything lower than half transparant make fully transparant
					if(a < COLORCONVERSIONALPHATHRESHOLD)
					{
						opaque = false;
						*p = transparant;
					}
					//otherwise check the luminance value and put to fully black or fully white
					else
					{
						if(lum < COLORCONVERSIONWHITETHRESHOLD)
							*p = black;
						else
							*p = white;
					}
				}
			}
			if (SDL_MUSTLOCK(Img))
				SDL_UnlockSurface(Img);
		}
	}
	return opaque;
}

bool pd_api_gfx_MakeSurfaceOpaque(SDL_Surface *Img)
{
	bool opaque = true;
	if (Img)
	{
		bool unlocked = true;
		if (SDL_MUSTLOCK(Img))
			unlocked = SDL_LockSurface(Img);
		if(unlocked)
		{
			Uint32 transparant = SDL_MapRGBA(Img->format, 0, 0, 0, 0);
			Uint8 r,g,b,a;
			for (int y = 0; y < Img->h; y++)
			{
				for(int x = 0; x< Img->w; x++)
				{
					Uint32 *p = (Uint32*)((Uint8 *)Img->pixels + ((y) * Img->pitch) + (x * Img->format->BytesPerPixel));
					if(*p == transparant)
					{
						opaque = false;
						continue;
					}
					SDL_GetRGBA(*p, Img->format, &r, &g, &b, &a);
					if(a > COLORCONVERSIONALPHATHRESHOLD)
						*p = SDL_MapRGBA(Img->format, r, g, b, 255);
					else
						*p = SDL_MapRGBA(Img->format, r, g, b, 0);
				}
			}
			if (SDL_MUSTLOCK(Img))
				SDL_UnlockSurface(Img);
		}
	}
	return opaque;
}


LCDBitmap* pd_api_gfx_PatternToBitmap(LCDPattern Pattern)
{
	LCDBitmap* result = Api->graphics->newBitmap(8, 8, kColorClear);
	LCDBitmap* mask = Api->graphics->newBitmap(8, 8, kColorBlack);
	bool surfaceLocked = true;
	bool maskLocked = true;
	if(SDL_MUSTLOCK(result->Tex))
		surfaceLocked = SDL_LockSurface(result->Tex);

	if(SDL_MUSTLOCK(mask->Tex))
		maskLocked = SDL_LockSurface(mask->Tex);

	
	if(surfaceLocked && maskLocked)
	{
		Uint32 black = SDL_MapRGBA(result->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
		Uint32 white = SDL_MapRGBA(mask->Tex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);

		for (int y = 0; y < 8; y++)
		{
			for(int x = 0; x< 8; x++)
			{
				Uint32 *p = (Uint32*)((Uint8 *)result->Tex->pixels + ((y) * result->Tex->pitch) + (x * result->Tex->format->BytesPerPixel));
				if (Pattern[y] & (1 << (7-x)))
					*p = white;
				else
					*p = black;

				
				Uint32 *p2 = (Uint32*)((Uint8 *)mask->Tex->pixels + ((y)  * mask->Tex->pitch) + (x * mask->Tex->format->BytesPerPixel));
				if (Pattern[y+8] & (1 << (7-x)))
					*p2 = white;
				else
					*p2 = black;				
			}
		}
	}

	if(SDL_MUSTLOCK(result->Tex))
		SDL_UnlockSurface(result->Tex);

	if(SDL_MUSTLOCK(mask->Tex))
		SDL_UnlockSurface(mask->Tex);

	Api->graphics->setBitmapMask(result, mask);
	//setBitmapMask makes a copy currently of the mask in my api
	//so need to free the current mask bitmap
	Api->graphics->freeBitmap(mask);
	return result;
}

void pd_api_gfx_recreatemaskedimage(LCDBitmap* bitmap)
{
	if (!bitmap || !ENABLEBITMAPMASKS)
		return;

	if(bitmap->Mask && (bitmap->MaskDirty || bitmap->BitmapDirty))
	{
		bitmap->MaskDirty = false;
		bitmap->BitmapDirty = false;
		if(bitmap->MaskedTex)
			SDL_FreeSurface(bitmap->MaskedTex);
		bitmap->MaskedTex = SDL_CreateRGBSurfaceWithFormat(0, bitmap->Tex->w, bitmap->Tex->h, bitmap->Tex->format->BitsPerPixel, bitmap->Tex->format->format);
		//must use blendmode_blend otherwise alpha does not work
		//SDL_SetSurfaceBlendMode(bitmap->MaskedTex, SDL_BLENDMODE_NONE);
		SDL_Surface* tmpMask = bitmap->Mask->Tex;
		SDL_Surface* tmpTex = bitmap->Tex;

		bool MaskedTextureUnlocked = true;
		bool maskUnlocked = true;
		bool texUnlocked = true;

		if (SDL_MUSTLOCK(bitmap->MaskedTex))
			MaskedTextureUnlocked = SDL_LockSurface(bitmap->MaskedTex) == 0;
		if (SDL_MUSTLOCK(tmpMask))
			maskUnlocked = SDL_LockSurface(tmpMask) == 0;
		if (SDL_MUSTLOCK(tmpTex))
			texUnlocked = SDL_LockSurface(tmpTex) == 0;
		
		if (MaskedTextureUnlocked && maskUnlocked && texUnlocked) 
		{
			Uint32 whitethreshold = SDL_MapRGBA(bitmap->MaskedTex->format, pd_api_gfx_color_whitetreshold.r, pd_api_gfx_color_whitetreshold.g, pd_api_gfx_color_whitetreshold.b, pd_api_gfx_color_whitetreshold.a);
			int width = std::min(bitmap->MaskedTex->w, tmpMask->w);
			int height = std::min(bitmap->MaskedTex->h, tmpMask->h);
			for (int yy = 0; (yy < height); yy++)
			{
				for(int xx = 0; (xx < width); xx++)
				{
					Uint32 *p = (Uint32*)((Uint8 *)bitmap->MaskedTex->pixels + (yy * bitmap->MaskedTex->pitch) + (xx * bitmap->MaskedTex->format->BytesPerPixel));
					Uint32 *p2 = (Uint32*)((Uint8 *)tmpMask->pixels + (yy  * tmpMask->pitch) + (xx * tmpMask->format->BytesPerPixel));
					Uint32 *p3 = (Uint32*)((Uint8 *)tmpTex->pixels + (yy  * tmpTex->pitch) + (xx * tmpTex->format->BytesPerPixel));
					Uint32 p2val = *p2;
					if(p2val > whitethreshold)
					{
						*p = *p3;
					}
				}
			}

			if (SDL_MUSTLOCK(bitmap->MaskedTex))
				SDL_UnlockSurface(bitmap->MaskedTex);
			if (SDL_MUSTLOCK(tmpMask))
				SDL_UnlockSurface(tmpMask);
			if (SDL_MUSTLOCK(tmpTex))
				SDL_UnlockSurface(tmpTex);
		
        }
	}
}


SDL_Surface* _pd_api_gfx_GetSDLTextureFromBitmap(LCDBitmap* bitmap)
{
    return bitmap->Tex;
}

void _pd_api_gfx_drawFPS(int x, int y)
{
    Api->graphics->pushContext(NULL);
    Api->graphics->setDrawMode(kDrawModeCopy);
    Api->graphics->clearClipRect();
    Api->graphics->setDrawOffset(0,0);
    char *Text;
    Api->system->formatString(&Text,"%2.0f", _pd_api_display_LastFPS);
    Api->graphics->setFont(_pd_api_gfx_FPS_Font);
    int w = Api->graphics->getTextWidth(_pd_api_gfx_FPS_Font, Text, strlen(Text), kASCIIEncoding, 0);
    int h = Api->graphics->getFontHeight(_pd_api_gfx_FPS_Font);
    Api->graphics->fillRect(x, y, w, h, kColorWhite);
    Api->graphics->drawText(Text, strlen(Text), kASCIIEncoding, x, y);
    Api->system->realloc(Text, 0);
    Api->graphics->popContext();
}

LCDBitmapDrawMode _pd_api_gfx_getCurrentDrawMode()
{
	return _pd_api_gfx_CurrentGfxContext->BitmapDrawMode;
}

LCDColor getBackgroundDrawColor()
{
    return _pd_api_gfx_CurrentGfxContext->BackgroundColor;
}

static LCDBitmap* _pd_api_gfx_getDrawTarget()
{
    LCDBitmap *drawTarget = _pd_api_gfx_CurrentGfxContext->DrawTarget;
    if (drawTarget->Parent)
    {
        drawTarget->Parent->MaskDirty = true;
        drawTarget = drawTarget->Parent->Mask;
    }
    return drawTarget;
}

// Drawing Functions
void pd_api_gfx_clear(LCDColor color)
{
    if (color == kColorXOR)
	{
		return;
	}

    LCDBitmap *drawTarget = _pd_api_gfx_getDrawTarget();

	//clear clears entire screen so need to remove cliprect temporary
	SDL_Rect rect;
	SDL_GetClipRect(drawTarget->Tex, &rect);
	SDL_SetClipRect(drawTarget->Tex, NULL);
	SDL_Color maskColor = pd_api_gfx_color_white;
    switch (color)
    {
        case kColorBlack:
            SDL_FillRect(drawTarget->Tex, NULL, SDL_MapRGBA(drawTarget->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a));
            break;
        case kColorWhite:
            SDL_FillRect(drawTarget->Tex, NULL, SDL_MapRGBA(drawTarget->Tex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a));
            break;
        case kColorClear:
            SDL_FillRect(drawTarget->Tex, NULL, SDL_MapRGBA(drawTarget->Tex->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a));
            maskColor = pd_api_gfx_color_black;
			break;
        default:
            break;
    }
	if (drawTarget->Mask)
         SDL_FillRect(drawTarget->Mask->Tex, &rect, SDL_MapRGBA(drawTarget->Mask->Tex->format,  maskColor.r, maskColor.g, maskColor.b, maskColor.a));
	SDL_SetClipRect(drawTarget->Tex, &rect);
	drawTarget->BitmapDirty = true;

    // If clearing the main screen and the 1-bit framebuffer is in use,
    // keep pd_gfx_framebuffer in sync so games that mix clear() with
    // direct framebuffer writes (via getFrame) see a consistent state.
    if (pd_gfx_framebuffer_valid && drawTarget == _pd_api_gfx_Playdate_Screen)
    {
        uint8_t fill = 0x00; // black = all bits 0
        if (color == kColorWhite) fill = 0xFF; // white = all bits 1
        memset(pd_gfx_framebuffer, fill, sizeof(pd_gfx_framebuffer));
        // Also clear the API layer — the screen is being reset
        if (pd_gfx_api_layer && pd_gfx_api_layer->Tex)
            SDL_FillRect(pd_gfx_api_layer->Tex, NULL,
                SDL_MapRGBA(pd_gfx_api_layer->Tex->format,
                    pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g,
                    pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a));
        // Reset got_frame only if getFrame() was NOT called in the current update
        // frame (e.g. it was called at kEventInit and is now stale). If getFrame()
        // was called this update, this clear() is a legitimate mid-frame clear and
        // we must keep got_frame=true so the layer keeps working.
        // Always reset got_frame on clear — game is starting a new render pass.
        // If it needs framebuffer push it will call markUpdatedRows.
        pd_gfx_framebuffer_got_frame = false;
    }
}

void pd_api_gfx_setBackgroundColor(LCDSolidColor color)
{
    _pd_api_gfx_CurrentGfxContext->BackgroundColor = color;
}

void pd_api_gfx_setStencil(LCDBitmap* stencil) // deprecated in favor of setStencilImage, which adds a "tile" flag
{
    _pd_api_gfx_CurrentGfxContext->stencil = stencil;
}

LCDBitmapDrawMode pd_api_gfx_setDrawMode(LCDBitmapDrawMode mode)
{
	LCDBitmapDrawMode tmp = _pd_api_gfx_CurrentGfxContext->BitmapDrawMode;
    _pd_api_gfx_CurrentGfxContext->BitmapDrawMode = mode;
	return tmp;
}

void pd_api_gfx_setDrawOffset(int dx, int dy)
{
    _pd_api_gfx_CurrentGfxContext->drawoffsetx = dx;
    _pd_api_gfx_CurrentGfxContext->drawoffsety = dy;
}

void _pd_api_gfx_getDrawOffset(int* dx, int* dy)
{
    if (dx) *dx = _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    if (dy) *dy = _pd_api_gfx_CurrentGfxContext->drawoffsety;
}

void pd_api_gfx_setClipRect(int x, int y, int width, int height)
{
	SDL_Rect cliprect;
    cliprect.x = x + _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    cliprect.y = y + _pd_api_gfx_CurrentGfxContext->drawoffsety;
    cliprect.w = width;
    cliprect.h = height;
    LCDBitmap *drawTarget = _pd_api_gfx_getDrawTarget();
    SDL_SetClipRect(drawTarget->Tex, &cliprect);
    if (drawTarget->Mask)
        SDL_SetClipRect(drawTarget->Mask->Tex, &cliprect);
}

void pd_api_gfx_clearClipRect(void)
{
    LCDBitmap *drawTarget = _pd_api_gfx_getDrawTarget();
    SDL_SetClipRect(drawTarget->Tex, NULL);
    if (drawTarget->Mask)
        SDL_SetClipRect(drawTarget->Mask->Tex, NULL);
    // Also clear the screen cliprect
    SDL_SetClipRect(_pd_api_gfx_Playdate_Screen->Tex, NULL);
}

void pd_api_gfx_setLineCapStyle(LCDLineCapStyle endCapStyle)
{
    _pd_api_gfx_CurrentGfxContext->linecapstyle = endCapStyle;
}

void pd_api_gfx_setFont(LCDFont* font)
{
    if(!font)
        _pd_api_gfx_CurrentGfxContext->font = _pd_api_gfx_Default_Font;
    else
        _pd_api_gfx_CurrentGfxContext->font = font;
}

void pd_api_gfx_setTextTracking(int tracking)
{
    _pd_api_gfx_CurrentGfxContext->tracking = tracking;
}

void pd_api_gfx_pushContext(LCDBitmap* target)
{	
    GfxContext* Tmp = new GfxContext();
	//bitmap drawmode remains same
	Tmp->BitmapDrawMode = _pd_api_gfx_CurrentGfxContext->BitmapDrawMode;
	//font is not changed it seems
	Tmp->font = _pd_api_gfx_CurrentGfxContext->font;
	//background color seems to be set to white on a push
	Tmp->BackgroundColor = kColorWhite; 
	//draw offset is reset
	Tmp->drawoffsetx = 0;
	Tmp->drawoffsety = 0;
	//need to verify these 3 below like its possible they get reset to some fixed value
	Tmp->linecapstyle = _pd_api_gfx_CurrentGfxContext->linecapstyle;
	Tmp->stencil = _pd_api_gfx_CurrentGfxContext->stencil;
	Tmp->tracking = _pd_api_gfx_CurrentGfxContext->tracking;

	//collisionworld ?
	//displaylist ?
	//invert
	//scale
	//mosaicx
	//mosaicy
	//offsetx
	//offsety 
	//need to assign the draw target
	if(target)
    {
        Tmp->DrawTarget = target;
    }
    else
    {
        Tmp->DrawTarget = _pd_api_gfx_Playdate_Screen;
    }
	//save the cliprect it is the cliprect from the target bitmap
	SDL_Rect clipRect;
	SDL_GetClipRect(Tmp->DrawTarget->Tex, &clipRect);
	Tmp->cliprect = clipRect; 
	gfxstack.push_back(Tmp);
    _pd_api_gfx_CurrentGfxContext = Tmp;
}

void pd_api_gfx_popContext(void)
{
    //we should awlays have 1 item it is added when creating the graphics api subset!
    if(gfxstack.size() > 1)
    {
		//restore the cliprect of the current drawtarget first (we had it store in pushcontext)
		SDL_SetClipRect(_pd_api_gfx_getDrawTarget()->Tex, &_pd_api_gfx_CurrentGfxContext->cliprect);
		delete gfxstack[gfxstack.size()-1];
		gfxstack.pop_back();
        _pd_api_gfx_CurrentGfxContext = gfxstack[gfxstack.size()-1];
        //restore drawtarget
		if(!_pd_api_gfx_CurrentGfxContext->DrawTarget)
		{
			_pd_api_gfx_CurrentGfxContext->DrawTarget = _pd_api_gfx_Playdate_Screen;
		}
    }
}
	
// LCDBitmap

LCDBitmap* pd_api_gfx_Create_LCDBitmap()
{
    LCDBitmap* tmp = (LCDBitmap* ) malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(LCDBitmap));
    return tmp;
}

void pd_api_gfx_clearBitmap(LCDBitmap* bitmap, LCDColor bgcolor)
{
    if (bitmap == NULL)
	{
        return;
	}

    if (bgcolor == kColorXOR)
	{
        return;
	}
	SDL_Rect rect;
	SDL_GetClipRect(bitmap->Tex, &rect);
	SDL_SetClipRect(bitmap->Tex, NULL);
	SDL_Color maskColor = pd_api_gfx_color_white;
	switch (bgcolor)
    {
        case kColorBlack:
            SDL_FillRect(bitmap->Tex, NULL, SDL_MapRGBA(bitmap->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a));
            break;
        case kColorWhite:
            SDL_FillRect(bitmap->Tex, NULL, SDL_MapRGBA(bitmap->Tex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a));
            break;
        case kColorClear:
            SDL_FillRect(bitmap->Tex, NULL, SDL_MapRGBA(bitmap->Tex->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a));
            maskColor = pd_api_gfx_color_black;
			break;
        default:
            // Pattern: fill with white first as base, then tile the pattern
			SDL_FillRect(bitmap->Tex, NULL, SDL_MapRGBA(bitmap->Tex->format, 
				pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, 
				pd_api_gfx_color_white.b, pd_api_gfx_color_white.a));
			// Now tile the pattern over it
			{
				uint8_t* patternBytes = (uint8_t*)(uintptr_t)bgcolor;
				LCDBitmap* patternBitmap = pd_api_gfx_PatternToBitmap(patternBytes);
				if (patternBitmap) {
					for (int py = 0; py < bitmap->h; py += 8)
						for (int px = 0; px < bitmap->w; px += 8) {
							SDL_Rect dst = {px, py, 8, 8};
							SDL_BlitSurface(patternBitmap->Tex, NULL, bitmap->Tex, &dst);
						}
					pd_api_gfx_freeBitmap(patternBitmap);
				}
			}
			break;
    }
	if (bitmap->Mask)
	{
         SDL_FillRect(bitmap->Mask->Tex, &rect, SDL_MapRGBA(bitmap->Mask->Tex->format,  pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a));	
		 bitmap->MaskDirty = true;
	}
	SDL_SetClipRect(bitmap->Tex, &rect);
	
}

LCDBitmap* pd_api_gfx_newBitmap(int width, int height, LCDColor bgcolor)
{
    LCDBitmap* result = pd_api_gfx_Create_LCDBitmap();
    if(result)
    {
        result->Tex = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, pd_api_gfx_PIXELFORMAT);
		//SDL_SetSurfaceRLE(result->Tex, SDL_RLEACCEL);
		result->w = width;
        result->h = height;
        result->TableBitmap = false;
        result->Opaque = bgcolor != kColorClear;
        SDL_SetSurfaceBlendMode(result->Tex, SDL_BLENDMODE_NONE);
        pd_api_gfx_clearBitmap(result, bgcolor);		
    }
    return result;
}

void pd_api_gfx_freeBitmap(LCDBitmap* Bitmap)
{
    if(Bitmap == NULL || Bitmap->TableBitmap)
	{
        return;
	}

	if(Bitmap->Mask)
	{
		pd_api_gfx_freeBitmap(Bitmap->Mask);
	}

	if(Bitmap->MaskedTex)
	{
		SDL_FreeSurface(Bitmap->MaskedTex);
		Bitmap->MaskedTex = NULL;
	}

	if(Bitmap->BitmapDataData)
	{
		free(Bitmap->BitmapDataData);
		Bitmap->BitmapDataData = NULL;
	}

	if(Bitmap->BitmapDataMask)
	{
		free(Bitmap->BitmapDataMask);
		Bitmap->BitmapDataMask = NULL;
	}

    if (Bitmap->Tex)
    {
        SDL_FreeSurface(Bitmap->Tex);
    }

    free(Bitmap);
    Bitmap = NULL;
}

LCDBitmap* pd_api_gfx_loadBitmap(const char* path, const char** outerr)
{
	if(outerr)
    	*outerr = loaderror;
    LCDBitmap* result = NULL;
    char* tmpfullpath = (char*)malloc(MAXPATH);
	char* fullpath    = (char*)malloc(MAXPATH);

    // Strip .pdi extension — Playdate compiled image format, we use .png instead
    char stripped[MAXPATH];
    strncpy(stripped, path, MAXPATH-1);
    stripped[MAXPATH-1] = '\0';
    if (strlen(stripped) > 4 && strcasecmp(stripped + strlen(stripped) - 4, ".pdi") == 0)
        stripped[strlen(stripped) - 4] = '\0';
    path = stripped;

    bool needextension = true;
    if(strlen(path) > 4)
    {
        const char* ext4 = path + (strlen(path) - 4);
        needextension = strcasecmp(ext4, ".png") != 0;
    }
    
	if (needextension)
    {
		snprintf(tmpfullpath, MAXPATH, "%s.png", path);
	}
    else
	{
        snprintf(tmpfullpath, MAXPATH, "%s", path);
	}

    const char* srcDir = _pd_api_get_current_source_dir();
    if (strcmp(srcDir, ".") == 0)
	{
        snprintf(fullpath, MAXPATH, "./%s", tmpfullpath);
	}
    else
	{
        snprintf(fullpath, MAXPATH, "./%s/%s", srcDir, tmpfullpath);
	}

	struct stat lstats;
	if(stat(fullpath, &lstats) != 0)
		snprintf(fullpath, MAXPATH, "./%s", tmpfullpath);

    SDL_Surface* Img = IMG_Load(fullpath);
    if(Img)
    {
		SDL_Surface* Img2 = SDL_ConvertSurfaceFormat(Img, pd_api_gfx_PIXELFORMAT, 0);
		if (Img2)
		{
			bool opaque = false;
			if (_pd_current_source_dir == 0)
				opaque = pd_api_gfx_MakeSurfaceBlackAndWhite(Img2);
			else
				opaque = pd_api_gfx_MakeSurfaceOpaque(Img2);
			result = pd_api_gfx_newBitmap(Img2->w, Img2->h, kColorClear);
			if(result)
			{
				if(outerr)
					*outerr = NULL;
				printfDebug(DebugLoadPaths, "INFO: Bitmap Loaded: %s\n", fullpath);
				result->Opaque = opaque;
				SDL_SetSurfaceBlendMode(Img2, SDL_BLENDMODE_BLEND);
				SDL_BlitSurface(Img2, NULL, result->Tex, NULL);				
				SDL_SetSurfaceBlendMode(result->Tex, SDL_BLENDMODE_NONE);
				//SDL_SetSurfaceRLE(result->Tex, SDL_RLEACCEL);
			}
			SDL_FreeSurface(Img2);
		}
		else
			printfDebug(DebugLoadPaths, "INFO: FAILED LOADING Bitmap (SDL_ConvertSurfaceFormat failed): %s\n", fullpath); 
		SDL_FreeSurface(Img);
    }
	else
		printfDebug(DebugLoadPaths, "INFO: FAILED LOADING Bitmap (imgLoad failed): %s\n", fullpath); 
    free(fullpath);
	free(tmpfullpath);
    return result;
}

LCDBitmap* pd_api_gfx_copyBitmap(LCDBitmap* bitmap)
{
    if(bitmap == NULL)
        return NULL;
    LCDBitmap* result = pd_api_gfx_newBitmap(bitmap->w, bitmap->h, kColorClear);
    if(result->Tex)
		SDL_FreeSurface(result->Tex);

	result->Tex = SDL_ConvertSurfaceFormat(bitmap->Tex, pd_api_gfx_PIXELFORMAT, 0);
	if(bitmap->Mask)
		result->Mask = pd_api_gfx_copyBitmap(bitmap->Mask);
	if(bitmap->MaskedTex)
		result->MaskedTex = SDL_ConvertSurfaceFormat(bitmap->MaskedTex, pd_api_gfx_PIXELFORMAT, 0);

	if(bitmap->BitmapDataData)
	{
		int rb = (int)ceil(bitmap->w /8.0f);
		result->BitmapDataData = (uint8_t*) malloc(rb*bitmap->h * sizeof(uint8_t));
		memcpy(result->BitmapDataData, bitmap->BitmapDataData, rb*bitmap->h * sizeof(uint8_t));
	}
	
	if(bitmap->BitmapDataMask)
	{
		int rb = (int)ceil(bitmap->w /8.0f);
		result->BitmapDataMask = (uint8_t*) malloc(rb*bitmap->h * sizeof(uint8_t));
		memcpy(result->BitmapDataMask, bitmap->BitmapDataMask, rb*bitmap->h * sizeof(uint8_t));
	}

	result->Parent = bitmap->Parent;
	result->Opaque = bitmap->Opaque;
	result->BitmapDataDataChecksum = bitmap->BitmapDataDataChecksum;
	result->BitmapDataMaskChecksum = bitmap->BitmapDataMaskChecksum;
    return result;
}

void pd_api_gfx_loadIntoBitmap(const char* path, LCDBitmap* bitmap, const char** outerr)
{
    LCDBitmap* tmpBitmap = pd_api_gfx_loadBitmap(path, outerr);
	if(tmpBitmap)
	{
		if(bitmap->MaskedTex)
			SDL_FreeSurface(bitmap->MaskedTex);
		bitmap->MaskedTex = SDL_ConvertSurfaceFormat(tmpBitmap->MaskedTex, pd_api_gfx_PIXELFORMAT, 0);

		if(bitmap->Tex)
			SDL_FreeSurface(bitmap->Tex);
		bitmap->Tex = SDL_ConvertSurfaceFormat(tmpBitmap->Tex, pd_api_gfx_PIXELFORMAT, 0);

		if(bitmap->Mask)
			pd_api_gfx_freeBitmap(bitmap->Mask);
		bitmap->Mask = NULL; 

		if(bitmap->BitmapDataData)
			free(bitmap->BitmapDataData);
		bitmap->BitmapDataData = NULL;

		if(bitmap->BitmapDataMask)
			free(bitmap->BitmapDataMask);
		bitmap->BitmapDataMask = NULL;

		bitmap->BitmapDirty = true;
		bitmap->h = tmpBitmap->h;
		bitmap->w = tmpBitmap->w;
		bitmap->MaskDirty = false;
	}
	pd_api_gfx_freeBitmap(tmpBitmap);
}

//mask and data changes are not applied to the bitmap but they can be read
void pd_api_gfx_getBitmapData(LCDBitmap* bitmap, int* width, int* height, int* rowbytes, uint8_t** mask, uint8_t** data)
{
    if(width)
    {
        *width = 0;
        if(bitmap != NULL)
        {
            *width = bitmap->w;
        }
    }
    if(height)
    {
        *height = 0;
        if(bitmap != NULL)
        {
            *height = bitmap->h;
        }
    }

	if(bitmap == NULL)
		return;

	int rb = (int)ceil(bitmap->w /8.0f);
    if(rowbytes)
    {
        *rowbytes = rb;
    }

	if(mask)
    {
		if(!bitmap->BitmapDataMask)
			bitmap->BitmapDataMask = (uint8_t*) malloc(rb*bitmap->h * sizeof(uint8_t));
		*mask = bitmap->BitmapDataMask;
    }

    if(data)
    {
		if(!bitmap->BitmapDataData)
			bitmap->BitmapDataData = (uint8_t*) malloc(rb*bitmap->h * sizeof(uint8_t));
		*data = bitmap->BitmapDataData;
	}

	if(mask || data)
	{
        uint32_t maskChecksum = 0;
        uint32_t dataChecksum = 0;
		if(mask)
			memset(*mask, 0, rb*bitmap->h);
		if(data)
			memset(*data, 0, rb*bitmap->h);
		bool unlocked = true;
		if (SDL_MUSTLOCK(bitmap->Tex))
			unlocked = SDL_LockSurface(bitmap->Tex) == 0;
		if (unlocked)
		{
			Uint32 clear = SDL_MapRGBA(bitmap->Tex->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
			Uint32 whitethreshold = SDL_MapRGBA(bitmap->Tex->format, pd_api_gfx_color_whitetreshold.r, pd_api_gfx_color_whitetreshold.g, pd_api_gfx_color_whitetreshold.b, pd_api_gfx_color_whitetreshold.a);
			Uint32 blackthreshold = SDL_MapRGBA(bitmap->Tex->format, pd_api_gfx_color_blacktreshold.r, pd_api_gfx_color_blacktreshold.g, pd_api_gfx_color_blacktreshold.b, pd_api_gfx_color_blacktreshold.a);
			Uint32 alpha = SDL_MapRGBA(bitmap->Tex->format,0,0,0,0);
			for (int y = 0; y < bitmap->h; y++)
				for (int x = 0; x < bitmap->w; x++)
				{
					Uint32 *p = (Uint32*)((Uint8 *)bitmap->Tex->pixels + (y * bitmap->Tex->pitch) + (x * bitmap->Tex->format->BytesPerPixel));
					Uint32 pval = *p;
					if ((pval == alpha) || (pval == clear))
					{
						if(mask)
                            pd_api_gfx_drawpixel(*mask, x, y, rb, kColorBlack);
					}
					else
					{
						if(mask)
						{
                            pd_api_gfx_drawpixel(*mask, x, y, rb, kColorWhite);
							maskChecksum += x + y * bitmap->w;
						}
						
						if(data)
						{
							if(pval >= whitethreshold)
							{
								pd_api_gfx_drawpixel(*data, x, y, rb, kColorWhite);
								dataChecksum += x + y * bitmap->w;
							}
							else if(pval <= blackthreshold)
							{
								pd_api_gfx_drawpixel(*data, x, y, rb, kColorBlack);
							}
						}
					}

				}

			if(SDL_MUSTLOCK(bitmap->Tex))
				SDL_UnlockSurface(bitmap->Tex);
		}
        if (mask)
            bitmap->BitmapDataMaskChecksum = maskChecksum;
        if (data)
            bitmap->BitmapDataDataChecksum = dataChecksum;
    }
}

LCDBitmap* pd_api_gfx_rotatedBitmap(LCDBitmap* bitmap, float rotation, float xscale, float yscale, int* allocedSize)
{
    return NULL;
}

LCDBitmapTable* pd_api_gfx_Create_LCDBitmapTable()
{
    LCDBitmapTable* tmp = (LCDBitmapTable* ) malloc(sizeof(*tmp));
    if(tmp)
    {
        tmp->count = 0;
        tmp->w = 0;
        tmp->h = 0;
        tmp->bitmaps = NULL;
    }
    return tmp;
}

// LCDBitmapTable
LCDBitmapTable* pd_api_gfx_newBitmapTable(int count, int width, int height)
{
    LCDBitmapTable* result = pd_api_gfx_Create_LCDBitmapTable();
    if (result)
    {
        result->count = count;
        result->w = width;
        result->h = height;
        result->bitmaps = (LCDBitmap **) malloc(count * sizeof (*result->bitmaps));
        for (int i = 0; i < count; i++)
        {
            result->bitmaps[i] = pd_api_gfx_newBitmap(width, height, kColorClear);
            result->bitmaps[i]->TableBitmap = true;
        }
    }
    return result;
}

void pd_api_gfx_freeBitmapTable(LCDBitmapTable* table)
{
    if(table == NULL)
        return;
    for (int i = 0; i < table->count; i++)
    {
        table->bitmaps[i]->TableBitmap = false;
        pd_api_gfx_freeBitmap(table->bitmaps[i]);
    }
	free(table->bitmaps);
	table->bitmaps = NULL;
    free(table);
    table = NULL;
}

LCDBitmapTable* _pd_api_gfx_do_loadBitmapTable(const char* path, const char** outerr)
{
	if(outerr)
    	*outerr = loaderror;
    LCDBitmapTable* result = NULL;
    char dir_name[255];
    char prefix[255];
    const char *partialPath = strrchr(path, '/');

    if(partialPath)
    {
        partialPath +=1; // Add 1 to skip the directory separator
        strncpy(dir_name, path, partialPath - path - 1); // -1 to exclude the directory separator
        dir_name[partialPath - path - 1] = '\0';
        strcpy(prefix, partialPath);
    }
    else
    {
        dir_name[0] = '\0';
        strcpy(prefix, path);
    }

    strcat(prefix, "-table-");

    
    DIR *dir = opendir(dir_name);
    if (dir == NULL) {
        printfDebug(DebugInfo, "error opening dir \"%s\"!", dir_name);
        return NULL;
    }
    
    char* fullpath = NULL; 
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncasecmp(prefix, entry->d_name, strlen(prefix)) == 0) 
        {
            fullpath =  (char *) malloc(((strlen(entry->d_name)+1)  + (strlen(dir_name) + 1) + 1) * sizeof(char));
            strcpy(fullpath, dir_name);
            strcat(fullpath, "/");
            strcat(fullpath, entry->d_name);
            break;
        }
    }
    closedir(dir);
    if (fullpath)
    {
        char *tmpPath =  (char *) malloc((strlen(fullpath)+1) * sizeof(char));
        strcpy(tmpPath, fullpath);
        
        char *s = tmpPath;
        while (*s) {
            *s = toupper((unsigned char) *s);
            s++;
        }
    
        char* substr = strstr(tmpPath, "-TABLE-");
        if(substr != NULL)
        {
            int num_digits = 0;
            char buffer[10];
            int w = 0, h = 0;

            // Iterate through each character in the filename
            for (size_t i = 0; i < strlen(substr); i++) {
                if (isdigit(substr[i])) {
                    // If the character is a digit, add it to the buffer
                    buffer[num_digits++] = substr[i];
                } else if (num_digits > 0) {
                    // If the character is not a digit and we have already accumulated digits, convert the buffer to an integer
                    buffer[num_digits] = '\0';
                    int num = atoi(buffer);
                    // Update w or h depending on whether we are parsing the width or height
                    if (w == 0) {
                        w = num;
                    } else {
                        h = num;
                    }
                    // Reset the buffer and digit counter
                    num_digits = 0;
                    memset(buffer, 0, sizeof(buffer));
                }
            }

            // Check if the last buffer contains digits
            if (num_digits > 0) {
                buffer[num_digits] = '\0';
                int num = atoi(buffer);
                if (w == 0) {
                    w = num;
                } else {
                    h = num;
                }
            }
			
			//format name-table-X (seems we need to load next few files then that exists with incrementing x)
			if ((w == 1) && (h == 0))
			{
				
				char* ext2 = strrchr(fullpath, '.');
				if(ext2)
				{
					char* substr2 = strstr(tmpPath, "-TABLE-");
					char* rootfilename = (char *) malloc((strlen(fullpath)+10) * sizeof(char));
					memset(rootfilename, 0, (strlen(fullpath)+10) * sizeof(char));
					strncpy(rootfilename, fullpath, strlen(fullpath) - strlen(substr2) + 7);
					char* fullpath2 = (char *) malloc((strlen(fullpath)+14) * sizeof(char));
					int counter = 1;
					sprintf(fullpath2, "%s%d%s", rootfilename, counter, ext2);
					SDL_Surface* Img = NULL;
					SDL_Surface* Tmp = IMG_Load(fullpath2);
					if(Tmp)
					{
						result = pd_api_gfx_Create_LCDBitmapTable();
						if(result)
						{
							result->bitmaps = (LCDBitmap **) malloc(sizeof(*result->bitmaps));
							result->w = Tmp->w;
							result->h = Tmp->h;
							result->across = (int)Tmp->w / w;
							if(outerr)
								*outerr = NULL;
								
							while(Tmp)
							{
								Img = SDL_ConvertSurfaceFormat(Tmp, pd_api_gfx_PIXELFORMAT, 0);
								SDL_FreeSurface(Tmp);
								bool opaque = false;
								if (_pd_current_source_dir == 0)
									opaque = pd_api_gfx_MakeSurfaceBlackAndWhite(Img);
								else
									opaque = pd_api_gfx_MakeSurfaceOpaque(Img);

								SDL_SetSurfaceBlendMode(Img, SDL_BLENDMODE_BLEND);
								result->bitmaps = (LCDBitmap **) realloc(result->bitmaps, (result->count+1) * sizeof(*result->bitmaps));
								result->bitmaps[result->count] = pd_api_gfx_newBitmap(result->w, result->h, kColorClear);
								if(result->bitmaps[result->count])
								{
									SDL_BlitSurface(Img, NULL, result->bitmaps[result->count]->Tex, NULL);
									SDL_SetSurfaceBlendMode(result->bitmaps[result->count]->Tex, SDL_BLENDMODE_NONE);
                                    result->bitmaps[result->count]->TableBitmap = true;
									result->bitmaps[result->count]->Opaque = opaque;
									result->count++;
								}
								else
								{
									if(outerr)
										*outerr = loaderror;
									return NULL;
								}
								
								SDL_FreeSurface(Img);

								counter++;
								sprintf(fullpath2, "%s%d%s", rootfilename, counter, ext2);
								Tmp = IMG_Load(fullpath2);							
							}	
						}
					}
					free(rootfilename);
					free(fullpath2);

				}
				
			}
			else
			{
				SDL_Surface* Img = NULL;
				SDL_Surface* Tmp = IMG_Load(fullpath);
				if(Tmp)
				{
					Img = SDL_ConvertSurfaceFormat(Tmp, pd_api_gfx_PIXELFORMAT, 0);
					SDL_FreeSurface(Tmp);
				}
				if(Img)
				{
					result = pd_api_gfx_Create_LCDBitmapTable();
					if(result)
					{
						bool opaque = false;
						if (_pd_current_source_dir == 0)
							opaque = pd_api_gfx_MakeSurfaceBlackAndWhite(Img);
						else
							opaque = pd_api_gfx_MakeSurfaceOpaque(Img);
						result->bitmaps = (LCDBitmap **) malloc(sizeof(*result->bitmaps));
						if(outerr)
							*outerr = NULL;
						result->w = w;
						result->h = h;
						result->across = (int)Img->w / w;
						SDL_SetSurfaceBlendMode(Img, SDL_BLENDMODE_BLEND);
						for (int y = 0; y < Img->h; y+=h)
						{
							for (int x = 0; x < Img->w ; x+=w)
							{     
								result->bitmaps = (LCDBitmap **) realloc(result->bitmaps, (result->count+1) * sizeof(*result->bitmaps));
								result->bitmaps[result->count] = pd_api_gfx_newBitmap(w, h, kColorClear);
								if(result->bitmaps[result->count])
								{
									SDL_Rect rect;
									rect.x = x;
									rect.y = y;
									rect.w = w;
									rect.h = h;
									SDL_BlitSurface(Img, &rect, result->bitmaps[result->count]->Tex, NULL);
									SDL_SetSurfaceBlendMode(result->bitmaps[result->count]->Tex, SDL_BLENDMODE_NONE);
                                    result->bitmaps[result->count]->TableBitmap = true;
									result->bitmaps[result->count]->Opaque = opaque;
									result->count++;
								}
								else
								{
									if(outerr)
										*outerr = loaderror;
									return NULL;
								}
							}
						}
					}
					SDL_FreeSurface(Img);
				}
		
			}
		}
		free(fullpath);
		free(tmpPath); 
    }
    return result;
}

// Load a multi-frame GIF into an LCDBitmapTable using IMG_LoadAnimation.
// Each frame becomes one entry in the table. Returns NULL if the file doesn't
// exist or isn't a multi-frame GIF (single-frame GIFs are handled by the
// normal sprite-sheet path).
static LCDBitmapTable* _pd_api_gfx_loadGifTable(const char* gifpath, const char** outerr)
{
    IMG_Animation* anim = IMG_LoadAnimation(gifpath);
    if (!anim)
        return NULL;
    if (anim->count < 1)
    {
        IMG_FreeAnimation(anim);
        return NULL;
    }

    LCDBitmapTable* result = pd_api_gfx_Create_LCDBitmapTable();
    if (!result)
    {
        IMG_FreeAnimation(anim);
        if (outerr) *outerr = loaderror;
        return NULL;
    }

    result->w      = anim->w;
    result->h      = anim->h;
    result->across = 1;
    result->count  = 0;
    result->bitmaps = (LCDBitmap**)malloc(anim->count * sizeof(LCDBitmap*));
    if (!result->bitmaps)
    {
        free(result);
        IMG_FreeAnimation(anim);
        if (outerr) *outerr = loaderror;
        return NULL;
    }

    if (outerr) *outerr = NULL;

    for (int i = 0; i < anim->count; i++)
    {
        SDL_Surface* src = anim->frames[i];
        SDL_Surface* converted = SDL_ConvertSurfaceFormat(src, pd_api_gfx_PIXELFORMAT, 0);
        if (!converted)
        {
            // free what we have so far and bail
            for (int j = 0; j < result->count; j++)
                pd_api_gfx_freeBitmap(result->bitmaps[j]);
            free(result->bitmaps);
            free(result);
            IMG_FreeAnimation(anim);
            if (outerr) *outerr = loaderror;
            return NULL;
        }

        bool opaque = false;
        if (_pd_current_source_dir == 0)
            opaque = pd_api_gfx_MakeSurfaceBlackAndWhite(converted);
        else
            opaque = pd_api_gfx_MakeSurfaceOpaque(converted);

        SDL_SetSurfaceBlendMode(converted, SDL_BLENDMODE_BLEND);

        LCDBitmap* bmp = pd_api_gfx_newBitmap(anim->w, anim->h, kColorClear);
        if (!bmp)
        {
            SDL_FreeSurface(converted);
            for (int j = 0; j < result->count; j++)
                pd_api_gfx_freeBitmap(result->bitmaps[j]);
            free(result->bitmaps);
            free(result);
            IMG_FreeAnimation(anim);
            if (outerr) *outerr = loaderror;
            return NULL;
        }

        SDL_BlitSurface(converted, NULL, bmp->Tex, NULL);
        SDL_SetSurfaceBlendMode(bmp->Tex, SDL_BLENDMODE_NONE);
        SDL_FreeSurface(converted);

        bmp->TableBitmap = true;
        bmp->Opaque      = opaque;
        result->bitmaps[result->count++] = bmp;
    }

    IMG_FreeAnimation(anim);
    printfDebug(DebugLoadPaths, "INFO: GIF BitmapTable loaded: %s (%d frames, %dx%d)\n",
                gifpath, result->count, result->w, result->h);
    return result;
}

LCDBitmapTable* pd_api_gfx_loadBitmapTable(const char* path, const char** outerr)
{
    // Strip .pdt extension — Playdate compiled image table format
    char stripped[MAXPATH];
	char ext[5];
	memset(ext, 0, 5);
    strncpy(stripped, path, MAXPATH-1);
    stripped[MAXPATH-1] = '\0';
	// need to strip the ext as well as loading bitmap tables using "player.png" while "player-table-....png" exists is valid
    if (strlen(stripped) > 4 && ((strcasecmp(stripped + strlen(stripped) - 4, ".pdt") == 0) ||
		(strcasecmp(stripped + strlen(stripped) - 4, ".png") == 0) || 
		(strcasecmp(stripped + strlen(stripped) - 4, ".bmp") == 0) ||
		(strcasecmp(stripped + strlen(stripped) - 4, ".tga") == 0) ||
		(strcasecmp(stripped + strlen(stripped) - 4, ".jpg") == 0)))
	{
        strncpy(ext, stripped + (strlen(stripped) - 4), 4);
		stripped[strlen(stripped) - 4] = '\0';
	}
    path = stripped;

	char* fullpath = (char*)malloc(MAXPATH);
    const char* srcDir = _pd_api_get_current_source_dir();
    if (strcmp(srcDir, ".") == 0)
	{
        snprintf(fullpath, MAXPATH, "./%s", path);
	}
    else
	{
        snprintf(fullpath, MAXPATH, "./%s/%s", srcDir, path);
	}
	// Try multi-frame GIF: if path already ends in .gif try it directly,
	// otherwise append .gif. Covers both explicit "anim.gif" and extensionless "anim".
	LCDBitmapTable* result = NULL;
	{
		auto tryGif = [&](const char* base) -> LCDBitmapTable* {
			size_t blen = strlen(base);
			bool hasGif = blen > 4 && strcasecmp(base + blen - 4, ".gif") == 0;
			if (hasGif)
				return _pd_api_gfx_loadGifTable(base, outerr);
			char gifpath[MAXPATH];
			snprintf(gifpath, MAXPATH, "%s.gif", base);
			return _pd_api_gfx_loadGifTable(gifpath, outerr);
		};
		result = tryGif(fullpath);
		if (!result)
			result = tryGif(path);
	}
	if (!result)
		result = _pd_api_gfx_do_loadBitmapTable(fullpath, outerr);
	if(!result)
	{
		result = _pd_api_gfx_do_loadBitmapTable(path, outerr);
		if(result)
		{
			printfDebug(DebugLoadPaths, "INFO: BitmapTable Loaded: %s%s\n", path, ext);
		}	
		else
		{
			printfDebug(DebugLoadPaths, "INFO: BitmapTable Failed Loading: %s%s\n", path, ext);
		}
	}
	else
		printfDebug(DebugLoadPaths, "INFO: BitmapTable Loaded: %s%s\n", fullpath, ext);
	free(fullpath);
	return result;
}

void pd_api_gfx_loadIntoBitmapTable(const char* path, LCDBitmapTable* table, const char** outerr)
{
    LCDBitmapTable* tmpTable = pd_api_gfx_loadBitmapTable(path, outerr);
	if(tmpTable)
	{
		for (int i = 0; i < table->count; i++)
		{
			if(table->bitmaps[i])
				pd_api_gfx_freeBitmap(table->bitmaps[i]);
			table->bitmaps[i] = pd_api_gfx_copyBitmap(tmpTable->bitmaps[i]);
		}
		table->h = tmpTable->h;
		table->w = tmpTable->w;
	}
}

LCDBitmap* pd_api_gfx_getTableBitmap(LCDBitmapTable* table, int idx)
{
    if(table == NULL)
        return NULL;
    if ((idx >= table->count) || (idx < 0))
        return NULL;
    return table->bitmaps[idx];
}

// raw framebuffer access

static void _pd_api_gfx_ensureApiLayer(void); // forward declaration
uint8_t* pd_api_gfx_getFrame(void)
{
    // Return the raw 1-bit framebuffer directly — no readback from Tex needed.
    // On real Playdate, getFrame() returns a pointer to the hardware framebuffer
    // which games write into directly and then signal via markUpdatedRows().
    // All known games that use getFrame() write the entire frame themselves each
    // time, so there is no need to pre-populate the buffer from Tex.
    // Doing a full Tex→1-bit readback here was correct but very slow (96k pixel
    // reads per call) and caused significant frame rate drops.
    pd_gfx_framebuffer_valid = true;
    // Only set got_frame/layer_active when inside the update callback.
    // getFrame() called at kEventInit is stale — should not enable layer/auto-push.
    if (pd_gfx_in_update_callback)
    {
        pd_gfx_framebuffer_got_frame = true;
        pd_gfx_layer_active = true;
    }
    pd_gfx_layer_has_content = false;
    // Clear the layer at the start of each frame so stale API draws don't persist.
    // On real Playdate the framebuffer is shared so old draws get overwritten naturally.
    _pd_api_gfx_ensureApiLayer();
    if (pd_gfx_api_layer && pd_gfx_api_layer->Tex)
        SDL_FillRect(pd_gfx_api_layer->Tex, NULL,
            SDL_MapRGBA(pd_gfx_api_layer->Tex->format,
                pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g,
                pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a));

    return pd_gfx_framebuffer;
}

uint8_t* pd_api_gfx_getDisplayFrame(void) // row stride = LCD_ROWSIZE
{
    // Returns the last completed frame — snapshot taken at end of previous display cycle
    return pd_gfx_display_framebuffer;
}

LCDBitmap* pd_api_gfx_getDebugBitmap(void) // valid in simulator only, function is NULL on device
{
    return NULL;
}

LCDBitmap* pd_api_gfx_copyFrameBufferBitmap(void)
{
    return pd_api_gfx_copyBitmap(_pd_api_gfx_Playdate_Screen);
}

// Push pd_gfx_framebuffer rows [start..end] to the SDL surface.
// Called by markUpdatedRows and automatically by _pd_api_gfx_flushFramebuffer.
static void _pd_api_gfx_pushRowsToSurface(int start, int end)
{
    SDL_Surface* surf = _pd_api_gfx_Playdate_Screen->Tex;
    Uint32 white = SDL_MapRGBA(surf->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
    Uint32 black = SDL_MapRGBA(surf->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);

    bool unlocked = true;
    if (SDL_MUSTLOCK(surf))
        unlocked = SDL_LockSurface(surf) == 0;

    if (unlocked)
    {
        for (int y = start; y <= end; y++)
        {
            uint8_t* row = pd_gfx_framebuffer + y * LCD_ROWSIZE;
            for (int x = 0; x < LCD_COLUMNS; x++)
            {
                bool isWhite = (row[x / 8] >> (7 - (x % 8))) & 1;
                Uint32* p = (Uint32*)((Uint8*)surf->pixels + y * surf->pitch + x * surf->format->BytesPerPixel);
                *p = isWhite ? white : black;
            }
        }
        if (SDL_MUSTLOCK(surf))
            SDL_UnlockSurface(surf);
    }
}

// Snapshots the current Tex into pd_gfx_display_framebuffer for getDisplayFrame().
// Called directly when the menu is open so we capture the menu-overlaid frame.
void _pd_api_gfx_snapshotFramebuffer(void)
{
    uint8_t* data = NULL;
    int w, h, rb;
    pd_api_gfx_getBitmapData(_pd_api_gfx_Playdate_Screen, &w, &h, &rb, NULL, &data);
    if (data)
    {
        memset(pd_gfx_display_framebuffer, 0, sizeof(pd_gfx_display_framebuffer));
        {
            SDL_Surface* surf = _pd_api_gfx_Playdate_Screen->Tex;
            Uint32 white = SDL_MapRGBA(surf->format,
                pd_api_gfx_color_white.r, pd_api_gfx_color_white.g,
                pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            for (int y = 0; y < LCD_ROWS; y++)
            {
                uint8_t* dst = pd_gfx_display_framebuffer + y * LCD_ROWSIZE;
                for (int x = 0; x < LCD_COLUMNS; x++)
                {
                    Uint32* p = (Uint32*)((Uint8*)surf->pixels + y * surf->pitch + x * surf->format->BytesPerPixel);
                    if (*p == white)
                        dst[x / 8] |= (1 << (7 - (x % 8)));
                }
            }
        }
    }
}

// Ensure the API layer bitmap exists. The layer is an LCDBitmap so it can
// be used directly as a draw target — all draw calls go into it with correct
// clipping, offsets and draw modes. Clear color (cyan) is used as transparent key.
// The layer persists across frames and is only cleared when clear() is called.
static void _pd_api_gfx_ensureApiLayer(void)
{
    if (pd_gfx_api_layer) return;
    pd_gfx_api_layer = pd_api_gfx_newBitmap(LCD_COLUMNS, LCD_ROWS, kColorClear);
}

// Switch draw target to the API layer. Returns true if switched.
// Call at start of a public draw function; if returns true, call
// _pd_api_gfx_endLayerDraw() after to restore and repeat on screen.
// XOR and NXOR modes read the destination pixels — they cannot work correctly
// on the layer (which has clear/transparent pixels, not the framebuffer content).
// Skip layer draw for those modes so only the screen draw happens.
static bool _pd_api_gfx_beginLayerDraw(void)
{
    if (!pd_gfx_layer_active) return false;
    if (_pd_api_gfx_getDrawTarget() != _pd_api_gfx_Playdate_Screen) return false;
    LCDBitmapDrawMode mode = _pd_api_gfx_CurrentGfxContext->BitmapDrawMode;
    if (mode == kDrawModeXOR || mode == kDrawModeNXOR) return false;
    _pd_api_gfx_ensureApiLayer();
    if (!pd_gfx_api_layer) return false;
    _pd_api_gfx_CurrentGfxContext->DrawTarget = pd_gfx_api_layer;
    pd_gfx_layer_has_content = true;
    return true;
}

// Restore draw target to screen after a layer draw.
static void _pd_api_gfx_endLayerDraw(void)
{
    _pd_api_gfx_CurrentGfxContext->DrawTarget = _pd_api_gfx_Playdate_Screen;
}

// Called by _pd_api_display() to ensure any getFrame() writes are visible
// even if the game never called markUpdatedRows (which is optional on real hardware).
// Also snapshots the completed frame into pd_gfx_display_framebuffer for getDisplayFrame().
void _pd_api_gfx_flushFramebuffer(void)
{
    // Auto-push if:
    //   a) the game called markUpdatedRows this frame (explicit push), OR
    //   b) the game called getFrame() this frame and wrote to it but skipped markUpdatedRows.
    // Do NOT push if getFrame() was only called at init (stale) and game uses normal draw
    // API calls — that would overwrite the correctly drawn content on Tex.
    if (pd_gfx_framebuffer_valid && !pd_gfx_rows_pushed &&
        (pd_gfx_framebuffer_written || pd_gfx_framebuffer_got_frame))
    {
        _pd_api_gfx_pushRowsToSurface(0, LCD_ROWS - 1);
        // Composite the API layer on top — same as markUpdatedRows does.
        // Games that skip markUpdatedRows (e.g. voxel-terrain-pd) still need
        // API draws (drawText, drawFPS) to appear over the framebuffer content.
        if (pd_gfx_layer_has_content && pd_gfx_api_layer && pd_gfx_api_layer->Tex)
        {
            SDL_Surface* layTex = pd_gfx_api_layer->Tex;
            Uint32 clearPix = SDL_MapRGBA(layTex->format,
                pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g,
                pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            SDL_SetColorKey(layTex, SDL_TRUE, clearPix);
            SDL_BlitSurface(layTex, NULL, _pd_api_gfx_Playdate_Screen->Tex, NULL);
            SDL_SetColorKey(layTex, SDL_FALSE, 0);
        }
    }

    pd_gfx_rows_pushed = false;
    pd_gfx_framebuffer_written = false;
    pd_gfx_framebuffer_got_frame = false;
    pd_gfx_layer_has_content = false;
    pd_gfx_layer_active = false;

    _pd_api_gfx_snapshotFramebuffer();
}

void pd_api_gfx_markUpdatedRows(int start, int end)
{
    if (start > end)
	{
		int tmp = end;
		end = start;
		start = tmp;
	}

	if (start < 0)
		start = 0;

    if (end >= LCD_ROWS)
		end = LCD_ROWS - 1;

    if (!pd_gfx_framebuffer_valid)
		return;

    // Don't push when the menu is open — the menu draws directly onto Tex
    // and the push would overwrite the menu overlay.
    if (!pd_menu_isOpen)
    {
        _pd_api_gfx_pushRowsToSurface(start, end);
        // Composite persistent API layer on top of the framebuffer push.
        // The layer contains drawBitmap/fillRect/drawText draws that happened
        // while getFrame() was active. Clear pixels in the layer are transparent.
        if (pd_gfx_layer_has_content && pd_gfx_api_layer && pd_gfx_api_layer->Tex)
        {
            SDL_Surface* layTex = pd_gfx_api_layer->Tex;
            Uint32 clearPix = SDL_MapRGBA(layTex->format,
                pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g,
                pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            SDL_SetColorKey(layTex, SDL_TRUE, clearPix);
            SDL_Rect rows = { 0, start, LCD_COLUMNS, end - start + 1 };
            SDL_BlitSurface(layTex, &rows, _pd_api_gfx_Playdate_Screen->Tex, &rows);
            SDL_SetColorKey(layTex, SDL_FALSE, 0);
        }
        _pd_api_gfx_Playdate_Screen->BitmapDirty = true;
    }
    pd_gfx_rows_pushed = true;
    pd_gfx_framebuffer_written = true;
}

void pd_api_gfx_display(void)
{
	_pd_api_display();
}

//video

LCDVideoPlayer* pd_api_gfx_loadVideo(const char* path)
{
    return NULL;
}

void pd_api_gfx_freeVideoPlayer(LCDVideoPlayer* p)
{

}

int pd_api_gfx_setContext(LCDVideoPlayer* p, LCDBitmap* context)
{
    return 0;
}

void pd_api_gfx_useScreenContext(LCDVideoPlayer* p)
{

}

int pd_api_gfx_renderFrame(LCDVideoPlayer* p, int n)
{
    return 0;
}

const char* pd_api_gfx_getError(LCDVideoPlayer* p)
{
    return NULL;
}

void pd_api_gfx_getInfo(LCDVideoPlayer* p, int* outWidth, int* outHeight, float* outFrameRate, int* outFrameCount, int* outCurrentFrame)
{

}

LCDBitmap* pd_api_gfx_getContext(LCDVideoPlayer *p)
{
    return NULL;
}


// misc util.
void pd_api_gfx_setColorToPattern(LCDColor* color, LCDBitmap* bitmap, int x, int y)
{

}

int pd_api_gfx_checkMaskCollision(LCDBitmap* bitmap1, int x1, int y1, LCDBitmapFlip flip1, LCDBitmap* bitmap2, int x2, int y2, LCDBitmapFlip flip2, LCDRect rect)
{
    return 0;
}
	
// 1.1
void pd_api_gfx_setScreenClipRect(int x, int y, int width, int height)
{
	SDL_Rect cliprect;
    cliprect.x = x;
    cliprect.y = y;
    cliprect.w = width;
    cliprect.h = height;
    SDL_SetClipRect(_pd_api_gfx_Playdate_Screen->Tex, &cliprect);
}
	
// 1.1.1
void pd_api_gfx_fillPolygon(int nPoints, int* coords, LCDColor color, LCDPolygonFillRule fillrule)
{
    LCDBitmap *mask = _pd_api_gfx_getDrawTarget()->Mask;
    LCDBitmap *bitmap = NULL;
	LCDBitmap *pattern = NULL;
	
	Sint16 vx[nPoints];
    Sint16 vy[nPoints];
	Sint16 minx = INT16_MAX;
	Sint16 miny = INT16_MAX;
	Sint16 maxx = INT16_MIN;
	Sint16 maxy = INT16_MIN;
	Sint16 width;
    Sint16 height;
    for (int i=0; i<nPoints;i++)
    {
        vx[i] = coords[(i*2)];
        vy[i] = coords[(i*2)+1];
		if(minx > vx[i])
			minx = vx[i];
		if(miny > vy[i])
			miny = vy[i];
		if(maxx < vx[i])
			maxx = vx[i];
		if(maxy < vy[i])
			maxy = vy[i];
		
    }

	switch (color)
    {
        case kColorBlack:
            break;
        case kColorWhite:
            break;
        case kColorXOR:
            break;
        case kColorClear:
            break;
		default:
			//assume lcd pattern
			pattern = pd_api_gfx_PatternToBitmap((uint8_t*)color);
		break;
    }

	if (color == kColorXOR || pattern != NULL)
    {
       	for (int i=0; i<nPoints;i++)
		{
			vx[i] -= minx;
			vy[i] -= miny;
			
		};

		width = maxx-minx+1;
    	height = maxy-miny+1;
              
		if(color == kColorXOR)
		{
        	bitmap = Api->graphics->newBitmap(width, height, kColorClear);
        	Api->graphics->pushContext(bitmap);
		}
    }
	else
	{
		for (int i=0; i<nPoints;i++)
		{
			vx[i] += _pd_api_gfx_CurrentGfxContext->drawoffsetx;
			vy[i] += _pd_api_gfx_CurrentGfxContext->drawoffsety;
		};
	} 
    
    
	if(pattern)
	{
		int yoffset = miny % 8;
		int xoffset = minx % 8;
		int tilesx = (width /  8)+2;
		int tilesy = (height / 8)+2;
       	bitmap = Api->graphics->newBitmap(width, height, kColorClear);
		bitmap->Mask = pd_api_gfx_newBitmap(bitmap->w, bitmap->h, kColorBlack);
		pd_api_gfx_pushDrawOffset();
		
		Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
		Api->graphics->setDrawMode(kDrawModeCopy);
		Api->graphics->pushContext(bitmap);	
#ifdef MASKPRIMITIVES		
		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
		 		_pd_api_gfx_drawBitmapAll(pattern, (xx*8) -std::abs(xoffset), (yy*8) -std::abs(yoffset), 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped,false,true);
#else
		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
			{
				SDL_Rect dst;
				dst.x = xx*8 -std::abs(xoffset);
				dst.y = yy*8 -std::abs(yoffset);
				dst.w = 8;
				dst.h = 8;
				SDL_BlitSurface(pattern->Tex, NULL, bitmap->Tex, &dst);
			}
#endif		
		Api->graphics->popContext();
#ifdef MASKPRIMITIVES		
		SDL_FillRect(bitmap->Mask->Tex, NULL, SDL_MapRGBA(bitmap->Mask->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a));
#endif		
		filledPolygonRGBASurface(bitmap->Mask->Tex, vx, vy , nPoints, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
		bitmap->BitmapDirty = true;
		pd_api_gfx_popDrawOffset();
		Api->graphics->drawBitmap(bitmap,minx, miny, kBitmapUnflipped);
		Api->graphics->popContext();
		Api->graphics->freeBitmap(bitmap);
		Api->graphics->freeBitmap(pattern);
	}
	else
	{
    	SDL_Color maskColor = pd_api_gfx_color_white;
		switch (color)
		{
			case kColorBlack:
				filledPolygonRGBASurface(_pd_api_gfx_getDrawTarget()->Tex, vx, vy, nPoints, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
				break;
			case kColorWhite:
				filledPolygonRGBASurface(_pd_api_gfx_getDrawTarget()->Tex, vx, vy, nPoints, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
				break;
			case kColorXOR:
				filledPolygonRGBASurface(_pd_api_gfx_getDrawTarget()->Tex, vx, vy, nPoints, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
				break;
			case kColorClear:
				filledPolygonRGBASurface(_pd_api_gfx_getDrawTarget()->Tex, vx, vy, nPoints, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
				maskColor = pd_api_gfx_color_black;
				break;
			default:
				break;
		}
		if (mask && color != kColorXOR)
			filledPolygonRGBASurface(mask->Tex, vx, vy, nPoints, maskColor.r, maskColor.g, maskColor.b, maskColor.a);
	}

    if (color == kColorXOR)
    {
        pd_api_gfx_popContext();
		pd_api_gfx_pushDrawOffset();
        pd_api_gfx_pushContext(_pd_api_gfx_getDrawTarget());
        pd_api_gfx_setDrawMode(kDrawModeXOR);
		pd_api_gfx_popDrawOffset();
        _pd_api_gfx_drawBitmapAll(bitmap, minx, miny, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped, true,true);
        pd_api_gfx_popContext();
        pd_api_gfx_freeBitmap(bitmap);
    }
}

uint8_t pd_api_gfx_getFontHeight(LCDFont* font)
{
    LCDFont *f = font;
    if(!f)
        f = _pd_api_gfx_CurrentGfxContext->font;
    if(!f)
        f = _pd_api_gfx_Default_Font;

    if(!f)
        return 0;

    #if !TTFONLY
    if(f->isFntFont)
        return (uint8_t)f->cellH;
    #endif

    if(!f->font)
        return 0;

    return TTF_FontHeight(f->font);
}
	
// 1.7
LCDBitmap* pd_api_gfx_getDisplayBufferBitmap(void)
{
    return _pd_api_gfx_Playdate_Screen;
}

void pd_api_gfx_setTextLeading(int lineHeightAdustment)
{
    _pd_api_gfx_CurrentGfxContext->leading = lineHeightAdustment;
}

// 1.8
int pd_api_gfx_setBitmapMask(LCDBitmap* bitmap, LCDBitmap* mask)
{
	if(!ENABLEBITMAPMASKS)
		return 0;

	if(!mask || !bitmap)
		return 0;

	if (!mask->Tex || !bitmap->Tex)
		return 0;

	if ((mask->Tex->w != bitmap->Tex->w) || (mask->Tex->h != bitmap->Tex->h))
		return 0;

	if (bitmap->Mask == mask)
	{
		bitmap->MaskDirty = true;
		return 1;
	}

	//can't seem to set a mask on this
	if (bitmap == pd_api_gfx_getDisplayBufferBitmap())
		return 0;

	//the mask seems to be a copy
	if(bitmap->Mask)
		pd_api_gfx_freeBitmap(bitmap->Mask);
	
	bitmap->Mask = pd_api_gfx_copyBitmap(mask);
	bitmap->MaskDirty = true;
	bitmap->Opaque = false;
	return 1;
}

LCDBitmap* pd_api_gfx_getBitmapMask(LCDBitmap* bitmap)
{
	//getbitmapmask always seem to return null on pd_api_gfx_getDisplayBufferBitmap
	if(!bitmap || bitmap == pd_api_gfx_getDisplayBufferBitmap() ||  bitmap->Opaque || !ENABLEBITMAPMASKS)
		return NULL;

    if(!bitmap->Mask)
    {
        LCDBitmap *mask = pd_api_gfx_newBitmap(bitmap->w, bitmap->h, kColorBlack);

        bool texUnlocked = true;
        bool maskUnlocked = true;

        if (SDL_MUSTLOCK(bitmap->Tex))
            texUnlocked = SDL_LockSurface(bitmap->Tex) == 0;
        if (SDL_MUSTLOCK(mask->Tex))
            maskUnlocked = SDL_LockSurface(mask->Tex) == 0;
        if (texUnlocked && maskUnlocked)
        {
            Uint32 clear = SDL_MapRGBA(bitmap->Tex->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            Uint32 alpha = SDL_MapRGBA(bitmap->Tex->format,0,0,0,0);
            Uint32 white = SDL_MapRGBA(mask->Tex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            Uint32 black = SDL_MapRGBA(mask->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            for (int y = 0; y < bitmap->h; y++)
                for (int x = 0; x < bitmap->w; x++)
                {
                    Uint32 *p = (Uint32*)((Uint8 *)bitmap->Tex->pixels + (y * bitmap->Tex->pitch) + (x * bitmap->Tex->format->BytesPerPixel));
                    Uint32 pval = *p;
                    Uint32 *p2 = (Uint32*)((Uint8 *)mask->Tex->pixels + (y * mask->Tex->pitch) + (x * mask->Tex->format->BytesPerPixel));
                    if ((pval == alpha) || (pval == clear))
                        *p2 = black;
                    else
                        *p2 = white;
                }

            if(SDL_MUSTLOCK(bitmap->Tex))
                SDL_UnlockSurface(bitmap->Tex);
        }

        bitmap->Mask = mask;
    }

    LCDBitmap *maskRef = pd_api_gfx_Create_LCDBitmap();
    maskRef->Parent = bitmap;
    maskRef->w = bitmap->w;
    maskRef->h = bitmap->h;
    maskRef->Opaque = true;

	return maskRef;
}
	
// 1.10
void pd_api_gfx_setStencilImage(LCDBitmap* stencil, int tile)
{
	_pd_api_gfx_CurrentGfxContext->stencil = stencil;
}
	
// 1.12
LCDFont* pd_api_gfx_makeFontFromData(LCDFontData* data, int wide)
{
    return NULL;
}

// 2.1
int pd_api_gfx_getTextTracking()
{
	return _pd_api_gfx_CurrentGfxContext->tracking;
}

int printcount = 1;

void _pd_api_gfx_drawBitmapAll(LCDBitmap* bitmap, int x, int y, float xscale, float yscale, bool isRotatedBitmap, const double angle, float centerx, float centery, LCDBitmapFlip flip, bool IgnoreClearColorXOR, bool IgnoreStencil)
{
    _pd_api_gfx_checkBitmapNeedsRedraw(bitmap);
	pd_api_gfx_recreatemaskedimage(bitmap);
    if (xscale *yscale == 0.0f)
		return;
	//keep values sane don't know if playdate got such limits
	//but here it can cause issues (see twinracer)
	if (bitmap->w * xscale > 2500.0f)
		return;
	if (bitmap->h * yscale > 2500.0f)
		return;
	SDL_Rect dstrect;
    dstrect.x = x +  _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    dstrect.y = y +  _pd_api_gfx_CurrentGfxContext->drawoffsety;
    dstrect.w = bitmap->w;
    dstrect.h = bitmap->h;
	SDL_Surface *tmpTexture1 = NULL;
	SDL_Surface *tmpTexture = NULL;

	if(bitmap->Parent)
	 	//we are drawing a bitmap mask which got a reference to a parent bitmap
	 	tmpTexture = bitmap->Parent->Mask->Tex;
	else if(bitmap->MaskedTex)
		tmpTexture = bitmap->MaskedTex;
	else
		tmpTexture = bitmap->Tex;

	bool isOrginalTexture = true;

	
	//flip first as rotation changes width & height
	if(flip != kBitmapUnflipped)
	{
		switch (flip)
		{
			case kBitmapUnflipped:
				//nothing needs to be done
				break;
			case kBitmapFlippedX:
				tmpTexture1 = rotozoomSurfaceXY(tmpTexture, 0, -1, 1, 0);
				if(!isOrginalTexture)
					SDL_FreeSurface(tmpTexture);
				tmpTexture = tmpTexture1;
				 isOrginalTexture = false;
				break;
			case kBitmapFlippedY:
				tmpTexture1 = rotozoomSurfaceXY(tmpTexture, 0, 1, -1, 0);
				if(!isOrginalTexture)
					SDL_FreeSurface(tmpTexture);
				tmpTexture = tmpTexture1;
				isOrginalTexture = false;
				break;
			case kBitmapFlippedXY:
				tmpTexture1 = rotozoomSurfaceXY(tmpTexture, 0, -1, -1, 0);
				if(!isOrginalTexture)
					SDL_FreeSurface(tmpTexture);
				tmpTexture = tmpTexture1;
				isOrginalTexture = false;
				break;
			default:
				break;
		}
	}

	if((xscale != 1.0f) || (yscale != 1.0f))
	{
		// tmpTexture1 = SDL_CreateRGBSurfaceWithFormat(0,  dstrect.w*xscale, dstrect.h*yscale, 32, pd_api_gfx_PIXELFORMAT);		
		// SDL_FillRect(tmpTexture1, NULL, SDL_MapRGBA(tmpTexture1->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a));	
		// SDL_BlitScaled(tmpTexture, NULL, tmpTexture1, &dstrect);
		// SDL_FreeSurface(tmpTexture);
		// tmpTexture = tmpTexture1;

		if (isRotatedBitmap && ((xscale < 0) || (yscale < 0)))
		{
			if (printcount > 0)
			{
				printcount--;
				printfDebug(DebugInfo, "Warning negative xscale or yscale does not work correctly with rotatedbitmaps !");
			}
		}
		tmpTexture1 = zoomSurface(tmpTexture, xscale, yscale, 0);
		if(tmpTexture1)
		{
			if(!isOrginalTexture)
				SDL_FreeSurface(tmpTexture);
			tmpTexture = tmpTexture1;
			isOrginalTexture = false;
			SDL_SetSurfaceBlendMode(tmpTexture, SDL_BLENDMODE_NONE);
		}
	}

	if(isRotatedBitmap)
	{
		SDL_Rect rect_dest;
    	double cangle, sangle;
		SDL_FPoint center ={(centerx) * tmpTexture->w , (centery) * tmpTexture->h };
		//first offset by making width and height larger, in case of rotation the image is put with its center
		//on 0,0 and then rotated as playdate rotates on top left of image
		tmpTexture1 = SDL_CreateRGBSurfaceWithFormat(0, tmpTexture->w + (tmpTexture->w>>1), tmpTexture->h + (tmpTexture->h>>1), 32, pd_api_gfx_PIXELFORMAT);
		SDL_SetSurfaceBlendMode(tmpTexture1, SDL_BLENDMODE_NONE);
		SDL_FillRect(tmpTexture1, NULL, SDL_MapRGBA(tmpTexture1->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a));	
		SDL_BlitSurface(tmpTexture, NULL, tmpTexture1, NULL);
		if(!isOrginalTexture)
			SDL_FreeSurface(tmpTexture);
		tmpTexture = tmpTexture1;
		isOrginalTexture = false;

		//rotozoom uses colorkey for background temporary enable it
		Uint32 color = SDL_MapRGBA(tmpTexture->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
		SDL_SetColorKey(tmpTexture, SDL_TRUE, color);
		SDLgfx_rotozoomSurfaceSizeTrig(tmpTexture->w, tmpTexture->h, angle, &center, &rect_dest, &cangle, &sangle);
		tmpTexture1 = SDLgfx_rotateSurface(tmpTexture, angle, 0, 0, 0, &rect_dest, cangle, sangle, &center);
		SDL_FreeSurface(tmpTexture);
		tmpTexture = tmpTexture1;
		//disable colorkey again for now
		color = SDL_MapRGBA(tmpTexture->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
		SDL_SetColorKey(tmpTexture,SDL_FALSE,color);
		SDL_SetSurfaceBlendMode(tmpTexture, SDL_BLENDMODE_NONE);

		//need to apply offsets
		dstrect.x += rect_dest.x - center.x;
		dstrect.y += rect_dest.y - center.y;
	}

	
	//rotations and scalings needs adjusted positions
	dstrect.w = tmpTexture->w;
	dstrect.h = tmpTexture->h;

    if (_pd_api_gfx_CurrentGfxContext->BitmapDrawMode != kDrawModeCopy)
	{
		tmpTexture1 = SDL_CreateRGBSurfaceWithFormat(0, tmpTexture->w , tmpTexture->h, 32, pd_api_gfx_PIXELFORMAT);
		SDL_SetSurfaceBlendMode(tmpTexture1, SDL_BLENDMODE_NONE);
		SDL_BlitSurface(tmpTexture, NULL, tmpTexture1, NULL);
		if(!isOrginalTexture)
			SDL_FreeSurface(tmpTexture);
		tmpTexture = tmpTexture1;
		isOrginalTexture = false;
	
		// only needed for xor & nxor
		SDL_Surface* drawtargetsurface = NULL;
		bool requiresTargetTexture = (_pd_api_gfx_CurrentGfxContext->BitmapDrawMode == kDrawModeNXOR) || (_pd_api_gfx_CurrentGfxContext->BitmapDrawMode == kDrawModeXOR);
		if (requiresTargetTexture)
		{
			drawtargetsurface = SDL_CreateRGBSurfaceWithFormat(0, dstrect.w, dstrect.h, 32, pd_api_gfx_PIXELFORMAT);
			SDL_BlitSurface(_pd_api_gfx_getDrawTarget()->Tex, &dstrect, drawtargetsurface, NULL);
			SDL_SetSurfaceBlendMode(drawtargetsurface, SDL_BLENDMODE_NONE);
		}
		
		bool textureUnlocked = true;
		if (SDL_MUSTLOCK(tmpTexture))
			textureUnlocked = SDL_LockSurface(tmpTexture) == 0;
		if (textureUnlocked) 
		{
			bool targetTextureUnlocked = true;
			if (requiresTargetTexture)
				if SDL_MUSTLOCK(drawtargetsurface)
					targetTextureUnlocked = SDL_LockSurface(drawtargetsurface) == 0;

			//remember colors
			Uint32 clear = SDL_MapRGBA(tmpTexture->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
			Uint32 white = SDL_MapRGBA(tmpTexture->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
			Uint32 black = SDL_MapRGBA(tmpTexture->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
			Uint32 blackthreshold = SDL_MapRGBA(tmpTexture->format, pd_api_gfx_color_blacktreshold.r, pd_api_gfx_color_blacktreshold.g, pd_api_gfx_color_blacktreshold.b, pd_api_gfx_color_blacktreshold.a);
			Uint32 whitethreshold = SDL_MapRGBA(tmpTexture->format, pd_api_gfx_color_whitetreshold.r, pd_api_gfx_color_whitetreshold.g, pd_api_gfx_color_whitetreshold.b, pd_api_gfx_color_whitetreshold.a);
			Uint32 alpha = SDL_MapRGBA(tmpTexture->format,0,0,0,0);
			Uint32 drawtargetclear = 0;
			Uint32 drawtargetalpha = 0;
			if(requiresTargetTexture)
			{
				drawtargetclear = SDL_MapRGBA(drawtargetsurface->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
				drawtargetalpha = SDL_MapRGBA(drawtargetsurface->format,0,0,0,0);
			}

			
			if (clear <= blackthreshold)
				printfDebug(DebugInfo,"clear color is lower than black threshold color this is wrong and will cause issues !\n");

			if (clear >= whitethreshold)
				printfDebug(DebugInfo,"clear color is bigger than white threshold color this is wrong and will cause issues !\n");
			
			if (clear >= white)
				printfDebug(DebugInfo,"clear color is bigger than white color this is wrong and will cause issues !\n");
			
			if (clear <= black)
				printfDebug(DebugInfo,"clear color is lower than white color this is wrong and will cause issues !\n");
			
			
			//apply drawmode changes to the current tmpsurface only (we will draw it later with colorkey)
			//like in case of clearpixel we will draw a cyan pixel that will be transparant
			//in case of xor nxor we also need the target surface to compare values
			//but the result is actually applied to the tmpsurface as well wich we'll draw one more time
			//at the end using regular functions
			switch (_pd_api_gfx_CurrentGfxContext->BitmapDrawMode)
			{           
				case kDrawModeBlackTransparent:
				{
					int width = tmpTexture->w;
					int height = tmpTexture->h;
					for (int yy = 0; (yy < height); yy++)
					{
						for(int xx = 0; (xx < width); xx++)
						{
							Uint32 *p = (Uint32*)((Uint8 *)tmpTexture->pixels + (yy * tmpTexture->pitch) + (xx * tmpTexture->format->BytesPerPixel));
							Uint32 pval = *p;
							if ((pval == alpha))
							{
								*p = clear;
								continue;
							}
							if ((pval == clear) || (pval >= blackthreshold))
								continue;
							*p = clear;
						}
					}
					break;
				}
				case kDrawModeWhiteTransparent:
				{
					int width = tmpTexture->w;
					int height = tmpTexture->h;
					for (int yy = 0; (yy < height); yy++)
					{
						for(int xx = 0; (xx < width); xx++)
						{
							Uint32 *p = (Uint32*)((Uint8 *)tmpTexture->pixels + (yy * tmpTexture->pitch) + (xx * tmpTexture->format->BytesPerPixel));
							Uint32 pval = *p;
							if ((pval == alpha))
							{
								*p = clear;
								continue;
							}
							if ((pval == clear) || pval <= whitethreshold)
								continue;
							*p = clear;
						}
					}
					break;
				}
				case kDrawModeFillWhite:
				{
					int width = tmpTexture->w;
					int height = tmpTexture->h;
					for (int yy = 0; (yy < height); yy++)
					{
						for(int xx = 0; (xx < width); xx++)
						{                   
							Uint32 *p = (Uint32*)((Uint8 *)tmpTexture->pixels + (yy * tmpTexture->pitch) + (xx * tmpTexture->format->BytesPerPixel));
							Uint32 pval = *p;
							if ((pval == alpha))
							{
								*p = clear;
								continue;
							}
							if ((pval == clear)  || (pval >= blackthreshold))
								continue;
							*p = white;
						}
					}
					break;
				}
				case kDrawModeFillBlack:
				{ 
					int width = tmpTexture->w;
					int height = tmpTexture->h;
					for (int yy = 0; (yy < height); yy++)
					{
						for(int xx = 0; (xx < width); xx++)
						{
							Uint32 *p = (Uint32*)((Uint8 *)tmpTexture->pixels + (yy * tmpTexture->pitch) + (xx * tmpTexture->format->BytesPerPixel));
							Uint32 pval = *p;
							if ((pval == alpha))
							{
								*p = clear;
								continue;
							}
							if ((pval == clear) || (pval <= whitethreshold))
								continue;
							*p = black;
						}
					}
					break;
				}
				case kDrawModeInverted:
				{
					int width = tmpTexture->w;
					int height = tmpTexture->h;
					for (int yy = 0; (yy < height); yy++)
					{
						for(int xx = 0; (xx < width); xx++)
						{
							Uint32 *p = (Uint32*)((Uint8 *)tmpTexture->pixels + (yy * tmpTexture->pitch) + (xx * tmpTexture->format->BytesPerPixel));
							Uint32 pval = *p;
							if ((pval == alpha))
							{
								*p = clear;
								continue;
							}
							if ((pval == clear))
								continue;
							if (pval > whitethreshold)
							{
								*p = black;
							}
							else
							{
								if (pval < blackthreshold)
								{
									*p = white;
								}
							}
						}
					}
					break;
				}
				case kDrawModeXOR:
				{
					if(requiresTargetTexture && targetTextureUnlocked && drawtargetsurface)
					{
						int width = tmpTexture->w;
						int height = tmpTexture->h;
						for (int yy = 0; (yy < height); yy++)
						{
							for(int xx = 0; (xx < width); xx++)
							{
								Uint32 *p = (Uint32*)((Uint8 *)tmpTexture->pixels + (yy * tmpTexture->pitch) + (xx * tmpTexture->format->BytesPerPixel));
								Uint32 pval = *p;
								if ((pval == alpha))
								{
									*p = clear;
									continue;
								}
								if ((pval == clear) )
									continue;
								Uint32 *p2 = (Uint32*)((Uint8 *)drawtargetsurface->pixels + (yy  * drawtargetsurface->pitch) + (xx * drawtargetsurface->format->BytesPerPixel));
								Uint32 p2val = *p2;
								//when color is clear of targetsurface xor does not actually draw something
								if(IgnoreClearColorXOR && (p2val == drawtargetclear || p2val == drawtargetalpha))
								{
									*p = clear;
									continue;
								}
								if (((pval > whitethreshold) && ((p2val < blackthreshold))) || 
									((pval < blackthreshold) && ((p2val > whitethreshold))))
								{
									*p = white;
								}
								else
								{
									*p = black;
								}
							}
						}
					}
					break;
				}
				case kDrawModeNXOR:
				{
					if(requiresTargetTexture && targetTextureUnlocked && drawtargetsurface)
					{
						int width = tmpTexture->w;
						int height = tmpTexture->h;
						for (int yy = 0; (yy < height); yy++)
						{
							for(int xx = 0; (xx < width); xx++)
							{
								Uint32 *p = (Uint32*)((Uint8 *)tmpTexture->pixels + (yy * tmpTexture->pitch) + (xx * tmpTexture->format->BytesPerPixel));
								Uint32 pval = *p;
								if ((pval == alpha))
								{
									*p = clear;
									continue;
								}
								if ((pval == clear) )
									continue;
								Uint32 *p2 = (Uint32*)((Uint8 *)drawtargetsurface->pixels + (yy  * drawtargetsurface->pitch) + (xx * drawtargetsurface->format->BytesPerPixel));
								Uint32 p2val = *p2;
								if (((pval > whitethreshold) && ((p2val < blackthreshold))) || 
									((pval < blackthreshold) && ((p2val> whitethreshold))))
								{
									*p = black;
								}
								else
								{
									if (*(Uint32 *)p != clear)
									{
										*p = white;
									}
								}
							}
						}
					}
					break;
				}
				default: //includes copy, nothing needs changing then
					break;
			}
						
			if(SDL_MUSTLOCK(tmpTexture))
				SDL_UnlockSurface(tmpTexture);

			if(requiresTargetTexture)
			{
				if(targetTextureUnlocked)
					if(SDL_MUSTLOCK(drawtargetsurface))
						SDL_UnlockSurface(drawtargetsurface);
				SDL_FreeSurface(drawtargetsurface);
			}
		}
	}

    LCDBitmap *drawTarget = _pd_api_gfx_getDrawTarget();

	// Apply stencil: pixels where stencil is black (below whitethreshold) become transparent
	Uint32 cclear = SDL_MapRGBA(tmpTexture->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
	if(SHOWCLEARCOLOR != 1)
		SDL_SetColorKey(tmpTexture, SDL_TRUE, cclear);

	LCDBitmap* stencil = _pd_api_gfx_CurrentGfxContext->stencil;
	if (stencil && stencil->Tex && !IgnoreStencil)
	{
		// Blit pixel-by-pixel, skipping pixels where stencil is black
		bool tmpLocked = false, stencilLocked = false, dstLocked = false;
		if (SDL_MUSTLOCK(tmpTexture)) tmpLocked = SDL_LockSurface(tmpTexture) == 0;
		if (SDL_MUSTLOCK(stencil->Tex)) stencilLocked = SDL_LockSurface(stencil->Tex) == 0;
		if (SDL_MUSTLOCK(drawTarget->Tex)) dstLocked = SDL_LockSurface(drawTarget->Tex) == 0;

		Uint32 swhitethreshold = SDL_MapRGBA(stencil->Tex->format,
			pd_api_gfx_color_whitetreshold.r, pd_api_gfx_color_whitetreshold.g,
			pd_api_gfx_color_whitetreshold.b, pd_api_gfx_color_whitetreshold.a);

		int bw = tmpTexture->w;
		int bh = tmpTexture->h;
		for (int sy = 0; sy < bh; sy++)
		{
			int dy = dstrect.y + sy;
			if (dy < 0 || dy >= drawTarget->Tex->h) continue;
			for (int sx = 0; sx < bw; sx++)
			{
				int dx = dstrect.x + sx;
				if (dx < 0 || dx >= drawTarget->Tex->w) continue;
				SDL_Rect clip;
				SDL_GetClipRect(drawTarget->Tex, &clip);
				if (dx < clip.x || dx >= clip.x + clip.w) continue;
				if (dy < clip.y || dy >= clip.y + clip.h) continue;

				int stx = dx;
				int sty = dy;
				Uint32* sp = (Uint32*)((Uint8*)stencil->Tex->pixels
					+ sty * stencil->Tex->pitch
					+ stx * stencil->Tex->format->BytesPerPixel);
				if (*sp <= swhitethreshold)
					continue; // stencil black = don't write

				Uint32* tp = (Uint32*)((Uint8*)tmpTexture->pixels
					+ sy * tmpTexture->pitch
					+ sx * tmpTexture->format->BytesPerPixel);
				if (*tp == cclear) continue; // transparent source pixel

				Uint32* dp = (Uint32*)((Uint8*)drawTarget->Tex->pixels
					+ dy * drawTarget->Tex->pitch
					+ dx * drawTarget->Tex->format->BytesPerPixel);
				*dp = *tp;
			}
		}

		if (tmpLocked) SDL_UnlockSurface(tmpTexture);
		if (stencilLocked) SDL_UnlockSurface(stencil->Tex);
		if (dstLocked) SDL_UnlockSurface(drawTarget->Tex);
	}
	else
	{
		SDL_BlitSurface(tmpTexture, NULL, drawTarget->Tex, &dstrect);
	}

    if (drawTarget->Mask)
    {
		bool textureUnlocked = true;
		if (SDL_MUSTLOCK(tmpTexture))
			textureUnlocked = SDL_LockSurface(tmpTexture) == 0;
		if (textureUnlocked) 
		{
			bool targetTextureUnlocked = true;
			if SDL_MUSTLOCK(drawTarget->Mask->Tex)
					targetTextureUnlocked = SDL_LockSurface(drawTarget->Mask->Tex) == 0;
		
			if(targetTextureUnlocked)
			{
				const Uint32 clear = SDL_MapRGBA(tmpTexture->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
				const Uint32 alpha = SDL_MapRGBA(tmpTexture->format,0,0,0,0);

				SDL_Surface *maskTex = drawTarget->Mask->Tex;
				const Uint32 white = SDL_MapRGBA(maskTex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
				SDL_Rect cliprecttarget;
				SDL_GetClipRect(drawTarget->Tex, &cliprecttarget);
				int startx = x + _pd_api_gfx_CurrentGfxContext->drawoffsetx;
				int starty = y + _pd_api_gfx_CurrentGfxContext->drawoffsety;

				int clip_x2 = cliprecttarget.x + cliprecttarget.w;
				int clip_y2 = cliprecttarget.y + cliprecttarget.h;

				int ystart = std::max(starty, cliprecttarget.y);
				int yend = std::min(starty + tmpTexture->h, std::min(maskTex->h, clip_y2));

				int xstart = std::max(startx, cliprecttarget.x);
				int xend = std::min(startx + tmpTexture->w, std::min(maskTex->w, clip_x2));


				for (int yy = ystart; yy < yend; yy++)
				{
					for (int xx = xstart; xx < xend; xx++)
					{
						const Uint32 *p  = (Uint32*)((Uint8*)tmpTexture->pixels + ((yy - starty) * tmpTexture->pitch) + ((xx - startx) * tmpTexture->format->BytesPerPixel));
						Uint32 *p2 = (Uint32*)((Uint8*)maskTex->pixels + (yy * maskTex->pitch) + (xx * maskTex->format->BytesPerPixel));
						const Uint32 pval = *p;
						if (pval != alpha && pval != clear)
							*p2 = white;
					}
				}
				if(SDL_MUSTLOCK(drawTarget->Mask->Tex))
					SDL_UnlockSurface(drawTarget->Mask->Tex);
			}
			if(SDL_MUSTLOCK(tmpTexture))
				SDL_UnlockSurface(tmpTexture);
		}
	}






    // if (drawTarget->Mask)
    // {
    //     const Uint32 clear = SDL_MapRGBA(tmpTexture->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
    //     const Uint32 alpha = SDL_MapRGBA(tmpTexture->format,0,0,0,0);

    //     SDL_Surface *maskTex = drawTarget->Mask->Tex;
    //     const Uint32 white = SDL_MapRGBA(maskTex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);

    //     const int srcx = x - _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    //     const int srcy = y - _pd_api_gfx_CurrentGfxContext->drawoffsety;

    //     const int width = std::min(tmpTexture->w, dstrect.w);
    //     const int height = std::min(tmpTexture->h, dstrect.h);
    //     for (int yy = 0; (yy < height); yy++)
    //     {
    //         for(int xx = 0; (xx < width); xx++)
    //         {
    //             const Uint32 *p = (Uint32*)((Uint8 *)tmpTexture->pixels + ((yy + srcy) * tmpTexture->pitch) + ((xx + srcx) * tmpTexture->format->BytesPerPixel));
    //             Uint32 *p2 = (Uint32*)((Uint8 *)maskTex->pixels + ((yy + dstrect.y) * maskTex->pitch) + ((xx + dstrect.x) * maskTex->format->BytesPerPixel));
    //             const Uint32 pval = *p;
    //             if (pval != alpha && pval != clear)
    //                 *p2 = white;
    //         }
    //     }
    // }


	if(!isOrginalTexture)
		SDL_FreeSurface(tmpTexture);
	
	_pd_api_gfx_getDrawTarget()->BitmapDirty = true;
}

void pd_api_gfx_drawRotatedBitmap(LCDBitmap* bitmap, int x, int y, float rotation, float centerx, float centery, float xscale, float yscale)
{
    if (bitmap == NULL)
        return;
    
    _pd_api_gfx_drawBitmapAll(bitmap, x, y,xscale, yscale, true, rotation, centerx, centery, kBitmapUnflipped, false, false);
}

void pd_api_gfx_drawBitmap(LCDBitmap* bitmap, int x, int y, LCDBitmapFlip flip)
{
    if (bitmap == NULL)
        return;
    // Draw to API layer first so it accumulates persistently over the framebuffer
    if (_pd_api_gfx_beginLayerDraw())
    {
        _pd_api_gfx_drawBitmapAll(bitmap, x, y, 1.0f, 1.0f, false, 0, 0, 0, flip, false, false);
        _pd_api_gfx_endLayerDraw();
    }
   
    _pd_api_gfx_drawBitmapAll(bitmap, x, y, 1.0f, 1.0f, false, 0, 0, 0, flip, false, false);
}

void pd_api_gfx_tileBitmap(LCDBitmap* bitmap, int x, int y, int width, int height, LCDBitmapFlip flip)
{
    const LCDBitmapDrawMode oldDrawMode = _pd_api_gfx_CurrentGfxContext->BitmapDrawMode;
    _pd_api_gfx_CurrentGfxContext->BitmapDrawMode = kDrawModeCopy;

    LCDBitmap *texture = pd_api_gfx_newBitmap(width, height, kColorClear);
    pd_api_gfx_pushContext(texture);
    for (int j = 0; j < height; j += bitmap->h)
    {
        for (int i = 0; i < width; i += bitmap->w)
        {
            _pd_api_gfx_drawBitmapAll(bitmap, i, j, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped, false, true);
        }
    }
    pd_api_gfx_popContext();
    _pd_api_gfx_CurrentGfxContext->BitmapDrawMode = oldDrawMode;
    _pd_api_gfx_drawBitmapAll(texture, x, y, 1.0f, 1.0f, false, 0, 0, 0, flip, false, false);
    pd_api_gfx_freeBitmap(texture);
}

void pd_api_gfx_drawScaledBitmap(LCDBitmap* bitmap, int x, int y, float xscale, float yscale)
{
    if (bitmap == NULL)
        return;

    _pd_api_gfx_drawBitmapAll(bitmap, x, y, xscale, yscale, false, 0, 0, 0, kBitmapUnflipped, false, false);
}



void pd_api_gfx_drawLine(int x1, int y1, int x2, int y2, int linewidth, LCDColor color)
{
	if(linewidth < 1)
		linewidth = 1;


	LCDBitmap *pattern = NULL;
	int minx;
    int maxx;
    int miny;
    int maxy;
	int width;
    int height;
	int drawLineWidth = make_even(linewidth+1);
	int halfdrawLineWidth = drawLineWidth >> 1;
	switch (color)
    {
        case kColorBlack:
            break;
        case kColorWhite:
            break;
        case kColorXOR:
            break;
        case kColorClear:
            break;
		default:
			//assume lcd pattern
			pattern = pd_api_gfx_PatternToBitmap((uint8_t*)color);
		break;
    }

    LCDBitmap *mask = _pd_api_gfx_getDrawTarget()->Mask;
    //for xor we are abusing the api to draw on a bitmap and then draw that bitmap on the current target using xor mode
    LCDBitmap *bitmap = NULL;
    if (color == kColorXOR || pattern != NULL)
    {
        minx = std::min(x1,x2);
        maxx = std::max(x1,x2);
        miny = std::min(y1,y2);
        maxy = std::max(y1,y2);

        width = maxx-minx + drawLineWidth;
        height = maxy-miny + drawLineWidth;
        
        x1 = x1 - minx + halfdrawLineWidth;
        x2 = x2 - (minx) + halfdrawLineWidth;
        y1 = y1 - (miny) + halfdrawLineWidth;
        y2 = y2 - (miny) + halfdrawLineWidth; 
		if(color == kColorXOR)
		{
        	bitmap = Api->graphics->newBitmap(width+1, height+1, kColorClear);
        	Api->graphics->pushContext(bitmap);
		}
    }
	else
	{
		x1 += _pd_api_gfx_CurrentGfxContext->drawoffsetx;
		y1 += _pd_api_gfx_CurrentGfxContext->drawoffsety;
		x2 += _pd_api_gfx_CurrentGfxContext->drawoffsetx;
		y2 += _pd_api_gfx_CurrentGfxContext->drawoffsety;
	}
	
	if(pattern)
	{
		int yoffset = miny % 8;
		int xoffset = minx % 8;
		int tilesx = (width /  8)+2;
		int tilesy = (height / 8)+2;
       	bitmap = Api->graphics->newBitmap(width + 1, height +1, kColorClear);
		bitmap->Mask = pd_api_gfx_newBitmap(bitmap->w, bitmap->h, kColorBlack);	
		pd_api_gfx_pushDrawOffset();
		Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
		Api->graphics->setDrawMode(kDrawModeCopy);
		Api->graphics->pushContext(bitmap);	
#ifdef MASKPRIMITIVES        
		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
		 		_pd_api_gfx_drawBitmapAll(pattern, (xx*8) -std::abs(xoffset), (yy*8) -std::abs(yoffset), 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped,false,true);
#else
		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
			{
				SDL_Rect dst;
				dst.x = xx*8 -std::abs(xoffset);
				dst.y = yy*8 -std::abs(yoffset);
				dst.w = 8;
				dst.h = 8;
				SDL_BlitSurface(pattern->Tex, NULL, bitmap->Tex, &dst);
			}
#endif		
		Api->graphics->popContext();
#ifdef MASKPRIMITIVES			
		SDL_FillRect(bitmap->Mask->Tex, NULL, SDL_MapRGBA(bitmap->Mask->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a));
#endif		
		drawLineRGBASurfacePlaydate(bitmap->Mask->Tex, x1, y1, x2, y2, linewidth, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);		
		bitmap->BitmapDirty = true;
		pd_api_gfx_popDrawOffset();
		Api->graphics->drawBitmap(bitmap,minx -halfdrawLineWidth,  miny -halfdrawLineWidth, kBitmapUnflipped);
		Api->graphics->popContext();
		Api->graphics->freeBitmap(bitmap);
		Api->graphics->freeBitmap(pattern);
	}
	else
	{
    
		SDL_Color maskColor = pd_api_gfx_color_white;
		switch (color)
		{
			case kColorBlack:
				drawLineRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, x1, y1, x2, y2, linewidth, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
				break;
			case kColorWhite:
				drawLineRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, x1, y1, x2, y2, linewidth, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
				break;
			case kColorXOR:
				drawLineRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, x1, y1, x2, y2, linewidth, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
				break;
			case kColorClear:
				drawLineRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, x1, y1, x2, y2, linewidth, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
				maskColor = pd_api_gfx_color_black;
				break;
			default:
				break;
		}
		if (mask && color != kColorXOR)
			drawLineRGBASurfacePlaydate(mask->Tex, x1, y1, x2, y2, linewidth, maskColor.r, maskColor.g, maskColor.b, maskColor.a);
	}

    if (color == kColorXOR)
    {
        Api->graphics->popContext();
		pd_api_gfx_pushDrawOffset();
        Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
        Api->graphics->setDrawMode(kDrawModeXOR);
		pd_api_gfx_popDrawOffset();
        _pd_api_gfx_drawBitmapAll(bitmap, minx -halfdrawLineWidth, miny -halfdrawLineWidth, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped, true,true);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }
	_pd_api_gfx_getDrawTarget()->BitmapDirty = true;
}

void pd_api_gfx_fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, LCDColor color)
{
    SDL_Point points[4];
   
    points[0].x = x1;
    points[0].y = y1;
    points[1].x = x2;
    points[1].y = y2;
    points[2].x = x3;
    points[2].y = y3;
    points[3].x = x1;
    points[3].y = y1;
    
	LCDBitmap *pattern = NULL;
    LCDBitmap *mask = _pd_api_gfx_getDrawTarget()->Mask;
    LCDBitmap *bitmap = NULL;
    int minx;
    int maxx;
    int miny;
    int maxy;
	int width;
	int height;
    switch (color)
    {
        case kColorBlack:
            break;
        case kColorWhite:
            break;
        case kColorXOR:
            break;
        case kColorClear:
            break;
		default:
			//assume lcd pattern
			pattern = pd_api_gfx_PatternToBitmap((uint8_t*)color);
		break;
    }
    
	if (color == kColorXOR || pattern != NULL)
    {
        minx = std::min(std::min(points[0].x,points[1].x),points[2].x);
        maxx = std::max(std::max(points[0].x,points[1].x),points[2].x);
        miny = std::min(std::min(points[0].y,points[1].y),points[2].y);
        maxy = std::max(std::max(points[0].y,points[1].y),points[2].y);
        
        width = maxx-minx;
        height = maxy-miny;
        
        points[0].x -= minx;
        points[1].x -= minx;
        points[2].x -= minx;
        points[3].x -= minx;

        points[0].y -= miny;
        points[1].y -= miny;
        points[2].y -= miny;
        points[3].y -= miny;
		if(color == kColorXOR)
		{
        	bitmap = Api->graphics->newBitmap(width+1, height+1, kColorClear);
        	Api->graphics->pushContext(bitmap);
		}
    }
	else
	{
		points[0].x += _pd_api_gfx_CurrentGfxContext->drawoffsetx;
		points[1].x += _pd_api_gfx_CurrentGfxContext->drawoffsetx;
		points[2].x += _pd_api_gfx_CurrentGfxContext->drawoffsetx;
		points[3].x += _pd_api_gfx_CurrentGfxContext->drawoffsetx;
		
		points[0].y += _pd_api_gfx_CurrentGfxContext->drawoffsety;
		points[1].y += _pd_api_gfx_CurrentGfxContext->drawoffsety;
		points[2].y += _pd_api_gfx_CurrentGfxContext->drawoffsety;
		points[3].y += _pd_api_gfx_CurrentGfxContext->drawoffsety;
	}

	if(pattern)
	{
		int yoffset = miny % 8;
		int xoffset = minx % 8;
		int tilesx = (width /  8)+2;
		int tilesy = (height / 8)+2;
       	bitmap = Api->graphics->newBitmap(width + 1, height +1, kColorClear);
		bitmap->Mask = pd_api_gfx_newBitmap(bitmap->w, bitmap->h, kColorBlack);
		pd_api_gfx_pushDrawOffset();
		Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
		Api->graphics->setDrawMode(kDrawModeCopy);
		Api->graphics->pushContext(bitmap);	
#ifdef MASKPRIMITIVES        
		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
				_pd_api_gfx_drawBitmapAll(pattern, (xx*8) -std::abs(xoffset), (yy*8) -std::abs(yoffset), 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped,false,true);
#else

		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
			{
				SDL_Rect dst;
				dst.x = xx*8 -std::abs(xoffset);
				dst.y = yy*8 -std::abs(yoffset);
				dst.w = 8;
				dst.h = 8;
				SDL_BlitSurface(pattern->Tex, NULL, bitmap->Tex, &dst);
			}
#endif		
		Api->graphics->popContext();
#ifdef MASKPRIMITIVES
		SDL_FillRect(bitmap->Mask->Tex, NULL, SDL_MapRGBA(bitmap->Mask->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a));
#endif		
		filledTrigonRGBASurface(bitmap->Mask->Tex, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
		bitmap->BitmapDirty = true;
		pd_api_gfx_popDrawOffset();
		Api->graphics->drawBitmap(bitmap,minx ,  miny , kBitmapUnflipped);
		Api->graphics->popContext();
		Api->graphics->freeBitmap(bitmap);
		Api->graphics->freeBitmap(pattern);
	}
	else
	{
		SDL_Color maskColor = pd_api_gfx_color_white;
		switch (color)
		{
			case kColorBlack:
				filledTrigonRGBASurface(_pd_api_gfx_getDrawTarget()->Tex, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
				break;
			case kColorWhite:
				filledTrigonRGBASurface(_pd_api_gfx_getDrawTarget()->Tex, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
				break;
			case kColorXOR:
				filledTrigonRGBASurface(_pd_api_gfx_getDrawTarget()->Tex, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
				break;
			case kColorClear:
				filledTrigonRGBASurface(_pd_api_gfx_getDrawTarget()->Tex, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
				maskColor = pd_api_gfx_color_black;
				break;
			default:
				break;
		}
		if (mask && color != kColorXOR)
			filledTrigonRGBASurface(mask->Tex, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, maskColor.r, maskColor.g, maskColor.b, maskColor.a);
	}

    if (color == kColorXOR)
    {
        Api->graphics->popContext();
		pd_api_gfx_pushDrawOffset();
        Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
        Api->graphics->setDrawMode(kDrawModeXOR);
		pd_api_gfx_popDrawOffset();
        _pd_api_gfx_drawBitmapAll(bitmap, minx , miny , 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped, true,true);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }
	_pd_api_gfx_getDrawTarget()->BitmapDirty = true;
}

void pd_api_gfx_drawRect(int x, int y, int width, int height, LCDColor color)
{
    LCDBitmap *mask = _pd_api_gfx_getDrawTarget()->Mask;
    //for xor we are abusing the api to draw on a bitmap and then draw that bitmap on the current target using xor mode
    LCDBitmap *bitmap = NULL;
	LCDBitmap *pattern = NULL;
    switch (color)
    {
        case kColorBlack:
            break;
        case kColorWhite:
            break;
        case kColorXOR:
            break;
        case kColorClear:
            break;
		default:
			//assume lcd pattern
			pattern = pd_api_gfx_PatternToBitmap((uint8_t*)color);
		break;
    }
    
	if (color == kColorXOR)
    {
        bitmap = Api->graphics->newBitmap(width, height, kColorClear);
        Api->graphics->pushContext(bitmap);
    }
    

    SDL_Rect rect;
    rect.x = color == kColorXOR || pattern != NULL ? 0: x + _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    rect.y = color == kColorXOR || pattern != NULL? 0: y + _pd_api_gfx_CurrentGfxContext->drawoffsety;
    rect.w = width;
    rect.h = height;
    if(pattern)
	{
		int yoffset = y % 8;
		int xoffset = x % 8;
		int tilesx = (width /  8)+2;
		int tilesy = (height / 8)+2;
       	bitmap = Api->graphics->newBitmap(width + 1, height +1, kColorClear);
		bitmap->Mask = pd_api_gfx_newBitmap(bitmap->w, bitmap->h, kColorBlack);
		pd_api_gfx_pushDrawOffset();
		Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
		Api->graphics->setDrawMode(kDrawModeCopy);
		Api->graphics->pushContext(bitmap);	
#ifdef MASKPRIMITIVES        
		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
				_pd_api_gfx_drawBitmapAll(pattern, (xx*8) -std::abs(xoffset), (yy*8) -std::abs(yoffset), 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped,false,true);
#else

		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
			{
				SDL_Rect dst;
				dst.x = xx*8 -std::abs(xoffset);
				dst.y = yy*8 -std::abs(yoffset);
				dst.w = 8;
				dst.h = 8;
				SDL_BlitSurface(pattern->Tex, NULL, bitmap->Tex, &dst);
			}
#endif		
		Api->graphics->popContext();
#ifdef MASKPRIMITIVES		
		SDL_FillRect(bitmap->Mask->Tex, NULL, SDL_MapRGBA(bitmap->Mask->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a));
#endif		
		rectangleRGBASurface(bitmap->Mask->Tex, rect.x, rect.y, rect.x + rect.w-1, rect.y + rect.h-1, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
		bitmap->BitmapDirty = true;
		pd_api_gfx_popDrawOffset();
		Api->graphics->drawBitmap(bitmap,x ,  y , kBitmapUnflipped);
		Api->graphics->popContext();
		Api->graphics->freeBitmap(bitmap);
		Api->graphics->freeBitmap(pattern);
	}
	else
	{
		SDL_Color maskColor = pd_api_gfx_color_white;
		switch (color)
		{
			case kColorBlack:
				rectangleRGBASurface(_pd_api_gfx_getDrawTarget()->Tex, rect.x, rect.y, rect.x + rect.w-1, rect.y + rect.h-1, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
				break;
			case kColorWhite:
				rectangleRGBASurface(_pd_api_gfx_getDrawTarget()->Tex, rect.x, rect.y, rect.x + rect.w-1, rect.y + rect.h-1, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
				break;
			case kColorXOR:
				rectangleRGBASurface(_pd_api_gfx_getDrawTarget()->Tex, rect.x, rect.y, rect.x + rect.w-1, rect.y + rect.h-1, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
				break;
			case kColorClear:
				rectangleRGBASurface(_pd_api_gfx_getDrawTarget()->Tex, rect.x, rect.y, rect.x + rect.w-1, rect.y + rect.h-1, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
				maskColor = pd_api_gfx_color_black;
				break;
			default:
				break;
		}
		if (mask && color != kColorXOR)
			rectangleRGBASurface(mask->Tex, rect.x, rect.y, rect.x + rect.w-1, rect.y + rect.h-1, maskColor.r, maskColor.g, maskColor.b, maskColor.a);
	}
	
    if (color == kColorXOR)
    {
        Api->graphics->popContext();
		pd_api_gfx_pushDrawOffset();
        Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
        Api->graphics->setDrawMode(kDrawModeXOR);
		pd_api_gfx_popDrawOffset();
        _pd_api_gfx_drawBitmapAll(bitmap, x, y, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped, true,true);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }
	_pd_api_gfx_getDrawTarget()->BitmapDirty = true;
}

void pd_api_gfx_fillRect(int x, int y, int width, int height, LCDColor color)
{
    // Draw to API layer first
    if (_pd_api_gfx_beginLayerDraw())
    {
        pd_api_gfx_fillRect(x, y, width, height, color); // recurse with layer as target
        _pd_api_gfx_endLayerDraw();
    }
	LCDBitmap *pattern = NULL;
    SDL_Color maskColor = pd_api_gfx_color_white;
	Uint32 RealColor;
	switch (color)
    {
        case kColorBlack:
            RealColor = SDL_MapRGBA(_pd_api_gfx_getDrawTarget()->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            RealColor = SDL_MapRGBA(_pd_api_gfx_getDrawTarget()->Tex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            RealColor = SDL_MapRGBA(_pd_api_gfx_getDrawTarget()->Tex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            RealColor = SDL_MapRGBA(_pd_api_gfx_getDrawTarget()->Tex->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            maskColor = pd_api_gfx_color_black;
            break;
        default:
			//assume lcd pattern
			RealColor = 0;
			pattern = pd_api_gfx_PatternToBitmap((uint8_t*)color);
			break;
    }

    LCDBitmap *mask = _pd_api_gfx_getDrawTarget()->Mask;
    //for xor we are abusing the api to draw on a bitmap and then draw that bitmap on the current target using xor mode
    LCDBitmap *bitmap = NULL;
    if (color == kColorXOR)
    {
        bitmap = Api->graphics->newBitmap(width, height, kColorClear);
        Api->graphics->pushContext(bitmap);
    }
    
	if(pattern)
	{
		int yoffset = y % 8;
		int xoffset = x % 8;
		int tilesx = (width /  8)+2;
		int tilesy = (height / 8)+2;
       	bitmap = Api->graphics->newBitmap(width, height, kColorClear);
		bitmap->Mask = pd_api_gfx_newBitmap(bitmap->w, bitmap->h, kColorWhite);	
		pd_api_gfx_pushDrawOffset();
		Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
		Api->graphics->setDrawMode(kDrawModeCopy);
		Api->graphics->pushContext(bitmap);	
#ifdef MASKPRIMITIVES        
		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
		 		_pd_api_gfx_drawBitmapAll(pattern, (xx*8) -std::abs(xoffset), (yy*8) -std::abs(yoffset), 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped,false,true);
#else

		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
			{
				SDL_Rect dst;
				dst.x = xx*8 -std::abs(xoffset);
				dst.y = yy*8 -std::abs(yoffset);
				dst.w = 8;
				dst.h = 8;
				SDL_BlitSurface(pattern->Tex, NULL, bitmap->Tex, &dst);
			}
#endif		
		Api->graphics->popContext();
		bitmap->BitmapDirty = true;
		pd_api_gfx_popDrawOffset();
		Api->graphics->drawBitmap(bitmap,x ,  y , kBitmapUnflipped);
		
		Api->graphics->popContext();
		Api->graphics->freeBitmap(bitmap);
		Api->graphics->freeBitmap(pattern);
	}
    else
	{
		SDL_Rect rect;
    	rect.x = color == kColorXOR ? 0: x + _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    	rect.y = color == kColorXOR ? 0: y + _pd_api_gfx_CurrentGfxContext->drawoffsety;
    	rect.w = width;
    	rect.h = height;

    	SDL_FillRect(_pd_api_gfx_getDrawTarget()->Tex, &rect, RealColor);
        if (mask && color != kColorXOR)
            SDL_FillRect(mask->Tex, &rect, SDL_MapRGBA(mask->Tex->format, maskColor.r, maskColor.g, maskColor.b, maskColor.a));
	}

	if (color == kColorXOR)
    {
        Api->graphics->popContext();
		pd_api_gfx_pushDrawOffset();
        Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());		
        Api->graphics->setDrawMode(kDrawModeXOR);
		pd_api_gfx_popDrawOffset();
        _pd_api_gfx_drawBitmapAll(bitmap, x, y, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped, true,true);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }
	_pd_api_gfx_getDrawTarget()->BitmapDirty = true;
    // If this is a full-screen fill on the main screen, sync to pd_gfx_framebuffer
    // so getFrame() returns a buffer that reflects the clear. Games that clear using
    // fillRect instead of clear() (e.g. PastaWipeout) need this.
    if (pd_gfx_framebuffer_valid &&
        _pd_api_gfx_getDrawTarget() == _pd_api_gfx_Playdate_Screen &&
        x <= 0 && y <= 0 &&
        width  >= LCD_COLUMNS &&
        height >= LCD_ROWS)
    {
        // Determine fill value: kColorBlack=0x00, kColorWhite=0xFF, others skip
        if (color == (LCDColor)kColorBlack)
            memset(pd_gfx_framebuffer, 0x00, sizeof(pd_gfx_framebuffer));
        else if (color == (LCDColor)kColorWhite)
            memset(pd_gfx_framebuffer, 0xFF, sizeof(pd_gfx_framebuffer));
        // Full-screen fill resets the frame — same logic as clear()
        // Always reset got_frame on clear — game is starting a new render pass.
        // If it needs framebuffer push it will call markUpdatedRows.
        pd_gfx_framebuffer_got_frame = false;
    }
}

void pd_api_gfx_drawEllipse(int x, int y, int width, int height, int lineWidth, float startAngle, float endAngle, LCDColor color) // stroked inside the rect
{
	if(width <= 0 || height <= 0)
		return;
    LCDBitmap *mask = _pd_api_gfx_getDrawTarget()->Mask;
    //for xor we are abusing the api to draw on a bitmap and then draw that bitmap on the current target using xor mode
    LCDBitmap *bitmap = NULL;
    LCDBitmap *pattern = NULL;
    switch (color)
    {
        case kColorBlack:
            break;
        case kColorWhite:
            break;
        case kColorXOR:
            break;
        case kColorClear:
            break;
		default:
			//assume lcd pattern
			pattern = pd_api_gfx_PatternToBitmap((uint8_t*)color);
		break;
    }
	
	if (color == kColorXOR)
    {
        bitmap = Api->graphics->newBitmap(width+1, height+1, kColorClear);
        Api->graphics->pushContext(bitmap);
    }
    int oldx = x;
	int oldy = y;
    x = color == kColorXOR || pattern != NULL ? 0: x + _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    y = color == kColorXOR || pattern != NULL ? 0: y + _pd_api_gfx_CurrentGfxContext->drawoffsety;
    if(pattern)
	{
		int yoffset = oldy % 8;
		int xoffset = oldx % 8;
		int tilesx = ((width+1) / 8)+2;
		int tilesy = ((height+1) / 8)+2;
       	bitmap = Api->graphics->newBitmap(width +  1, height + 1, kColorClear);
		bitmap->Mask = pd_api_gfx_newBitmap(bitmap->w, bitmap->h, kColorBlack);
		pd_api_gfx_pushDrawOffset();
		Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
		Api->graphics->setDrawMode(kDrawModeCopy);
		Api->graphics->pushContext(bitmap);	
#ifdef MASKPRIMITIVES        
		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
		 		_pd_api_gfx_drawBitmapAll(pattern, (xx*8) -std::abs(xoffset), (yy*8) -std::abs(yoffset), 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped,false,true);
#else

		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
			{
				SDL_Rect dst;
				dst.x = xx*8 -std::abs(xoffset);
				dst.y = yy*8 -std::abs(yoffset);
				dst.w = 8;
				dst.h = 8;
				SDL_BlitSurface(pattern->Tex, NULL, bitmap->Tex, &dst);
			}
#endif		
		Api->graphics->popContext();
#ifdef MASKPRIMITIVES		
		SDL_FillRect(bitmap->Mask->Tex, NULL, SDL_MapRGBA(bitmap->Mask->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a));
#endif		
		drawEllipseRGBASurfacePlaydate(bitmap->Mask->Tex, x, y, width, height, lineWidth, startAngle, endAngle, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a, false);
		bitmap->BitmapDirty = true;
		pd_api_gfx_popDrawOffset();
		Api->graphics->drawBitmap(bitmap,oldx, oldy, kBitmapUnflipped);
		Api->graphics->popContext();
		Api->graphics->freeBitmap(bitmap);
		Api->graphics->freeBitmap(pattern);
	}
	else
	{
		SDL_Color maskColor = pd_api_gfx_color_white;
		switch (color)
		{
			case kColorBlack:
				drawEllipseRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, x, y, width, height, lineWidth, startAngle, endAngle, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a, false);
				break;
			case kColorWhite:
				drawEllipseRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, x, y, width, height, lineWidth, startAngle, endAngle, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a, false);
				break;
			case kColorXOR:
				drawEllipseRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, x, y, width, height,  lineWidth, startAngle, endAngle, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a, false);
				break;
			case kColorClear:
				drawEllipseRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, x, y, width, height,  lineWidth, startAngle, endAngle, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a, false);
				maskColor = pd_api_gfx_color_black;
				break;
			default:
				break;
		}
		if (mask && color != kColorXOR)
			drawEllipseRGBASurfacePlaydate(mask->Tex, x, y, width, height, lineWidth, startAngle, endAngle, maskColor.r, maskColor.g, maskColor.b, maskColor.a, false);
	}

    if (color == kColorXOR)
    {
        Api->graphics->popContext();
		pd_api_gfx_pushDrawOffset();
        Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
        Api->graphics->setDrawMode(kDrawModeXOR);
		pd_api_gfx_popDrawOffset();
        _pd_api_gfx_drawBitmapAll(bitmap, oldx, oldy, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped, true,true);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);        
    }
	_pd_api_gfx_getDrawTarget()->BitmapDirty = true;
}

void pd_api_gfx_fillEllipse(int x, int y, int width, int height, float startAngle, float endAngle, LCDColor color)
{
	if(width <= 0 || height <= 0)
		return;
    LCDBitmap *mask = _pd_api_gfx_getDrawTarget()->Mask;
    //for xor we are abusing the api to draw on a bitmap and then draw that bitmap on the current target using xor mode
    LCDBitmap *bitmap = NULL;
	LCDBitmap *pattern = NULL;
    switch (color)
    {
        case kColorBlack:
            break;
        case kColorWhite:
            break;
        case kColorXOR:
            break;
        case kColorClear:
            break;
		default:
			//assume lcd pattern
			pattern = pd_api_gfx_PatternToBitmap((uint8_t*)color);
		break;
    }
    if (color == kColorXOR)
    {
        bitmap = Api->graphics->newBitmap(width+1, height+1, kColorClear);
        Api->graphics->pushContext(bitmap);
    }

    int oldx = x;
    int oldy = y;

    x = color == kColorXOR || pattern != NULL ? 0 : x + _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    y = color == kColorXOR || pattern != NULL ? 0 : y + _pd_api_gfx_CurrentGfxContext->drawoffsety;
	if(pattern)
	{
		int yoffset = oldy % 8;
		int xoffset = oldx % 8;
		int tilesx = (width /  8)+2;
		int tilesy = (height / 8)+2;
       	bitmap = Api->graphics->newBitmap(width + 1, height +1, kColorClear);
		bitmap->Mask = pd_api_gfx_newBitmap(bitmap->w, bitmap->h, kColorBlack);
		pd_api_gfx_pushDrawOffset();
		Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
		Api->graphics->setDrawMode(kDrawModeCopy);
		Api->graphics->pushContext(bitmap);	
#ifdef MASKPRIMITIVES        
		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
				_pd_api_gfx_drawBitmapAll(pattern, (xx*8) -std::abs(xoffset), (yy*8) -std::abs(yoffset), 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped,false,true);
#else

		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
			{
				SDL_Rect dst;
				dst.x = xx*8 -std::abs(xoffset);
				dst.y = yy*8 -std::abs(yoffset);
				dst.w = 8;
				dst.h = 8;
				SDL_BlitSurface(pattern->Tex, NULL, bitmap->Tex, &dst);
			}
#endif		
		Api->graphics->popContext();
#ifdef MASKPRIMITIVES
		SDL_FillRect(bitmap->Mask->Tex, NULL, SDL_MapRGBA(bitmap->Mask->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a));
#endif		
		drawEllipseRGBASurfacePlaydate(bitmap->Mask->Tex, x, y, width, height, 0, startAngle, endAngle, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a, true);
		bitmap->BitmapDirty = true;
		pd_api_gfx_popDrawOffset();
		Api->graphics->drawBitmap(bitmap,oldx,  oldy, kBitmapUnflipped);
		Api->graphics->popContext();
		Api->graphics->freeBitmap(bitmap);
		Api->graphics->freeBitmap(pattern);
	}
	else
	{
		SDL_Color maskColor = pd_api_gfx_color_white;
		switch (color)
		{
			case kColorBlack:
				drawEllipseRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, x, y, width, height, 0, startAngle, endAngle, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a, true);
				break;
			case kColorWhite:
				drawEllipseRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, x, y, width, height, 0, startAngle, endAngle, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a, true);
				break;
			case kColorXOR:
				drawEllipseRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, x, y, width, height, 0, startAngle, endAngle, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a, true);
				break;
			case kColorClear:
				drawEllipseRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, x, y, width, height, 0, startAngle, endAngle, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a, true);
				maskColor = pd_api_gfx_color_black;
				break;
			default:
				break;
		}
		if (mask && color != kColorXOR)
			drawEllipseRGBASurfacePlaydate(mask->Tex, x, y, width, height, 0, startAngle, endAngle, maskColor.r, maskColor.g, maskColor.b, maskColor.a, true);
	}

    if (color == kColorXOR)
    {
        Api->graphics->popContext();
		pd_api_gfx_pushDrawOffset();
        Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
		pd_api_gfx_popDrawOffset();
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, oldx, oldy, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped, true,true);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);        
    }
	_pd_api_gfx_getDrawTarget()->BitmapDirty = true;
}


// LCDFont
LCDFont* pd_api_gfx_Create_LCDFont()
{
    LCDFont* tmp = new LCDFont();
    tmp->font = NULL;
    tmp->isFntFont = false;
    tmp->cellW = 0;
    tmp->cellH = 0;
    tmp->fntTracking = 0;
    return tmp;
}

// ---- Native .fnt font system ----

static uint32_t fnt_utf8_decode(const char* s, int* bytesConsumed)
{
    const uint8_t* p = (const uint8_t*)s;
    if (!p || !*p) { *bytesConsumed = 0; return 0; }
    if (p[0] < 0x80) { *bytesConsumed = 1; return p[0]; }
    if ((p[0] & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80)
        { *bytesConsumed = 2; return ((p[0]&0x1F)<<6)|(p[1]&0x3F); }
    if ((p[0] & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80)
        { *bytesConsumed = 3; return ((p[0]&0x0F)<<12)|((p[1]&0x3F)<<6)|(p[2]&0x3F); }
    if ((p[0] & 0xF8) == 0xF0)
        { *bytesConsumed = 4; return ((p[0]&0x07)<<18)|((p[1]&0x3F)<<12)|((p[2]&0x3F)<<6)|(p[3]&0x3F); }
    *bytesConsumed = 0; return 0xFFFD;
}

static std::vector<std::pair<uint32_t,int>> fnt_parse_file(const char* path, int* outTracking,
    std::unordered_map<uint64_t,int>* outKerning)
{
    std::vector<std::pair<uint32_t,int>> result;
    if (outTracking) *outTracking = 0;
    FILE* f = fopen(path, "r");
    if (!f) return result;
    char line[512];
    while (fgets(line, sizeof(line), f))
    {
        int len = (int)strlen(line);
        while (len > 0 && (line[len-1]=='\r'||line[len-1]=='\n')) line[--len]='\0';
        if (!len) continue;

        // Handle metadata lines like "tracking=1"
        char* firstTab = strchr(line, '\t');
        char* eq = strchr(line, '=');
        if (eq && (!firstTab || eq < firstTab) && eq > line && isalpha((unsigned char)*(eq-1)))
        {
            *eq = '\0';
            if (strcmp(line, "tracking") == 0 && outTracking)
                *outTracking = atoi(eq + 1);
            continue;
        }

        // Find separator: tab-separated (standard) or space-separated (some fonts)
        char* lastTab = strrchr(line, '\t');
        int advance;
        if (lastTab)
        {
            advance = atoi(lastTab + 1);
            *lastTab = '\0';
            int chLen = (int)strlen(line);
            while (chLen > 0 && line[chLen-1] == '\t')
                line[--chLen] = '\0';
            if (!chLen) continue;
        }
        else
        {
            // Space-separated: advance is last whitespace-delimited token
            int len2 = (int)strlen(line);
            while (len2 > 0 && isspace((unsigned char)line[len2-1])) line[--len2] = '\0';
            while (len2 > 0 && !isspace((unsigned char)line[len2-1])) len2--;
            if (len2 == 0) continue;
            advance = atoi(line + len2);
            while (len2 > 0 && isspace((unsigned char)line[len2-1])) line[--len2] = '\0';
            if (!len2) continue;
        }

        const char* ch = line;
        uint32_t cp;
        if (strncmp(ch, "space", 5) == 0 || (ch[0] == ' ' && ch[1] == '\0'))
        {
            cp = ' ';
            result.push_back({cp, advance});
        }
        else
        {
            int consumed = 0;
            cp = fnt_utf8_decode(ch, &consumed);
            if (consumed == 0) continue; // skip truly invalid UTF-8 only
            const char* rest = ch + consumed;
            // Strip variation selectors / ZWJ
            while (*rest)
            {
                int c2 = 0;
                uint32_t cp2 = fnt_utf8_decode(rest, &c2);
                if (cp2 >= 0xFE00 && cp2 <= 0xFE0F) { rest += c2; continue; }
                if (cp2 == 0x200D) { rest += c2; continue; }
                break;
            }

            if (!*rest)
            {
                // Single codepoint — regular glyph entry
                result.push_back({cp, advance});
            }
            else if (outKerning)
            {
                // Two codepoints — kerning pair
                int consumed2 = 0;
                uint32_t cp2 = fnt_utf8_decode(rest, &consumed2);
                uint64_t key = ((uint64_t)cp << 32) | (uint64_t)cp2;
                (*outKerning)[key] = advance;
            }
        }
    }
    fclose(f);
    return result;
}

static std::string fnt_find_table_png(const char* fntPath)
{
    std::string path(fntPath);
    size_t slash = path.rfind('/');
    std::string dir  = (slash == std::string::npos) ? "" : path.substr(0, slash+1);
    std::string base = (slash == std::string::npos) ? path : path.substr(slash+1);
    size_t dot = base.rfind('.');
    if (dot != std::string::npos) base = base.substr(0, dot);
    std::string prefix = base + "-table-";

    // Collect all matching PNGs - when multiple exist (e.g. font-table-8-19.png
    // and font-table-18-13.png), pick the one with most cells to cover all glyphs.
    std::vector<std::string> candidates;
    DIR* d = opendir(dir.empty() ? "." : dir.c_str());
    if (!d) return "";
    struct dirent* entry;
    while ((entry = readdir(d)) != nullptr)
    {
        std::string name(entry->d_name);
        if (name.find(prefix) == 0
            && name.size() > 4
            && name.substr(name.size()-4) == ".png")
            candidates.push_back(dir + name);
    }
    closedir(d);

    if (candidates.empty()) return "";
    if (candidates.size() == 1) return candidates[0];

    // Multiple matches: pick the one with largest cell count
    std::string best; int bestCells = -1;
    for (const auto& c : candidates)
    {
        size_t tp = c.find("-table-"); if (tp == std::string::npos) continue;
        const char* wp = c.c_str() + tp + 7;
        int cw = atoi(wp);
        const char* hp = strchr(wp, '-');
        int ch = hp ? atoi(hp + 1) : 0;
        if (cw <= 0 || ch <= 0) continue;
        // Read PNG dimensions from header without full decode
        FILE* pf = fopen(c.c_str(), "rb");
        if (!pf) continue;
        uint8_t hdr[24]; int read = (int)fread(hdr, 1, 24, pf); fclose(pf);
        if (read < 24) continue;
        // PNG IHDR: bytes 16-19 = width, 20-23 = height (big-endian)
        int pw = (hdr[16]<<24)|(hdr[17]<<16)|(hdr[18]<<8)|hdr[19];
        int ph = (hdr[20]<<24)|(hdr[21]<<16)|(hdr[22]<<8)|hdr[23];
        int cells = (pw / cw) * (ph / ch);
        if (cells > bestCells) { bestCells = cells; best = c; }
    }
    return best.empty() ? candidates[0] : best;
}

// Check if fnt file uses embedded base64 PNG format
static bool fnt_is_embedded(const char* fntPath)
{
    FILE* f = fopen(fntPath, "rb");
    if (!f) return false;
    // Check for binary file - if first byte is null or non-ASCII control char, it's binary
    unsigned char first;
    if (fread(&first, 1, 1, f) != 1 || first < 0x09 || (first < 0x20 && first != 0x0A && first != 0x0D))
    {
        fclose(f);
        return false; // binary fnt - skip
    }
    fclose(f);

    f = fopen(fntPath, "r");
    if (!f) return false;
    char line[8192]; // large enough for --metrics= line which can be >1KB
    bool foundDataLen = false, foundData = false;
    // Need both datalen= AND data= to confirm embedded format
    for (int i = 0; i < 20 && fgets(line, sizeof(line), f); i++)
    {
        if (strncmp(line, "datalen=", 8) == 0) { foundDataLen = true; continue; }
        if (strncmp(line, "data=",    5) == 0) { foundData    = true; break; }
        // Stop if we hit glyph entries without finding data= (tab or space-separated)
        if ((strchr(line, '\t') || (strchr(line, ' ') && !strchr(line, '='))) && strncmp(line, "tracking=", 9) != 0) break;
    }
    fclose(f);
    return foundDataLen && foundData;
}

// base64 decode table
static const int b64_table[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

static std::vector<uint8_t> b64_decode(const char* src, size_t srcLen)
{
    std::vector<uint8_t> out;
    out.reserve(srcLen * 3 / 4);
    int val = 0, valb = -8;
    for (size_t i = 0; i < srcLen; i++)
    {
        int c = b64_table[(unsigned char)src[i]];
        if (c < 0) continue;
        val = (val << 6) + c;
        valb += 6;
        if (valb >= 0)
        {
            out.push_back((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    return out;
}

// Load fnt with embedded base64 PNG
static LCDFont* fnt_load_embedded_font(const char* fntPath)
{
    std::string b64data;
    int cellW = 0, cellH = 0;
    int fntTracking = 0;
    std::unordered_map<uint64_t,int> kerningMap;
    std::vector<std::pair<uint32_t,int>> charList;
    // Parse JSON kerning from --metrics line
    auto parseMetricsKerning = [&](const char* metricsLine) {
        // Find "pairs":{ and extract kerning
        const char* pairs = strstr(metricsLine, "\"pairs\":{");
        if (!pairs) return;
        pairs += 9; // skip "pairs":{
        while (*pairs && *pairs != '}')
        {
            // Read "xy":[advance,...]
            if (*pairs != '"') { pairs++; continue; }
            pairs++; // skip "
            // Read two chars
            if (!pairs[0] || !pairs[1]) break;
            int consumed1 = 0, consumed2 = 0;
            uint32_t cp1 = fnt_utf8_decode(pairs, &consumed1);
            uint32_t cp2 = fnt_utf8_decode(pairs + consumed1, &consumed2);
            pairs += consumed1 + consumed2;
            if (*pairs != '"') continue;
            pairs++; // skip closing "
            // Skip to [
            while (*pairs && *pairs != '[') pairs++;
            if (*pairs != '[') break;
            pairs++;
            int adj = atoi(pairs);
            uint64_t key = ((uint64_t)cp1 << 32) | (uint64_t)cp2;
            kerningMap[key] = adj;
            // Skip to next entry
            while (*pairs && *pairs != ',' && *pairs != '}') pairs++;
        }
    };

    // Read entire file into memory to handle large data= lines
    // Open in binary mode to avoid Windows \r\n translation messing up ftell/fread
    FILE* fb = fopen(fntPath, "rb");
    if (!fb) 
	{
		printfDebug(DebugLoadPaths, "INFO: FAILED LOADING embedded font (fopen failed): %s\n", fntPath); 
		return nullptr;
	}
    fseek(fb, 0, SEEK_END);
    long fileSize = ftell(fb);
    fseek(fb, 0, SEEK_SET);
    std::vector<char> fileBuf(fileSize + 1, 0);
    fread(fileBuf.data(), 1, fileSize, fb);
    fclose(fb);

    // Split into lines
    std::vector<std::string> lines;
    {
        char* p = fileBuf.data();
        char* end = p + fileSize;
        while (p < end)
        {
            char* nl = (char*)memchr(p, '\n', end - p);
            size_t len = nl ? (size_t)(nl - p) : (size_t)(end - p);
            // Strip \r
            if (len > 0 && p[len-1] == '\r') len--;
            lines.push_back(std::string(p, len));
            p = nl ? nl + 1 : end;
        }
    }

    for (const auto& lineStr : lines)
    {
        const char* line = lineStr.c_str();
        if (!*line) continue;

        if (strncmp(line, "--metrics=", 10) == 0)
        {
            parseMetricsKerning(line + 10);
        }
        else if (strncmp(line, "datalen=", 8) == 0)
        {
            // skip
        }
        else if (strncmp(line, "data=", 5) == 0)
        {
            b64data = std::string(line + 5);
        }
        else if (strncmp(line, "width=", 6) == 0)
        {
            cellW = atoi(line + 6);
        }
        else if (strncmp(line, "height=", 7) == 0)
        {
            cellH = atoi(line + 7);
        }
        else if (strncmp(line, "tracking=", 9) == 0)
        {
            fntTracking = atoi(line + 9);
        }
        else if (strchr(line, '\t') != nullptr || strchr(line, ' ') != nullptr)
        {
            // Glyph entry: tab-separated (standard) or space-separated format
            std::string lineCopy(line);
            char* lineM = &lineCopy[0];
            char* lastTab = strrchr(lineM, '\t');
            int advance;
            if (lastTab)
            {
                advance = atoi(lastTab + 1);
                *lastTab = '\0';
                int chLen = (int)strlen(lineM);
                while (chLen > 0 && lineM[chLen-1] == '\t') lineM[--chLen] = '\0';
                if (!chLen) continue;
            }
            else
            {
                // Space-separated: last whitespace-delimited token is advance
                int len2 = (int)strlen(lineM);
                while (len2 > 0 && isspace((unsigned char)lineM[len2-1])) lineM[--len2] = '\0';
                while (len2 > 0 && !isspace((unsigned char)lineM[len2-1])) len2--;
                if (len2 == 0) continue;
                advance = atoi(lineM + len2);
                while (len2 > 0 && isspace((unsigned char)lineM[len2-1])) lineM[--len2] = '\0';
                if (!len2) continue;
            }

            const char* ch = lineM;
            uint32_t cp;
            if (strncmp(ch, "space", 5) == 0 || (ch[0]==' ' && ch[1]=='\0'))
            {
                cp = ' ';
            }
            else
            {
                int consumed = 0;
                cp = fnt_utf8_decode(ch, &consumed);
                // Only skip truly invalid UTF-8 (consumed==0), not U+FFFD which is a valid glyph
                if (consumed == 0) continue;
                // Skip kerning pairs
                const char* rest = ch + consumed;
                bool isKerning = false;
                while (*rest) {
                    int c2 = 0; uint32_t cp2 = fnt_utf8_decode(rest, &c2);
                    if (cp2 >= 0xFE00 && cp2 <= 0xFE0F) { rest += c2; continue; }
                    if (cp2 == 0x200D) { rest += c2; continue; }
                    isKerning = true; break;
                }
                if (isKerning) continue;
            }
            charList.push_back({cp, advance});
        }
    }

    if (b64data.empty() || cellW <= 0 || cellH <= 0 || charList.empty())
	{
		printfDebug(DebugLoadPaths, "INFO: FAILED LOADING embedded font (b64data.empty() || cellW <= 0 || cellH <= 0 || charList.empty()): %s\n", fntPath); 
        return nullptr;
	}

    // Decode base64 PNG
    auto pngData = b64_decode(b64data.c_str(), b64data.size());
    if (pngData.empty()) 
	{
		printfDebug(DebugLoadPaths, "INFO: FAILED LOADING embedded font (empty pngData): %s\n", fntPath); 
		return nullptr;
	}

    // Load PNG from memory
    SDL_RWops* rw = SDL_RWFromMem(pngData.data(), (int)pngData.size());
    if (!rw)
	{
		printfDebug(DebugLoadPaths, "INFO: FAILED LOADING embedded font (reading (rw) png from memory failed): %s\n", fntPath); 
		return nullptr;
	}
    SDL_Surface* table = IMG_Load_RW(rw, 1); // 1 = auto-close rw
    if (!table)
	{
		printfDebug(DebugLoadPaths, "INFO: FAILED LOADING embedded font (reading (table) png from memory failed): %s\n", fntPath); 
		return nullptr;
	}

    // Convert and preprocess (same as fnt_load_font)
    SDL_Surface* tableConv = SDL_ConvertSurfaceFormat(table, pd_api_gfx_PIXELFORMAT, 0);
    SDL_FreeSurface(table);
    if (!tableConv)
	{
		printfDebug(DebugLoadPaths, "INFO: FAILED LOADING embedded font (SDL_ConvertSurfaceFormat failed): %s\n", fntPath);  
		return nullptr;
	}

    pd_api_gfx_MakeSurfaceBlackAndWhite(tableConv);

    Uint32 transparentVal = SDL_MapRGBA(tableConv->format, 0, 0, 0, 0);
    Uint32 clearColor = SDL_MapRGBA(tableConv->format,
        pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g,
        pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);

    int cols = tableConv->w / cellW;
    if (cols <= 0) cols = 1;

    bool tableLocked = false;
    if (SDL_MUSTLOCK(tableConv)) 
		tableLocked = (SDL_LockSurface(tableConv) == 0);

    LCDFont* result = new LCDFont();
    result->font      = nullptr;
    result->isFntFont = true;
    result->cellW     = cellW;
    result->cellH     = cellH;
    result->fntTracking = fntTracking;
    result->kerning   = std::move(kerningMap);

    int bpp = tableConv->format->BytesPerPixel;
    for (int i = 0; i < (int)charList.size(); i++)
    {
        uint32_t cp  = charList[i].first;
        int advance  = charList[i].second;
        int col      = i % cols;
        int row      = i / cols;
        int srcX     = col * cellW;
        int srcY     = row * cellH;

        SDL_Surface* gs = SDL_CreateRGBSurfaceWithFormat(0, cellW, cellH, 32, pd_api_gfx_PIXELFORMAT);
        SDL_SetSurfaceBlendMode(gs, SDL_BLENDMODE_NONE);
        SDL_FillRect(gs, nullptr, clearColor);

        bool glyphLocked = false;
        if (SDL_MUSTLOCK(gs)) 
			glyphLocked = (SDL_LockSurface(gs) == 0);

        for (int py = 0; py < cellH; py++)
        for (int px = 0; px < cellW; px++)
        {
            int sx = srcX + px, sy = srcY + py;
            if (sx >= tableConv->w || sy >= tableConv->h) continue;
            Uint32* sp = (Uint32*)((Uint8*)tableConv->pixels + sy * tableConv->pitch + sx * bpp);
            Uint32* dp = (Uint32*)((Uint8*)gs->pixels + py * gs->pitch + px * gs->format->BytesPerPixel);
            *dp = (*sp == transparentVal) ? clearColor : *sp;
        }

        if (glyphLocked) SDL_UnlockSurface(gs);

        LCDFontGlyphData gd;
        gd.surface = gs;
        gd.advance = advance;
        result->glyphs[cp] = gd;
    }

    if (tableLocked) 
		SDL_UnlockSurface(tableConv);
    SDL_FreeSurface(tableConv);

    printfDebug(DebugLoadPaths, "INFO: Loaded embedded .fnt font: %s (cellW=%d cellH=%d glyphs=%d)\n",
                fntPath, cellW, cellH, (int)charList.size());
    return result;
}

static LCDFont* fnt_load_font(const char* fntPath, const char* pngPath)
{
    int fntTracking = 0;
    std::unordered_map<uint64_t,int> kerningMap;
    auto charList = fnt_parse_file(fntPath, &fntTracking, &kerningMap);
    if (charList.empty())
	{
		printfDebug(DebugLoadPaths, "INFO: FAILED LOADING fnt font (fnt_parse_file failed): %s\n", fntPath);
		return nullptr;
	}

    SDL_Surface* table = IMG_Load(pngPath);
    if (!table)
	{
		printfDebug(DebugLoadPaths, "INFO: FAILED LOADING fnt font (IMG_Load(%s) failed): %s\n", fntPath, pngPath);
		return nullptr;
	}

    // Extract cellW, cellH from filename: ...-table-W-H.png
    int cellW = 0, cellH = 0;
    {
        std::string name(pngPath);
        size_t sl = name.rfind('/');
        if (sl != std::string::npos) name = name.substr(sl+1);
        size_t dt = name.rfind('.');
        if (dt != std::string::npos) name = name.substr(0, dt);
        size_t p2 = name.rfind('-');
        if (p2 != std::string::npos) {
            cellH = atoi(name.c_str() + p2 + 1);
            name = name.substr(0, p2);
            size_t p1 = name.rfind('-');
            if (p1 != std::string::npos) cellW = atoi(name.c_str() + p1 + 1);
        }
    }
    if (cellW <= 0 || cellH <= 0) {
        int nGlyphs = (int)charList.size();
        int cols = 16;
        cellW = table->w / cols;
        cellH = table->h / ((nGlyphs + cols - 1) / cols);
    }

    SDL_Surface* tableConv = SDL_ConvertSurfaceFormat(table, pd_api_gfx_PIXELFORMAT, 0);
    SDL_FreeSurface(table);
    if (!tableConv) 
	{
		printfDebug(DebugLoadPaths, "INFO: FAILED LOADING fnt font (SDL_ConvertSurfaceFormat failed): %s\n", fntPath);
		return nullptr;
	}

    // Apply same preprocessing as loadBitmap does
    pd_api_gfx_MakeSurfaceBlackAndWhite(tableConv);

    // After MakeSurfaceBlackAndWhite: pixels are either
    // pd_api_gfx_color_white, pd_api_gfx_color_black, or transparent {0,0,0,0}
    // Transparent = background, white or black = glyph pixel
    Uint32 transparentVal = SDL_MapRGBA(tableConv->format, 0, 0, 0, 0);
    Uint32 clearColor = SDL_MapRGBA(tableConv->format,
        pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g,
        pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);

    int cols = tableConv->w / cellW;
    if (cols <= 0) cols = 1;

    bool tableLocked = false;
    if (SDL_MUSTLOCK(tableConv)) 
		tableLocked = (SDL_LockSurface(tableConv) == 0);

    LCDFont* result = new LCDFont();
    result->font      = nullptr;
    result->isFntFont = true;
    result->cellW     = cellW;
    result->cellH     = cellH;
    result->fntTracking = fntTracking;
    result->kerning   = std::move(kerningMap);

    int bpp = tableConv->format->BytesPerPixel;
    for (int i = 0; i < (int)charList.size(); i++)
    {
        uint32_t cp  = charList[i].first;
        int advance  = charList[i].second;
        int col      = i % cols;
        int row      = i / cols;
        int srcX     = col * cellW;
        int srcY     = row * cellH;

        SDL_Surface* gs = SDL_CreateRGBSurfaceWithFormat(0, cellW, cellH, 32, pd_api_gfx_PIXELFORMAT);
        SDL_SetSurfaceBlendMode(gs, SDL_BLENDMODE_NONE);
        SDL_FillRect(gs, nullptr, clearColor);

        bool glyphLocked = false;
        if (SDL_MUSTLOCK(gs)) 
			glyphLocked = (SDL_LockSurface(gs) == 0);

        for (int py = 0; py < cellH; py++)
        for (int px = 0; px < cellW; px++)
        {
            int sx = srcX + px, sy = srcY + py;
            if (sx >= tableConv->w || sy >= tableConv->h) continue;
            Uint32* sp = (Uint32*)((Uint8*)tableConv->pixels + sy * tableConv->pitch + sx * bpp);
            Uint32* dp = (Uint32*)((Uint8*)gs->pixels + py * gs->pitch + px * gs->format->BytesPerPixel);
            // After MakeSurfaceBlackAndWhite: transparent = background, anything else = glyph pixel
            // Store actual color (white or black) as-is — draw mode will handle rendering
            *dp = (*sp == transparentVal) ? clearColor : *sp;
        }

        if (glyphLocked) SDL_UnlockSurface(gs);

        LCDFontGlyphData gd;
        gd.surface = gs;
        gd.advance = advance;
        result->glyphs[cp] = gd;
    }

    if (tableLocked) SDL_UnlockSurface(tableConv);
    SDL_FreeSurface(tableConv);

    printfDebug(DebugLoadPaths, "INFO: Loaded native .fnt font: %s (cellW=%d cellH=%d glyphs=%d)\n",
           fntPath, cellW, cellH, (int)charList.size());
    return result;
}

LCDFont* pd_api_gfx_loadFont(const char* path, const char** outErr)
{
    LCDFont* result = NULL;
    if (outErr) *outErr = "font not found"; // safe default — overwritten on success or specific failure
    // Build paths with enough room for sourceDir prefix + .ttf extension
    // Use MAXPATH to avoid any overflow issues
    char* tmpfullpath = (char*)malloc(MAXPATH);
    char* fullpath    = (char*)malloc(MAXPATH);

    // Strip .pft extension — Playdate compiled font format, we use .fnt/.ttf instead
    char stripped[MAXPATH];
    strncpy(stripped, path, MAXPATH-1);
    stripped[MAXPATH-1] = '\0';
    if (strlen(stripped) > 4 && strcasecmp(stripped + strlen(stripped) - 4, ".pft") == 0)
        stripped[strlen(stripped) - 4] = '\0';
    path = stripped;

    // Normalize the input extension to .ttf for the TTF path
    // Copy path, find extension, force .ttf (4-char extensions only, e.g. .fnt->.ttf)
    strncpy(tmpfullpath, path, MAXPATH-5);
	tmpfullpath[MAXPATH-5] = '\0';
    char* ext = strrchr(tmpfullpath, '.');
    if (ext && strlen(ext) == 4) 
	{
        ext[1]='t';
		ext[2]='t';
		ext[3]='f';
    }
	else
	{
		if (ext && strlen(ext) != 4)
		{
			// non-4-char extension: append .ttf
			strncat(tmpfullpath, ".ttf", MAXPATH-strlen(tmpfullpath)-1);
		}
		else
		{
			if (!ext)
			{
				strncat(tmpfullpath, ".ttf", MAXPATH-strlen(tmpfullpath)-1);
			}
		}
	}

    // Build full path: "./{sourceDir}/{path}.ttf"
    // When sourceDir="." this gives "./{path}.ttf" (no double ./)
    const char* srcDir = _pd_api_get_current_source_dir();
    if (strcmp(srcDir, ".") == 0)
        snprintf(fullpath, MAXPATH, "./%s", tmpfullpath);
    else
        snprintf(fullpath, MAXPATH, "./%s/%s", srcDir, tmpfullpath);

    // If not found with source dir, fall back to plain "./{path}.ttf"
    struct stat lstats;
    if (stat(fullpath, &lstats) != 0)
        snprintf(fullpath, MAXPATH, "./%s", tmpfullpath);

	//check font list to see if we already loaded the font
    for (auto& font : fontlist)
    {
        if (strcasecmp(font->path, fullpath) == 0)
        {
			if(outErr)
            	*outErr = NULL;
            free(fullpath);
			free(tmpfullpath);
            return font->font;
        }
    }

    // Try native .fnt font first
    #if !TTFONLY
    {
        char fntpath[1024];
        strncpy(fntpath, fullpath, 1023);
        fntpath[1023] = '\0';
        char* fext = strrchr(fntpath, '.');
        if (fext) strcpy(fext, ".fnt");
        else strncat(fntpath, ".fnt", 1023 - strlen(fntpath));

        struct stat fntstat;
        if (stat(fntpath, &fntstat) == 0)
        {
            // Try embedded format first (base64 PNG inside fnt)
            if (fnt_is_embedded(fntpath))
            {
                LCDFont* fntFont = fnt_load_embedded_font(fntpath);
                if (fntFont)
                {
                    if (outErr) *outErr = nullptr;
                    FontListEntry* Tmp = new FontListEntry();
                    Tmp->path = (char*)malloc(strlen(fullpath) + 1);
                    strcpy(Tmp->path, fullpath);
                    Tmp->font = fntFont;
                    fontlist.push_back(Tmp);
                    free(fullpath);
                    free(tmpfullpath);
                    return fntFont;
                }
            }

            // Try separate table PNG
            std::string pngPath = fnt_find_table_png(fntpath);
            if (!pngPath.empty())
            {
                LCDFont* fntFont = fnt_load_font(fntpath, pngPath.c_str());
                if (fntFont)
                {
                    if (outErr) *outErr = nullptr;
                    FontListEntry* Tmp = new FontListEntry();
                    Tmp->path = (char*)malloc(strlen(fullpath) + 1);
                    strcpy(Tmp->path, fullpath);
                    Tmp->font = fntFont;
                    fontlist.push_back(Tmp);
                    free(fullpath);
                    free(tmpfullpath);
                    return fntFont;
                }
            }
        }
    }
    #endif

    TTF_Font* font = TTF_OpenFont(fullpath, 12);
    if(font)
    {
        result = pd_api_gfx_Create_LCDFont();
        if(result)
        {
			if(outErr)
            	*outErr = NULL;
            TTF_SetFontStyle(font, TTF_STYLE_NORMAL);
            result->font = font;
            printfDebug(DebugLoadPaths, "INFO: Loaded TTF font: %s\n", fullpath);

            FontListEntry* Tmp = new FontListEntry();
            Tmp->path = (char *) malloc(strlen(fullpath) + 1);
            strcpy(Tmp->path, fullpath);
            Tmp->font = result;
            fontlist.push_back(Tmp);
        }
		else
		{
            if(outErr) *outErr = "failed to create LCDFont";
			printfDebug(DebugLoadPaths, "INFO: FAILED LOADING TTF font (pd_api_gfx_Create_LCDFont failed): %s\n", fullpath);
		}
    }
	else
	{
		if(outErr) *outErr = "font file not found";
		printfDebug(DebugLoadPaths, "INFO: FAILED LOADING TTF font (TTF_OpenFont failed): %s\n", fullpath);
	}

    free(fullpath);
	free(tmpfullpath);
    return result;
}

void _pd_api_gfx_loadDefaultFonts()
{
	const char* err;
	_pd_api_gfx_Default_Font =  pd_api_gfx_loadFont("System/Fonts/Asheville-Sans-14-Light", &err);
    _pd_api_gfx_FPS_Font =  pd_api_gfx_loadFont("System/Fonts/Roobert-10-Bold", &err);
}

void _pd_api_gfx_resetContext()
{
	while(gfxstack.size() > 1)
		pd_api_gfx_popContext();
	_pd_api_gfx_CurrentGfxContext->BackgroundColor = kColorWhite;
    _pd_api_gfx_CurrentGfxContext->BitmapDrawMode = kDrawModeCopy;
    _pd_api_gfx_CurrentGfxContext->cliprect.x = -1;
    _pd_api_gfx_CurrentGfxContext->cliprect.y = -1;
    _pd_api_gfx_CurrentGfxContext->cliprect.w = -1;
    _pd_api_gfx_CurrentGfxContext->cliprect.h = -1;
    _pd_api_gfx_CurrentGfxContext->drawoffsetx = 0;
    _pd_api_gfx_CurrentGfxContext->drawoffsety = 0;
    _pd_api_gfx_CurrentGfxContext->DrawTarget = _pd_api_gfx_Playdate_Screen;
    _pd_api_gfx_CurrentGfxContext->font = _pd_api_gfx_Default_Font;
    _pd_api_gfx_CurrentGfxContext->linecapstyle = kLineCapStyleButt;
    _pd_api_gfx_CurrentGfxContext->stencil = NULL;
    _pd_api_gfx_CurrentGfxContext->tracking = 0;
    _pd_api_gfx_CurrentGfxContext->leading = 0;
}

LCDFontPage* pd_api_gfx_getFontPage(LCDFont* font, uint32_t c)
{
    return NULL;
}

LCDFontGlyph* pd_api_gfx_getPageGlyph(LCDFontPage* page, uint32_t c, LCDBitmap** bitmap, int* advance)
{
    *bitmap = NULL;
    return NULL;
}

int pd_api_gfx_getGlyphKerning(LCDFontGlyph* glyph, uint32_t glyphcode, uint32_t nextcode)
{
    return 0;
}

//taken from SDL_TTF
static void LATIN1_to_UTF8_backport(const char *src, Uint8 *dst)
{
    while (*src) {
        Uint8 ch = *(const Uint8 *)src++;
        if (ch <= 0x7F) {
            *dst++ = ch;
        } else {
            *dst++ = 0xC0 | ((ch >> 6) & 0x1F);
            *dst++ = 0x80 | (ch & 0x3F);
        }
    }
    *dst = '\0';
}

//taken from SDL_TTF
/* Gets the number of bytes needed to convert a Latin-1 string to UTF-8 */
static size_t LATIN1_to_UTF8_len_backport(const char *text)
{
    size_t bytes = 1;
    while (*text) {
        Uint8 ch = *(const Uint8 *)text++;
        if (ch <= 0x7F) {
            bytes += 1;
        } else {
            bytes += 2;
        }
    }
    return bytes;
}

//taken from SDL_TTF
/* Gets a unicode value from a UTF-8 encoded string
 * Ouputs increment to advance the string */
#define UNKNOWN_UNICODE 0xFFFD
static Uint32 UTF8_getch_backport(const char *src, size_t srclen, int *inc)
{
    const Uint8 *p = (const Uint8 *)src;
    size_t left = 0;
    size_t save_srclen = srclen;
    SDL_bool overlong = SDL_FALSE;
    SDL_bool underflow = SDL_FALSE;
    Uint32 ch = UNKNOWN_UNICODE;

    if (srclen == 0) {
        return UNKNOWN_UNICODE;
    }
    if (p[0] >= 0xFC) {
        if ((p[0] & 0xFE) == 0xFC) {
            if (p[0] == 0xFC && (p[1] & 0xFC) == 0x80) {
                overlong = SDL_TRUE;
            }
            ch = (Uint32) (p[0] & 0x01);
            left = 5;
        }
    } else if (p[0] >= 0xF8) {
        if ((p[0] & 0xFC) == 0xF8) {
            if (p[0] == 0xF8 && (p[1] & 0xF8) == 0x80) {
                overlong = SDL_TRUE;
            }
            ch = (Uint32) (p[0] & 0x03);
            left = 4;
        }
    } else if (p[0] >= 0xF0) {
        if ((p[0] & 0xF8) == 0xF0) {
            if (p[0] == 0xF0 && (p[1] & 0xF0) == 0x80) {
                overlong = SDL_TRUE;
            }
            ch = (Uint32) (p[0] & 0x07);
            left = 3;
        }
    } else if (p[0] >= 0xE0) {
        if ((p[0] & 0xF0) == 0xE0) {
            if (p[0] == 0xE0 && (p[1] & 0xE0) == 0x80) {
                overlong = SDL_TRUE;
            }
            ch = (Uint32) (p[0] & 0x0F);
            left = 2;
        }
    } else if (p[0] >= 0xC0) {
        if ((p[0] & 0xE0) == 0xC0) {
            if ((p[0] & 0xDE) == 0xC0) {
                overlong = SDL_TRUE;
            }
            ch = (Uint32) (p[0] & 0x1F);
            left = 1;
        }
    } else {
        if ((p[0] & 0x80) == 0x00) {
            ch = (Uint32) p[0];
        }
    }
    --srclen;
    while (left > 0 && srclen > 0) {
        ++p;
        if ((p[0] & 0xC0) != 0x80) {
            ch = UNKNOWN_UNICODE;
            break;
        }
        ch <<= 6;
        ch |= (p[0] & 0x3F);
        --srclen;
        --left;
    }
    if (left > 0) {
        underflow = SDL_TRUE;
    }
    /* Technically overlong sequences are invalid and should not be interpreted.
       However, it doesn't cause a security risk here and I don't see any harm in
       displaying them. The application is responsible for any other side effects
       of allowing overlong sequences (e.g. string compares failing, etc.)
       See bug 1931 for sample input that triggers this.
    */
    /* if (overlong) return UNKNOWN_UNICODE; */

    (void) overlong;

    if (underflow ||
        (ch >= 0xD800 && ch <= 0xDFFF) ||
        (ch == 0xFFFE || ch == 0xFFFF) || ch > 0x10FFFF) {
        ch = UNKNOWN_UNICODE;
    }

    *inc = (int)(save_srclen - srclen);

    return ch;
}

//taken from SDL_TTF
static SDL_bool CharacterIsNewLine_backport(Uint32 c)
{
    if (c == '\n') {
        return SDL_TRUE;
    }
    return SDL_FALSE;
}

//based on functionality from SDL_TTF
int pd_api_gfx_getTextWidth(LCDFont* font, const void* text, size_t len, PDStringEncoding encoding, int tracking)
{
    LCDFont *f = font;
    if(!f)
        f = _pd_api_gfx_CurrentGfxContext->font;  // use current context font, not default
    if(!f)
        f = _pd_api_gfx_Default_Font;
    
    if(!f)
	{
        return 0;
	}

    if(len == 0)
	{
		return 0;
	}

	if(!text || strlen((char*)text) == 0)
	{
		return 0;
	}

    // Native .fnt font - measure from advance widths directly
    #if !TTFONLY
    if(f->isFntFont)
    {
        const char* s = (const char*)text;
        size_t strLen = strlen(s);
        size_t bytePos = 0, cpCount = 0;
        int width = 0;
        int maxWidth = 0;
        int track = tracking + f->fntTracking;
        uint32_t prevCp = 0;
        while(bytePos < strLen && (encoding == kASCIIEncoding ? bytePos < (size_t)len : cpCount < (size_t)len))
        {
            int consumed = 0;
            uint32_t cp = fnt_utf8_decode(s + bytePos, &consumed);
            if(!cp || consumed == 0) break;
            bytePos += consumed;
            cpCount++;
            if(cp == '\n')
            {
                if(width > 0 && track > 0) width -= track;
                if(width > maxWidth) maxWidth = width;
                width = 0;
                prevCp = 0;
                continue;
            }
            auto it = f->glyphs.find(cp);
            int adv = (it != f->glyphs.end()) ? it->second.advance : f->cellW / 2;
            if (prevCp != 0 && !f->kerning.empty())
            {
                uint64_t key = ((uint64_t)prevCp << 32) | (uint64_t)cp;
                auto kit = f->kerning.find(key);
                if (kit != f->kerning.end())
                    width += kit->second;
            }
            prevCp = cp;
            width += adv + track;
        }
        if(width > 0 && track > 0) width -= track;
        if(width > maxWidth) maxWidth = width;
        return (maxWidth < 0) ? 0 : maxWidth;
    }
    #endif

    if(!f->font)
	{
        return 0;
	}

	// char *sizedtext = (char *) malloc((len + 1) * sizeof(char));
    // char *sizedtexttmp = (char *) malloc((len + 1) * sizeof(char));
	// strncpy(sizedtext,(char *) text, len);
    // sizedtext[len] = '\0';
    // int w, wtmp, htmp;
	// char *p = sizedtext;
	// char *ptmp = sizedtexttmp;
	// w = 0;
	// while(*p != '\0')
	// {
	// 	if(*p == '\n')
	// 	{
	// 		*ptmp = '\0';
	// 		if(strlen(sizedtexttmp) > 0)
	// 		{
	// 			TTF_SizeText(f->font, sizedtexttmp, &wtmp, &htmp);
	// 			if(wtmp > w)
	// 				w = wtmp;
	// 		}
	// 		ptmp = sizedtexttmp;
	// 	}
	// 	else
	// 	{
	// 		*ptmp = *p;
	// 		ptmp++;
	// 	}
	// 	p++;
	// }
	// //in case '\n' was last char sizedtextmp was reset to initial position and does not contain 0 char
	// *ptmp = '\0';
	// if(strlen(sizedtexttmp) > 0)
	// {
	// 	TTF_SizeText(f->font, sizedtexttmp, &wtmp, &htmp);
	// 	if(wtmp > w)
	// 		w = wtmp;
	// }
    // free(sizedtext);
	// free(sizedtexttmp);
    // return w;


	int width, height;
	Uint8 *utf8_alloc = NULL;

	size_t i, numLines;
	int rowHeight, lineskip;
	char **strLines = NULL, *text_cpy;
	const char* sizedtext = (const char *) text; 
	/* Convert input string to default encoding UTF-8 */
	if (encoding == kASCIIEncoding) {
		utf8_alloc = SDL_stack_alloc(Uint8, LATIN1_to_UTF8_len_backport(sizedtext));
		if (utf8_alloc == NULL) {
			SDL_OutOfMemory();
			return 0;
		}
		LATIN1_to_UTF8_backport(sizedtext, utf8_alloc);
		text_cpy = (char *)utf8_alloc;
	} else {
		/* Use a copy anyway */
		size_t str_len = SDL_strlen(sizedtext);
		utf8_alloc = SDL_stack_alloc(Uint8, str_len + 1);
		if (utf8_alloc == NULL) {
			SDL_OutOfMemory();
			return 0;
		}
		SDL_memcpy(utf8_alloc, text, str_len + 1);
		text_cpy = (char *)utf8_alloc;
	}

    /* Truncate text_cpy at the given length. */
    i = 0;
    const size_t bytecount = strlen(text_cpy);
    for (size_t charindex = 0; charindex < len && i < bytecount && text_cpy[i] != '\0'; charindex++) {
        const uint8_t entry = text_cpy[i];
        if (entry <= 127) {
            i++;
        } else if (entry >> 5 == 6) {
            i += 2;
        } else if (entry >> 4 == 14) {
            i += 3;
        } else if (entry >> 3 == 30) {
            i += 4;
        } else {
            // UTF-8 error
            i++;
        }
    }
    if (i < bytecount) {
        text_cpy[i] = '\0';
    }

	/* Get the dimensions of the text surface */
	if ((TTF_SizeUTF8(f->font, text_cpy, &width, &height) < 0) || !width) {
		SDL_SetError("Text has zero width");
		if (utf8_alloc)
		{
			SDL_stack_free(utf8_alloc);
		}
		return 0;
	}

	numLines = 1;

	if (*text_cpy) {
		size_t maxNumLines = 0;
		size_t textlen = SDL_strlen(text_cpy);
		numLines = 0;
		size_t numChars = 0;
		do {
			size_t save_textlen = (size_t)(-1);
			char *save_text  = NULL;

			if (numLines >= maxNumLines) {
				char **saved = strLines;
				
				maxNumLines += 32;
				strLines = (char **)SDL_realloc(strLines, maxNumLines * sizeof (*strLines));
				if (strLines == NULL) {
					strLines = saved;
					SDL_OutOfMemory();
					if (utf8_alloc)
					{
						SDL_stack_free(utf8_alloc);
					}
					if (strLines)
					{
						SDL_free(strLines);
					}
					return 0;
				}
			}

			strLines[numLines++] = text_cpy;

			while ((textlen > 0) && (numChars < len)) {
				int inc = 0;
				int is_delim;
				Uint32 c = UTF8_getch_backport(text_cpy, textlen, &inc);
				text_cpy += inc;
				textlen -= inc;

				if (c == UNICODE_BOM_NATIVE || c == UNICODE_BOM_SWAPPED) {
					continue;
				}

				numChars += 1;
				/* With wrapLength == 0, normal text rendering but newline aware */
				is_delim = CharacterIsNewLine_backport(c);

				/* Record last delimiter position */
				if (is_delim) {
					save_textlen = textlen;
					save_text = text_cpy;
					/* Break, if new line */
					if (c == '\n' || c == '\r') {
						*(text_cpy - 1) = '\0';
						break;
					}
				}
		
				//this means we had a character limit and need to apply a null char then
				if((numChars >= len))
				{
					*(text_cpy) = '\0';
				}
				
			}

			/* Cut at last delimiter/new lines, otherwise in the middle of the word */
			if (save_text && textlen) {
				text_cpy = save_text;
				textlen = save_textlen;
			}
		} while ((textlen > 0) && (numChars < len));
	}

	lineskip = TTF_FontLineSkip(f->font);
	rowHeight = SDL_max(height, lineskip);
	char* newtext;
	//if (wrapLength == 0) {
	if(true) {
		/* Find the max of all line lengths */
		if (numLines > 1) {
			width = 0;
			for (i = 0; i < numLines; i++) {
				char save_c = 0;
				int w, h;

				/* Add end-of-line */
				if (strLines) {
					newtext = strLines[i];
					if (i + 1 < numLines) {
						save_c = strLines[i + 1][0];
						strLines[i + 1][0] = '\0';
					}
				}

				if (TTF_SizeUTF8(f->font, newtext, &w, &h) == 0) {
					width = SDL_max(w, width);
				}

				/* Remove end-of-line */
				if (strLines) {
					if (i + 1 < numLines) {
						strLines[i + 1][0] = save_c;
					}
				}
			}
			/* In case there are all newlines */
			width = SDL_max(width, 1);
		}
	} 
	height = rowHeight + (lineskip + _pd_api_gfx_CurrentGfxContext->leading) * (numLines - 1);

	return width;
}

//based on functionality from SDL_TTF
int pd_api_gfx_drawText(const void* text, size_t len, PDStringEncoding encoding, int x, int y)
{
    // Draw to API layer first
    if (_pd_api_gfx_beginLayerDraw())
    {
        pd_api_gfx_drawText(text, len, encoding, x, y); // recurse with layer as target
        _pd_api_gfx_endLayerDraw();
    }
    if(len == 0)
        return 0;

    if(_pd_api_gfx_CurrentGfxContext->font == NULL)
    {
        return -1;
    }

    // Native .fnt font path
    #if !TTFONLY
    if(_pd_api_gfx_CurrentGfxContext->font->isFntFont)
    {
        LCDFont* f = _pd_api_gfx_CurrentGfxContext->font;
        const char* s = (const char*)text;
        size_t strLen = strlen(s);
        size_t bytePos = 0, cpCount = 0;
        int curX = x;
        int curY = y;
        int track = _pd_api_gfx_CurrentGfxContext->tracking + f->fntTracking;
        uint32_t prevCp = 0;
        while(bytePos < strLen && (encoding == kASCIIEncoding ? bytePos < (size_t)len : cpCount < (size_t)len))
        {
            int consumed = 0;
            // kASCIIEncoding and kUTF8Encoding both use UTF-8; encoding only affects length counting
            uint32_t cp = fnt_utf8_decode(s + bytePos, &consumed);
            if(!cp || consumed == 0) break;
            bytePos += consumed;
            cpCount++;

            if(cp == '\n') {
                curX = x;
                curY += f->cellH + _pd_api_gfx_CurrentGfxContext->leading;
                prevCp = 0;
                continue;
            }

            auto it = f->glyphs.find(cp);
            if(it == f->glyphs.end()) {
                // Apply kerning for missing glyphs too
                if (prevCp != 0 && !f->kerning.empty())
                {
                    uint64_t key = ((uint64_t)prevCp << 32) | (uint64_t)cp;
                    auto kit = f->kerning.find(key);
                    if (kit != f->kerning.end())
                        curX += kit->second;
                }
                curX += f->cellW / 2 + track;
                prevCp = cp;
                continue;
            }

            // Apply kerning BEFORE drawing so dstX is correct
            if (prevCp != 0 && !f->kerning.empty())
            {
                uint64_t key = ((uint64_t)prevCp << 32) | (uint64_t)cp;
                auto kit = f->kerning.find(key);
                if (kit != f->kerning.end())
                    curX += kit->second;
            }

            const LCDFontGlyphData& gd = it->second;

            // Blit glyph directly, skipping clear pixels, applying draw mode
            LCDBitmap* drawTarget = _pd_api_gfx_getDrawTarget();
            SDL_Surface* dst = drawTarget->Tex;

            int dstX = curX + _pd_api_gfx_CurrentGfxContext->drawoffsetx;
            int dstY = curY + _pd_api_gfx_CurrentGfxContext->drawoffsety;

            SDL_Rect clip;
            SDL_GetClipRect(dst, &clip);

            Uint32 clearVal = SDL_MapRGBA(gd.surface->format,
                pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g,
                pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            Uint32 whiteVal = SDL_MapRGBA(dst->format,
                pd_api_gfx_color_white.r, pd_api_gfx_color_white.g,
                pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            Uint32 blackVal = SDL_MapRGBA(dst->format,
                pd_api_gfx_color_black.r, pd_api_gfx_color_black.g,
                pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);

            bool srcLocked = false, dstLocked = false;
            if (SDL_MUSTLOCK(gd.surface)) srcLocked = SDL_LockSurface(gd.surface) == 0;
            if (SDL_MUSTLOCK(dst)) dstLocked = SDL_LockSurface(dst) == 0;

            LCDBitmapDrawMode mode = _pd_api_gfx_CurrentGfxContext->BitmapDrawMode;

            Uint32 srcWhiteVal = SDL_MapRGBA(gd.surface->format,
                pd_api_gfx_color_white.r, pd_api_gfx_color_white.g,
                pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);

            for (int py = 0; py < gd.surface->h; py++)
            {
                int dy = dstY + py;
                if (dy < 0 || dy >= dst->h) continue;
                if (dy < clip.y || dy >= clip.y + clip.h) continue;
                for (int px = 0; px < gd.surface->w; px++)
                {
                    int dx = dstX + px;
                    if (dx < 0 || dx >= dst->w) continue;
                    if (dx < clip.x || dx >= clip.x + clip.w) continue;

                    Uint32* sp = (Uint32*)((Uint8*)gd.surface->pixels
                        + py * gd.surface->pitch
                        + px * gd.surface->format->BytesPerPixel);
                    if (*sp == clearVal) continue; // transparent bg pixel

                    Uint32* dp = (Uint32*)((Uint8*)dst->pixels
                        + dy * dst->pitch
                        + dx * dst->format->BytesPerPixel);

                    // Determine if source glyph pixel is white or black
                    bool isWhiteGlyph = (*sp == srcWhiteVal);

                    switch (mode)
                    {
                        case kDrawModeCopy:
                            // Copy as-is: white glyph→white, black glyph→black
                            *dp = isWhiteGlyph ? whiteVal : blackVal;
                            break;
                        case kDrawModeFillWhite:
                            *dp = whiteVal;
                            break;
                        case kDrawModeFillBlack:
                            *dp = blackVal;
                            break;
                        case kDrawModeInverted:
                            // Invert: white glyph→black, black glyph→white
                            *dp = isWhiteGlyph ? blackVal : whiteVal;
                            break;
                        case kDrawModeXOR:
                        case kDrawModeNXOR:
                        {
                            Uint8 r,g,b,a;
                            SDL_GetRGBA(*dp, dst->format, &r,&g,&b,&a);
                            *dp = (r > 128) ? blackVal : whiteVal;
                            break;
                        }
                        case kDrawModeBlackTransparent:
                            if (!isWhiteGlyph) *dp = blackVal;
                            break;
                        case kDrawModeWhiteTransparent:
                            if (isWhiteGlyph) *dp = whiteVal;
                            break;
                        default:
                            *dp = isWhiteGlyph ? whiteVal : blackVal;
                            break;
                    }
                }
            }

            if (srcLocked) SDL_UnlockSurface(gd.surface);
            if (dstLocked) SDL_UnlockSurface(dst);
            drawTarget->BitmapDirty = true;

            prevCp = cp;
            curX += gd.advance + track;
        }
        return 0;
    }
    #endif

    int result = -1;
    if (_pd_api_gfx_CurrentGfxContext->font->font)
    {
        const char* sizedtext = (const char *) text;

		int width, height;
		Uint8 *utf8_alloc = NULL;

		int i, numLines, rowHeight, lineskip;
		char **strLines = NULL, *text_cpy;

		/* Convert input string to default encoding UTF-8 */
		if (encoding == kASCIIEncoding) {
			utf8_alloc = SDL_stack_alloc(Uint8, LATIN1_to_UTF8_len_backport(sizedtext));
			if (utf8_alloc == NULL) {
				SDL_OutOfMemory();
				return result;
			}
			LATIN1_to_UTF8_backport(sizedtext, utf8_alloc);
			text_cpy = (char *)utf8_alloc;
		 } else {
		 	/* Use a copy anyway */
		 	size_t str_len = SDL_strlen(sizedtext);
		 	utf8_alloc = SDL_stack_alloc(Uint8, str_len + 1);
		 	if (utf8_alloc == NULL) {
		 		SDL_OutOfMemory();
				return result;
		 	}
		 	SDL_memcpy(utf8_alloc, sizedtext, str_len + 1);
		 	text_cpy = (char *)utf8_alloc;
		}

		/* Get the dimensions of the text surface */
		if ((TTF_SizeUTF8(_pd_api_gfx_CurrentGfxContext->font->font, text_cpy, &width, &height) < 0) || !width) {
			SDL_SetError("Text has zero width");
			if (utf8_alloc)
			{
				SDL_stack_free(utf8_alloc);
			}
			return result;
		}

		numLines = 1;

		if (*text_cpy) {
			int maxNumLines = 0;
			size_t textlen = SDL_strlen(text_cpy);
			numLines = 0;
			size_t numChars = 0;
			do {
				size_t save_textlen = (size_t)(-1);
				char *save_text  = NULL;

				if (numLines >= maxNumLines) {
					char **saved = strLines;
					
					maxNumLines += 32;
					strLines = (char **)SDL_realloc(strLines, maxNumLines * sizeof (*strLines));
					if (strLines == NULL) {
						strLines = saved;
						SDL_OutOfMemory();
						if (utf8_alloc)
						{
							SDL_stack_free(utf8_alloc);
						}
						if (strLines)
							SDL_free(strLines);
						return result;
					}
				}

				strLines[numLines++] = text_cpy;
				int inc = 0;
				while ((textlen > 0) && (numChars < len)) {
					inc = 0;
					int is_delim;
					Uint32 c = UTF8_getch_backport(text_cpy, textlen, &inc);
					text_cpy += inc;
					textlen -= inc;

					if (c == UNICODE_BOM_NATIVE || c == UNICODE_BOM_SWAPPED) {
						continue;
					}

					numChars += 1;
					/* With wrapLength == 0, normal text rendering but newline aware */
					is_delim = CharacterIsNewLine_backport(c);

					/* Record last delimiter position */
					if (is_delim) {
						save_textlen = textlen;
						save_text = text_cpy;
						/* Break, if new line */
						if (c == '\n' || c == '\r') {
							*(text_cpy - 1) = '\0';
							break;
						}
					}
				}
				//this means we had a character limit and need to apply a null char then
				if((numChars >= len))
				{
					*(text_cpy) = '\0';
				}

				/* Cut at last delimiter/new lines, otherwise in the middle of the word */
				if (save_text && textlen) {
					text_cpy = save_text;
					textlen = save_textlen;
				}

				
			} while ((textlen > 0) && (numChars < len));
		}

		lineskip = TTF_FontLineSkip(_pd_api_gfx_CurrentGfxContext->font->font);
		rowHeight = SDL_max(height, lineskip);
		char* newtext;
		//if (wrapLength == 0) {
		if(true) {
			/* Find the max of all line lengths */
			if (numLines > 1) {
				width = 0;
				for (i = 0; i < numLines; i++) {
					char save_c = 0;
					int w, h;

					/* Add end-of-line */
					if (strLines) {
						newtext = strLines[i];
						if (i + 1 < numLines) {
							save_c = strLines[i + 1][0];
							strLines[i + 1][0] = '\0';
						}
					}

					if (TTF_SizeUTF8(_pd_api_gfx_CurrentGfxContext->font->font, newtext, &w, &h) == 0) {
						width = SDL_max(w, width);
					}

					/* Remove end-of-line */
					if (strLines) {
						if (i + 1 < numLines) {
							strLines[i + 1][0] = save_c;
						}
					}
				}
				/* In case there are all newlines */
				width = SDL_max(width, 1);
			}
		} 
		height = rowHeight + (lineskip + _pd_api_gfx_CurrentGfxContext->leading) * (numLines - 1);

		/* Render each line */
		for (i = 0; i < numLines; i++) {
			int xstart, ystart;
			char save_c = 0;

			/* Add end-of-line */
			if (strLines) {
				newtext = strLines[i];
				if (i + 1 < numLines) {
					save_c = strLines[i + 1][0];
					strLines[i + 1][0] = '\0';
				}
			}


			ystart = y;
			xstart = x;
			/* Move to i-th line, including leading */
			ystart += i * (lineskip + _pd_api_gfx_CurrentGfxContext->leading);
		
			SDL_Surface* TextSurface = TTF_RenderUTF8_Solid(_pd_api_gfx_CurrentGfxContext->font->font, newtext, pd_api_gfx_color_black);
			if (TextSurface) 
        	{
            	SDL_Surface* Texture = SDL_ConvertSurfaceFormat(TextSurface, _pd_api_gfx_getDrawTarget()->Tex->format->format, 0);
            	if(Texture)
            	{                
                	LCDBitmap * bitmap = pd_api_gfx_newBitmap(TextSurface->w,TextSurface->h, kColorClear);
                	if(bitmap)
                	{
						SDL_BlitSurface(TextSurface, NULL, bitmap->Tex, NULL);
						pd_api_gfx_drawBitmap(bitmap, xstart, ystart, kBitmapUnflipped);
                    	pd_api_gfx_freeBitmap(bitmap);
					}
				}
                SDL_FreeSurface(Texture);
				result = 0;
            }
            SDL_FreeSurface(TextSurface);


			/* Remove end-of-line */
			if (strLines) {
				if (i + 1 < numLines) {
					strLines[i + 1][0] = save_c;
				}
			}
		}

		if (strLines) {
			SDL_free(strLines);
		}
		if (utf8_alloc) {
			SDL_stack_free(utf8_alloc);
		}
	}
	return result;
}

playdate_video* pd_api_gfx_Create_playdate_video()
{
    playdate_video *Tmp = (playdate_video*) malloc(sizeof(*Tmp));
    Tmp->loadVideo = pd_api_gfx_loadVideo;
    Tmp->freePlayer = pd_api_gfx_freeVideoPlayer;
    Tmp->setContext = pd_api_gfx_setContext;
    Tmp->useScreenContext = pd_api_gfx_useScreenContext;
    Tmp->renderFrame = pd_api_gfx_renderFrame;
    Tmp->getError = pd_api_gfx_getError;
    Tmp->getInfo = pd_api_gfx_getInfo;
    Tmp->getContext = pd_api_gfx_getContext;
    return Tmp;
}

// 2.5
void pd_api_gfx_setPixel(int x, int y, LCDColor c)
{
	pd_api_gfx_fillRect(x,y,1,1,c);
}

LCDSolidColor pd_api_gfx_getBitmapPixel(LCDBitmap* bitmap, int x, int y)
{
    if (!bitmap)
        return kColorClear;

    SDL_Surface* tex = bitmap->MaskedTex ? bitmap->MaskedTex : bitmap->Tex;

    if (!tex)
        return kColorClear;

    if (x < 0 || y < 0 || x >= tex->w || y >= tex->h)
        return kColorClear;

    bool locked = false;
    if (SDL_MUSTLOCK(tex))
        locked = SDL_LockSurface(tex) == 0;

    Uint32* p = (Uint32*)((Uint8*)tex->pixels
        + y * tex->pitch
        + x * tex->format->BytesPerPixel);
    Uint32 pval = *p;

    if (locked)
        SDL_UnlockSurface(tex);

    Uint32 clear = SDL_MapRGBA(tex->format,
        pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g,
        pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
    Uint32 whitethreshold = SDL_MapRGBA(tex->format,
        pd_api_gfx_color_whitetreshold.r, pd_api_gfx_color_whitetreshold.g,
        pd_api_gfx_color_whitetreshold.b, pd_api_gfx_color_whitetreshold.a);

    if (pval == clear)
        return kColorClear;
    if (pval > whitethreshold)
        return kColorWhite;
    return kColorBlack;
}

void pd_api_gfx_getBitmapTableInfo(LCDBitmapTable* table, int* count, int* width)
{
	if(table)
	{
		if(count)
			*count = table->count;
		if(width)
			*width = table->across;
	}
}

// 2.6
void pd_api_gfx_drawTextInRect(const void* text, size_t len, PDStringEncoding encoding, int x, int y, int width, int height, PDTextWrappingMode wrap, PDTextAlignment align)
{
    if (!text || len == 0 || width <= 0 || height <= 0)
        return;

    LCDFont* font = _pd_api_gfx_CurrentGfxContext->font;
    if (!font)
        return;

    int lineHeight = pd_api_gfx_getFontHeight(font);
    if (lineHeight <= 0)
        lineHeight = 12;

    // Work with a null-terminated copy
    std::string src((const char*)text, len);
    // Strip to actual content length
    while (!src.empty() && src.back() == '\0')
        src.pop_back();

    int drawY = y;

    if (wrap == kWrapClip)
    {
        // Single line, clipped to width
        if (drawY + lineHeight > y + height)
            return;
        int w = pd_api_gfx_getTextWidth(font, src.c_str(), src.size(), encoding, 0);
        int drawX = x;
        if (align == kAlignTextCenter)
            drawX = x + (width - w) / 2;
        else if (align == kAlignTextRight)
            drawX = x + width - w;
        pd_api_gfx_drawText(src.c_str(), src.size(), encoding, drawX, drawY);
        return;
    }

    // Word or character wrap: split into lines
    // First split on explicit newlines, then wrap each segment
    std::vector<std::string> lines;
    std::istringstream stream(src);
    std::string segment;
    while (std::getline(stream, segment))
    {
        if (segment.empty())
        {
            lines.push_back("");
            continue;
        }

        if (wrap == kWrapWord)
        {
            // Word wrap
            std::string currentLine;
            std::istringstream words(segment);
            std::string word;
            while (words >> word)
            {
                std::string test = currentLine.empty() ? word : currentLine + " " + word;
                int w = pd_api_gfx_getTextWidth(font, test.c_str(), test.size(), encoding, 0);
                if (w <= width)
                {
                    currentLine = test;
                }
                else
                {
                    if (!currentLine.empty())
                        lines.push_back(currentLine);
                    // If the word itself is wider than width, just add it anyway
                    currentLine = word;
                }
            }
            if (!currentLine.empty())
                lines.push_back(currentLine);
        }
        else // kWrapCharacter
        {
            std::string currentLine;
            for (size_t i = 0; i < segment.size(); )
            {
                // Get next UTF-8 character (simple byte-by-byte for ASCII)
                size_t charLen = 1;
                unsigned char c = (unsigned char)segment[i];
                if      (c >= 0xF0) charLen = 4;
                else if (c >= 0xE0) charLen = 3;
                else if (c >= 0xC0) charLen = 2;
                std::string ch = segment.substr(i, charLen);
                i += charLen;

                std::string test = currentLine + ch;
                int w = pd_api_gfx_getTextWidth(font, test.c_str(), test.size(), encoding, 0);
                if (w <= width)
                {
                    currentLine = test;
                }
                else
                {
                    if (!currentLine.empty())
                        lines.push_back(currentLine);
                    currentLine = ch;
                }
            }
            if (!currentLine.empty())
                lines.push_back(currentLine);
        }
    }

    // Draw each line
    for (const std::string& line : lines)
    {
        if (drawY + lineHeight > y + height)
            break;

        int w = line.empty() ? 0 : pd_api_gfx_getTextWidth(font, line.c_str(), line.size(), encoding, 0);
        int drawX = x;
        if (align == kAlignTextCenter)
            drawX = x + (width - w) / 2;
        else if (align == kAlignTextRight)
            drawX = x + width - w;

        if (!line.empty())
            pd_api_gfx_drawText(line.c_str(), line.size(), encoding, drawX, drawY);

        drawY += lineHeight;
    }
}

// 2.7
int pd_api_gfx_getTextHeightForMaxWidth(LCDFont* font, const void* text, size_t len, int maxwidth, PDStringEncoding encoding, PDTextWrappingMode wrap, int tracking, int extraLeading)
{
	//this is wrong but does at least do something
	int width, height;
	Uint8 *utf8_alloc = NULL;

	int i, numLines, rowHeight, lineskip;
	char **strLines = NULL, *text_cpy;
	const char* sizedtext = (const char *) text; 
	LCDFont *f = font;
    if(!f)
        f = _pd_api_gfx_CurrentGfxContext->font;  // use current context font, not default
    if(!f)
        f = _pd_api_gfx_Default_Font;

    if(!f)
	{
        return 0;
	}

    // Native .fnt font - word wrap and count lines
    #if !TTFONLY
    if(f->isFntFont)
    {
        const char* s = (const char*)text;
        size_t strLen = strlen(s);
        size_t bytePos = 0, cpCount = 0;
        int lineWidth = 0;
        int lines = 1;
        while(bytePos < strLen && (encoding == kASCIIEncoding ? bytePos < (size_t)len : cpCount < (size_t)len))
        {
            int consumed = 0;
            uint32_t cp = fnt_utf8_decode(s + bytePos, &consumed);
            if(!cp || consumed == 0) break;
            bytePos += consumed;
            cpCount++;
            if(cp == '\n') { lines++; lineWidth = 0; continue; }
            auto it = f->glyphs.find(cp);
            int adv = (it != f->glyphs.end()) ? it->second.advance : f->cellW/2;
            lineWidth += adv + tracking;
            if(maxwidth > 0 && lineWidth > maxwidth) { lines++; lineWidth = adv; }
        }
        return lines * (f->cellH + extraLeading);
    }
    #endif

    if(!f->font)
	{
        return 0;
	}
	/* Convert input string to default encoding UTF-8 */
	if (encoding == kASCIIEncoding) {
		utf8_alloc = SDL_stack_alloc(Uint8, LATIN1_to_UTF8_len_backport(sizedtext));
		if (utf8_alloc == NULL) {
			SDL_OutOfMemory();
			return 0;
		}
		LATIN1_to_UTF8_backport(sizedtext, utf8_alloc);
		text_cpy = (char *)utf8_alloc;
	} else {
		/* Use a copy anyway */
		size_t str_len = SDL_strlen(sizedtext);
		utf8_alloc = SDL_stack_alloc(Uint8, str_len + 1);
		if (utf8_alloc == NULL) {
			SDL_OutOfMemory();
			return 0;
		}
		SDL_memcpy(utf8_alloc, text, str_len + 1);
		text_cpy = (char *)utf8_alloc;
	}

	/* Get the dimensions of the text surface */
	if ((TTF_SizeUTF8(f->font, text_cpy, &width, &height) < 0) || !width) {
		SDL_SetError("Text has zero width");
		if (utf8_alloc)
		{
			SDL_stack_free(utf8_alloc);
		}
		return 0;
	}

	numLines = 1;

	if (*text_cpy) {
		int maxNumLines = 0;
		size_t textlen = SDL_strlen(text_cpy);
		numLines = 0;
		size_t numChars = 0;
		do {
			size_t save_textlen = (size_t)(-1);
			char *save_text  = NULL;

			if (numLines >= maxNumLines) {
				char **saved = strLines;
				
				maxNumLines += 32;
				strLines = (char **)SDL_realloc(strLines, maxNumLines * sizeof (*strLines));
				if (strLines == NULL) {
					strLines = saved;
					SDL_OutOfMemory();
					if (utf8_alloc)
					{
						SDL_stack_free(utf8_alloc);
					}
					if (strLines)
					{
						SDL_free(strLines);
					}
					return 0;
				}
			}

			strLines[numLines++] = text_cpy;

			while ((textlen > 0) && (numChars < len)) {
				int inc = 0;
				int is_delim;
				Uint32 c = UTF8_getch_backport(text_cpy, textlen, &inc);
				text_cpy += inc;
				textlen -= inc;

				if (c == UNICODE_BOM_NATIVE || c == UNICODE_BOM_SWAPPED) {
					continue;
				}

				numChars += 1;
				/* With wrapLength == 0, normal text rendering but newline aware */
				is_delim = CharacterIsNewLine_backport(c);

				/* Record last delimiter position */
				if (is_delim) {
					save_textlen = textlen;
					save_text = text_cpy;
					/* Break, if new line */
					if (c == '\n' || c == '\r') {
						*(text_cpy - 1) = '\0';
						break;
					}
				}
		
				//this means we had a character limit and need to apply a null char then
				if((numChars >= len))
				{
					*(text_cpy) = '\0';
				}
				
			}

			/* Cut at last delimiter/new lines, otherwise in the middle of the word */
			if (save_text && textlen) {
				text_cpy = save_text;
				textlen = save_textlen;
			}
		} while ((textlen > 0) && (numChars < len));
	}

	lineskip = TTF_FontLineSkip(f->font);
	rowHeight = SDL_max(height, lineskip);
	char* newtext;
	//if (wrapLength == 0) {
	if(true) {
		/* Find the max of all line lengths */
		if (numLines > 1) {
			width = 0;
			for (i = 0; i < numLines; i++) {
				char save_c = 0;
				int w, h;

				/* Add end-of-line */
				if (strLines) {
					newtext = strLines[i];
					if (i + 1 < numLines) {
						save_c = strLines[i + 1][0];
						strLines[i + 1][0] = '\0';
					}
				}

				if (TTF_SizeUTF8(f->font, newtext, &w, &h) == 0) {
					width = SDL_max(w, width);
				}

				/* Remove end-of-line */
				if (strLines) {
					if (i + 1 < numLines) {
						strLines[i + 1][0] = save_c;
					}
				}
			}
			/* In case there are all newlines */
			width = SDL_max(width, 1);
		}
	} 
	height = rowHeight + (lineskip + _pd_api_gfx_CurrentGfxContext->leading) * (numLines - 1);

	return height;
}

void pd_api_gfx_drawRoundRect(int x, int y, int width, int height, int radius, int lineWidth, LCDColor color)
{
	if (lineWidth <= 0)
		lineWidth = 1;	
    LCDBitmap *mask = _pd_api_gfx_getDrawTarget()->Mask;
	//for xor we are abusing the api to draw on a bitmap and then draw that bitmap on the current target using xor mode
    LCDBitmap *bitmap = NULL;
	LCDBitmap *pattern = NULL;
    switch (color)
    {
        case kColorBlack:
            break;
        case kColorWhite:
            break;
        case kColorXOR:
            break;
        case kColorClear:
            break;
		default:
			//assume lcd pattern
			pattern = pd_api_gfx_PatternToBitmap((uint8_t*)color);
		break;
    }
	int drawLineWidth = make_even(lineWidth+1);
	int halfdrawLineWidth = drawLineWidth >> 1;
    if (color == kColorXOR)
    {
        bitmap = Api->graphics->newBitmap(width +drawLineWidth+1, height + drawLineWidth+1, kColorClear);
        Api->graphics->pushContext(bitmap);
    }
    

    SDL_Rect rect;
    rect.x = color == kColorXOR || pattern != NULL ? halfdrawLineWidth: x + _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    rect.y = color == kColorXOR || pattern != NULL ? halfdrawLineWidth: y + _pd_api_gfx_CurrentGfxContext->drawoffsety;
    rect.w = width;
    rect.h = height;
	if(pattern)
	{
		int yoffset = y % 8;
		int xoffset = x % 8;
		int tilesx = ((width +drawLineWidth+1) /  8)+2;
		int tilesy = ((height +drawLineWidth+1) / 8)+2;
		bitmap = Api->graphics->newBitmap(width +drawLineWidth+1, height + drawLineWidth+1, kColorClear);
		bitmap->Mask = pd_api_gfx_newBitmap(bitmap->w, bitmap->h, kColorBlack);
		pd_api_gfx_pushDrawOffset();		
		Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
		Api->graphics->setDrawMode(kDrawModeCopy);
		Api->graphics->pushContext(bitmap);	
#ifdef MASKPRIMITIVES        
		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
				_pd_api_gfx_drawBitmapAll(pattern, (xx*8) -std::abs(xoffset), (yy*8) -std::abs(yoffset), 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped,false,true);
#else

		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
			{
				SDL_Rect dst;
				dst.x = xx*8 -std::abs(xoffset);
				dst.y = yy*8 -std::abs(yoffset);
				dst.w = 8;
				dst.h = 8;
				SDL_BlitSurface(pattern->Tex, NULL, bitmap->Tex, &dst);
			}
#endif		
		Api->graphics->popContext();
#ifdef MASKPRIMITIVES		
		SDL_FillRect(bitmap->Mask->Tex, NULL, SDL_MapRGBA(bitmap->Mask->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a));
#endif		
		drawRoundRectRGBASurfacePlaydate(bitmap->Mask->Tex, rect.x, rect.y, rect.w, rect.h, radius, lineWidth, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
		bitmap->BitmapDirty = true;
		pd_api_gfx_popDrawOffset();
		Api->graphics->drawBitmap(bitmap,x -halfdrawLineWidth,  y -halfdrawLineWidth, kBitmapUnflipped);
		Api->graphics->popContext();
		Api->graphics->freeBitmap(bitmap);
		Api->graphics->freeBitmap(pattern);
	}
	else
	{
		SDL_Color maskColor = pd_api_gfx_color_white;
		switch (color)
		{
			case kColorBlack:
				drawRoundRectRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, rect.x, rect.y, rect.w, rect.h, radius, lineWidth, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
				break;
			case kColorWhite:
				drawRoundRectRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, rect.x, rect.y, rect.w, rect.h, radius, lineWidth, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
				break;
			case kColorXOR:
				drawRoundRectRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, rect.x, rect.y, rect.w, rect.h, radius, lineWidth, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
				break;
			case kColorClear:
				drawRoundRectRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, rect.x, rect.y, rect.w, rect.h, radius, lineWidth, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
				maskColor = pd_api_gfx_color_black;
				break;
			default:
				break;
		
		}
		if (mask && color != kColorXOR)
			drawRoundRectRGBASurfacePlaydate(mask->Tex, rect.x, rect.y, rect.w, rect.h, radius, lineWidth, maskColor.r, maskColor.g, maskColor.b, maskColor.a);

	}
		
    if (color == kColorXOR)
    {
        Api->graphics->popContext();
		pd_api_gfx_pushDrawOffset();
        Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
        Api->graphics->setDrawMode(kDrawModeXOR);
		pd_api_gfx_popDrawOffset();
        _pd_api_gfx_drawBitmapAll(bitmap, x-halfdrawLineWidth, y-halfdrawLineWidth, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped,true,true);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }
	_pd_api_gfx_getDrawTarget()->BitmapDirty = true;
}

void pd_api_gfx_fillRoundRect(int x, int y, int width, int height, int radius, LCDColor color)
{
	if (radius < 1)
		radius = 1;
	LCDBitmap *mask = _pd_api_gfx_getDrawTarget()->Mask;
	//for xor we are abusing the api to draw on a bitmap and then draw that bitmap on the current target using xor mode
    LCDBitmap *bitmap = NULL;
	LCDBitmap *pattern = NULL;
    switch (color)
    {
        case kColorBlack:
            break;
        case kColorWhite:
            break;
        case kColorXOR:
            break;
        case kColorClear:
            break;
		default:
			//assume lcd pattern
			pattern = pd_api_gfx_PatternToBitmap((uint8_t*)color);
		break;
    }

	if (color == kColorXOR)
    {
        bitmap = Api->graphics->newBitmap(width+1, height+1, kColorClear);
        Api->graphics->pushContext(bitmap);
    }
    

    SDL_Rect rect;
    rect.x = color == kColorXOR || pattern != NULL ? 0: x + _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    rect.y = color == kColorXOR || pattern != NULL ? 0: y + _pd_api_gfx_CurrentGfxContext->drawoffsety;
    rect.w = width;
    rect.h = height;
	if(pattern)
	{
		int yoffset = y % 8;
		int xoffset = x % 8;
		int tilesx = (width /  8)+2;
		int tilesy = (height / 8)+2;
		bitmap = Api->graphics->newBitmap(width + 1, height +1, kColorClear);
		bitmap->Mask = pd_api_gfx_newBitmap(bitmap->w, bitmap->h, kColorBlack);
		pd_api_gfx_pushDrawOffset();
		Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
		Api->graphics->setDrawMode(kDrawModeCopy);
		Api->graphics->pushContext(bitmap);	
#ifdef MASKPRIMITIVES        
		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
				_pd_api_gfx_drawBitmapAll(pattern, (xx*8) -std::abs(xoffset), (yy*8) -std::abs(yoffset), 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped,false,true);
#else				

		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
			{
				SDL_Rect dst;
				dst.x = xx*8 -std::abs(xoffset);
				dst.y = yy*8 -std::abs(yoffset);
				dst.w = 8;
				dst.h = 8;
				SDL_BlitSurface(pattern->Tex, NULL, bitmap->Tex, &dst);
			}
#endif		
		Api->graphics->popContext();
#ifdef MASKPRIMITIVES		
		SDL_FillRect(bitmap->Mask->Tex, NULL, SDL_MapRGBA(bitmap->Mask->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a));
#endif
		fillRoundRectRGBASurfacePlaydate(bitmap->Mask->Tex, rect.x, rect.y, rect.w, rect.h, radius, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
		bitmap->BitmapDirty = true;
		pd_api_gfx_popDrawOffset();
		Api->graphics->drawBitmap(bitmap,x,  y, kBitmapUnflipped);
		Api->graphics->popContext();
		Api->graphics->freeBitmap(bitmap);
		Api->graphics->freeBitmap(pattern);
	}
	else
	{
		SDL_Color maskColor = pd_api_gfx_color_white;
		switch (color)
		{
			case kColorBlack:
				fillRoundRectRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, rect.x, rect.y,rect.w, rect.h, radius, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
				break;
			case kColorWhite:
				fillRoundRectRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, rect.x, rect.y, rect.w, rect.h, radius, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
				break;
			case kColorXOR:
				fillRoundRectRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, rect.x, rect.y, rect.w, rect.h, radius, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
				break;
			case kColorClear:
				fillRoundRectRGBASurfacePlaydate(_pd_api_gfx_getDrawTarget()->Tex, rect.x, rect.y, rect.w, rect.h, radius, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
				maskColor = pd_api_gfx_color_black;
				break;
			default:
				break;
		}
		if (mask && color != kColorXOR)
			fillRoundRectRGBASurfacePlaydate(mask->Tex, rect.x, rect.y, rect.w, rect.h, radius, maskColor.r, maskColor.g, maskColor.b, maskColor.a);
	}

    if (color == kColorXOR)
    {
        Api->graphics->popContext();
		pd_api_gfx_pushDrawOffset();
        Api->graphics->pushContext(_pd_api_gfx_getDrawTarget());
        Api->graphics->setDrawMode(kDrawModeXOR);
		pd_api_gfx_popDrawOffset();
        _pd_api_gfx_drawBitmapAll(bitmap, x, y, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped, true,true);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }
	_pd_api_gfx_getDrawTarget()->BitmapDirty = true;
}


// 3.0
LCDStreamPlayer* pd_api_gfx_playdate_videostream_newPlayer(void)
{
	return NULL;
}

void pd_api_gfx_playdate_videostream_freePlayer(LCDStreamPlayer* p)
{

}

void pd_api_gfx_playdate_videostream_setBufferSize(LCDStreamPlayer* p, int video, int audio)
{

}

void pd_api_gfx_playdate_videostream_setFile(LCDStreamPlayer* p, SDFile* file)
{

}

void pd_api_gfx_playdate_videostream_setHTTPConnection(LCDStreamPlayer* p, HTTPConnection* conn)
{

}

FilePlayer* pd_api_gfx_playdate_videostream_getFilePlayer(LCDStreamPlayer* p)
{
	return NULL;
}

LCDVideoPlayer* pd_api_gfx_playdate_videostream_getVideoPlayer(LCDStreamPlayer* p)
{
	return NULL;
}

//	int (*setContext)(LCDStreamPlayer* p, LCDBitmap* context);
//	LCDBitmap* (*getContext)(LCDStreamPlayer* p);

// returns true if it drew a frame, else false
bool pd_api_gfx_playdate_videostream_update(LCDStreamPlayer* p)
{
	return false;
}

int pd_api_gfx_playdate_videostream_getBufferedFrameCount(LCDStreamPlayer* p)
{
	return 0;
}

uint32_t pd_api_gfx_playdate_videostream_getBytesRead(LCDStreamPlayer* p)
{
	return 0;
}

// 3.0
void pd_api_gfx_playdate_videostream_setTCPConnection(LCDStreamPlayer* p, TCPConnection* conn)
{

}


playdate_videostream* pd_api_gfx_Create_playdate_videostream()
{
	playdate_videostream *Tmp = (playdate_videostream*) malloc(sizeof(*Tmp));
    Tmp->freePlayer = pd_api_gfx_playdate_videostream_freePlayer;
	Tmp->getBufferedFrameCount = pd_api_gfx_playdate_videostream_getBufferedFrameCount;
	Tmp->getBytesRead = pd_api_gfx_playdate_videostream_getBytesRead;
	Tmp->getFilePlayer = pd_api_gfx_playdate_videostream_getFilePlayer;
	Tmp->getVideoPlayer = pd_api_gfx_playdate_videostream_getVideoPlayer;
	Tmp->newPlayer = pd_api_gfx_playdate_videostream_newPlayer;
	Tmp->setBufferSize = pd_api_gfx_playdate_videostream_setBufferSize;
	Tmp->setFile = pd_api_gfx_playdate_videostream_setFile;
	Tmp->setHTTPConnection = pd_api_gfx_playdate_videostream_setHTTPConnection;
	Tmp->setTCPConnection = pd_api_gfx_playdate_videostream_setTCPConnection ;
	Tmp->update = pd_api_gfx_playdate_videostream_update;
    return Tmp;
}

LCDTileMap* pd_api_gfx_playdate_tilemap_newTilemap(void)
{
	return NULL;
}

void pd_api_gfx_playdate_tilemap_freeTilemap(LCDTileMap* m)
{

}

void pd_api_gfx_playdate_tilemap_setImageTable(LCDTileMap* m, LCDBitmapTable* table)
{

}

LCDBitmapTable* pd_api_gfx_playdate_tilemap_getImageTable(LCDTileMap* m)
{
	return NULL;
}

void pd_api_gfx_playdate_tilemap_setSize(LCDTileMap* m, int tilesWide, int tilesHigh)
{

}

void pd_api_gfx_playdate_tilemap_getSize(LCDTileMap* m, int* tilesWide, int* tilesHigh)
{

}

void pd_api_gfx_playdate_tilemap_getPixelSize(LCDTileMap* m, uint32_t* outWidth, uint32_t* outHeight)
{

}

void pd_api_gfx_playdate_tilemap_setTiles(LCDTileMap* m, uint16_t* indexes, int count, int rowwidth)
{

}

void pd_api_gfx_playdate_tilemap_setTileAtPosition(LCDTileMap* m, int x, int y, uint16_t idx)
{

}

int pd_api_gfx_playdate_tilemap_getTileAtPosition(LCDTileMap* m, int x, int y)
{
	return 0;
}

void pd_api_gfx_playdate_tilemap_drawAtPoint(LCDTileMap* m, float x, float y)
{

}


playdate_tilemap* pd_api_gfx_Create_playdate_tilemap()
{
	playdate_tilemap *Tmp = (playdate_tilemap*) malloc(sizeof(*Tmp));
    Tmp->drawAtPoint = pd_api_gfx_playdate_tilemap_drawAtPoint;
    Tmp->freeTilemap = pd_api_gfx_playdate_tilemap_freeTilemap;
    Tmp->getImageTable = pd_api_gfx_playdate_tilemap_getImageTable;
    Tmp->getPixelSize = pd_api_gfx_playdate_tilemap_getPixelSize;
    Tmp->getSize = pd_api_gfx_playdate_tilemap_getSize;
    Tmp->getTileAtPosition = pd_api_gfx_playdate_tilemap_getTileAtPosition;
    Tmp->newTilemap = pd_api_gfx_playdate_tilemap_newTilemap;
    Tmp->setImageTable = pd_api_gfx_playdate_tilemap_setImageTable;
	Tmp->setSize = pd_api_gfx_playdate_tilemap_setSize;
	Tmp->setTileAtPosition = pd_api_gfx_playdate_tilemap_setTileAtPosition;
	Tmp->setTiles = pd_api_gfx_playdate_tilemap_setTiles;
    return Tmp;
}

playdate_graphics* pd_api_gfx_Create_playdate_graphics()
{
    playdate_graphics *Tmp = (playdate_graphics*) malloc(sizeof(*Tmp));
    
    _pd_api_gfx_Playdate_Screen = pd_api_gfx_newBitmap(LCD_COLUMNS, LCD_ROWS, kColorWhite);
    
    _pd_api_gfx_CurrentGfxContext = new GfxContext();
    _pd_api_gfx_CurrentGfxContext->BackgroundColor = kColorWhite;
    _pd_api_gfx_CurrentGfxContext->BitmapDrawMode = kDrawModeCopy;
    _pd_api_gfx_CurrentGfxContext->cliprect.x = -1;
    _pd_api_gfx_CurrentGfxContext->cliprect.y = -1;
    _pd_api_gfx_CurrentGfxContext->cliprect.w = -1;
    _pd_api_gfx_CurrentGfxContext->cliprect.h = -1;
    _pd_api_gfx_CurrentGfxContext->drawoffsetx = 0;
    _pd_api_gfx_CurrentGfxContext->drawoffsety = 0;
    _pd_api_gfx_CurrentGfxContext->DrawTarget = _pd_api_gfx_Playdate_Screen;
    _pd_api_gfx_CurrentGfxContext->font = _pd_api_gfx_Default_Font;
    _pd_api_gfx_CurrentGfxContext->linecapstyle = kLineCapStyleButt;
    _pd_api_gfx_CurrentGfxContext->stencil = NULL;
    _pd_api_gfx_CurrentGfxContext->tracking = 0;
    _pd_api_gfx_CurrentGfxContext->leading = 0;
    gfxstack.push_back(_pd_api_gfx_CurrentGfxContext);

    //SDL_SetRenderTarget(Renderer, _Playdate_Screen->Tex);
    SDL_SetSurfaceBlendMode(_pd_api_gfx_Playdate_Screen->Tex, SDL_BLENDMODE_NONE);
    Tmp->video = pd_api_gfx_Create_playdate_video();

	// LCDBitmap
	Tmp->newBitmap = pd_api_gfx_newBitmap;
	Tmp->freeBitmap = pd_api_gfx_freeBitmap;
	Tmp->loadBitmap = pd_api_gfx_loadBitmap;
	Tmp->copyBitmap = pd_api_gfx_copyBitmap;
	Tmp->loadIntoBitmap = pd_api_gfx_loadIntoBitmap;
	Tmp->getBitmapData = pd_api_gfx_getBitmapData;
	Tmp->clearBitmap = pd_api_gfx_clearBitmap;
	Tmp->rotatedBitmap = pd_api_gfx_rotatedBitmap;

	// LCDBitmapTable
	Tmp->newBitmapTable = pd_api_gfx_newBitmapTable;
	Tmp->freeBitmapTable = pd_api_gfx_freeBitmapTable;
	Tmp->loadBitmapTable = pd_api_gfx_loadBitmapTable;
	Tmp->loadIntoBitmapTable = pd_api_gfx_loadIntoBitmapTable;
	Tmp->getTableBitmap = pd_api_gfx_getTableBitmap;

    // Drawing Functions
	Tmp->clear = pd_api_gfx_clear;
	Tmp->setBackgroundColor = pd_api_gfx_setBackgroundColor;
	Tmp->setStencil = pd_api_gfx_setStencil;
	Tmp->setDrawMode = pd_api_gfx_setDrawMode;
	Tmp->setDrawOffset = pd_api_gfx_setDrawOffset;
	Tmp->setClipRect = pd_api_gfx_setClipRect;
	Tmp->clearClipRect = pd_api_gfx_clearClipRect;
	Tmp->setLineCapStyle = pd_api_gfx_setLineCapStyle;
	Tmp->setFont = pd_api_gfx_setFont;
	Tmp->setTextTracking = pd_api_gfx_setTextTracking;
	Tmp->pushContext = pd_api_gfx_pushContext;
	Tmp->popContext = pd_api_gfx_popContext;

	Tmp->drawBitmap = pd_api_gfx_drawBitmap;
	Tmp->tileBitmap = pd_api_gfx_tileBitmap;
	Tmp->drawLine = pd_api_gfx_drawLine;
	Tmp->fillTriangle = pd_api_gfx_fillTriangle;
	Tmp->drawRect = pd_api_gfx_drawRect;
	Tmp->fillRect = pd_api_gfx_fillRect;
	Tmp->drawEllipse = pd_api_gfx_drawEllipse;
	Tmp->fillEllipse = pd_api_gfx_fillEllipse;
	Tmp->drawScaledBitmap = pd_api_gfx_drawScaledBitmap;
	Tmp->drawText = pd_api_gfx_drawText;

    //font
    Tmp->loadFont = pd_api_gfx_loadFont;
	Tmp->getFontPage = pd_api_gfx_getFontPage;
	Tmp->getPageGlyph = pd_api_gfx_getPageGlyph;
	Tmp->getGlyphKerning = pd_api_gfx_getGlyphKerning;
	Tmp->getTextWidth = pd_api_gfx_getTextWidth;

    // raw framebuffer access
	Tmp->getFrame = pd_api_gfx_getFrame;
	Tmp->getDisplayFrame = pd_api_gfx_getDisplayFrame;
	Tmp->getDebugBitmap = pd_api_gfx_getDebugBitmap;
	Tmp->copyFrameBufferBitmap = pd_api_gfx_copyFrameBufferBitmap;
	Tmp->markUpdatedRows = pd_api_gfx_markUpdatedRows;
	Tmp->display = pd_api_gfx_display;

	// misc util.
	Tmp->setColorToPattern = pd_api_gfx_setColorToPattern;
	Tmp->checkMaskCollision = pd_api_gfx_checkMaskCollision;
	
	// 1.1
	Tmp->setScreenClipRect = pd_api_gfx_setScreenClipRect;
	
	// 1.1.1
	Tmp->fillPolygon = pd_api_gfx_fillPolygon;
	Tmp->getFontHeight = pd_api_gfx_getFontHeight;
	
	// 1.7
	Tmp->getDisplayBufferBitmap = pd_api_gfx_getDisplayBufferBitmap;
	Tmp->drawRotatedBitmap = pd_api_gfx_drawRotatedBitmap;
	Tmp->setTextLeading = pd_api_gfx_setTextLeading;

	// 1.8
	Tmp->setBitmapMask = pd_api_gfx_setBitmapMask;
    Tmp->getBitmapMask = pd_api_gfx_getBitmapMask;
	
	// 1.10
	Tmp->setStencilImage = pd_api_gfx_setStencilImage;
	
	// 1.12
	Tmp->makeFontFromData = pd_api_gfx_makeFontFromData;

	// 2.1
	Tmp->getTextTracking = pd_api_gfx_getTextTracking;
	
	// 2.5
	Tmp->setPixel = pd_api_gfx_setPixel;
	Tmp->getBitmapPixel = pd_api_gfx_getBitmapPixel;
	Tmp->getBitmapTableInfo = pd_api_gfx_getBitmapTableInfo;

	// 2.6
	Tmp->drawTextInRect = pd_api_gfx_drawTextInRect;

	// 2.7
	Tmp->getTextHeightForMaxWidth = pd_api_gfx_getTextHeightForMaxWidth;
	Tmp->drawRoundRect = pd_api_gfx_drawRoundRect;
	Tmp->fillRoundRect = pd_api_gfx_fillRoundRect;

	// 3.0
	Tmp->tilemap = pd_api_gfx_Create_playdate_tilemap();
	Tmp->videostream = pd_api_gfx_Create_playdate_videostream();
	

	return Tmp;
}


void _pd_api_gfx_freeFontList()
{
    for (auto& font:fontlist)
    {
        if(font->path)
            free(font->path);
        if(font->font)
		{
            if(font->font->isFntFont)
            {
                for(auto& pair : font->font->glyphs)
                    if(pair.second.surface)
                        SDL_FreeSurface(pair.second.surface);
                font->font->glyphs.clear();
                font->font->kerning.clear();
            }
            else if(font->font->font)
            {
                TTF_CloseFont(font->font->font);
            }
            delete font->font;
		}
		delete font;
    }
    fontlist.clear();
}

void _pd_api_gfx_cleanUp()
{
	Api->graphics->freeBitmap(_pd_api_gfx_Playdate_Screen);
}

uint32_t _pd_api_gfx_getBitmapChecksum(LCDBitmap *bitmap, uint8_t *buffer)
{
    uint32_t checksum = 0;
    const unsigned int rowbytes = (int)ceil(bitmap->w/8.0f);
    for (int y = 0; y < bitmap->h; y++)
    {
        for (int x = 0; x < bitmap->w; x++)
        {
            if (pd_api_gfx_samplepixel(buffer, x, y, rowbytes) == kColorWhite) {
                checksum += x + y * bitmap->w;
            }
        }
    }
    return checksum;
}

void _pd_api_gfx_checkBitmapNeedsRedraw(LCDBitmap *bitmap)
{
    if (!bitmap)
    {
        return;
    }
    // Never rewrite the screen bitmap from BitmapDataData — it is managed by
    // getFrame/markUpdatedRows and _pd_api_gfx_pushRowsToSurface already keeps
    // Tex in sync.  Letting the checksum logic run here would overwrite the
    // rendered frame with stale pre-render data.
    if (bitmap == _pd_api_gfx_Playdate_Screen)
    {
        return;
    }
    uint32_t dataChecksum = 0;
    uint32_t maskChecksum = 0;
    const bool mustRedrawData = bitmap->BitmapDataData && bitmap->BitmapDataDataChecksum != (dataChecksum = _pd_api_gfx_getBitmapChecksum(bitmap, bitmap->BitmapDataData));
    const bool mustRedrawMask = bitmap->BitmapDataMask && bitmap->BitmapDataMaskChecksum != (maskChecksum = _pd_api_gfx_getBitmapChecksum(bitmap, bitmap->BitmapDataMask));
    
    if (mustRedrawData || mustRedrawMask)
    {
        SDL_Surface* tmpTex = bitmap->Tex;
        bool texUnlocked = true;
        if (SDL_MUSTLOCK(tmpTex))
            texUnlocked = SDL_LockSurface(tmpTex) == 0;

        if (texUnlocked)
        {
            const unsigned int width = tmpTex->w;
            const unsigned int rowbytes = (int)ceil(width/8.0f);
            const unsigned int count = width * tmpTex->h;
            for (unsigned int index = 0; index < count; index++)
            {
                const int x = index % width;
                const int y = index / width;
                const int pixelIndex = (y * tmpTex->pitch) + (x * tmpTex->format->BytesPerPixel);

                SDL_Color color;
                Uint32 *pixel = (Uint32*)((Uint8*)tmpTex->pixels + pixelIndex);
                SDL_GetRGBA(*pixel, tmpTex->format, &color.r, &color.g, &color.b, &color.a);

                if (bitmap->BitmapDataData && pd_api_gfx_samplepixel(bitmap->BitmapDataData, x, y, rowbytes) == kColorWhite) {
                    color.r = pd_api_gfx_color_white.r;
                    color.g = pd_api_gfx_color_white.g;
                    color.b = pd_api_gfx_color_white.b;
                } else if (bitmap->BitmapDataData) {
                    color.r = pd_api_gfx_color_black.r;
                    color.g = pd_api_gfx_color_black.g;
                    color.b = pd_api_gfx_color_black.b;
                }

                if (mustRedrawMask && pd_api_gfx_samplepixel(bitmap->BitmapDataMask, x, y, rowbytes) == kColorWhite)
                {
                    color.a = SDL_ALPHA_OPAQUE;
                }
                else if (mustRedrawMask)
                {
                    color = pd_api_gfx_color_clear;
                }
                *pixel = SDL_MapRGBA(tmpTex->format, color.r, color.g, color.b, color.a);
            }

            if (SDL_MUSTLOCK(tmpTex))
                SDL_UnlockSurface(tmpTex);
        }

        if (mustRedrawData)
            bitmap->BitmapDataDataChecksum = dataChecksum;
        if (mustRedrawMask)
            bitmap->BitmapDataMaskChecksum = maskChecksum;
    }
}
