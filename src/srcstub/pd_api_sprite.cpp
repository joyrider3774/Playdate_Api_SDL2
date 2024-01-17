#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <vector>
#include "bump/World.hpp"
#include "bump/Collision.hpp"
#include "bump/CollisionResponses.hpp"
#include "bump/Rectangle.hpp"
#include "pd_api/pd_api_gfx.h"
#include "pd_api/pd_api_sprite.h"
#include "gamestubcallbacks.h"
#include "gamestub.h"
#include "defines.h"
#include "pd_api.h"
#include "sprite.h"
#include "debug.h"

using namespace plugin::physics::bump;
using namespace std;
//using namespace __gnu_debug;
vector <std::shared_ptr<LCDSprite>> spriteList;
int cc;
bool _pd_api_sprite_SpriteListNeedsSort = false;
bool _pd_api_sprite_AlwaysRedraw = false;
bool _pd_api_sprite_need_cleanup = false;
World* world = new World(INT_MAX);

void printSpliteListInfo()
{
	printfDebug(DebugInfo, "spriteList ");
	for (auto& spritePtr : spriteList)
	{	
		LCDSprite* sprite = spritePtr.get();
		if(sprite->Loaded && sprite->LoadedInList)
			printfDebug(DebugInfo, "3 ");
		else 
		{
			if(sprite->Loaded && !sprite->LoadedInList)
			{
				printfDebug(DebugInfo, "2 ");
			}
			else
			{
				if(!sprite->Loaded && sprite->LoadedInList)
				{
					printfDebug(DebugInfo, "1 ");
				}
				else
				{
					printfDebug(DebugInfo, "0 ");
				}
			}
		}
	}
}

void _pd_api_sprite_DefaultSpriteDrawFunction(LCDSprite* sprite, PDRect bounds, PDRect drawrect)
{
	//int w,h;
	if(sprite)
	{
		if(sprite->Loaded && sprite->LoadedInList && sprite->Bitmap && (sprite->Dirty || _pd_api_sprite_AlwaysRedraw))
		{
			//Api->graphics->getBitmapData(sprite->Bitmap, &w, &h, NULL, NULL, NULL);
			Api->graphics->drawBitmap(sprite->Bitmap, drawrect.x, drawrect.y, sprite->FlipMode);		
		}
	}
}

void _pd_api_sprite_DefaultUpdateFunction(LCDSprite* sprite)
{

}

struct CWCollisionInfo
{

};

typedef struct CWCollisionInfo CWCollisionInfo;

struct CWItemInfo
{

};

typedef struct CWItemInfo CWItemInfo;

void _pd_api_sprite_cleanup_sprites(bool OnlyNotLoadedSprites)
{
	if(OnlyNotLoadedSprites && !_pd_api_sprite_need_cleanup)
		return;
	_pd_api_sprite_need_cleanup = false;
	// printf("Deleted sprites before (bumpcount:%d)\n", world->CountItems());
    // int c = 0;
	// for (auto& spriteptr: spriteList)
	// {
	// 	auto sprite = spriteptr.get();
    //     if(sprite)
    //     {
	// 		if(sprite->BumpItem)
	// 			c++;
	// 		if(sprite->Loaded) 
	// 			printf("1 ");
	// 		else
	// 			printf("0 ");
	// 	}
	// }
	// printf("\n");
	//printSpliteListInfo();
	//printf("sprite list size before:%d\n", spriteList.size());
	//printf("Bump Item count before %d %d\n", c, cc);
	spriteList.erase(std::remove_if(spriteList.begin(), spriteList.end(), [OnlyNotLoadedSprites](auto& spriteptr){
        LCDSprite* sprite = spriteptr.get();
        if(sprite)
        {
            if(!sprite->Loaded || !OnlyNotLoadedSprites)
            {
                world->Remove(sprite->BumpItem);
                sprite->BumpItem = nullptr;
                sprite->shared_ptr_self = nullptr;
                return true;
           }
        }
		return false;
		}), spriteList.end());
	//printf("sprite list size after:%d\n", spriteList.size());
	
	

	// printf("Deleted sprites After (bumpcount:%d)\n", world->CountItems());
	// c = 0;
	// for (auto& sprite: spriteList)
	// {
	// 	if(sprite->BumpItem)
	// 		c++;
	// 	if(sprite->Loaded) 
	// 		printf("1 ");
	// 	else
	// 		printf("0 ");
	// }
	// printf("\n");
	//printf("Bump Item count after %d %d\n", c, cc);
    
}

bool _pd_api_sprite_spriteListSortFunc (std::shared_ptr<LCDSprite> i, std::shared_ptr<LCDSprite> j) 
{ 
	return i.get()->zIndex < j.get()->zIndex; 
}

void _pd_api_sprite_SortList()
{
	printfDebug(DebugTraceFunctions,"_pd_api_sprite_SortList\n");
	if (_pd_api_sprite_SpriteListNeedsSort)
	{
		sort(spriteList.begin(),spriteList.end(), _pd_api_sprite_spriteListSortFunc);
		_pd_api_sprite_SpriteListNeedsSort = false;
	}
	printfDebug(DebugTraceFunctions,"_pd_api_sprite_SortList end\n");
}

void pd_api_sprite_setAlwaysRedraw(int flag)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setAlwaysRedraw\n");
	_pd_api_sprite_AlwaysRedraw = flag;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setAlwaysRedraw end\n");
}

void pd_api_sprite_addDirtyRect(LCDRect dirtyRect)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_addDirtyRect\n");
	printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_addDirtyRect not implemented !\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_addDirtyRect end\n");
}

void _pd_api_sprite_drawBumpItems(void)
{
	printfDebug(DebugTraceFunctions,"_pd_api_sprite_drawBumpItems\n");
	int c =0;	
	for (auto& item : world->GetItems())
	{	
		c++;
		Api->graphics->drawRect(item->GetRectangle().pos.x, item->GetRectangle().pos.y, item->GetRectangle().scale.x, item->GetRectangle().scale.y, kColorBlack);
	}
	printfDebug(DebugInfo, "_pd_api_sprite_drawBumpItems draws done: %d\n",c);
	printfDebug(DebugTraceFunctions,"_pd_api_sprite_drawBumpItems end\n");
}

