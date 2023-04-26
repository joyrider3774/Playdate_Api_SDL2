#pragma once
#include <plugin/math/vec2.hpp>
#include <iostream>

namespace plugin::physics::bump
{
    class Collision;

    struct IntersectionIndicie
    {
        float ti1=0;
        float ti2=1;
        math::vec2 normalA;
        math::vec2 normalB;
    };

    class Rectangle
    {
    public:
        Rectangle(math::vec2 pos, math::vec2 scale){
            this->pos = pos;
            this->scale = scale;
        };
        Rectangle(float x, float y, float w, float h)
        {
            this->pos = math::vec2(x,y);
            this->scale = math::vec2(w,h);
        }
        Rectangle(){
            this->pos = math::vec2(0,0);
            this->scale = math::vec2(0,0);
        }
        bool ContainsPoint(math::vec2 point) const;
        bool IsIntersecting(const Rectangle& other) const;
        float GetSquareDistance(const Rectangle& other) const;
        math::vec2 GetNearestCorner(const math::vec2& other) const;
        Rectangle GetDifference(const Rectangle& other) const;
        bool GetSegmentIntersectionIndices(math::vec2 top, math::vec2 bottom, IntersectionIndicie& indicie) const;
        bool DetectCollision(const Rectangle& other, Collision* collision) const
        {return DetectCollision(other, collision, pos);}
        bool DetectCollision(const Rectangle& other,Collision* collision, math::vec2 goal) const;

        bool compare(const Rectangle& rhs){return (pos == rhs.pos and pos == rhs.scale);}
        inline bool operator==(const Rectangle& rhs){ return compare(rhs); }
        inline bool operator!=(const Rectangle& rhs){ return not compare(rhs); }
        inline friend std::ostream& operator<<(std::ostream& out, const Rectangle& rect) {
            out << "Rectangle(" << rect.pos.x << "," << rect.pos.y << "," << rect.scale.x << "," << rect.scale.y << ")";
            return out;
          }


    public:
        math::vec2 pos;
        math::vec2 scale;
    };
}
