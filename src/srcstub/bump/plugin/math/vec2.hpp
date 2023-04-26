#pragma once
#include <cmath>
#include <vector>
#include <iostream>

namespace math
{
    class vec2
    {
    public:
        vec2(){};
        vec2(float x,float y){this->x =x; this->y = y;}
        math::vec2 abs(){return math::vec2(std::abs(x),std::abs(y));}
        float volume(){
            auto a = abs();
            return a.x + a.y;
        };


        bool compare(const vec2& rhs) const {return (x == rhs.x and y == rhs.y);}
        inline bool operator==(const vec2& rhs)const { return compare(rhs); }
        inline bool operator!=(const vec2& rhs)const { return not compare(rhs); }


        vec2 operator+(const vec2& rhs) const { return vec2(x + rhs.x, y + rhs.y);}
        vec2 operator-(const vec2& rhs) const { return vec2(x - rhs.x, y - rhs.y);}
        vec2 operator*(const vec2& rhs )const { return vec2(x * rhs.x, y * rhs.y);}
        vec2 operator/(const vec2& rhs) const { return vec2(x / rhs.x, y / rhs.y);}
        vec2 operator*(const  float& dom) const { return vec2(x * dom, y * dom);}
        vec2 operator/(const float& dom) const { return vec2(x / dom, y / dom);}

        inline friend std::ostream& operator<<(std::ostream& out, const vec2& p) {
            out << "vec2(" << p.x << "," << p.y << ")";
            return out;
          }
    public:
        float x=0;
        float y=0;
    };
}