void pd_api_sprite_drawSprites(void)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_drawSprites\n");
	LCDColor tmp = getBackgroundDrawColor();
	if(tmp != kColorClear)
		Api->graphics->clear(tmp);
	_pd_api_sprite_SortList();
	int c =0;	
	for (auto& spritePtr : spriteList)
	{	
		LCDSprite* sprite = spritePtr.get();
	 	if(sprite->Loaded && sprite->LoadedInList && (sprite->Visible || _pd_api_sprite_AlwaysRedraw))
	 	{
			c++;	
			LCDBitmapDrawMode tmpDrawMode = _pd_api_gfx_getCurrentDrawMode();
	 		Api->graphics->setDrawMode(sprite->DrawMode);
			sprite->DrawFunction(sprite, sprite->BoundsRect, sprite->BoundsRect);
			Api->graphics->setDrawMode(tmpDrawMode);
			//Api->graphics->drawRect(sprite->BoundsRect.x, sprite->BoundsRect.y, sprite->BoundsRect.width, sprite->BoundsRect.height, kColorBlack);
			//Api->graphics->drawRect(sprite->BoundsRect.x + sprite->CollideRect.x, sprite->BoundsRect.y + sprite->CollideRect.y, sprite->CollideRect.width, sprite->CollideRect.height, kColorBlack);		
			if(sprite)
			{			
				if(sprite->BumpItem != nullptr)
				{
			//		Api->graphics->drawRect(sprite->BumpItem->GetRectangle().pos.x, sprite->BumpItem->GetRectangle().pos.y, sprite->BumpItem->GetRectangle().scale.x, sprite->BumpItem->GetRectangle().scale.y, kColorBlack);					
				}
			}			
	 	}
	}
	if (DRAWBUMPRECTS)
		_pd_api_sprite_drawBumpItems();

	printfDebug(DebugInfo, "pd_api_sprite_drawSprites draws done: %d\n",c);
	printfDebug(DebugTraceFunctions,"pd_api_sprite_drawSprites end\n");
}

void pd_api_sprite_updateAndDrawSprites(void)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_updateAndDrawSprites\n");
	_pd_api_sprite_SortList();
	//Api->graphics->clear(kColorClear);
	int c = 0;
	for (auto& spritePtr : spriteList)
	{	
		LCDSprite* sprite = spritePtr.get();
		if(sprite->Loaded && sprite->LoadedInList && sprite->UpdatesEnabled)
		{
			c++;
			sprite->UpdateFunction(sprite);
		}
	}
	printfDebug(DebugInfo, "pd_api_sprite_updateAndDrawSprites updates done: %d\n",c);
	pd_api_sprite_drawSprites();
	printfDebug(DebugTraceFunctions,"pd_api_sprite_updateAndDrawSprites end\n");
}

LCDSprite* pd_api_sprite_newSprite(void)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_newSprite\n");
    LCDSprite* Tmp = new LCDSprite;
	Tmp->shared_ptr_self = std::shared_ptr<LCDSprite>(Tmp);
	Tmp->UpdateFunction = _pd_api_sprite_DefaultUpdateFunction;
	Tmp->DrawFunction = _pd_api_sprite_DefaultSpriteDrawFunction;
    spriteList.push_back(Tmp->shared_ptr_self);
	printfDebug(DebugTraceFunctions,"pd_api_sprite_newSprite end\n");
	return Tmp;
}

void pd_api_sprite_freeSprite(LCDSprite *sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_freeSprite\n");
	if(sprite == NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_copy end Sprite == NULL\n");
		return;
	}

	if(sprite->Loaded)
	{
		printfDebug(DebugInfo, "free Sprite %p\n", sprite);
		_pd_api_sprite_need_cleanup = true;
    	sprite->Loaded = false;
		if(sprite->BumpItem)
		{
			world->Remove(sprite->BumpItem);
			sprite->BumpItem = nullptr;
		}
	}


	printfDebug(DebugTraceFunctions,"pd_api_sprite_freeSprite end\n");
}

LCDSprite* pd_api_sprite_copy(LCDSprite *sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_copy\n");
	
	if(sprite == NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_copy end Sprite == NULL\n");
		return NULL;
	}

	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_copy end Sprite not loaded\n");
		return NULL;
	}
	LCDSprite *newSprite = Api->sprite->newSprite();
	newSprite->Bitmap =  Api->graphics->copyBitmap(sprite->Bitmap);
	newSprite->BoundsRect = sprite->BoundsRect;
	newSprite->CenterX = sprite->CenterX;
	newSprite->CenterY = sprite->CenterY;
	newSprite->ClipRect = sprite->ClipRect;
	newSprite->CollideRect = sprite->CollideRect;
	newSprite->CollisionResponseTypeFunction = sprite->CollisionResponseTypeFunction;
    newSprite->CollisionsEnabled = sprite->CollisionsEnabled;
	newSprite->Dirty = sprite->Dirty;
	newSprite->DrawFunction = sprite->DrawFunction;
	newSprite->DrawMode = sprite->DrawMode;
	newSprite->FlipMode = sprite->FlipMode;
	newSprite->hasNewMoveBy = sprite->hasNewMoveBy;
	newSprite->hasNewPos = sprite->hasNewPos;
	newSprite->Tag = sprite->Tag;
	newSprite->UpdateFunction = sprite->UpdateFunction;
	newSprite->UpdatesEnabled = sprite->UpdatesEnabled;
	newSprite->userdata = sprite->userdata;
	newSprite->Visible = sprite->Visible;
	newSprite->zIndex = sprite->zIndex;
	newSprite->Loaded = sprite->Loaded;
	newSprite->CenterPointX = newSprite->CenterPointX;
	newSprite->CenterPointY = newSprite->CenterPointY;
	//newSprite->setBumpItem(sprite->BumpItem());
	printfDebug(DebugTraceFunctions,"pd_api_sprite_copy end\n");
	return newSprite;
}

void pd_api_sprite_addSprite(LCDSprite *sprite)
{
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_addSprite end Sprite == NULL\n");
		return;
	}

	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_addSprite end Sprite not loaded\n");
		return;	
	}
	sprite->LoadedInList = true;
	_pd_api_sprite_SpriteListNeedsSort = true;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_addSprite end\n");
}

void pd_api_sprite_removeSprite(LCDSprite *sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_removeSprite %p\n", sprite);
	if(sprite == NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_removeSprite end sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_removeSprite end Sprite not loaded\n");
		return;
	}

	if(sprite->BumpItem)
	{
		world->Remove(sprite->BumpItem);
		sprite->BumpItem = nullptr;
	}

	sprite->LoadedInList = false;		
	
	printfDebug(DebugTraceFunctions,"pd_api_sprite_removeSprite end\n");
}

void pd_api_sprite_removeSprites(LCDSprite **sprites, int count)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_removeSprites\n");
	for (auto i = 0; i < count; i++ )
	{
		pd_api_sprite_removeSprite(sprites[i]);
	}
	printfDebug(DebugTraceFunctions,"pd_api_sprite_removeSprites end\n");
}

