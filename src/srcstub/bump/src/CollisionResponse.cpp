#include "../CollisionResponses.hpp"
#include "../World.hpp"

using namespace plugin::physics::bump::response;

void Touch::Resolve(
    CollisionResolution& resolution,
    Collision* col,
    const Rectangle& rect,
    math::vec2 goal,
    Filter filter
)
{
    resolution.pos = col->touch;
}



void Cross::Resolve(
    CollisionResolution& resolution,
    Collision* col,
    const Rectangle& rect,
    math::vec2 goal,
    Filter filter
)
{
    Collisions collisions;
    world->Project(collisions, col->item, rect, goal, filter);
    resolution.pos = goal;
    resolution.collisions = collisions;
}

void Slide::Resolve(
    CollisionResolution& resolution,
    Collision* col,
    const Rectangle& rect,
    math::vec2 goal,
    Filter filter
)
{
    if (col->move.x != 0.0f or col->move.y != 0.0f)
    {
        if (col->normal.x != 0.0f)
        {
            goal.x = col->touch.x;
        }
        else
        {
            goal.y = col->touch.y;
        }
    }


    Rectangle tRect = Rectangle(col->touch, rect.scale);

    Collisions collisions;
    world->Project(collisions, col->item, tRect, goal, filter);
    this->slide = goal;
    resolution.pos = goal;
    resolution.collisions = collisions;

}


void Bounce::Resolve(
    CollisionResolution& resolution,
    Collision* col,
    const Rectangle& rect,
    math::vec2 goal,
    Filter filter
)
{

    auto b = col->touch;

    if (col->move.x != 0 or col->move.y != 0)
    {
        auto bn = goal - col->touch;
        if (col->normal.x == 0)
        {
            bn.y = -bn.y;
        }else
        {
            bn.x = -bn.x;
        }
        b.x = col->touch.x + bn.x;
        b.y = col->touch.y + bn.y;
    }

    goal.x = b.x;
    goal.y = b.y;

    Rectangle tRect = Rectangle(col->touch, rect.scale);

    this->bounce = goal;

    Collisions collisions;
    world->Project(collisions, col->item, tRect, goal, filter);

    resolution.pos = goal;
    resolution.collisions = collisions;
}
