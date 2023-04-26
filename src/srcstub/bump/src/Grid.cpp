#include "../Grid.hpp"

using namespace plugin::physics::bump;

math::vec2 Grid::ToWorld(const math::vec2& pos)
{
    return math::vec2((pos.x - 1.0f)*cellSize, (pos.y-1.0f)*cellSize);
}
math::vec2 Grid::ToCell(const math::vec2& pos)
{
    return math::vec2(
        std::floor(pos.x / cellSize) + 1.0f,
        std::floor(pos.y / cellSize) + 1.0f
    );
}

GridTraverse Grid::TraverseInitStep(float ct, float tx, float ty)
{
    auto v = ty - tx;

    GridTraverse traverse;
    if (v > 0)
    {
        traverse.step = 1.0f;
        traverse.delta =  cellSize / v;
        traverse.touch = ((ct + v) * cellSize - tx) / v;
    }
    else if(v < 0)
    {
        traverse.step = -1.0f;
        traverse.delta =  -cellSize / v;
        traverse.touch = ((ct + v - 1.0f) * cellSize - tx) / v;
    }
    else
    {
        traverse.step = 0.0f;
        traverse.delta = std::numeric_limits<float>::max();
        traverse.touch = std::numeric_limits<float>::max();
    }
    return traverse;
}


void Grid::Traverse(math::vec2 top, math::vec2 bottom, std::function<void(int x,int y)> f)
{
    auto cTop = ToCell(top);
    auto cBottom = ToCell(bottom);

    auto traverseX = TraverseInitStep(cTop.x, top.x, bottom.x);
    auto traverseY = TraverseInitStep(cTop.y, top.y, bottom.y);

    math::vec2 center = cTop;

    f(center.x, center.y);

    while (std::abs(center.x - cBottom.x) + std::abs(center.y - cBottom.y) > 1.0f)
    {
        if (traverseX.touch < traverseY.touch)
        {
            traverseX.touch = traverseX.touch + traverseX.delta;
            center.x = center.x + traverseX.step;
            f(center.x,center.y);
        }
        else
        {
            if (traverseX.touch == traverseY.touch)
            {
                f(center.x + traverseX.step, center.y);
            }
            traverseY.touch = traverseY.touch + traverseY.delta;
            center.y = center.y + traverseY.step;
            f(center.x,center.y);
        }
    }
    if (center.x != cBottom.x or center.y != cBottom.y)
    {
        f(center.x,center.y);
    }
}
Rectangle Grid::ToCellRect(const Rectangle& rect)
{
    auto center = ToCell(rect.pos);
    float cr = std::ceil((rect.pos.x+rect.scale.x) / cellSize);
    float cb = std::ceil((rect.pos.y+rect.scale.y) / cellSize);
    return Rectangle(
        math::vec2(center.x,center.y),
        math::vec2(cr - center.x + 1, cb - center.y + 1)
    );
}