void pd_api_sprite_removeAllSprites(void)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_removeAllSprites\n");
	for (auto& spritePtr : spriteList)
	{	
		LCDSprite* sprite = spritePtr.get();	
		pd_api_sprite_removeSprite(sprite);
	}
	printfDebug(DebugTraceFunctions,"pd_api_sprite_removeAllSprites end\n");
}

int pd_api_sprite_getSpriteCount(void)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getSpriteCount\n");
	return spriteList.size();
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getSpriteCount end\n");
}

void pd_api_sprite_setBounds(LCDSprite *sprite, PDRect bounds)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setBounds\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setBounds end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setBounds end Sprite not loaded\n");
		return;
	}
	sprite->BoundsRect = bounds;
	sprite->CollideRectBump.x = sprite->BoundsRect.x + sprite->CollideRect.x;
	sprite->CollideRectBump.y = sprite->BoundsRect.y + sprite->CollideRect.y;	
	sprite->CenterPointX = bounds.x + (bounds.width * sprite->CenterX);
	sprite->CenterPointY = bounds.y + (bounds.height * sprite->CenterY);
	if(sprite->BumpItem != nullptr)
	{
		world->Update(sprite->BumpItem, Rectangle(sprite->CenterPointX - (sprite->CollideRect.width * sprite->CenterX), sprite->CenterPointY - (sprite->CollideRect.height * sprite->CenterY), sprite->CollideRectBump.width, sprite->CollideRectBump.height));		
	}
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setBounds end\n");
}
	

PDRect pd_api_sprite_getBounds(LCDSprite *sprite)
{
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getBounds end\n");
		return {0,0,0,0};
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getBounds end Sprite not loaded\n");
		return {0,0,0,0};
	}
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getBounds Sprite == NULL\n");
	return sprite->BoundsRect;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getBounds end\n");
}

void pd_api_sprite_moveTo(LCDSprite *sprite, float x, float y)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_moveTo\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_moveTo end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_moveTo end Sprite not loaded\n");
		return;
	}
	PDRect Bounds = PDRectMake(x - (sprite->BoundsRect.width / 2.0f), y - (sprite->BoundsRect.height / 2.0f), sprite->BoundsRect.width, sprite->BoundsRect.height);
	if(sprite->Bitmap)
	{
		int w, h;
		Api->graphics->getBitmapData(sprite->Bitmap, &w, &h, NULL, NULL, NULL);
		Bounds = PDRectMake(x - ((float)w / 2.0f), y - ((float)h / 2.0f), (float)w, (float)h);
	}
	Api->sprite->setBounds(sprite, Bounds);	
	printfDebug(DebugTraceFunctions,"pd_api_sprite_moveTo end\n");
}

void pd_api_sprite_moveBy(LCDSprite *sprite, float dx, float dy)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_moveBy\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_moveBy end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_moveBy end Sprite not loaded\n");
		return;
	}
	PDRect BoundsRect = Api->sprite->getBounds(sprite);
	BoundsRect.x += dx;
	BoundsRect.y += dy;	
	Api->sprite->setBounds(sprite, BoundsRect);
	
	
	printfDebug(DebugTraceFunctions,"pd_api_sprite_moveBy end\n");
}

void pd_api_sprite_setImage(LCDSprite *sprite, LCDBitmap *image, LCDBitmapFlip flip)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setImage\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setImage end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setImage end Sprite not loaded\n");
		return;
	}
	sprite->Bitmap = image;
	sprite->FlipMode = flip;
	if(sprite->Bitmap)
	{
		int w,h;
		float x,y;
		Api->graphics->getBitmapData(sprite->Bitmap, &w, &h, NULL, NULL, NULL);
		Api->sprite->getPosition(sprite, &x,&y);
		Api->sprite->setBounds(sprite, PDRectMake(x - ((float)w / 2.0f), y - ((float)h / 2.0f), (float)w, (float)h));
	}
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setImage end\n");
}

LCDBitmap* pd_api_sprite_getImage(LCDSprite *sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getImage\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getImage end Sprite == NULL\n");
		return NULL;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getImage end Sprite not loaded\n");
		return NULL;
	}
    return sprite->Bitmap;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getImage end\n");
}

void pd_api_sprite_setSize(LCDSprite *s, float width, float height)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setSize\n");
	if(!s)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setSize end Sprite == NULL\n");
		return;
	}
	if(!s->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setSize end Sprite not loaded\n");
		return;
	}
	if(!s->Bitmap)
	{	
		float x, y;
		Api->sprite->getPosition(s, &x,&y);
		int w,h;
		Api->graphics->getBitmapData(s->Bitmap, &w, &h, NULL, NULL, NULL);
		Api->sprite->setBounds(s, PDRectMake(x - ((float)w / 2.0f), y - ((float)h / 2.0f), (float)w, (float)h));
	}
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setSize end\n");
}

void pd_api_sprite_setZIndex(LCDSprite *sprite, int16_t zIndex)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setZIndex\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setZIndex end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setZIndex end Sprite not loaded\n");
		return;
	}
	sprite->zIndex = zIndex;
	_pd_api_sprite_SpriteListNeedsSort = true;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setZIndex end\n");
}

int16_t pd_api_sprite_getZIndex(LCDSprite *sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getZIndex\n");
    if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getZIndex end Sprite == NULL\n");
		return 0;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getZIndex end Sprite not loaded\n");
		return 0;
	}
	return sprite->zIndex;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getZIndex end\n");
}

void pd_api_sprite_setDrawMode(LCDSprite *sprite, LCDBitmapDrawMode mode)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setDrawMode\n");
    if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setDrawMode end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setDrawMode end Sprite not loaded\n");
		return;
	}
	sprite->DrawMode = mode;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setDrawMode end\n");
}

void pd_api_sprite_setImageFlip(LCDSprite *sprite, LCDBitmapFlip flip)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setImageFlip\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setImageFlip end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setImageFlip end Sprite not loaded\n");
		return;
	}
	sprite->FlipMode = flip;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setImageFlip end\n");
}

LCDBitmapFlip pd_api_sprite_getImageFlip(LCDSprite *sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getImageFlip\n");
    if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getImageFlip end Sprite == NULL\n");
		return kBitmapUnflipped;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getImageFlip end Sprite not loaded\n");
		return kBitmapUnflipped;
	}
	return sprite->FlipMode;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getImageFlip end\n");
}

void pd_api_sprite_setStencil(LCDSprite *sprite, LCDBitmap* stencil) // deprecated in favor of setStencilImage()
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getImageFlip\n");
	printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_getImageFlip not implemented!\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getImageFlip end\n");
}

void pd_api_sprite_setClipRect(LCDSprite *sprite, LCDRect clipRect)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setClipRect\n");
    if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setClipRect end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setClipRect end Sprite not loaded\n");
		return;
	}	
	sprite->ClipRect = clipRect;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setClipRect ed\n");
}

