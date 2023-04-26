#include "../Rectangle.hpp"
#include "../Collision.hpp"
#include "../Utilities.hpp"

#include <iostream>
using namespace plugin::physics::bump;

bool Rectangle::ContainsPoint(math::vec2 point) const
{
    return (point.x - pos.x > DELTA) and
            (point.y - pos.y > DELTA) and
            (pos.x + scale.x - point.x > DELTA) and
            (pos.y + scale.y - point.y > DELTA);
}
bool Rectangle::IsIntersecting(const Rectangle& other) const
{
    return (pos.x < other.pos.x + other.scale.x) and
    (other.pos.x < pos.x + scale.x) and
    (pos.y  < other.pos.y + other.scale.y) and
    (other.pos.y < pos.y + scale.y);
}
float Rectangle::GetSquareDistance(const Rectangle& other) const
{
    auto delta = pos - other.pos + (scale - other.scale)*0.5f;
    return delta.x*delta.x + delta.y*delta.y;;
}

math::vec2 Rectangle::GetNearestCorner(const math::vec2& other) const
{
    return math::vec2(
        nearest(other.x, pos.x, pos.x + scale.x),
        nearest(other.y, pos.y, pos.y + scale.y)
    );
}
Rectangle Rectangle::GetDifference(const Rectangle& other) const
{
    return Rectangle(
        other.pos.x - pos.x - scale.x,
        other.pos.y - pos.y - scale.y,
        scale.x + other.scale.x,
        scale.y + other.scale.y
    );
}

bool Rectangle::GetSegmentIntersectionIndices(math::vec2 top, math::vec2 bottom, IntersectionIndicie& indicie) const
{
    math::vec2 delta = bottom - top;
    math::vec2 normal;

    float p=0;
    float q=0;
    float r=0;

    for (int side =1; side < 5; side++)
    {
        if (side == 1)
        {
            normal.x = -1;
            normal.y = 0;
            p = -delta.x;
            q = top.x - pos.x;
        }
        else if( side == 2)
        {
            normal.x = 1;
            normal.y = 0;
            p = delta.x;
            q = pos.x + scale.x - top.x;
        }
        else if (side == 3)
        {
            normal.x = 0;
            normal.y = -1;
            p = -delta.y;
            q = top.y - pos.y;
        }
        else
        {
            normal.x = 0;
            normal.y = 1;
            p = delta.y;
            q = pos.y + scale.y - top.y;
        }

        if (p == 0.0f)
        {
            if (q <= 0.0f)
            {
                return false;
            }
        }
        else
        {
            r = q / p;
            if (p < 0.0f)
            {
                if ( r > indicie.ti2){return false;}
                else if (r > indicie.ti1){
                    indicie.ti1 = r;
                    indicie.normalA = normal;
                }
            }
            else
            {
                if (r < indicie.ti1){return false;}
                else if(r < indicie.ti2) {
                    indicie.ti2 = r;
                    indicie.normalB = normal;
                }
            }
        }
    }
    return true;
    // return {ti1, ti2, normalA, normalB}
}

bool Rectangle::DetectCollision(const Rectangle& other,Collision* collision, math::vec2 goal) const
{
    collision->move = goal - pos;
    auto difference = GetDifference(other);

    if (difference.ContainsPoint(ZERO))
    {
        auto diffCorner = difference.GetNearestCorner(ZERO);
        float absdiffy = std::abs(diffCorner.y);
        float absdiffx = std::abs(diffCorner.x);
        auto intersectionArea = math::vec2(std::min(scale.x, absdiffx), std::min(scale.y, absdiffy));
        collision->ti = -intersectionArea.x * intersectionArea.y;  // ti is the negative area of intersection
        collision->overlaps = true;
    }
    else
    {
        IntersectionIndicie indicie;
        indicie.ti1 = -std::numeric_limits<float>::max();
        indicie.ti2 = std::numeric_limits<float>::max();
        auto valid = difference.GetSegmentIntersectionIndices(ZERO,collision->move, indicie);

        if (
            valid and indicie.ti1 < 1.0f and
            (std::abs(indicie.ti1 - indicie.ti2) >= DELTA) and
            ( (0.0f < indicie.ti1 + DELTA) or (0.0f == indicie.ti1 and indicie.ti2 > 0.0f) )
        )
        {
            collision->ti = indicie.ti1;
            collision->normal = indicie.normalA;
            collision->overlaps = false;
        }
        else
        {
            return false;
        }
    }

    math::vec2 t;

    if (collision->overlaps)
    {

        if (collision->move.x == 0.0f and collision->move.y == 0.0f)
        {
            auto diffCorner = difference.GetNearestCorner(ZERO);
            if (std::abs(diffCorner.x) < std::abs(diffCorner.y)){diffCorner.y = 0.0f;}else{diffCorner.x = 0.0f;}
            collision->normal = sign(diffCorner);
            collision->touch = pos + diffCorner;
        }
        else
        {
            IntersectionIndicie indicie;
            indicie.ti1 = -std::numeric_limits<float>::max();
            indicie.ti2 = 1;
            auto valid = difference.GetSegmentIntersectionIndices(ZERO,collision->move, indicie);
            if (not valid)
            {
                return false;
            }
            else
            {
                collision->touch.x = pos.x + collision->move.x * indicie.ti1;
                collision->touch.y = pos.y + collision->move.y * indicie.ti1;
                collision->normal = indicie.normalA;
            }
        }
    }
    else
    {
        collision->touch = pos + collision->move*collision->ti;
    }

    collision->itemRect = *this;
    collision->otherRect = Rectangle(other);
    return true;
}
