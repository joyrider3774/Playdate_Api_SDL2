#include <SDL.h>
#include <SDL_image.h>
#include <SDL2_gfxPrimitives.h>
#include <SDL_ttf.h>
#include <string.h>
#include <dirent.h>
#include <vector>
#include "pd_api/pd_api_gfx.h"
#include "gamestubcallbacks.h"
#include "gamestub.h"
#include "defines.h"
#include "debug.h"


const char loaderror[] = "Failed loading!";

struct LCDBitmap {
    SDL_Texture* Tex;
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

SDL_Texture* _pd_api_gfx_GetSDLTextureFromBitmap(LCDBitmap* bitmap)
{
    return bitmap->Tex;
}
void _pd_api_gfx_drawFPS(int x, int y)
{
    Api->graphics->pushContext(NULL);
    char *Text;
    Api->system->formatString(&Text,"%2.0f", _LastFPS);
    int w = Api->graphics->getTextWidth(CurrentGfxContext->font, Text, strlen(Text), kASCIIEncoding, 0);
    int h = Api->graphics->getFontHeight(CurrentGfxContext->font);
    Api->graphics->fillRect(0, 0, w, h, kColorWhite);
    Api->graphics->drawText(Text, strlen(Text), kASCIIEncoding, x, y);
    Api->system->realloc(Text, 0);
    Api->graphics->popContext();
}

LCDColor getBackgroundDrawColor()
{
    return CurrentGfxContext->BackgroundColor;
}

// Drawing Functions
void pd_api_gfx_clear(LCDColor color)
{
    if (color == kColorXOR)
        return;
    Uint8 r,g,b,a;
    SDL_GetRenderDrawColor(Renderer, &r, &g, & b, &a);
    switch (color)
    {
        case kColorBlack:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
    SDL_RenderClear(Renderer);
    SDL_SetRenderDrawColor(Renderer, r, g, b, a);
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
    CurrentGfxContext->cliprect.x = x;
    CurrentGfxContext->cliprect.y = y;
    CurrentGfxContext->cliprect.w = width;
    CurrentGfxContext->cliprect.h = height;
    SDL_RenderSetClipRect(Renderer, &CurrentGfxContext->cliprect);
}

void pd_api_gfx_clearClipRect(void)
{
    CurrentGfxContext->cliprect.x = -1;
    CurrentGfxContext->cliprect.y = -1;
    CurrentGfxContext->cliprect.w = -1;
    CurrentGfxContext->cliprect.h = -1;
    SDL_RenderSetClipRect(Renderer, NULL);
}

void pd_api_gfx_setLineCapStyle(LCDLineCapStyle endCapStyle)
{
    CurrentGfxContext->linecapstyle = endCapStyle;
}

void pd_api_gfx_setFont(LCDFont* font)
{
    if(!font)
        printfDebug(DebugInfo, "pd_api_gfx_setFont null");
    CurrentGfxContext->font = font;
}

void pd_api_gfx_setTextTracking(int tracking)
{
    CurrentGfxContext->tracking = tracking;
}

void pd_api_gfx_pushContext(LCDBitmap* target)
{
  
    GfxContext* Tmp = new GfxContext();
    Tmp->Assign(CurrentGfxContext);
    if(target)
    {
        Tmp->DrawTarget = target;
    }
    else
    {
        Tmp->DrawTarget = _Playdate_Screen;
    }
    gfxstack.push_back(Tmp);
    CurrentGfxContext = Tmp;
    
    SDL_SetRenderTarget(Renderer, CurrentGfxContext->DrawTarget->Tex);
}

void pd_api_gfx_popContext(void)
{
    //we should awlays have 1 item it is added when creating the graphics api subset!
    if(gfxstack.size() > 1)
    {
        gfxstack.pop_back();
        CurrentGfxContext = gfxstack[gfxstack.size()-1];
        //restore drawtarget
        if(CurrentGfxContext->DrawTarget)
        {
            SDL_SetRenderTarget(Renderer, CurrentGfxContext->DrawTarget->Tex);
        }
        else
        {
            SDL_SetRenderTarget(Renderer, _Playdate_Screen->Tex);
        }

        //restore cliprect
        if ((CurrentGfxContext->cliprect.x > -1) &&
            (CurrentGfxContext->cliprect.y > -1) &&
            (CurrentGfxContext->cliprect.w > -1) &&
            (CurrentGfxContext->cliprect.h > -1))
        {
            SDL_RenderSetClipRect(Renderer, &CurrentGfxContext->cliprect);
        }
        else
        {
            SDL_RenderSetClipRect(Renderer, NULL);
        }

    }
}
	
// LCDBitmap

LCDBitmap* pd_api_gfx_Create_LCDBitmap()
{
    LCDBitmap* tmp = (LCDBitmap* ) malloc(sizeof(*tmp));
    tmp->Tex = NULL;
    tmp->w = 0;
    tmp->h = 0;
    return tmp;
}

void pd_api_gfx_clearBitmap(LCDBitmap* bitmap, LCDColor bgcolor)
{
    if(bitmap == NULL)
        return;
    if (bgcolor == kColorXOR)
        return;
    Uint8 r,g,b,a;
    SDL_Texture *tmp = SDL_GetRenderTarget(Renderer);
    SDL_SetRenderTarget(Renderer, bitmap->Tex);
    SDL_GetRenderDrawColor(Renderer, &r, &g, & b, &a);
    switch (bgcolor)
    {
        case kColorBlack:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
    SDL_RenderClear(Renderer);
    SDL_SetRenderDrawColor(Renderer, r, g, b, a);
    SDL_SetRenderTarget(Renderer, tmp);
}

LCDBitmap* pd_api_gfx_newBitmap(int width, int height, LCDColor bgcolor)
{
    LCDBitmap* result = pd_api_gfx_Create_LCDBitmap();
    if(result)
    {
        result->Tex = SDL_CreateTexture(Renderer, pd_api_gfx_PIXELFORMAT, SDL_TEXTUREACCESS_TARGET, width, height);
        result->w = width;
        result->h = height;
        SDL_SetTextureBlendMode(result->Tex, SDL_BLENDMODE_BLEND);
        pd_api_gfx_clearBitmap(result, bgcolor);
    }
    return result;
}

void pd_api_gfx_freeBitmap(LCDBitmap* Bitmap)
{
    if(Bitmap == NULL)
        return;
    SDL_DestroyTexture(Bitmap->Tex);
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

    SDL_Texture* RenderTarget = SDL_GetRenderTarget(Renderer);
    SDL_Texture* Img = IMG_LoadTexture(Renderer, fullpath);
    if(Img)
    {
        result = pd_api_gfx_Create_LCDBitmap();
        if(result)
        {
            *outerr = NULL;
            Uint32 format;
            int access;
            SDL_QueryTexture(Img, &format, &access, &result->w, &result->h);
            result->Tex = SDL_CreateTexture(Renderer, pd_api_gfx_PIXELFORMAT, SDL_TEXTUREACCESS_TARGET, result->w, result->h);
            SDL_SetTextureBlendMode(result->Tex, SDL_BLENDMODE_BLEND);
            
            SDL_SetRenderTarget(Renderer, result->Tex);
            SDL_RenderCopy(Renderer, Img, NULL, NULL);
        }
        SDL_DestroyTexture(Img);
    }   
    SDL_SetRenderTarget(Renderer, RenderTarget); 
    free(fullpath);
    return result;
}

LCDBitmap* pd_api_gfx_copyBitmap(LCDBitmap* bitmap)
{
    if(bitmap == NULL)
        return NULL;
    
    SDL_Texture* tmp = SDL_GetRenderTarget(Renderer);
    
    LCDBitmap* result = pd_api_gfx_newBitmap(bitmap->w, bitmap->h, kColorClear);
    SDL_SetRenderTarget(Renderer, result->Tex);
    SDL_RenderCopy(Renderer, bitmap->Tex, NULL, NULL);
    SDL_SetRenderTarget(Renderer, tmp);
    
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
            
            SDL_Texture* drawTarget = SDL_GetRenderTarget(Renderer);
            SDL_Texture* Img = IMG_LoadTexture(Renderer, fullpath);
            if(Img)
            {
                Uint32 format;
                int access, fullw, fullh;                
                
                result = pd_api_gfx_Create_LCDBitmapTable();
                if(result)
                {
                    result->bitmaps = (LCDBitmap **) malloc(sizeof(*result->bitmaps));
                    *outerr = NULL;
                    result->w = w;
                    result->h = h;
                    SDL_QueryTexture(Img, &format, &access, &fullw, &fullh);
                    for (int y = 0; y < fullh; y+=h)
                    {
                        for (int x = 0; x < fullw ; x+=w)
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
                                SDL_SetRenderTarget(Renderer, result->bitmaps[result->count]->Tex);
                                SDL_RenderCopy(Renderer, Img, &rect, NULL);
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
            }
            SDL_SetRenderTarget(Renderer, drawTarget);
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

}
	
// 1.1.1
void pd_api_gfx_fillPolygon(int nPoints, int* coords, LCDColor color, LCDPolygonFillRule fillrule)
{
    Uint8 r,g,b,a;
    
    SDL_GetRenderDrawColor(Renderer, &r, &g, & b, &a);    
    //for xor we are abusing the api to draw on a bitmap and then draw that bitmap on the current target using xor mode
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
            filledPolygonRGBA(Renderer, vx, vy, nPoints, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            filledPolygonRGBA(Renderer, vx, vy, nPoints, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            filledPolygonRGBA(Renderer, vx, vy, nPoints, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            filledPolygonRGBA(Renderer, vx, vy, nPoints, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }

    if (color == kColorXOR)
    {
        pd_api_gfx_popContext();
        pd_api_gfx_pushContext(CurrentGfxContext->DrawTarget);
        pd_api_gfx_setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, 0,0, 1.0f, 1.0f, 0, 0, 0, kBitmapUnflipped);
        pd_api_gfx_popContext();
        pd_api_gfx_freeBitmap(bitmap);        
    }

    SDL_SetRenderDrawColor(Renderer, r, g, b, a);
}

uint8_t pd_api_gfx_getFontHeight(LCDFont* font)
{
    if(font->font)
    {
        return TTF_FontHeight(font->font);
    }
    return 0;
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
    return 0;
}

LCDBitmap* pd_api_gfx_getBitmapMask(LCDBitmap* bitmap)
{
    return NULL;
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


void _pd_api_gfx_drawBitmapAll(LCDBitmap* bitmap, int x, int y, float xscale, float yscale, const double angle, int centerx, int centery, LCDBitmapFlip flip)
{
    //create dest rect first with x, y set to 0 so we can draw the scaled bitmap to a temp texture
    SDL_Rect dstrect;
    dstrect.x = 0;
    dstrect.y = 0;
    dstrect.w = bitmap->w * xscale;
    dstrect.h = bitmap->h * yscale;
    //printf("xs:%f ys:%f w:%d h:%d\n", xscale, yscale, dstrect.w, dstrect.h);

    //remember current drawtarget
    SDL_Texture* RenderTarget = SDL_GetRenderTarget(Renderer);
    
    //draw bitmap first with scaling or flipped or so
    SDL_Texture *tmpTexture = SDL_CreateTexture(Renderer, pd_api_gfx_PIXELFORMAT, SDL_TEXTUREACCESS_TARGET, dstrect.w, dstrect.h);
    SDL_SetRenderTarget(Renderer, tmpTexture);
    SDL_Point CenterPos;
    CenterPos.x = centery;
    CenterPos.y = centery;
    switch (flip)
    {
        case kBitmapUnflipped:
            SDL_RenderCopyEx(Renderer, bitmap->Tex, NULL, &dstrect, angle, &CenterPos, SDL_FLIP_NONE);
            break;
        case kBitmapFlippedX:
            SDL_RenderCopyEx(Renderer, bitmap->Tex, NULL, &dstrect, angle, &CenterPos, SDL_FLIP_HORIZONTAL);
            break;
        case kBitmapFlippedY:
            SDL_RenderCopyEx(Renderer, bitmap->Tex, NULL, &dstrect, angle, &CenterPos, SDL_FLIP_VERTICAL);
            break;
        case kBitmapFlippedXY:
            SDL_Texture *tmpTexture2 = SDL_CreateTexture( Renderer, pd_api_gfx_PIXELFORMAT, SDL_TEXTUREACCESS_TARGET, dstrect.w, dstrect.h );
            SDL_SetRenderTarget(Renderer, tmpTexture2);   
            SDL_RenderCopyEx(Renderer, bitmap->Tex, NULL, &dstrect, 0, NULL, SDL_FLIP_HORIZONTAL);
            SDL_SetRenderTarget(Renderer, tmpTexture);
            SDL_RenderCopyEx(Renderer, tmpTexture2, NULL, &dstrect, angle, &CenterPos, SDL_FLIP_VERTICAL);
    }
    
   
    //set target x & y
    dstrect.x = x + CurrentGfxContext->drawoffsetx;
    dstrect.y = y + CurrentGfxContext->drawoffsety;


    //get Streaming Texture from resulting bitmap
    SDL_Texture* streamingTexture = SDL_CreateTexture(Renderer, pd_api_gfx_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, dstrect.w, dstrect.h );
    if(streamingTexture)
    {

        void* streamingPixels;
        int streamingPitch;
        if (SDL_LockTexture(streamingTexture, NULL, &streamingPixels, &streamingPitch) == 0)
        {
             SDL_RenderReadPixels(Renderer, NULL, pd_api_gfx_PIXELFORMAT, streamingPixels, streamingPitch);
             SDL_UnlockTexture(streamingTexture);
        }
        
        // only needed for xor & nxor
        SDL_Texture* streamingTextureDrawTarget = NULL;
        bool requiresTargetTexture = (CurrentGfxContext->BitmapDrawMode == kDrawModeNXOR) || (CurrentGfxContext->BitmapDrawMode == kDrawModeXOR);
        if (requiresTargetTexture)
        {
            //get Streaming Texture from drawTarget
            SDL_SetRenderTarget(Renderer, CurrentGfxContext->DrawTarget->Tex);
            streamingTextureDrawTarget = SDL_CreateTexture( Renderer, pd_api_gfx_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, dstrect.w, dstrect.h );
            //SDL_SetTextureBlendMode(streamingTextureDrawTarget, SDL_BLENDMODE_BLEND);
            if(streamingTextureDrawTarget)
            {
                void* streamingPixelsDrawTarget;
                int streamingPitchDrawTarget;
                if (SDL_LockTexture(streamingTextureDrawTarget, NULL, &streamingPixelsDrawTarget, &streamingPitchDrawTarget ) == 0)
                {
                    //only read from destination rect, 
                    //make sure we don't read outside the bounds of drawtarget
                    //todo: is this correct ?
                    // SDL_Rect tmprect;
                    // tmprect.x = std::max(0, dstrect.x);
                    // tmprect.y = std::max(0, dstrect.y);
                    // tmprect.w = std::min(dstrect.w, gfxstack[gfxstacktop].DrawTarget->w - abs(dstrect.x));
                    // tmprect.h = std::min(dstrect.h, gfxstack[gfxstacktop].DrawTarget->h - abs(dstrect.y));
                    // apparantly SDL checks the supplied rect value no need to verify our selfves       
                    SDL_RenderReadPixels(Renderer, &dstrect, pd_api_gfx_PIXELFORMAT, streamingPixelsDrawTarget, streamingPitchDrawTarget);
                    SDL_UnlockTexture(streamingTextureDrawTarget);
                }
            }
            //SDL_SetRenderTarget(Renderer, tmpTexture);
        }
        

        //surfaces we can use to lock the from the streaming texture of both render target and bitmap
        //so we can check the values
        SDL_Surface *tmpsurface = NULL, *drawtargetsurface = NULL;

        if (SDL_LockTextureToSurface(streamingTexture, NULL, &tmpsurface) == 0) 
        {
            bool targetTextureUnlocked = true;
            if (requiresTargetTexture)
                targetTextureUnlocked = SDL_LockTextureToSurface(streamingTextureDrawTarget, NULL, &drawtargetsurface) == 0;

            //remember colors
            Uint32 clear = SDL_MapRGBA(tmpsurface->format, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            Uint32 white = SDL_MapRGBA(tmpsurface->format, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            Uint32 black = SDL_MapRGBA(tmpsurface->format, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            Uint32 blackthreshold = SDL_MapRGBA(tmpsurface->format, pd_api_gfx_color_blacktreshold.r, pd_api_gfx_color_blacktreshold.g, pd_api_gfx_color_blacktreshold.b, pd_api_gfx_color_blacktreshold.a);
            Uint32 whitethreshold = SDL_MapRGBA(tmpsurface->format, pd_api_gfx_color_whitetreshold.r, pd_api_gfx_color_whitetreshold.g, pd_api_gfx_color_whitetreshold.b, pd_api_gfx_color_whitetreshold.a);
            Uint32 alpha = SDL_MapRGBA(tmpsurface->format,0,0,0,0);
            
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
                            Uint32 *p = (Uint32*)((Uint8 *)tmpsurface->pixels + (yy * tmpsurface->pitch) + (xx * tmpsurface->format->BytesPerPixel));
                            Uint32 pval = *p;
                            if ((pval == clear) || (pval == alpha) || (pval >= blackthreshold))
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
                            Uint32 *p = (Uint32*)((Uint8 *)tmpsurface->pixels + (yy * tmpsurface->pitch) + (xx * tmpsurface->format->BytesPerPixel));
                            Uint32 pval = *p;
                            if ((pval == clear) || (pval == alpha) || pval <= whitethreshold)
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
                            Uint32 *p = (Uint32*)((Uint8 *)tmpsurface->pixels + (yy * tmpsurface->pitch) + (xx * tmpsurface->format->BytesPerPixel));
                            Uint32 pval = *p;
                            if ((pval == clear) || (pval == alpha) || (pval >= blackthreshold))
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
                            Uint32 *p = (Uint32*)((Uint8 *)tmpsurface->pixels + (yy * tmpsurface->pitch) + (xx * tmpsurface->format->BytesPerPixel));
                            Uint32 pval = *p;
                            if ((pval == clear) || (pval == alpha) || (pval <= whitethreshold))
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
                            Uint32 *p = (Uint32*)((Uint8 *)tmpsurface->pixels + (yy * tmpsurface->pitch) + (xx * tmpsurface->format->BytesPerPixel));
                            Uint32 pval = *p;
                            if ((pval == clear) || (pval == alpha))
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
                                Uint32 *p = (Uint32*)((Uint8 *)tmpsurface->pixels + (yy * tmpsurface->pitch) + (xx * tmpsurface->format->BytesPerPixel));
                                Uint32 pval = *p;
                                if ((pval == clear) || (pval == alpha))
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
                                Uint32 *p = (Uint32*)((Uint8 *)tmpsurface->pixels + (yy * tmpsurface->pitch) + (xx * tmpsurface->format->BytesPerPixel));
                                Uint32 pval = *p;
                                if ((pval == clear) || (pval == alpha))
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
            SDL_SetColorKey(tmpsurface, SDL_ENABLE, clear);
            SDL_Texture* newtex = SDL_CreateTextureFromSurface(Renderer, tmpsurface);
            SDL_UnlockTexture(streamingTexture);
            SDL_DestroyTexture(streamingTexture);
            if(requiresTargetTexture)
            {
                if(streamingTextureDrawTarget)
                {
                    if(targetTextureUnlocked)
                        SDL_UnlockTexture(streamingTextureDrawTarget);
                    SDL_DestroyTexture(streamingTextureDrawTarget);
                }
            }

            if(newtex)
            {
                SDL_SetTextureBlendMode(newtex, SDL_BLENDMODE_BLEND);
                SDL_SetRenderTarget(Renderer, CurrentGfxContext->DrawTarget->Tex);
                SDL_RenderCopy(Renderer, newtex, NULL, &dstrect);
                SDL_DestroyTexture(newtex);
            }
            SDL_DestroyTexture(tmpTexture);
        }
    }
    SDL_SetRenderTarget(Renderer, RenderTarget);

}

void pd_api_gfx_drawRotatedBitmap(LCDBitmap* bitmap, int x, int y, float rotation, float centerx, float centery, float xscale, float yscale)
{
    if (bitmap == NULL)
        return;
    
    _pd_api_gfx_drawBitmapAll(bitmap, x, y, 1.0f, 1.0f, rotation, (int)centerx, (int)centery, kBitmapUnflipped);
}

void pd_api_gfx_drawBitmap(LCDBitmap* bitmap, int x, int y, LCDBitmapFlip flip)
{
    if (bitmap == NULL)
        return;

    
    _pd_api_gfx_drawBitmapAll(bitmap, x, y, 1.0f, 1.0f, 0, 0, 0, flip);
}

void pd_api_gfx_tileBitmap(LCDBitmap* bitmap, int x, int y, int width, int height, LCDBitmapFlip flip)
{

}

void pd_api_gfx_drawScaledBitmap(LCDBitmap* bitmap, int x, int y, float xscale, float yscale)
{
    if (bitmap == NULL)
        return;

    _pd_api_gfx_drawBitmapAll(bitmap, x, y, xscale, yscale, 0, 0, 0, kBitmapUnflipped);
}

void pd_api_gfx_drawLine(int x1, int y1, int x2, int y2, int width, LCDColor color)
{
    Uint8 r,g,b,a;
    SDL_GetRenderDrawColor(Renderer, &r, &g, & b, &a);
    switch (color)
    {
        case kColorBlack:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }

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

    SDL_RenderDrawLine(Renderer, x1, y1, x2, y2);

    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, minx, miny, 1.0f, 1.0f, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);        
    }
    SDL_SetRenderDrawColor(Renderer, r, g, b, a);    
}

void pd_api_gfx_fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, LCDColor color)
{
    SDL_Point points[4];
    Uint8 r,g,b,a;
    SDL_GetRenderDrawColor(Renderer, &r, &g, & b, &a);
    switch (color)
    {
        case kColorBlack:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
    
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

    SDL_RenderDrawLines(Renderer, points, 4);

    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, minx, miny, 1.0f, 1.0f, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);        
    }

    SDL_SetRenderDrawColor(Renderer, r, g, b, a);
}

void pd_api_gfx_drawRect(int x, int y, int width, int height, LCDColor color)
{
    Uint8 r,g,b,a;
    SDL_GetRenderDrawColor(Renderer, &r, &g, & b, &a);
    switch (color)
    {
        case kColorBlack:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
    //for xor we are abusing the api to draw on a bitmap and then draw that bitmap on the current target using xor mode
    LCDBitmap *bitmap = NULL;
    if (color == kColorXOR)
    {
        bitmap = Api->graphics->newBitmap(width, height, kColorClear);
        Api->graphics->pushContext(bitmap);
    }
    

    SDL_Rect rect;
    rect.x = color == kColorXOR ? 0: x;
    rect.y = color == kColorXOR ? 0: y;
    rect.w = width;
    rect.h = height;
    SDL_RenderDrawRect(Renderer, &rect);

    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, x, y, 1.0f, 1.0f, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);        
    }

    SDL_SetRenderDrawColor(Renderer, r, g, b, a);
}

void pd_api_gfx_fillRect(int x, int y, int width, int height, LCDColor color)
{
    Uint8 r,g,b,a;
    
    SDL_GetRenderDrawColor(Renderer, &r, &g, & b, &a);
    switch (color)
    {
        case kColorBlack:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
    //for xor we are abusing the api to draw on a bitmap and then draw that bitmap on the current target using xor mode
    LCDBitmap *bitmap = NULL;
    if (color == kColorXOR)
    {
        bitmap = Api->graphics->newBitmap(width, height, kColorClear);
        Api->graphics->pushContext(bitmap);
    }
    
    SDL_Rect rect;
    rect.x = color == kColorXOR ? 0: x;
    rect.y = color == kColorXOR ? 0: y;
    rect.w = width;
    rect.h = height;

    SDL_RenderFillRect(Renderer, &rect);

    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, x, y, 1.0f, 1.0f, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);
    }

    SDL_SetRenderDrawColor(Renderer, r, g, b, a);
}

void pd_api_gfx_drawEllipse(int x, int y, int width, int height, int lineWidth, float startAngle, float endAngle, LCDColor color) // stroked inside the rect
{
    Uint8 r,g,b,a;
    
    SDL_GetRenderDrawColor(Renderer, &r, &g, & b, &a);    
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

    x = color == kColorXOR ? width  : x + width;
    y = color == kColorXOR ? height:  y + height;
    
    switch (color)
    {
        case kColorBlack:
            ellipseRGBA(Renderer, x, y, width, height, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            ellipseRGBA(Renderer, x, y, width, height, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            ellipseRGBA(Renderer, x, y, width, height, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            ellipseRGBA(Renderer, x, y, width, height, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
    

    if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, oldx, oldy, 1.0f, 1.0f, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);        
    }

    SDL_SetRenderDrawColor(Renderer, r, g, b, a);
}

void pd_api_gfx_fillEllipse(int x, int y, int width, int height, float startAngle, float endAngle, LCDColor color)
{
    Uint8 r,g,b,a;
    
    SDL_GetRenderDrawColor(Renderer, &r, &g, & b, &a);    
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

    x = color == kColorXOR ? width  : x + width;
    y = color == kColorXOR ? height:  y + height;

         
    switch (color)
    {
        case kColorBlack:
            filledEllipseRGBA(Renderer, x, y, width, height, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
            break;
        case kColorWhite:
            filledEllipseRGBA(Renderer, x, y, width, height, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorXOR:
            filledEllipseRGBA(Renderer, x, y, width, height, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
            break;
        case kColorClear:
            filledEllipseRGBA(Renderer, x, y, width, height, pd_api_gfx_color_clear.r, pd_api_gfx_color_clear.g, pd_api_gfx_color_clear.b, pd_api_gfx_color_clear.a);
            break;
        default:
            break;
    }
    

     if (color == kColorXOR)
    {
        Api->graphics->popContext();
        Api->graphics->pushContext(CurrentGfxContext->DrawTarget);
        Api->graphics->setDrawMode(kDrawModeXOR);
        _pd_api_gfx_drawBitmapAll(bitmap, oldx, oldy, 1.0f, 1.0f, 0, 0, 0, kBitmapUnflipped);
        Api->graphics->popContext();
        Api->graphics->freeBitmap(bitmap);        
    }

    SDL_SetRenderDrawColor(Renderer, r, g, b, a);
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
    if(!font)
        return 0;
    if(font->font)
    {
        char *sizedtext = (char *) malloc((len + 1) * sizeof(char));
        strncpy(sizedtext,(char *) text, len);
        sizedtext[len] = '\0';
        int w,h;
        TTF_SizeText(font->font, sizedtext, &w, &h);
        free(sizedtext);
        return w;
    }
    return 0;
}

int pd_api_gfx_drawText(const void* text, size_t len, PDStringEncoding encoding, int x, int y)
{
    if(len == 0)
        return 0;
    if(CurrentGfxContext->font == NULL)
        return -1;
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
            SDL_Texture* Texture = SDL_CreateTextureFromSurface(Renderer, TextSurface);
            if(Texture)
            {                
                LCDBitmap * bitmap = pd_api_gfx_newBitmap(TextSurface->w,TextSurface->h, kColorClear);
                if(bitmap)
                {
                    SDL_Texture* RenderTarget = SDL_GetRenderTarget(Renderer);
                    SDL_SetRenderTarget(Renderer, bitmap->Tex);
                    SDL_RenderCopy(Renderer, Texture, NULL, NULL);
                    SDL_SetRenderTarget(Renderer, RenderTarget);
                    pd_api_gfx_drawBitmap(bitmap, x, y, kBitmapUnflipped);
                    pd_api_gfx_freeBitmap(bitmap);
                }
                SDL_DestroyTexture(Texture);
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
    
    _Playdate_Screen = pd_api_gfx_newBitmap(LCD_COLUMNS, LCD_ROWS, kColorClear);
    CurrentGfxContext = new GfxContext();
    CurrentGfxContext->BackgroundColor = kColorClear;
    CurrentGfxContext->BitmapDrawMode = kDrawModeCopy;
    CurrentGfxContext->cliprect.x = -1;
    CurrentGfxContext->cliprect.y = -1;
    CurrentGfxContext->cliprect.w = -1;
    CurrentGfxContext->cliprect.h = -1;
    CurrentGfxContext->drawoffsetx = 0;
    CurrentGfxContext->drawoffsety = 0;
    CurrentGfxContext->DrawTarget = _Playdate_Screen;
    CurrentGfxContext->font = NULL;
    CurrentGfxContext->linecapstyle = kLineCapStyleButt;
    CurrentGfxContext->stencil = NULL;
    CurrentGfxContext->tracking = 0;
    gfxstack.push_back(CurrentGfxContext);

    SDL_SetRenderTarget(Renderer, _Playdate_Screen->Tex);
    SDL_SetRenderDrawBlendMode(Renderer, SDL_BLENDMODE_BLEND);
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