void pd_api_sprite_clearClipRect(LCDSprite *sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_clearClipRect\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_clearClipRect end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_clearClipRect end Sprite not loaded\n");
		return;
	}	
	sprite->ClipRect.bottom = -1;
	sprite->ClipRect.top = -1;
	sprite->ClipRect.left = -1;
	sprite->ClipRect.right = -1;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_clearClipRect end\n");
}

void pd_api_sprite_setClipRectsInRange(LCDRect clipRect, int startZ, int endZ)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setClipRectsInRange\n");
	printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_setClipRectsInRange not implemented\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setClipRectsInRange end\n");
}

void pd_api_sprite_clearClipRectsInRange(int startZ, int endZ)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_clearClipRectsInRange\n");
	printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_clearClipRectsInRange not implemented!\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_clearClipRectsInRange end\n");
}

void pd_api_sprite_setUpdatesEnabled(LCDSprite *sprite, int flag)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setUpdatesEnabled\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setUpdatesEnabled end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setUpdatesEnabled end Sprite not loaded\n");
		return;
	}	
	sprite->UpdatesEnabled = flag;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setUpdatesEnabled end\n");
}

int pd_api_sprite_updatesEnabled(LCDSprite *sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_updatesEnabled\n");
    if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_updatesEnabled end Sprite == NULL\n");
		return 0;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_updatesEnabled end Sprite not loaded\n");
		return 0;
	}
	return sprite->UpdatesEnabled;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_updatesEnabled end\n");
}

void pd_api_sprite_setCollisionsEnabled(LCDSprite *sprite, int flag)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setCollisionsEnabled\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setCollisionsEnabled end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setCollisionsEnabled end Sprite not loaded\n");
		return;
	}	

	sprite->CollisionsEnabled = flag;

	//sprite->CollisionsEnabled = flag;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setCollisionsEnabled end\n");
}

int pd_api_sprite_collisionsEnabled(LCDSprite *sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_collisionsEnabled\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_collisionsEnabled end Sprite == NULL\n");
		return 0;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_collisionsEnabled end Sprite not loaded\n");
		return 0;
	}	
    return sprite->CollisionsEnabled;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_collisionsEnabled end\n");
}

void pd_api_sprite_setVisible(LCDSprite *sprite, int flag)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setVisible\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setVisible end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setVisible end Sprite not loaded\n");
		return;
	}
	sprite->Visible = flag;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setVisible end\n");
}

int pd_api_sprite_isVisible(LCDSprite *sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_isVisible\n");
    if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_isVisible end Sprite == NULL\n");
		return 0;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_isVisible end Sprite not loaded\n");
		return 0;
	}
	return sprite->Visible;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_isVisible end\n");
}

void pd_api_sprite_setOpaque(LCDSprite *sprite, int flag)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setOpaque\n");
	printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_setOpaque not implemented!\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setOpaque end\n");
}

void pd_api_sprite_markDirty(LCDSprite *sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_markDirty\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_markDirty end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_markDirty end Sprite not loaded\n");
		return;
	}
	sprite->Dirty = true;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_markDirty end\n");
}


void pd_api_sprite_setTag(LCDSprite *sprite, uint8_t tag)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setTag\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setTag end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setTag end Sprite not loaded\n");
		return;
	}
	sprite->Tag = tag;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setTag end\n");
}

uint8_t pd_api_sprite_getTag(LCDSprite *sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getTag\n");
    if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getTag end Sprite == NULL\n");
		return 255;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getTag end Sprite not loaded\n");
		return 255;
	}
	return sprite->Tag;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getTag end\n");
}

void pd_api_sprite_setIgnoresDrawOffset(LCDSprite *sprite, int flag)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setIgnoresDrawOffset\n");
	printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_setIgnoresDrawOffset not implemented!\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setIgnoresDrawOffset end\n");
}

void pd_api_sprite_setUpdateFunction(LCDSprite *sprite, LCDSpriteUpdateFunction *func)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setUpdateFunction\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setUpdateFunction end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setUpdateFunction end Sprite not loaded\n");
		return;
	}
	sprite->UpdateFunction = func;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setUpdateFunction end\n");
}

void pd_api_sprite_setDrawFunction(LCDSprite *sprite, LCDSpriteDrawFunction *func)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setDrawFunction\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setDrawFunction end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setDrawFunction end Sprite not loaded\n");
		return;
	}
	sprite->DrawFunction = func;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setDrawFunction end\n");
}

void pd_api_sprite_getPosition(LCDSprite *sprite, float *x, float *y)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getPosition\n");
	if(sprite==NULL)
	{
		*x = -100;
		*y = -100;
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getPosition end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		*x = -100;
		*y = -100;
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getPosition end Sprite not loaded\n");
		return;
	}
	*x = sprite->CenterPointX;
	*y = sprite->CenterPointY;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getPosition end\n");
}

// Collisions
void pd_api_sprite_resetCollisionWorld(void)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_resetCollisionWorld\n");
	printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_resetCollisionWorld not implemented!\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_resetCollisionWorld end\n");
}

void pd_api_sprite_setCollideRect(LCDSprite *sprite, PDRect collideRect)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setCollideRect\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setCollideRect end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setCollideRect end Sprite not loaded\n");
		return;
	}
	sprite->CollideRect = collideRect;	
	//colidrect can be set 0,0,0,0 for clearing it	
	if((collideRect.x == 0) && (collideRect.y == 0) && (collideRect.width == 0) && (collideRect.height == 0))
	{
		sprite->CollideRectBump.x = 0;
		sprite->CollideRectBump.y = 0;
		sprite->CollideRectBump.width = 0;
		sprite->CollideRectBump.height = 0;
		printfDebug(DebugInfo,"setCollideRect: %f %f %f %f \n", collideRect.x, collideRect.y, collideRect.width, collideRect.height);

		if(sprite->BumpItem != nullptr)
		{
	  		printfDebug(DebugInfo,"setCollideRect has BumpItem so remove\n");			
			cc--;
			world->Remove(sprite->BumpItem);
			sprite->BumpItem = nullptr;
		}
	}
	else
	{
		sprite->CollideRectBump.x = sprite->BoundsRect.x + collideRect.x;
		sprite->CollideRectBump.y = sprite->BoundsRect.y + collideRect.y;
		sprite->CollideRectBump.width = collideRect.width;
		sprite->CollideRectBump.height = collideRect.height;		
		
		if(sprite->BumpItem != nullptr)
		{
			world->Update(sprite->BumpItem, Rectangle(sprite->CollideRectBump.x, sprite->CollideRectBump.y, sprite->CollideRectBump.width, sprite->CollideRectBump.height));
			printfDebug(DebugInfo,"setCollideRect BumpItem updated it\n");
		}
		else	
		{
			sprite->BumpItem = world->Add(sprite->shared_ptr_self, Rectangle(sprite->CollideRectBump.x, sprite->CollideRectBump.y, sprite->CollideRectBump.width, sprite->CollideRectBump.height));
			cc++;
			printfDebug(DebugInfo,"setCollideRect no BumpItem (added it)\n");
		}
	}
	
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setCollideRect end\n");
}

