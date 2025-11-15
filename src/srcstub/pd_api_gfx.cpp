#include <memory>
#include <string.h>
#include <dirent.h>
#include <vector>
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
#include "defines.h"
#include "debug.h"


const char loaderror[] = "Failed loading!";

struct LCDBitmap;

struct LCDBitmap {
    SDL_Surface* Tex;
	LCDBitmap* Mask;
	SDL_Surface* MaskedTex;
	uint8_t *BitmapDataMask;
	uint8_t *BitmapDataData;
	bool MaskDirty;
	bool BitmapDirty;
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

struct LCDFont {
    TTF_Font * font;
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

void pd_api_gfx_MakeSurfaceBlackAndWhite(SDL_Surface *Img)
{
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
						continue;
					// Convert the pixel value to grayscale i.e. intensity
					SDL_GetRGBA(*p, Img->format, &r, &g, &b, &a);
					lum = 0.212671f  *r + 0.715160f  * g + 0.072169f  *b;
					//everything lower than half transparant make fully transparant
					if(a < COLORCONVERSIONALPHATHRESHOLD)
					{
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
}

void pd_api_gfx_MakeSurfaceOpaque(SDL_Surface *Img)
{
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
						continue;
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

// Drawing Functions
void pd_api_gfx_clear(LCDColor color)
{
    if (color == kColorXOR)
	{
		return;
	}

	//clear clears entire screen so need to remove cliprect temporary
	SDL_Rect rect;
	SDL_GetClipRect(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, &rect);
	SDL_SetClipRect(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, NULL);

    switch (color)
    {
        case kColorBlack:
            SDL_FillRect(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, NULL, SDL_MapRGBA(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a));
            break;
        case kColorWhite:
            SDL_FillRect(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, NULL, SDL_MapRGBA(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a));
            break;
        case kColorClear:
            SDL_FillRect(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, NULL, SDL_MapRGBA(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a));
            break;
        default:
            break;
    }
	SDL_SetClipRect(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, &rect);
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

void pd_api_gfx_setClipRect(int x, int y, int width, int height)
{
	SDL_Rect cliprect;
    cliprect.x = x + _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    cliprect.y = y + _pd_api_gfx_CurrentGfxContext->drawoffsety;
    cliprect.w = width;
    cliprect.h = height;
    SDL_SetClipRect(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, &cliprect);
}

void pd_api_gfx_clearClipRect(void)
{
    SDL_SetClipRect(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, NULL);
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
		SDL_SetClipRect(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, &_pd_api_gfx_CurrentGfxContext->cliprect);
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
    tmp->Tex = NULL;
	tmp->Mask = NULL;
	tmp->MaskedTex = NULL;
	tmp->BitmapDataMask = NULL;
	tmp->BitmapDataData = NULL;
    tmp->w = 0;
    tmp->h = 0;
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
            break;
        default:
            break;
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
        SDL_SetSurfaceBlendMode(result->Tex, SDL_BLENDMODE_NONE);
        pd_api_gfx_clearBitmap(result, bgcolor);		
    }
    return result;
}

void pd_api_gfx_freeBitmap(LCDBitmap* Bitmap)
{
    if(Bitmap == NULL)
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

	SDL_FreeSurface(Bitmap->Tex);

    free(Bitmap);
    Bitmap = NULL;
}

LCDBitmap* pd_api_gfx_loadBitmap(const char* path, const char** outerr)
{
    *outerr = loaderror;    
    LCDBitmap* result = NULL;
    char ext[5];
    char* tmpfullpath = (char *) malloc((strlen(path) + 7) * sizeof(char));
	char* fullpath = (char *) malloc((strlen(path) + 17) * sizeof(char));
    bool needextension = true;
    if(strlen(path) > 4)
    {
        strcpy(ext, path + (strlen(path) - 4));
        needextension = strcasecmp(ext, ".PNG") != 0;
    }
    if (needextension)
	{
        sprintf(tmpfullpath,"./%s.png", path);
	}
    else
	{
        sprintf(tmpfullpath, "./%s", path);
	}

	sprintf(fullpath, "./%s/%s",_pd_api_get_current_source_dir(), tmpfullpath);
	struct stat lstats;
	if(stat(fullpath, &lstats) != 0)
		sprintf(fullpath, "./%s", tmpfullpath);

    SDL_Surface* Img = IMG_Load(fullpath);
    if(Img)
    {
		SDL_Surface* Img2 = SDL_ConvertSurfaceFormat(Img, pd_api_gfx_PIXELFORMAT, 0);
		if (Img2)
		{
			if (_pd_current_source_dir == 0)
				pd_api_gfx_MakeSurfaceBlackAndWhite(Img2);
			else
				pd_api_gfx_MakeSurfaceOpaque(Img2);
			result = pd_api_gfx_newBitmap(Img2->w, Img2->h, kColorClear);
			if(result)
			{
				*outerr = NULL;
				SDL_SetSurfaceBlendMode(Img2, SDL_BLENDMODE_BLEND);
				SDL_BlitSurface(Img2, NULL, result->Tex, NULL);				
				SDL_SetSurfaceBlendMode(result->Tex, SDL_BLENDMODE_NONE);
				//SDL_SetSurfaceRLE(result->Tex, SDL_RLEACCEL);
				}
			SDL_FreeSurface(Img2);
		}
		SDL_FreeSurface(Img);
    }   
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
	int rb = (int)ceil(bitmap->w /8);
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
							pd_api_gfx_drawpixel(*mask, x, y, rb, kColorWhite); 
						
						if(data)
						{
							if(pval >= whitethreshold)
							{

								pd_api_gfx_drawpixel(*data, x, y, rb, kColorWhite); 
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
        pd_api_gfx_freeBitmap(table->bitmaps[i]);
    }
	free(table->bitmaps);
	table->bitmaps = NULL;
    free(table);
    table = NULL;
}

LCDBitmapTable* _pd_api_gfx_do_loadBitmapTable(const char* path, const char** outerr)
{
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
							*outerr = NULL;
								
							while(Tmp)
							{
								Img = SDL_ConvertSurfaceFormat(Tmp, pd_api_gfx_PIXELFORMAT, 0);
								SDL_FreeSurface(Tmp);
								
								if (_pd_current_source_dir == 0)
									pd_api_gfx_MakeSurfaceBlackAndWhite(Img);
								else
									pd_api_gfx_MakeSurfaceOpaque(Img);

								SDL_SetSurfaceBlendMode(Img, SDL_BLENDMODE_BLEND);
								result->bitmaps = (LCDBitmap **) realloc(result->bitmaps, (result->count+1) * sizeof(*result->bitmaps));
								result->bitmaps[result->count] = pd_api_gfx_newBitmap(result->w, result->h, kColorClear);
								if(result->bitmaps[result->count])
								{
									SDL_BlitSurface(Img, NULL, result->bitmaps[result->count]->Tex, NULL);
									SDL_SetSurfaceBlendMode(result->bitmaps[result->count]->Tex, SDL_BLENDMODE_NONE);
									result->count++;
								}
								else
								{
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
						if (_pd_current_source_dir == 0)
							pd_api_gfx_MakeSurfaceBlackAndWhite(Img);
						else
							pd_api_gfx_MakeSurfaceOpaque(Img);
						result->bitmaps = (LCDBitmap **) malloc(sizeof(*result->bitmaps));
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
									result->count++;
								}
								else
								{
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

LCDBitmapTable* pd_api_gfx_loadBitmapTable(const char* path, const char** outerr)
{
	char* fullpath = (char *) malloc((strlen(path) + 17) * sizeof(char));
    sprintf(fullpath, "./%s/%s", _pd_api_get_current_source_dir(), path);
	LCDBitmapTable *result = _pd_api_gfx_do_loadBitmapTable(fullpath, outerr);
	free(fullpath);
	if(!result)
		result = _pd_api_gfx_do_loadBitmapTable(path, outerr);
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
uint8_t* pd_api_gfx_getFrame(void) // row stride = LCD_ROWSIZE
{
    return 0;
}

uint8_t* pd_api_gfx_getDisplayFrame(void) // row stride = LCD_ROWSIZE
{
    return 0;
}

LCDBitmap* pd_api_gfx_getDebugBitmap(void) // valid in simulator only, function is NULL on device
{
    return NULL;
}

LCDBitmap* pd_api_gfx_copyFrameBufferBitmap(void)
{
    return pd_api_gfx_copyBitmap(_pd_api_gfx_Playdate_Screen);
}

void pd_api_gfx_markUpdatedRows(int start, int end)
{

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
    SDL_SetClipRect(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, &cliprect);
}
	
// 1.1.1
void pd_api_gfx_fillPolygon(int nPoints, int* coords, LCDColor color, LCDPolygonFillRule fillrule)
{
    LCDBitmap *bitmap = NULL;
    if (color == kColorXOR)
    {
        bitmap = pd_api_gfx_copyBitmap(_pd_api_gfx_CurrentGfxContext->DrawTarget);
        pd_api_gfx_pushContext(bitmap);
        pd_api_gfx_clear(kColorClear);
    }
    
    
    Sint16 vx[nPoints];
    Sint16 vy[nPoints];
  
    for (int i=0; i<nPoints*2;i+=2)
    {
        vx[i] = coords[i];
        vy[i] = coords[i+1];
    }

    switch (color)
    {
        case kColorBlack:
            filledPolygonRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, vx, vy, nPoints, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            filledPolygonRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, vx, vy, nPoints, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            filledPolygonRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, vx, vy, nPoints, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            filledPolygonRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, vx, vy, nPoints, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }

    if (color == kColorXOR)
    {
        pd_api_gfx_popContext();
        pd_api_gfx_pushContext(_pd_api_gfx_CurrentGfxContext->DrawTarget);
        pd_api_gfx_setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, 0,0, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped);
        pd_api_gfx_popContext();
        pd_api_gfx_freeBitmap(bitmap);
    }
}

uint8_t pd_api_gfx_getFontHeight(LCDFont* font)
{
    LCDFont *f = font;
    if(!f)
        f = _pd_api_gfx_Default_Font;

    if(!f)
        return 0;

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

	//the mask seems to be a copy
	if(bitmap->Mask)
		pd_api_gfx_freeBitmap(bitmap->Mask);
	
	bitmap->Mask = pd_api_gfx_copyBitmap(mask);
	bitmap->MaskDirty = true;

	return 1;
}

LCDBitmap* pd_api_gfx_getBitmapMask(LCDBitmap* bitmap)
{
	if(!bitmap || !ENABLEBITMAPMASKS)
		return NULL;

	return bitmap->Mask;
}
	
// 1.10
void pd_api_gfx_setStencilImage(LCDBitmap* stencil, int tile)
{

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

void _pd_api_gfx_drawBitmapAll(LCDBitmap* bitmap, int x, int y, float xscale, float yscale, bool isRotatedBitmap, const double angle, float centerx, float centery, LCDBitmapFlip flip)
{
	pd_api_gfx_recreatemaskedimage(bitmap);
    SDL_Rect dstrect;
    dstrect.x = x +  _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    dstrect.y = y +  _pd_api_gfx_CurrentGfxContext->drawoffsety;
    dstrect.w = bitmap->w;
    dstrect.h = bitmap->h;

	SDL_Surface *tmpTexture1 = NULL;
	SDL_Surface *tmpTexture = NULL;

	if(bitmap->MaskedTex)
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

		if(!isOrginalTexture)
			SDL_FreeSurface(tmpTexture);
		tmpTexture = tmpTexture1;
		isOrginalTexture = false;
		SDL_SetSurfaceBlendMode(tmpTexture, SDL_BLENDMODE_NONE);
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
			SDL_BlitSurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, &dstrect, drawtargetsurface, NULL);
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
					int width = std::min(_pd_api_gfx_CurrentGfxContext->DrawTarget->w, dstrect.w);
					int height = std::min(_pd_api_gfx_CurrentGfxContext->DrawTarget->h, dstrect.h);
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
					int width = std::min(_pd_api_gfx_CurrentGfxContext->DrawTarget->w, dstrect.w);
					int height = std::min(_pd_api_gfx_CurrentGfxContext->DrawTarget->h, dstrect.h);
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
					int width = std::min(_pd_api_gfx_CurrentGfxContext->DrawTarget->w, dstrect.w);
					int height = std::min(_pd_api_gfx_CurrentGfxContext->DrawTarget->h, dstrect.h);
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
					int width = std::min(_pd_api_gfx_CurrentGfxContext->DrawTarget->w, dstrect.w);
					int height = std::min(_pd_api_gfx_CurrentGfxContext->DrawTarget->h, dstrect.h);
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
					int width = std::min(_pd_api_gfx_CurrentGfxContext->DrawTarget->w, dstrect.w);
					int height = std::min(_pd_api_gfx_CurrentGfxContext->DrawTarget->h, dstrect.h);
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
						int width = std::min(_pd_api_gfx_CurrentGfxContext->DrawTarget->w, dstrect.w);
						int height = std::min(_pd_api_gfx_CurrentGfxContext->DrawTarget->h, dstrect.h);
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
						int width = std::min(_pd_api_gfx_CurrentGfxContext->DrawTarget->w, dstrect.w);
						int height = std::min(_pd_api_gfx_CurrentGfxContext->DrawTarget->h, dstrect.h);
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

	Uint32 cclear = SDL_MapRGBA(tmpTexture->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
	SDL_SetColorKey(tmpTexture, SDL_TRUE, cclear);
	SDL_BlitSurface(tmpTexture, NULL, _pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, &dstrect);


	if(!isOrginalTexture)
		SDL_FreeSurface(tmpTexture);
	
	_pd_api_gfx_CurrentGfxContext->DrawTarget->BitmapDirty = true;
}

void pd_api_gfx_drawRotatedBitmap(LCDBitmap* bitmap, int x, int y, float rotation, float centerx, float centery, float xscale, float yscale)
{
    if (bitmap == NULL)
        return;
    
    _pd_api_gfx_drawBitmapAll(bitmap, x, y,xscale, yscale, true, rotation, centerx, centery, kBitmapUnflipped);
}

void pd_api_gfx_drawBitmap(LCDBitmap* bitmap, int x, int y, LCDBitmapFlip flip)
{
    if (bitmap == NULL)
        return;
   
    _pd_api_gfx_drawBitmapAll(bitmap, x, y, 1.0f, 1.0f, false, 0, 0, 0, flip);
}

void pd_api_gfx_tileBitmap(LCDBitmap* bitmap, int x, int y, int width, int height, LCDBitmapFlip flip)
{

}

void pd_api_gfx_drawScaledBitmap(LCDBitmap* bitmap, int x, int y, float xscale, float yscale)
{
    if (bitmap == NULL)
        return;

    _pd_api_gfx_drawBitmapAll(bitmap, x, y, xscale, yscale, false, 0, 0, 0, kBitmapUnflipped);
}

void pd_api_gfx_drawLine(int x1, int y1, int x2, int y2, int width, LCDColor color)
{
    //for xor we are abusing the api to draw on a bitmap and then draw that bitmap on the current target using xor mode
    LCDBitmap *bitmap = NULL;
    int minx;
    int maxx;
    int miny;
    int maxy;

    if (color == kColorXOR)
    {
        minx = std::min(x1,x2);
        maxx = std::max(x1,x2);
        miny = std::min(y1,y2);
        maxy = std::max(y1,y2);
        
        int width = maxx-minx;
        int height = maxy-miny;
        
        x1 -= minx;
        x2 -= minx;
        y1 -= miny;
        y2 -= miny; 

        bitmap = Api->graphics->newBitmap(width, height, kColorClear);
        Api->graphics->pushContext(bitmap);
    }
	else
	{
		x1 += _pd_api_gfx_CurrentGfxContext->drawoffsetx;
		y1 += _pd_api_gfx_CurrentGfxContext->drawoffsety;
		x2 += _pd_api_gfx_CurrentGfxContext->drawoffsetx;
		y2 += _pd_api_gfx_CurrentGfxContext->drawoffsety;
	}
	
	switch (color)
    {
        case kColorBlack:
            lineRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, x1, y1, x2, y2, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            lineRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, x1, y1, x2, y2, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            lineRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, x1, y1, x2, y2, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            lineRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, x1, y1, x2, y2, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }

    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(_pd_api_gfx_CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, minx, miny, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }
	_pd_api_gfx_CurrentGfxContext->DrawTarget->BitmapDirty = true;
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
    
    LCDBitmap *bitmap = NULL;
    int minx;
    int maxx;
    int miny;
    int maxy;
    
    if (color == kColorXOR)
    {
        minx = std::min(std::min(points[0].x,points[1].x),points[2].x);
        maxx = std::max(std::max(points[0].x,points[1].x),points[2].x);
        miny = std::min(std::min(points[0].y,points[1].y),points[2].y);
        maxy = std::max(std::max(points[0].y,points[1].y),points[2].y);
        
        int width = maxx-minx;
        int height = maxy-miny;
        
        points[0].x -= minx;
        points[1].x -= minx;
        points[2].x -= minx;
        points[3].x -= minx;

        points[0].y -= miny;
        points[1].y -= miny;
        points[2].y -= miny;
        points[3].y -= miny;

        bitmap = Api->graphics->newBitmap(width, height, kColorClear);
        Api->graphics->pushContext(bitmap);
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

	
	switch (color)
    {
        case kColorBlack:
            filledTrigonRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            filledTrigonRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            filledTrigonRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            filledTrigonRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
    
   
    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(_pd_api_gfx_CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, minx, miny, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }
	_pd_api_gfx_CurrentGfxContext->DrawTarget->BitmapDirty = true;
}

void pd_api_gfx_drawRect(int x, int y, int width, int height, LCDColor color)
{
    //for xor we are abusing the api to draw on a bitmap and then draw that bitmap on the current target using xor mode
    LCDBitmap *bitmap = NULL;
    if (color == kColorXOR)
    {
        bitmap = Api->graphics->newBitmap(width, height, kColorClear);
        Api->graphics->pushContext(bitmap);
    }
    

    SDL_Rect rect;
    rect.x = color == kColorXOR ? 0: x + _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    rect.y = color == kColorXOR ? 0: y + _pd_api_gfx_CurrentGfxContext->drawoffsety;
    rect.w = width;
    rect.h = height;
    
	switch (color)
    {
        case kColorBlack:
            rectangleRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, rect.x, rect.y, rect.x + rect.w-1, rect.y + rect.h-1, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            rectangleRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, rect.x, rect.y, rect.x + rect.w-1, rect.y + rect.h-1, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            rectangleRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, rect.x, rect.y, rect.x + rect.w-1, rect.y + rect.h-1, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            rectangleRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, rect.x, rect.y, rect.x + rect.w-1, rect.y + rect.h-1, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
	
    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(_pd_api_gfx_CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, x, y, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }
	_pd_api_gfx_CurrentGfxContext->DrawTarget->BitmapDirty = true;
}

void pd_api_gfx_fillRect(int x, int y, int width, int height, LCDColor color)
{
	LCDBitmap *pattern = NULL;
	Uint32 RealColor;
	switch (color)
    {
        case kColorBlack:
            RealColor = SDL_MapRGBA(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            RealColor = SDL_MapRGBA(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            RealColor = SDL_MapRGBA(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            RealColor = SDL_MapRGBA(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
			//assume lcd pattern
			RealColor = 0;
			pattern = pd_api_gfx_PatternToBitmap((uint8_t*)color);
			break;
    }

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
		Api->graphics->pushContext(bitmap);
		for (int yy = 0; yy < tilesy; yy++)
			for (int xx = 0; xx < tilesx; xx++)
				_pd_api_gfx_drawBitmapAll(pattern, (xx*8) -xoffset, (yy*8) -yoffset, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped);
		Api->graphics->popContext();
		Api->graphics->drawBitmap(bitmap,x + _pd_api_gfx_CurrentGfxContext->drawoffsetx,  y + _pd_api_gfx_CurrentGfxContext->drawoffsety, kBitmapUnflipped);
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

    	SDL_FillRect(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, &rect, RealColor);
	}

	if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(_pd_api_gfx_CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, x, y, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }
	_pd_api_gfx_CurrentGfxContext->DrawTarget->BitmapDirty = true;
}

void pd_api_gfx_drawEllipse(int x, int y, int width, int height, int lineWidth, float startAngle, float endAngle, LCDColor color) // stroked inside the rect
{
    //for xor we are abusing the api to draw on a bitmap and then draw that bitmap on the current target using xor mode
    LCDBitmap *bitmap = NULL;
    if (color == kColorXOR)
    {
        bitmap = Api->graphics->newBitmap(width+1, height+1, kColorClear);
        Api->graphics->pushContext(bitmap);
    }
    //SDL's implementation takes double while with and height with playdate api is the with of the bounding rectangle
    width = width >> 1;
    height = height >> 1;
    
    int oldx = x;
    int oldy = y;

    x = color == kColorXOR ? width  : x + width + _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    y = color == kColorXOR ? height:  y + height + _pd_api_gfx_CurrentGfxContext->drawoffsety;
    
    switch (color)
    {
        case kColorBlack:
            ellipseRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            ellipseRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            ellipseRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            ellipseRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
    

    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(_pd_api_gfx_CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, oldx, oldy, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);        
    }
	_pd_api_gfx_CurrentGfxContext->DrawTarget->BitmapDirty = true;
}

void pd_api_gfx_fillEllipse(int x, int y, int width, int height, float startAngle, float endAngle, LCDColor color)
{
    //for xor we are abusing the api to draw on a bitmap and then draw that bitmap on the current target using xor mode
    LCDBitmap *bitmap = NULL;
    if (color == kColorXOR)
    {
        bitmap = Api->graphics->newBitmap(width+1, height+1, kColorClear);
        Api->graphics->pushContext(bitmap);
    }

    //SDL's implementation takes double while with and height with playdate api is the with of the bounding rectangle
    width = width >> 1;
    height = height >> 1;

    int oldx = x;
    int oldy = y;

    x = color == kColorXOR ? width  : x + width + _pd_api_gfx_CurrentGfxContext->drawoffsetx;
    y = color == kColorXOR ? height:  y + height + _pd_api_gfx_CurrentGfxContext->drawoffsety;

         
    switch (color)
    {
        case kColorBlack:
            filledEllipseRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            filledEllipseRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            filledEllipseRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            filledEllipseRGBASurface(_pd_api_gfx_CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
    
    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(_pd_api_gfx_CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, oldx, oldy, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);        
    }
	_pd_api_gfx_CurrentGfxContext->DrawTarget->BitmapDirty = true;
}


// LCDFont
LCDFont* pd_api_gfx_Create_LCDFont()
{
    LCDFont* tmp = (LCDFont* ) malloc(sizeof(*tmp));
    tmp->font = NULL;
    return tmp;
}

LCDFont* pd_api_gfx_loadFont(const char* path, const char** outErr)
{
    *outErr = loaderror;    
    LCDFont* result = NULL;
    char* tmpfullpath =  (char *) malloc((strlen(path) + 7) * sizeof(char));
	char* fullpath =  (char *) malloc((strlen(path) + 17) * sizeof(char));
    sprintf(tmpfullpath, "./%s", path);
    char* ext = strrchr(tmpfullpath, '.');
    if (ext)
    {
        if(strlen(ext) == 4)
        {
            ext[1] = 't';
            ext[2] = 't';
            ext[3] = 'f';
        }
        else
		{
			sprintf(tmpfullpath, "./%s.ttf", path);
		}
    }
    else
	{
        sprintf(tmpfullpath, "./%s.ttf", path);
	}


	sprintf(fullpath, "./%s/%s",_pd_api_get_current_source_dir(), tmpfullpath);
	struct stat lstats;
	if(stat(fullpath, &lstats) != 0)
		sprintf(fullpath, "%s", tmpfullpath);

	//check font list to see if we already loaded the font
    for (auto& font : fontlist)
    {
        if (strcasecmp(font->path, fullpath) == 0)
        {
            *outErr = NULL;
            free(fullpath);
			free(tmpfullpath);
            return font->font;
        }
    }

    TTF_Font* font = TTF_OpenFont(fullpath, 12);
    if(font)
    {
        result = pd_api_gfx_Create_LCDFont();
        if(result)
        {
            *outErr = NULL;
            TTF_SetFontStyle(font, TTF_STYLE_NORMAL);
            result->font = font;

            FontListEntry* Tmp = new FontListEntry();
            Tmp->path = (char *) malloc(strlen(fullpath) + 1);
            strcpy(Tmp->path, fullpath);
            Tmp->font = result;
            fontlist.push_back(Tmp);
        }
    }   

    free(fullpath);
	free(tmpfullpath);
    return result;
}

void _pd_api_gfx_loadDefaultFonts()
{
	const char* err;
	_pd_api_gfx_Default_Font =  pd_api_gfx_loadFont("System/Fonts/Asheville-Sans-14-Light.ttf", &err);
    _pd_api_gfx_FPS_Font =  pd_api_gfx_loadFont("System/Fonts/Roobert-10-Bold.ttf", &err);
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
	{
        f = _pd_api_gfx_Default_Font;
	}
    
    if(!f)
	{
        return 0;
	}
    
    if(!f->font)
	{
        return 0;
	}
	
	if(len == 0)
	{
		return 0;
	}

	if(strlen((char*)text) == 0)
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

	int i, numLines, rowHeight, lineskip;
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

	/* Get the dimensions of the text surface */
	if ((TTF_SizeUTF8(f->font, text_cpy, &width, &height) < 0) || !width) {
		SDL_SetError("Text has zero width");
		if (utf8_alloc)
			free(utf8_alloc);
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
						free(utf8_alloc);
					if (strLines)
						free(strLines);
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
	height = rowHeight + lineskip * (numLines - 1);

	return width;
}

//based on functionality from SDL_TTF
int pd_api_gfx_drawText(const void* text, size_t len, PDStringEncoding encoding, int x, int y)
{
    if(len == 0)
        return 0;

    if(_pd_api_gfx_CurrentGfxContext->font == NULL)
    {
        return -1;
    }

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
				free(utf8_alloc);
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
							free(utf8_alloc);
						if (strLines)
							free(strLines);
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
		height = rowHeight + lineskip * (numLines - 1);

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
			/* Move to i-th line */
			ystart += i * lineskip;
		
			SDL_Surface* TextSurface = TTF_RenderUTF8_Solid(_pd_api_gfx_CurrentGfxContext->font->font, newtext, pd_api_gfx_color_black);
			if (TextSurface) 
        	{
            	SDL_Surface* Texture = SDL_ConvertSurfaceFormat(TextSurface, _pd_api_gfx_CurrentGfxContext->DrawTarget->Tex->format->format, 0);
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

}

LCDSolidColor pd_api_gfx_getBitmapPixel(LCDBitmap* bitmap, int x, int y)
{
	return kColorBlack;
}

void pd_api_gfx_getBitmapTableInfo(LCDBitmapTable* table, int* count, int* width)
{
	if(table)
	{
		*count = table->count;
		*width = table->across;
	}
}

// 2.6
void pd_api_gfx_drawTextInRect(const void* text, size_t len, PDStringEncoding encoding, int x, int y, int width, int height, PDTextWrappingMode wrap, PDTextAlignment align)
{

}

// 2.7
int pd_api_gfx_getTextHeightForMaxWidth(LCDFont* font, const void* text, size_t len, int maxwidth, PDStringEncoding encoding, PDTextWrappingMode wrap, int tracking, int extraLeading)
{
	return 0;
}

void pd_api_gfx_drawRoundRect(int x, int y, int width, int height, int radius, int lineWidth, LCDColor color)
{

}

void pd_api_gfx_fillRoundRect(int x, int y, int width, int height, int radius, LCDColor color)
{

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
    _pd_api_gfx_loadDefaultFonts();
    
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
            if(font->font->font)
                TTF_CloseFont(font->font->font);
			free(font->font);
		}
		delete font;
    }
    fontlist.clear();
}

void _pd_api_gfx_cleanUp()
{
	Api->graphics->freeBitmap(_pd_api_gfx_Playdate_Screen);
}