#include <stdint.h>
#include <stdlib.h>
#include "bump/World.hpp"
#include "pd_api/pd_api_gfx.h"
#include "pd_api/pd_api_sprite.h"

using namespace plugin::physics::bump;

class LCDSprite;

class LCDSprite
{
	public:
		LCDSprite();
		~LCDSprite();
		std::shared_ptr<LCDSprite> shared_ptr_self;
		Item* BumpItem;
		bool Dirty;
		LCDBitmap* Bitmap;
		LCDBitmapFlip FlipMode;
		PDRect BoundsRect;
		PDRect CollideRectBump;
		LCDRect ClipRect;
		PDRect CollideRect;
		float CenterX;
		float CenterY;
		bool hasNewPos;
		bool hasNewMoveBy;
		int16_t zIndex;
		LCDBitmapDrawMode DrawMode;
		LCDSpriteDrawFunction *DrawFunction;
		LCDSpriteUpdateFunction *UpdateFunction;
		LCDSpriteCollisionFilterProc *CollisionResponseTypeFunction;
		bool UpdatesEnabled;
		bool Visible;
		bool CollisionsEnabled;
		uint8_t Tag;
		void* userdata;
        bool LoadedInList;
		bool Loaded;
		float CenterPointX;
		float CenterPointY;
};