PDRect pd_api_sprite_getCollideRect(LCDSprite *sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getCollideRect\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getCollideRect end Sprite == NULL \n");
		return {0,0,0,0};
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getCollideRect end Sprite not loaded\n");
		return {0,0,0,0};
	}
	return sprite->CollideRect;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getCollideRect end\n");
}

void pd_api_sprite_clearCollideRect(LCDSprite *sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_clearCollideRect\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_clearCollideRect end Sprite == NULL \n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_clearCollideRect end Sprite not loaded\n");
		return;
	}
	Api->sprite->setCollideRect(sprite, PDRectMake(0,0,0,0));
	printfDebug(DebugTraceFunctions,"pd_api_sprite_clearCollideRect end\n");
}

// caller is responsible for freeing the returned array for all collision methods
void pd_api_sprite_setCollisionResponseFunction(LCDSprite *sprite, LCDSpriteCollisionFilterProc *func)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setCollisionResponseFunction\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setCollisionResponseFunction end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setCollisionResponseFunction end Sprite not loaded\n");
		return;
	}
	sprite->CollisionResponseTypeFunction = func;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setCollisionResponseFunction end\n");
}


SpriteCollisionInfo* pd_api_sprite_checkCollisions(LCDSprite *sprite, float goalX, float goalY, float *actualX, float *actualY, int *len)// access results using SpriteCollisionInfo *info = &results[i];
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_checkCollisions\n");
	if(len)
	 	*len = 0;
	float internalActualX = sprite->BoundsRect.x + (sprite->BoundsRect.width / 2);
	if(actualX)
		*actualX = internalActualX;
	float internalActualY = sprite->BoundsRect.y + (sprite->BoundsRect.height / 2);
	if(actualY)
		*actualY = internalActualY;



	if(sprite == NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_checkCollisions sprite == NULL\n");
		return NULL;
	}

	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_checkCollisions end Sprite not loaded\n");
		return NULL;
	}
	
	SpriteCollisionInfo *Tmp = (SpriteCollisionInfo*) malloc(sizeof(SpriteCollisionInfo));
	memset(Tmp, 0, sizeof(SpriteCollisionInfo));
	
	if(sprite->BumpItem != nullptr)
	{
		CollisionResolution resolution;
		int ccc = 0;
		int c = 0;
		world->Check(resolution, sprite->BumpItem, math::vec2(goalX - (sprite->CollideRectBump.width / 2.0f), goalY - (sprite->CollideRectBump.height / 2.0f)),[&c,&ccc](auto item, auto other, Collision* col){	
			if((item == nullptr) || (other == nullptr) || (item->UserData.GetRaw() == other->UserData.GetRaw()))
				return false;
			
			ccc++;
			Rectangle ItemRect = item->GetRectangle();
			Rectangle OtherRect = other->GetRectangle();
			LCDSprite *spriteCurrent = (LCDSprite*)(item->UserData.GetRaw());
			LCDSprite *spriteOther = (LCDSprite*)(other->UserData.GetRaw());
			
			//printf("#Rects %d Checking Item %p: %f %f %f %f, Checking other %p:%f %f %f %f\n", ccc, spriteCurrent, item->GetRectangle().pos.x, item->GetRectangle().pos.y, item->GetRectangle().scale.x, item->GetRectangle().scale.y,
			//		spriteOther, other->GetRectangle().pos.x, other->GetRectangle().pos.y, other->GetRectangle().scale.x, other->GetRectangle().scale.y);
			//printf("#Sprite %d Checking Item %p: %f %f %f %f, Checking other %p:%f %f %f %f\n", ccc, spriteCurrent, ItemRect.x, ItemRect.y, ItemRect.width, ItemRect.height,
			//		spriteOther, OtherRect.x, OtherRect.y, OtherRect.width, OtherRect.height);
			// if ((ItemRect.x < OtherRect.x + OtherRect.width) &&
			// 	(OtherRect.x < ItemRect.x + ItemRect.width) &&
			// 	(ItemRect.y  < OtherRect.y + OtherRect.height) &&
			// 	(OtherRect.y < ItemRect.y + ItemRect.height))
			if (ItemRect.IsIntersecting(OtherRect))
			{
				c++;
				//printf("#%d InterSecting Item %p: %f %f %f %f, InterSecting other %p:%f %f %f %f\n", c, spriteCurrent, item->GetRectangle().pos.x, item->GetRectangle().pos.y, item->GetRectangle().scale.x, item->GetRectangle().scale.y,
				//	spriteOther, other->GetRectangle().pos.x, other->GetRectangle().pos.y, other->GetRectangle().scale.x, other->GetRectangle().scale.y);
				
				//printf("sprite L:%d C:%d LIL:%d other: L:%d C:%d LIL:%d\n", spriteCurrent->Loaded, spriteCurrent->CollisionsEnabled, spriteCurrent->LoadedInList,spriteOther->LoadedInList, spriteOther->CollisionsEnabled, spriteOther->Loaded);
				if(!spriteCurrent->Loaded || !spriteOther->Loaded || !spriteCurrent->CollisionsEnabled || !spriteOther->CollisionsEnabled || !spriteCurrent->LoadedInList || !spriteOther->LoadedInList)
				{
					//col->Respond<plugin::physics::bump::response::Cross>();
					return false;
				}
				else
				{
					SpriteCollisionResponseType response = kCollisionTypeFreeze;
					if (!((spriteCurrent->CollisionResponseTypeFunction == NULL) ||
						(spriteCurrent->CollisionResponseTypeFunction == nullptr)))
						response = spriteCurrent->CollisionResponseTypeFunction(spriteCurrent, spriteOther);
					switch (response)
					{
						case kCollisionTypeBounce:
							col->Respond<plugin::physics::bump::response::Bounce>();
							break;				
						case kCollisionTypeOverlap:
							col->Respond<plugin::physics::bump::response::Cross>();
							break;
						case kCollisionTypeSlide:
							col->Respond<plugin::physics::bump::response::Slide>();
							break;
						default: //kCollisionTypeFreeze
							col->Respond<plugin::physics::bump::response::Touch>();
							break;
				
					}
					return true;

				}
			}
			return false;
			//printf("Collision doresolve = %d\n", doResolve);	
		});			
		if(actualX)
			*actualX = resolution.pos.x + (sprite->CollideRectBump.width  / 2.0f);
		if(actualY) 
			*actualY = resolution.pos.y + (sprite->CollideRectBump.height / 2.0f);

		for (auto& col : resolution.collisions)
		{
			Tmp = (SpriteCollisionInfo*) realloc(Tmp, ((*len) + 1) * sizeof(SpriteCollisionInfo));
			Tmp[*len].responseType = kCollisionTypeFreeze;
			if(col->Is<response::Touch>())
			{
				Tmp[*len].responseType = kCollisionTypeFreeze;
			}
			if(col->Is<response::Cross>())
			{
				Tmp[*len].responseType = kCollisionTypeOverlap;
			}
			if(col->Is<response::Bounce>())
			{
				Tmp[*len].responseType = kCollisionTypeBounce;
			}
			if(col->Is<response::Slide>())
			{
				Tmp[*len].responseType = kCollisionTypeSlide;
			}
			
			Tmp[*len].other = (LCDSprite *) col->other->UserData.GetRaw();
			Tmp[*len].otherRect.height = col->otherRect.scale.y;
			Tmp[*len].otherRect.width = col->otherRect.scale.x;
			Tmp[*len].otherRect.x = col->otherRect.pos.x;
			Tmp[*len].otherRect.y = col->otherRect.pos.y;
			Tmp[*len].sprite = (LCDSprite *) col->item->UserData.GetRaw();
			Tmp[*len].spriteRect.height = col->itemRect.scale.y;
			Tmp[*len].spriteRect.width = col->itemRect.scale.x;
			Tmp[*len].spriteRect.x = col->itemRect.pos.x;
			Tmp[*len].spriteRect.y = col->itemRect.pos.y;
			Tmp[*len].overlaps = col->overlaps;
			Tmp[*len].normal.x = col->normal.x;
			Tmp[*len].normal.y = col->normal.y;
			Tmp[*len].ti = col->ti;
			Tmp[*len].move.x = col->move.x;
			Tmp[*len].move.x = col->move.y;
			Tmp[*len].touch.x = col->touch.x;
			Tmp[*len].touch.y = col->touch.y;		
			*len = (*len) +1;
		}
	}
	printfDebug(DebugTraceFunctions,"pd_api_sprite_checkCollisions end\n");
	if (*len == 0)
	{
		free(Tmp);
		return NULL;
	}
	return Tmp;
}

