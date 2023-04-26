#include "sprite.h"
#include "defines.h"
#include "debug.h"
#include "bump/World.hpp"

LCDSprite::LCDSprite()
{
	this->shared_ptr_self = nullptr;
	this->Bitmap = NULL;
	this->BoundsRect = {0,0,0,0};
	this->CollideRect = {0,0,0,0};
	this->CollideRectBump = {0,0,0,0}; 
	this->CenterX = 0.5f;
	this->CenterY = 0.5f;
	this->ClipRect.bottom = 0;
	this->ClipRect.right = 0;
	this->ClipRect.left = 0;
	this->ClipRect.top = 0;
	this->Dirty = true;
	this->DrawFunction = NULL; 
	this->UpdateFunction = NULL; 
	this->CollisionResponseTypeFunction = NULL;
	this->CenterPointX = 0.0f;
	this->CenterPointY = 0.0f;
	this->zIndex = 0;
	this->DrawMode = kDrawModeCopy;
	this->FlipMode = kBitmapUnflipped;
	this->Visible = true;
	this->CollisionsEnabled = true;
	this->UpdatesEnabled = true;
	this->Tag = 0;
	this->hasNewMoveBy = false;
	this->hasNewPos = false;
	this->userdata = NULL;
    this->LoadedInList = false;
	this->Loaded = true;
	this->BumpItem = nullptr;
    printfDebug(DebugInfo, "Sprite Created %p\n", this);
}

LCDSprite::~LCDSprite()
{
    printfDebug(DebugInfo, "Sprite Destroyed %p\n", this);
}
