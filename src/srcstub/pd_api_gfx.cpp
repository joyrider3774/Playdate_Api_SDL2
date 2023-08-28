#include <string.h>
#include <dirent.h>
#include <vector>
#include <SDL_rect.h>
#include <SDL_blendmode.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_rotate.h>
#include <SDL2_rotozoom.h>
#include <SDL_ttf.h>
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
    int w;
    int h;
};

struct LCDBitmapTable {
    int count;
    int w;
    int h;
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

GfxContext* CurrentGfxContext;
LCDBitmap* _Playdate_Screen;
LCDFont* _Default_Font = NULL;
LCDFont* _FPS_Font = NULL;

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
				if (Pattern[7-y] & (1 << (7-x)))
					*p = white;
				else
					*p = black;

				
				Uint32 *p2 = (Uint32*)((Uint8 *)mask->Tex->pixels + ((y)  * mask->Tex->pitch) + (x * mask->Tex->format->BytesPerPixel));
				if (Pattern[15-y] & (1 << (7-x)))
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
	return result;
}

void pd_api_gfx_recreatemaskedimage(LCDBitmap* bitmap)
{
	if (pd_api_gfx_disable_masks)
		return;
	if(bitmap->Mask)
	{
		if(bitmap->MaskedTex)
			SDL_FreeSurface(bitmap->MaskedTex);
		SDL_BlendMode blendMode;
		bitmap->MaskedTex = SDL_CreateRGBSurfaceWithFormat(0, bitmap->Tex->w, bitmap->Tex->h, bitmap->Tex->format->BitsPerPixel, bitmap->Tex->format->format);
		SDL_SetSurfaceBlendMode(bitmap->MaskedTex, SDL_BLENDMODE_NONE);
		Uint32 clear = SDL_MapRGBA(bitmap->MaskedTex->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
		//SDL_FillRect(bitmap->MaskedTex, NULL, clear);
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
			Uint32 black = SDL_MapRGBA(bitmap->MaskedTex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
			Uint32 white = SDL_MapRGBA(bitmap->MaskedTex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
			
			Uint32 blackthreshold = SDL_MapRGBA(bitmap->MaskedTex->format, pd_api_gfx_color_blacktreshold.r, pd_api_gfx_color_blacktreshold.g, pd_api_gfx_color_blacktreshold.b, pd_api_gfx_color_blacktreshold.a);
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
					Uint32 pval = *p;
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
    Api->system->formatString(&Text,"%2.0f", _LastFPS);
    Api->graphics->setFont(_FPS_Font);
    int w = Api->graphics->getTextWidth(_FPS_Font, Text, strlen(Text), kASCIIEncoding, 0);
    int h = Api->graphics->getFontHeight(_FPS_Font);
    Api->graphics->fillRect(x, y, w, h, kColorWhite);
    Api->graphics->drawText(Text, strlen(Text), kASCIIEncoding, x, y);
    Api->system->realloc(Text, 0);
    Api->graphics->popContext();
}

LCDBitmapDrawMode _pd_api_gfx_getCurrentDrawMode()
{
	return CurrentGfxContext->BitmapDrawMode;
}

LCDColor getBackgroundDrawColor()
{
    return CurrentGfxContext->BackgroundColor;
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
	SDL_GetClipRect(CurrentGfxContext->DrawTarget->Tex, &rect);
	SDL_SetClipRect(CurrentGfxContext->DrawTarget->Tex, NULL);

    switch (color)
    {
        case kColorBlack:
            SDL_FillRect(CurrentGfxContext->DrawTarget->Tex, NULL, SDL_MapRGBA(CurrentGfxContext->DrawTarget->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a));
            break;
        case kColorWhite:
            SDL_FillRect(CurrentGfxContext->DrawTarget->Tex, NULL, SDL_MapRGBA(CurrentGfxContext->DrawTarget->Tex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a));
            break;
        case kColorClear:
            SDL_FillRect(CurrentGfxContext->DrawTarget->Tex, NULL, SDL_MapRGBA(CurrentGfxContext->DrawTarget->Tex->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a));
            break;
        default:
            break;
    }
	SDL_SetClipRect(CurrentGfxContext->DrawTarget->Tex, &rect);
}

void pd_api_gfx_setBackgroundColor(LCDSolidColor color)
{
    CurrentGfxContext->BackgroundColor = color;
}

void pd_api_gfx_setStencil(LCDBitmap* stencil) // deprecated in favor of setStencilImage, which adds a "tile" flag
{
    CurrentGfxContext->stencil = stencil;
}

void pd_api_gfx_setDrawMode(LCDBitmapDrawMode mode)
{
    CurrentGfxContext->BitmapDrawMode = mode;
}

void pd_api_gfx_setDrawOffset(int dx, int dy)
{
    CurrentGfxContext->drawoffsetx = dx;
    CurrentGfxContext->drawoffsety = dy;
}

void pd_api_gfx_setClipRect(int x, int y, int width, int height)
{
	SDL_Rect cliprect;
    cliprect.x = x + CurrentGfxContext->drawoffsetx;
    cliprect.y = y + CurrentGfxContext->drawoffsety;
    cliprect.w = width;
    cliprect.h = height;
    SDL_SetClipRect(CurrentGfxContext->DrawTarget->Tex, &cliprect);
}

void pd_api_gfx_clearClipRect(void)
{
    SDL_SetClipRect(CurrentGfxContext->DrawTarget->Tex, NULL);
}

void pd_api_gfx_setLineCapStyle(LCDLineCapStyle endCapStyle)
{
    CurrentGfxContext->linecapstyle = endCapStyle;
}

void pd_api_gfx_setFont(LCDFont* font)
{
    if(!font)
        CurrentGfxContext->font = _Default_Font;
    else
        CurrentGfxContext->font = font;
}

void pd_api_gfx_setTextTracking(int tracking)
{
    CurrentGfxContext->tracking = tracking;
}

void pd_api_gfx_pushContext(LCDBitmap* target)
{	
    GfxContext* Tmp = new GfxContext();
	//bitmap drawmode remains same
	Tmp->BitmapDrawMode = CurrentGfxContext->BitmapDrawMode;
	//font is not changed it seems
	Tmp->font = CurrentGfxContext->font;
	//background color seems to be set to white on a push
	Tmp->BackgroundColor = kColorWhite; 
	//draw offset is reset
	Tmp->drawoffsetx = 0;
	Tmp->drawoffsety = 0;
	//need to verify these 3 below like its possible they get reset to some fixed value
	Tmp->linecapstyle = CurrentGfxContext->linecapstyle;
	Tmp->stencil = CurrentGfxContext->stencil;
	Tmp->tracking = CurrentGfxContext->tracking;

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
        Tmp->DrawTarget = _Playdate_Screen;
    }
	//save the cliprect it is the cliprect from the target bitmap
	SDL_Rect clipRect;
	SDL_GetClipRect(Tmp->DrawTarget->Tex, &clipRect);
	Tmp->cliprect = clipRect; 
	gfxstack.push_back(Tmp);
    CurrentGfxContext = Tmp;
}

void pd_api_gfx_popContext(void)
{
    //we should awlays have 1 item it is added when creating the graphics api subset!
    if(gfxstack.size() > 1)
    {
		//restore the cliprect of the current drawtarget first (we had it store in pushcontext)
		SDL_SetClipRect(CurrentGfxContext->DrawTarget->Tex, &CurrentGfxContext->cliprect);
		gfxstack.pop_back();
        CurrentGfxContext = gfxstack[gfxstack.size()-1];
        //restore drawtarget
		if(!CurrentGfxContext->DrawTarget)
		{
			CurrentGfxContext->DrawTarget = _Playdate_Screen;
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

	SDL_FreeSurface(Bitmap->Tex);

    free(Bitmap);
    Bitmap = NULL;
}

LCDBitmap* pd_api_gfx_loadBitmap(const char* path, const char** outerr)
{
    *outerr = loaderror;    
    LCDBitmap* result = NULL;
    char ext[5];
    char* fullpath = (char *) malloc((strlen(path) + 7) * sizeof(char));
    bool needextension = true;
    if(strlen(path) > 4)
    {
        strcpy(ext, path + (strlen(path) - 4));
        needextension = strcasecmp(ext, ".PNG") != 0;
    }
    if (needextension)
        sprintf(fullpath,"./%s.png", path);
    else
        sprintf(fullpath, "./%s", path);

    SDL_Surface* Img = IMG_Load(fullpath);
    if(Img)
    {
		SDL_Surface* Img2 = SDL_ConvertSurfaceFormat(Img, pd_api_gfx_PIXELFORMAT, 0);
		if (Img2)
		{
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
    bitmap = pd_api_gfx_loadBitmap(path, outerr);
}

void pd_api_gfx_getBitmapData(LCDBitmap* bitmap, int* width, int* height, int* rowbytes, uint8_t** mask, uint8_t** data)
{
    if(mask)
    {
        *mask = NULL;
    }
    if(data)
    {
        *data = NULL;
    }
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
    if(rowbytes)
    {
        rowbytes = 0;
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
    free(table);
    table = NULL;
}

LCDBitmapTable* pd_api_gfx_loadBitmapTable(const char* path, const char** outerr)
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
                    result->bitmaps = (LCDBitmap **) malloc(sizeof(*result->bitmaps));
                    *outerr = NULL;
                    result->w = w;
                    result->h = h;
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
        free(fullpath);
        free(tmpPath); 
    }
    return result;
}

void pd_api_gfx_loadIntoBitmapTable(const char* path, LCDBitmapTable* table, const char** outerr)
{
    table = pd_api_gfx_loadBitmapTable(path, outerr);
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
    return pd_api_gfx_copyBitmap(_Playdate_Screen);
}

void pd_api_gfx_markUpdatedRows(int start, int end)
{

}

void pd_api_gfx_display(void)
{

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
    SDL_SetClipRect(CurrentGfxContext->DrawTarget->Tex, &cliprect);
}
	
// 1.1.1
void pd_api_gfx_fillPolygon(int nPoints, int* coords, LCDColor color, LCDPolygonFillRule fillrule)
{
    LCDBitmap *bitmap = NULL;
    if (color == kColorXOR)
    {
        bitmap = pd_api_gfx_copyBitmap(CurrentGfxContext->DrawTarget);
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
            filledPolygonRGBASurface(CurrentGfxContext->DrawTarget->Tex, vx, vy, nPoints, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            filledPolygonRGBASurface(CurrentGfxContext->DrawTarget->Tex, vx, vy, nPoints, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            filledPolygonRGBASurface(CurrentGfxContext->DrawTarget->Tex, vx, vy, nPoints, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            filledPolygonRGBASurface(CurrentGfxContext->DrawTarget->Tex, vx, vy, nPoints, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }

    if (color == kColorXOR)
    {
        pd_api_gfx_popContext();
        pd_api_gfx_pushContext(CurrentGfxContext->DrawTarget);
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
        f = _Default_Font;

    if(!f)
        return 0;

    if(!f->font)
        return 0;

    return TTF_FontHeight(f->font);
}
	
// 1.7
LCDBitmap* pd_api_gfx_getDisplayBufferBitmap(void)
{
    return _Playdate_Screen;
}

void pd_api_gfx_setTextLeading(int lineHeightAdustment)
{

}

// 1.8
int pd_api_gfx_setBitmapMask(LCDBitmap* bitmap, LCDBitmap* mask)
{
	if(pd_api_gfx_disable_masks)
		return 0;

	if(!mask || !bitmap)
		return 0;

	if (!mask->Tex || !bitmap->Tex)
		return 0;

	if ((mask->Tex->w != bitmap->Tex->w) || (mask->Tex->h != bitmap->Tex->h))
		return 0;

	//the mask seems to be a copy
	if(bitmap->Mask)
		pd_api_gfx_freeBitmap(bitmap->Mask);
	
	bitmap->Mask = pd_api_gfx_copyBitmap(mask);
	
	pd_api_gfx_recreatemaskedimage(bitmap);
	
	return 1;
}

LCDBitmap* pd_api_gfx_getBitmapMask(LCDBitmap* bitmap)
{
	if(!bitmap || pd_api_gfx_disable_masks)
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

int printcount = 1;

void _pd_api_gfx_drawBitmapAll(LCDBitmap* bitmap, int x, int y, float xscale, float yscale, bool isRotatedBitmap, const double angle, float centerx, float centery, LCDBitmapFlip flip)
{
    SDL_Rect dstrect;
    dstrect.x = x +  CurrentGfxContext->drawoffsetx;
    dstrect.y = y +  CurrentGfxContext->drawoffsety;
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

    if (CurrentGfxContext->BitmapDrawMode != kDrawModeCopy)
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
		bool requiresTargetTexture = (CurrentGfxContext->BitmapDrawMode == kDrawModeNXOR) || (CurrentGfxContext->BitmapDrawMode == kDrawModeXOR);
		if (requiresTargetTexture)
		{
			drawtargetsurface = SDL_CreateRGBSurfaceWithFormat(0, dstrect.w, dstrect.h, 32, pd_api_gfx_PIXELFORMAT);
			SDL_BlitSurface(CurrentGfxContext->DrawTarget->Tex, &dstrect, drawtargetsurface, NULL);
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
			switch (CurrentGfxContext->BitmapDrawMode)
			{           
				case kDrawModeBlackTransparent:
				{
					int width = std::min(CurrentGfxContext->DrawTarget->w, dstrect.w);
					int height = std::min(CurrentGfxContext->DrawTarget->h, dstrect.h);
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
					int width = std::min(CurrentGfxContext->DrawTarget->w, dstrect.w);
					int height = std::min(CurrentGfxContext->DrawTarget->h, dstrect.h);
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
					int width = std::min(CurrentGfxContext->DrawTarget->w, dstrect.w);
					int height = std::min(CurrentGfxContext->DrawTarget->h, dstrect.h);
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
					int width = std::min(CurrentGfxContext->DrawTarget->w, dstrect.w);
					int height = std::min(CurrentGfxContext->DrawTarget->h, dstrect.h);
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
					int width = std::min(CurrentGfxContext->DrawTarget->w, dstrect.w);
					int height = std::min(CurrentGfxContext->DrawTarget->h, dstrect.h);
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
						int width = std::min(CurrentGfxContext->DrawTarget->w, dstrect.w);
						int height = std::min(CurrentGfxContext->DrawTarget->h, dstrect.h);
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
						int width = std::min(CurrentGfxContext->DrawTarget->w, dstrect.w);
						int height = std::min(CurrentGfxContext->DrawTarget->h, dstrect.h);
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
	SDL_BlitSurface(tmpTexture, NULL, CurrentGfxContext->DrawTarget->Tex, &dstrect);


	if(!isOrginalTexture)
		SDL_FreeSurface(tmpTexture);
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
		x1 += CurrentGfxContext->drawoffsetx;
		y1 += CurrentGfxContext->drawoffsety;
		x2 += CurrentGfxContext->drawoffsetx;
		y2 += CurrentGfxContext->drawoffsety;
	}
	
	switch (color)
    {
        case kColorBlack:
            lineRGBASurface(CurrentGfxContext->DrawTarget->Tex, x1, y1, x2, y2, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            lineRGBASurface(CurrentGfxContext->DrawTarget->Tex, x1, y1, x2, y2, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            lineRGBASurface(CurrentGfxContext->DrawTarget->Tex, x1, y1, x2, y2, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            lineRGBASurface(CurrentGfxContext->DrawTarget->Tex, x1, y1, x2, y2, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }

    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, minx, miny, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }
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
		points[0].x += CurrentGfxContext->drawoffsetx;
		points[1].x += CurrentGfxContext->drawoffsetx;
		points[2].x += CurrentGfxContext->drawoffsetx;
		points[3].x += CurrentGfxContext->drawoffsetx;
		
		points[0].y += CurrentGfxContext->drawoffsety;
		points[1].y += CurrentGfxContext->drawoffsety;
		points[2].y += CurrentGfxContext->drawoffsety;
		points[3].y += CurrentGfxContext->drawoffsety;
	}

	
	switch (color)
    {
        case kColorBlack:
            filledTrigonRGBASurface(CurrentGfxContext->DrawTarget->Tex, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            filledTrigonRGBASurface(CurrentGfxContext->DrawTarget->Tex, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            filledTrigonRGBASurface(CurrentGfxContext->DrawTarget->Tex, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            filledTrigonRGBASurface(CurrentGfxContext->DrawTarget->Tex, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
    
   
    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, minx, miny, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }
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
    rect.x = color == kColorXOR ? 0: x + CurrentGfxContext->drawoffsetx;
    rect.y = color == kColorXOR ? 0: y + CurrentGfxContext->drawoffsety;
    rect.w = width;
    rect.h = height;
    
	switch (color)
    {
        case kColorBlack:
            rectangleRGBASurface(CurrentGfxContext->DrawTarget->Tex, rect.x, rect.y, rect.x + rect.w-1, rect.y + rect.h-1, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            rectangleRGBASurface(CurrentGfxContext->DrawTarget->Tex, rect.x, rect.y, rect.x + rect.w-1, rect.y + rect.h-1, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            rectangleRGBASurface(CurrentGfxContext->DrawTarget->Tex, rect.x, rect.y, rect.x + rect.w-1, rect.y + rect.h-1, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            rectangleRGBASurface(CurrentGfxContext->DrawTarget->Tex, rect.x, rect.y, rect.x + rect.w-1, rect.y + rect.h-1, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
	
    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, x, y, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }
}

void pd_api_gfx_fillRect(int x, int y, int width, int height, LCDColor color)
{
	LCDBitmap *pattern = NULL;
	Uint32 RealColor;
	switch (color)
    {
        case kColorBlack:
            RealColor = SDL_MapRGBA(CurrentGfxContext->DrawTarget->Tex->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            RealColor = SDL_MapRGBA(CurrentGfxContext->DrawTarget->Tex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            RealColor = SDL_MapRGBA(CurrentGfxContext->DrawTarget->Tex->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            RealColor = SDL_MapRGBA(CurrentGfxContext->DrawTarget->Tex->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
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
		int tilesx = (width /  8)+1;
		int tilesy = (height / 8)+1;
		bitmap = Api->graphics->newBitmap(width, height, kColorClear);
		Api->graphics->pushContext(bitmap);
		for (int y = 0; y < tilesy; y++)
			for (int x = 0; x < tilesx; x++)
				Api->graphics->drawBitmap(pattern, x*8, y*8, kBitmapUnflipped);
		Api->graphics->popContext();
		Api->graphics->drawBitmap(bitmap,x + CurrentGfxContext->drawoffsetx,  y + CurrentGfxContext->drawoffsety, kBitmapUnflipped);
		Api->graphics->freeBitmap(bitmap);
		Api->graphics->freeBitmap(pattern);
	}
    else
	{
		SDL_Rect rect;
    	rect.x = color == kColorXOR ? 0: x + CurrentGfxContext->drawoffsetx;
    	rect.y = color == kColorXOR ? 0: y + CurrentGfxContext->drawoffsety;
    	rect.w = width;
    	rect.h = height;

    	SDL_FillRect(CurrentGfxContext->DrawTarget->Tex, &rect, RealColor);
	}

	if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, x, y, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }
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

    x = color == kColorXOR ? width  : x + width + CurrentGfxContext->drawoffsetx;
    y = color == kColorXOR ? height:  y + height + CurrentGfxContext->drawoffsety;
    
    switch (color)
    {
        case kColorBlack:
            ellipseRGBASurface(CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            ellipseRGBASurface(CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            ellipseRGBASurface(CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            ellipseRGBASurface(CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
    

    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, oldx, oldy, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);        
    }
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

    x = color == kColorXOR ? width  : x + width + CurrentGfxContext->drawoffsetx;
    y = color == kColorXOR ? height:  y + height + CurrentGfxContext->drawoffsety;

         
    switch (color)
    {
        case kColorBlack:
            filledEllipseRGBASurface(CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            filledEllipseRGBASurface(CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            filledEllipseRGBASurface(CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            filledEllipseRGBASurface(CurrentGfxContext->DrawTarget->Tex, x, y, width, height, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
    
    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, oldx, oldy, 1.0f, 1.0f, false, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);        
    }
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
    char* fullpath =  (char *) malloc((strlen(path) + 7) * sizeof(char));
    sprintf(fullpath, "./%s", path);
    char* ext = strrchr(fullpath, '.');
    if (ext)
    {
        if(strlen(ext) == 4)
        {
            ext[1] = 't';
            ext[2] = 't';
            ext[3] = 'f';
        }
        else
            sprintf(fullpath, "./%s.ttf", path);
    }
    else
        sprintf(fullpath, "./%s.ttf", path);

    //check font list to see if we already loaded the font
    for (auto& font : fontlist)
    {
        if (strcasecmp(font->path, fullpath) == 0)
        {
            *outErr = NULL;
            free(fullpath);
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
    return result;
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

int pd_api_gfx_getTextWidth(LCDFont* font, const void* text, size_t len, PDStringEncoding encoding, int tracking)
{
    LCDFont *f = font;
    if(!f)
        f = _Default_Font;
    
    if(!f)
        return 0;
    
    if(!f->font)
        return 0;
    
    char *sizedtext = (char *) malloc((len + 1) * sizeof(char));
    strncpy(sizedtext,(char *) text, len);
    sizedtext[len] = '\0';
    int w,h;
    TTF_SizeText(f->font, sizedtext, &w, &h);
    free(sizedtext);
    return w;
}

int pd_api_gfx_drawText(const void* text, size_t len, PDStringEncoding encoding, int x, int y)
{
    if(len == 0)
        return 0;

    if(CurrentGfxContext->font == NULL)
    {
        return -1;
    }

    int result = -1;
    if (CurrentGfxContext->font->font)
    {
        char *sizedtext = (char *) malloc((len + 1) * sizeof(char));
        strncpy(sizedtext, (char *)text, len);
        sizedtext[len] = '\0';
        SDL_Surface* TextSurface = TTF_RenderText_Solid_Wrapped(CurrentGfxContext->font->font, sizedtext, pd_api_gfx_color_black, 0);
        free(sizedtext);
        if (TextSurface) 
        {
            SDL_Surface* Texture = SDL_ConvertSurfaceFormat(TextSurface, CurrentGfxContext->DrawTarget->Tex->format->format, 0);
            if(Texture)
            {                
                LCDBitmap * bitmap = pd_api_gfx_newBitmap(TextSurface->w,TextSurface->h, kColorClear);
                if(bitmap)
                {
					SDL_BlitSurface(Texture, NULL, bitmap->Tex, NULL);					
                    pd_api_gfx_drawBitmap(bitmap, x, y, kBitmapUnflipped);
                    pd_api_gfx_freeBitmap(bitmap);
                }
                SDL_FreeSurface(Texture);
                result = 0;
            }
            SDL_FreeSurface(TextSurface);
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

playdate_graphics* pd_api_gfx_Create_playdate_graphics()
{
    playdate_graphics *Tmp = (playdate_graphics*) malloc(sizeof(*Tmp));
    
    const char* err;
    _Playdate_Screen = pd_api_gfx_newBitmap(LCD_COLUMNS, LCD_ROWS, kColorWhite);
    _Default_Font =  pd_api_gfx_loadFont("System/Fonts/Asheville-Sans-14-Light.ttf", &err);
    _FPS_Font =  pd_api_gfx_loadFont("System/Fonts/Roobert-10-Bold.ttf", &err);
    
    CurrentGfxContext = new GfxContext();
    CurrentGfxContext->BackgroundColor = kColorWhite;
    CurrentGfxContext->BitmapDrawMode = kDrawModeCopy;
    CurrentGfxContext->cliprect.x = -1;
    CurrentGfxContext->cliprect.y = -1;
    CurrentGfxContext->cliprect.w = -1;
    CurrentGfxContext->cliprect.h = -1;
    CurrentGfxContext->drawoffsetx = 0;
    CurrentGfxContext->drawoffsety = 0;
    CurrentGfxContext->DrawTarget = _Playdate_Screen;
    CurrentGfxContext->font = _Default_Font;
    CurrentGfxContext->linecapstyle = kLineCapStyleButt;
    CurrentGfxContext->stencil = NULL;
    CurrentGfxContext->tracking = 0;
    gfxstack.push_back(CurrentGfxContext);

    //SDL_SetRenderTarget(Renderer, _Playdate_Screen->Tex);
    SDL_SetSurfaceBlendMode(_Playdate_Screen->Tex, SDL_BLENDMODE_NONE);
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

    return Tmp;
}


void _pd_api_gfx_freeFontList()
{
    for (auto& font:fontlist)
    {
        if(font->path)
            free(font->path);
        if(font->font)
            if(font->font->font)
                TTF_CloseFont(font->font->font);
    }
    fontlist.clear();
}