SpriteCollisionInfo* pd_api_sprite_moveWithCollisions(LCDSprite *sprite, float goalX, float goalY, float *actualX, float *actualY, int *len)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_moveWithCollisions\n");
	if(len)
	 	*len = 0;
	float internalActualX = sprite->BoundsRect.x + (sprite->BoundsRect.width / 2);
	if(actualX)
		*actualX = internalActualX;
	float internalActualY = sprite->BoundsRect.y + (sprite->BoundsRect.height / 2);
	if(actualY)
		*actualY = internalActualY;

	if(sprite == NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_moveWithCollisions sprite == NULL\n");
		return NULL;
	}
	
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_moveWithCollisions end Sprite not loaded\n");
		return NULL;
	}
	
	SpriteCollisionInfo *Tmp = (SpriteCollisionInfo*) malloc(sizeof(SpriteCollisionInfo));
	memset(Tmp, 0, sizeof(SpriteCollisionInfo));


	if(sprite->BumpItem != nullptr)
	{
		CollisionResolution resolution;
		int ccc = 0;
		int c = 0;;
		world->Move(resolution, sprite->BumpItem, math::vec2(goalX - (sprite->CollideRectBump.width / 2.0f), goalY - (sprite->CollideRectBump.height / 2.0f)),[&ccc, &c](auto item, auto other, Collision* col)
		{	
			if((item == nullptr) || (other == nullptr) || (item->UserData.GetRaw() == other->UserData.GetRaw()))
				return false;
			
			ccc++;
			Rectangle ItemRect = item->GetRectangle();
			Rectangle OtherRect = other->GetRectangle();
			LCDSprite *spriteCurrent = (LCDSprite*)(item->UserData.GetRaw());
			LCDSprite *spriteOther = (LCDSprite*)(other->UserData.GetRaw());
			
			//printf("#Rects %d Checking Item %p: %f %f %f %f, Checking other %p:%f %f %f %f\n", ccc, spriteCurrent, item->GetRectangle().pos.x, item->GetRectangle().pos.y, item->GetRectangle().scale.x, item->GetRectangle().scale.y,
			//		spriteOther, other->GetRectangle().pos.x, other->GetRectangle().pos.y, other->GetRectangle().scale.x, other->GetRectangle().scale.y);
			//printf("#Sprite %d Checking Item %p: %f %f %f %f, Checking other %p:%f %f %f %f\n", ccc, spriteCurrent, ItemRect.x, ItemRect.y, ItemRect.width, ItemRect.height,
			//		spriteOther, OtherRect.x, OtherRect.y, OtherRect.width, OtherRect.height);
			// if ((ItemRect.x < OtherRect.x + OtherRect.width) &&
			// 	(OtherRect.x < ItemRect.x + ItemRect.width) &&
			// 	(ItemRect.y  < OtherRect.y + OtherRect.height) &&
			// 	(OtherRect.y < ItemRect.y + ItemRect.height))
			if (ItemRect.IsIntersecting(OtherRect))
			{
				c++;
				//printf("#%d InterSecting Item %p: %f %f %f %f, InterSecting other %p:%f %f %f %f\n", c, spriteCurrent, item->GetRectangle().pos.x, item->GetRectangle().pos.y, item->GetRectangle().scale.x, item->GetRectangle().scale.y,
				//	spriteOther, other->GetRectangle().pos.x, other->GetRectangle().pos.y, other->GetRectangle().scale.x, other->GetRectangle().scale.y);
				
				//printf("sprite L:%d C:%d LIL:%d other: L:%d C:%d LIL:%d\n", spriteCurrent->Loaded, spriteCurrent->CollisionsEnabled, spriteCurrent->LoadedInList,spriteOther->LoadedInList, spriteOther->CollisionsEnabled, spriteOther->Loaded);
				if(!spriteCurrent->Loaded || !spriteOther->Loaded || !spriteCurrent->CollisionsEnabled || !spriteOther->CollisionsEnabled || !spriteCurrent->LoadedInList || !spriteOther->LoadedInList)
				{
					//col->Respond<plugin::physics::bump::response::Cross>();
					return false;
				}
				else
				{
					SpriteCollisionResponseType response = kCollisionTypeFreeze;
					if (!((spriteCurrent->CollisionResponseTypeFunction == NULL) ||
						(spriteCurrent->CollisionResponseTypeFunction == nullptr)))
						response = spriteCurrent->CollisionResponseTypeFunction(spriteCurrent, spriteOther);
					switch (response)
					{
						case kCollisionTypeBounce:
							col->Respond<plugin::physics::bump::response::Bounce>();
							break;				
						case kCollisionTypeOverlap:
							col->Respond<plugin::physics::bump::response::Cross>();
							break;
						case kCollisionTypeSlide:
							col->Respond<plugin::physics::bump::response::Slide>();
							break;
						default: //kCollisionTypeFreeze
							col->Respond<plugin::physics::bump::response::Touch>();
							break;
				
					}
					return true;

				}
			}
			return false;
			//printf("Collision doresolve = %d\n", doResolve);	
		});	
		float newActualx =  resolution.pos.x + (sprite->CollideRectBump.width / 2.0f);
		float newActualy =  resolution.pos.y + (sprite->CollideRectBump.height / 2.0f);
		//PDRect Rect = makePDRect()
		//Api->sprite->setBounds(sprite, PDRectMake(resolution.pos.x, resolution.pos.y, sprite->BoundsRect.width, sprite->BoundsRect.height));
		//sprite->BoundsRect.x = resolution.pos.x;
		//sprite->BoundsRect.y = resolution.pos.y;
		Api->sprite->moveTo(sprite, newActualx, newActualy);
		if(actualX)
			*actualX = newActualx;
		if(actualY) 
			*actualY = newActualy;
		//printfDebug(DebugInfo, "Collision len in resolution.collisions %d\n", resolution.collisions.size());
		for (auto& col : resolution.collisions)
		{
			Tmp = (SpriteCollisionInfo*) realloc(Tmp, ((*len) + 1) * sizeof(SpriteCollisionInfo));
			Tmp[*len].responseType = kCollisionTypeFreeze;
			if(col->Is<response::Touch>())
			{
				Tmp[*len].responseType = kCollisionTypeFreeze;
			}
			if(col->Is<response::Cross>())
			{
				Tmp[*len].responseType = kCollisionTypeOverlap;
			}
			if(col->Is<response::Bounce>())
			{
				Tmp[*len].responseType = kCollisionTypeBounce;
			}
			if(col->Is<response::Slide>())
			{
				Tmp[*len].responseType = kCollisionTypeSlide;
			}
			
			Tmp[*len].other = (LCDSprite *) col->other->UserData.GetRaw();
			Tmp[*len].otherRect.height = col->otherRect.scale.y;
			Tmp[*len].otherRect.width = col->otherRect.scale.x;
			Tmp[*len].otherRect.x = col->otherRect.pos.x;
			Tmp[*len].otherRect.y = col->otherRect.pos.y;
			Tmp[*len].sprite = (LCDSprite *) col->item->UserData.GetRaw();
			Tmp[*len].spriteRect.height = col->itemRect.scale.y;
			Tmp[*len].spriteRect.width = col->itemRect.scale.x;
			Tmp[*len].spriteRect.x = col->itemRect.pos.x;
			Tmp[*len].spriteRect.y = col->itemRect.pos.y;
			Tmp[*len].overlaps = col->overlaps;
			Tmp[*len].normal.x = col->normal.x;
			Tmp[*len].normal.y = col->normal.y; //seems inverse in simulator
			Tmp[*len].ti = col->ti;
			Tmp[*len].move.x = col->move.x;
			Tmp[*len].move.x = col->move.y;
			Tmp[*len].touch.x = col->touch.x;
			Tmp[*len].touch.y = col->touch.y;		
			*len = (*len) +1;
		}
	}
	printfDebug(DebugTraceFunctions,"pd_api_sprite_moveWithCollisions end\n");
	if (*len == 0)
	{
		free(Tmp);
		return NULL;
	}
	return Tmp;
}

