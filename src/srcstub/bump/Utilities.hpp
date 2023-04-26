#pragma once
#include <limits>
#include <plugin/math/vec2.hpp>

namespace plugin::physics::bump
{
    static float DELTA = 1e-10f;
    static math::vec2 ZERO = math::vec2(0,0);

    static float nearest(float x, float a, float b)
    {
        if (std::abs(a - x) < std::abs(b - x) ){return a;} else {return b;}
    };

    static int sign(float x)
    {
        if (x > 0){return 1;}
        if (x == 0){return 0;}
        return -1;
    }
    static math::vec2 sign(const math::vec2& other)
    {
        return math::vec2(sign(other.x),sign(other.y));
    }

}
