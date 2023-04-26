#pragma once
#include "Rectangle.hpp"
#include <plugin/math/vec2.hpp>
#include <functional>

namespace plugin::physics::bump
{
    struct GridTraverse
    {
        float step;
        float delta;
        float touch;
    };

    class Grid
    {
    public:
        Grid(int cellSize){this->cellSize = cellSize;};

        math::vec2 ToWorld(const math::vec2& pos);
        math::vec2 ToCell(const math::vec2& pos);
        GridTraverse TraverseInitStep(float ct, float tx,float ty);
        void Traverse(math::vec2 top, math::vec2 bottom, std::function<void(int x,int y)> f);
        Rectangle ToCellRect(const Rectangle& rect);

    public:
        int cellSize;
        math::vec2 center;
    };
}