LCDSprite** pd_api_sprite_querySpritesAtPoint(float x, float y, int *len)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_querySpritesAtPoint\n");
	if(len)
		*len = 0;
    LCDSprite ** Tmp = (LCDSprite **) malloc(sizeof(*Tmp));
    printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_querySpritesAtPoint not implemented!\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_querySpritesAtPoint end\n");
	return Tmp;
}

LCDSprite** pd_api_sprite_querySpritesInRect(float x, float y, float width, float height, int *len)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_querySpritesInRect end\n");
	if(len)
		*len = 0;
    LCDSprite ** Tmp = (LCDSprite **) malloc(sizeof(*Tmp));
	printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_querySpritesInRect not implemented\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_querySpritesInRect end\n");
    return Tmp;
}

LCDSprite** pd_api_sprite_querySpritesAlongLine(float x1, float y1, float x2, float y2, int *len)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_querySpritesAlongLine\n");
	if(len)
		*len = 0;
    LCDSprite ** Tmp = (LCDSprite **) malloc(sizeof(*Tmp));
	printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_querySpritesAlongLine not implemented\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_querySpritesAlongLine end\n");
    return Tmp;
}

SpriteQueryInfo* pd_api_sprite_querySpriteInfoAlongLine(float x1, float y1, float x2, float y2, int *len)// access results using SpriteQueryInfo *info = &results[i]{}
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_querySpriteInfoAlongLine\n");
    if(len)
		*len = 0;
	printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_querySpriteInfoAlongLine not implemented\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_querySpriteInfoAlongLine end\n");
	return NULL;
}

LCDSprite** pd_api_sprite_overlappingSprites(LCDSprite *sprite, int *len)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_overlappingSprites\n");
	if(len)
		*len = 0;
    LCDSprite ** Tmp = (LCDSprite **) malloc(sizeof(*Tmp));
	printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_overlappingSprites not implemented!\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_overlappingSprites end\n");
    return Tmp;
}

LCDSprite** pd_api_sprite_allOverlappingSprites(int *len)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_allOverlappingSprites\n");
	if(len)
		*len = 0;
    LCDSprite ** Tmp = (LCDSprite **) malloc(sizeof(*Tmp));
	printfDebug(DEBUGNOTIMPLEMENTEDFUNCTIONS,"pd_api_sprite_allOverlappingSprites not implemented!\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_allOverlappingSprites end\n");
    return Tmp;
}


// added in 1.7
void pd_api_sprite_setStencilPattern(LCDSprite* sprite, uint8_t pattern[8])
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setStencilPattern\n");
	printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_setStencilPattern not implemented!\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setStencilPattern end\n");
}

void pd_api_sprite_clearStencil(LCDSprite* sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_clearStencil end\n");
	printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_clearStencil not implemented!\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_clearStencil end\n");
}

void pd_api_sprite_setUserdata(LCDSprite* sprite, void* userdata)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setUserdata end\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setUserdata end Sprite == NULL\n");
		return;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_setUserdata end Sprite not loaded\n");
		return;
	}
	sprite->userdata = userdata;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setUserdata end\n");
}

void* pd_api_sprite_getUserdata(LCDSprite* sprite)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getUserdata end\n");
	if(sprite==NULL)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getUserdata end Sprite == NULL\n");
		return NULL;
	}
	if(!sprite->Loaded)
	{
		printfDebug(DebugTraceFunctions,"pd_api_sprite_getUserdata end Sprite not loaded\n");
		return NULL;
	}
	return sprite->userdata;
	printfDebug(DebugTraceFunctions,"pd_api_sprite_getUserdata end\n");
}

// added in 1.10
void pd_api_sprite_setStencilImage(LCDSprite *sprite, LCDBitmap* stencil, int tile)
{
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setStencilImage\n");
	printfDebug(DebugNotImplementedFunctions,"pd_api_sprite_setStencilImage not implemented!\n");
	printfDebug(DebugTraceFunctions,"pd_api_sprite_setStencilImage end\n");
}

// 2.1
void pd_api_sprite_setCenter(LCDSprite* s, float x, float y)
{
	if(!s)
		return;

	s->CenterX = s->CenterX;
	s->CenterY = s->CenterY;
}

void pd_api_sprite_getCenter(LCDSprite* s, float* x, float* y)
{
	if(!s)
		return;

	if(x)
		*x = s->CenterX;
	if(y)
		*y = s->CenterY;
}

playdate_sprite* pd_api_sprite_Create_playdate_sprite()
{
    playdate_sprite *Tmp = (playdate_sprite*) malloc(sizeof(*Tmp));
    Tmp->setAlwaysRedraw = pd_api_sprite_setAlwaysRedraw;
	Tmp->addDirtyRect = pd_api_sprite_addDirtyRect;
	Tmp->drawSprites = pd_api_sprite_drawSprites;
	Tmp->updateAndDrawSprites = pd_api_sprite_updateAndDrawSprites;

	Tmp->newSprite = pd_api_sprite_newSprite;
	Tmp->freeSprite = pd_api_sprite_freeSprite;
	Tmp->copy = pd_api_sprite_copy;
	
	Tmp->addSprite = pd_api_sprite_addSprite;
	Tmp->removeSprite = pd_api_sprite_removeSprite;
	Tmp->removeSprites = pd_api_sprite_removeSprites;
	Tmp->removeAllSprites = pd_api_sprite_removeAllSprites;
	Tmp->getSpriteCount = pd_api_sprite_getSpriteCount;
	
	Tmp->setBounds = pd_api_sprite_setBounds;
	Tmp->getBounds = pd_api_sprite_getBounds;
	Tmp->moveTo = pd_api_sprite_moveTo;
	Tmp->moveBy = pd_api_sprite_moveBy;

	Tmp->setImage = pd_api_sprite_setImage;
	Tmp->getImage = pd_api_sprite_getImage;
	Tmp->setSize = pd_api_sprite_setSize;
	Tmp->setZIndex = pd_api_sprite_setZIndex;
	Tmp->getZIndex = pd_api_sprite_getZIndex;
	
	Tmp->setDrawMode = pd_api_sprite_setDrawMode;
	Tmp->setImageFlip = pd_api_sprite_setImageFlip;
	Tmp->getImageFlip = pd_api_sprite_getImageFlip;
	Tmp->setStencil = pd_api_sprite_setStencil;
	
	Tmp->setClipRect = pd_api_sprite_setClipRect;
	Tmp->clearClipRect = pd_api_sprite_clearClipRect;
	Tmp->setClipRectsInRange = pd_api_sprite_setClipRectsInRange;
	Tmp->clearClipRectsInRange = pd_api_sprite_clearClipRectsInRange;
	
	Tmp->setUpdatesEnabled = pd_api_sprite_setUpdatesEnabled;
	Tmp->updatesEnabled = pd_api_sprite_updatesEnabled;
	Tmp->setCollisionsEnabled = pd_api_sprite_setCollisionsEnabled;
	Tmp->collisionsEnabled = pd_api_sprite_collisionsEnabled;
	Tmp->setVisible = pd_api_sprite_setVisible;
	Tmp->isVisible = pd_api_sprite_isVisible;
	Tmp->setOpaque = pd_api_sprite_setOpaque;
	Tmp->markDirty = pd_api_sprite_markDirty;
	
	Tmp->setTag = pd_api_sprite_setTag;
	Tmp->getTag = pd_api_sprite_getTag;
	
	Tmp->setIgnoresDrawOffset = pd_api_sprite_setIgnoresDrawOffset;
	
	Tmp->setUpdateFunction = pd_api_sprite_setUpdateFunction;
	Tmp->setDrawFunction = pd_api_sprite_setDrawFunction;
	Tmp->getPosition = pd_api_sprite_getPosition;

	// Collisions
	Tmp->resetCollisionWorld = pd_api_sprite_resetCollisionWorld;
	Tmp->setCollideRect = pd_api_sprite_setCollideRect;
	Tmp->getCollideRect = pd_api_sprite_getCollideRect;
	Tmp->clearCollideRect = pd_api_sprite_clearCollideRect;
	
	// caller is responsible for freeing the returned array for all collision methods
	Tmp->setCollisionResponseFunction = pd_api_sprite_setCollisionResponseFunction;
	Tmp->checkCollisions = pd_api_sprite_checkCollisions;
	Tmp->moveWithCollisions = pd_api_sprite_moveWithCollisions;
	Tmp->querySpritesAtPoint = pd_api_sprite_querySpritesAtPoint;
	Tmp->querySpritesInRect = pd_api_sprite_querySpritesInRect;
	Tmp->querySpritesAlongLine = pd_api_sprite_querySpritesAlongLine;
	Tmp->querySpriteInfoAlongLine = pd_api_sprite_querySpriteInfoAlongLine;
	Tmp->overlappingSprites = pd_api_sprite_overlappingSprites;
	Tmp->allOverlappingSprites = pd_api_sprite_allOverlappingSprites;

	// added in 1.7
	Tmp->setStencilPattern = pd_api_sprite_setStencilPattern;
	Tmp->clearStencil = pd_api_sprite_clearStencil;

	Tmp->setUserdata = pd_api_sprite_setUserdata;
	Tmp->getUserdata = pd_api_sprite_getUserdata;
	
	// added in 1.10
	Tmp->setStencilImage = pd_api_sprite_setStencilImage;

	// 2.1
	Tmp->setCenter = pd_api_sprite_setCenter;
	Tmp->getCenter = pd_api_sprite_getCenter; 

    return Tmp;
}