#include "../World.hpp"
#include "../CollisionResponses.hpp"
#include "../Collision.hpp"
#include <iostream>
#include <algorithm>

using namespace plugin::physics::bump;

World::World(int cellsize) : grid(cellsize)
{

}

template<typename CONTAINER, typename LAMBDA>
inline static bool removeIf(CONTAINER& container, LAMBDA lambda)
{
    auto it = std::remove_if(container.begin(), container.end(), lambda);
    bool any_change = it != container.end();
    container.erase(it, container.end());

    return any_change;
}

void World::Project(Collisions& colissions, Item* item,const Rectangle& rect, math::vec2 goal, Filter filter)
{
    // assertIsRect(x,y,w,h)
    std::set<Item*> visited;
    if (item != nullptr)
    {
        visited.insert(item);
    }

    float tl = std::min(goal.x, rect.pos.x);
    float tt = std::min(goal.y, rect.pos.y);

    float tr = std::max(goal.x + rect.scale.x, rect.pos.x + rect.scale.x);
    float tb = std::max(goal.y + rect.scale.y, rect.pos.y + rect.scale.y);
    float tw = tr - tl;
    float th = tb - tt;

    auto c = grid.ToCellRect(Rectangle(tl,tt,tw,th));
    std::set<Item*> dictItemsInCellRect;
    GetDictItemsInCellRect(dictItemsInCellRect, c);

    for (auto other: dictItemsInCellRect)
    {
        if (not (visited.find(other) != visited.end()))
        {
            visited.insert(other);
            std::shared_ptr<Collision> col = std::make_shared<Collision>();

            bool respond=false;
            if (filter)
            {
                respond = filter(item,other, col.get());
                if (respond and not col->Response())
                {
                    col->Respond<response::DefaultCollisionResponse>();
                }
            }
            else
            {
                respond = true;
                col->Respond<response::DefaultCollisionResponse>();
            }

            if (respond)
            {
                col->response->world = this;


                if (rect.DetectCollision(other->rect,col.get(),goal))
                {
                    col->other = other;
                    col->item = item;
                    colissions.push_back(col);
                }
            }
        }
    }
    std::sort(colissions.begin(), colissions.end(), [](auto a, auto b){
        if (a->ti == b->ti)
        {
            auto ir = a->itemRect;
            auto ar = a->otherRect;
            auto br = b->otherRect;

            auto ad = ir.GetSquareDistance(ar);
            auto bd = ir.GetSquareDistance(br);

            return ad < bd;
        }
        return a->ti < b->ti;
    });
}

int World::CountCells()
{
    int count = 0;
    for (auto& row: rows)
    {
        count = count + row.second.size();
    }
    return count;
}
int World::CountCellItems()
{
    std::set<Item*> items;
    int count = 0;
    for (auto& row: rows)
    {
        for (auto& col: row.second)
        {
            for (auto item: col.second.items)
            {
                items.insert(item);
            }
        }
    }
    return items.size();
}
int World::CountItems()
{
    return items.size();
}
std::vector<Item*> World::GetItems()
{
    std::vector<Item*> out;
    for (auto& item : items)
    {
        out.push_back(item.get());
    }
    return out;
}

void World::QueryRect(std::set<Item*>& items, const Rectangle& rect, FilterSingle filter)
{
    auto c = grid.ToCellRect(rect);

    std::set<Item*> dictItemsInCellRect;
    GetDictItemsInCellRect(dictItemsInCellRect, c);

    for (auto& item: dictItemsInCellRect)
    {
        auto rectOther = item->rect;

        if ( (not filter or filter(item)) and rect.IsIntersecting(rectOther))
        {
            items.insert(item);
        }
    }
}

void World::QueryPoint(std::set<Item*>&  items, math::vec2 point, FilterSingle filter)
{
    auto c = ToCell(point);
    std::set<Item*> dictItemsInCellRect;
    GetDictItemsInCellRect(dictItemsInCellRect, Rectangle(c, math::vec2(1,1)));

    Rectangle rect;
    for (auto item : dictItemsInCellRect)
    {
        rect = item->rect;
        if ( (not filter or filter(item)) and rect.ContainsPoint(point))
        {
            items.insert(item);
        }
    }
}
void World::QuerySegment(std::set<Item*>& items, math::vec2 top, math::vec2 bottom, FilterSingle filter)
{
    std::vector<ItemInfo> itemInfo;

    GetInfoAboutItemsTouchedBySegment(itemInfo, top, bottom, filter);
    for (auto& info: itemInfo)
    {
        items.insert(info.item);
    }
}
void World::QuerySegmentWithCoords(std::vector<ItemInfo>& items, math::vec2 top, math::vec2 bottom, FilterSingle filter)
{

    std::vector<ItemInfo> itemInfo;
    GetInfoAboutItemsTouchedBySegment(itemInfo, top, bottom, filter);

    auto delta = bottom - top;

    float ti1;
    float ti2;

    for (auto& info : itemInfo)
    {
        ti1 = info.ti1;
        ti2 = info.ti2;

        info.weight = 0;
        info.top = top + delta * ti1;
        info.bottom = bottom + delta * ti2;
    }
}

Item* World::Add(util::UserData data, Rectangle rect)
{
    if (HasItem(data.GetRaw()))
    {
        return nullptr;
    }

    auto& item = items.emplace_back(std::make_unique<Item>(data, rect));
    auto cellRect = grid.ToCellRect(rect);

    for (int cy=cellRect.pos.y; cy < cellRect.pos.y + cellRect.scale.y; cy++)
    {
        for (int cx=cellRect.pos.x; cx < cellRect.pos.x + cellRect.scale.x; cx++)
        {
            AddItemToCell(item.get(), cx,cy);
        }
    }
    return item.get();
}

bool World::Remove(void* itemPtr)
{
    if (itemPtr == nullptr){return false;} //sanity check
    
    Item* worldItem=nullptr;
    for(auto& candidate: items)
    {
        if (candidate->UserData.GetRaw() == itemPtr)
        {
            worldItem = candidate.get();
            break;
        }
    }
    if (worldItem==nullptr){return false;}
    auto cellRect = grid.ToCellRect(worldItem->rect);
    for (int cy=cellRect.pos.y; cy < cellRect.pos.y + cellRect.scale.y; cy++)
    {
        for (int cx=cellRect.pos.x; cx < cellRect.pos.x + cellRect.scale.x; cx++)
        {
            RemoveItemFromCell(worldItem, cx,cy);
        }
    }
    return removeIf(items, [itemPtr](auto& candidate){
        if (candidate->UserData.GetRaw() == itemPtr)
        {            
            return true;
        }
        return false;
    });
}
bool World::Remove(Item* item)
{
    if (item == nullptr){return false;} //sanity check
    return Remove(item->UserData.GetRaw());
}

void World::Update(Item* item, const Rectangle& rectOther)
{
    auto rect = item->rect;

    if (rect != rectOther)
    {

        auto cellRect = grid.ToCellRect(rect);
        auto cellRectOther = grid.ToCellRect(rectOther);
        if (cellRect != cellRectOther)
        {
            float cr1 = cellRect.pos.x + cellRect.scale.x - 1;
            float cb1 = cellRect.pos.y + cellRect.scale.y - 1;

            float cr2 = cellRectOther.pos.x + cellRectOther.scale.x - 1;
            float cb2 = cellRectOther.pos.y + cellRectOther.scale.y - 1;

            bool cyOut;

            for (int cy = cellRect.pos.y; cy <= cb1; cy++)
            {
                cyOut = cy < cellRectOther.pos.y or cy > cb2;

                for (int cx = cellRect.pos.x; cx <= cr1; cx++)
                {
                    if (cyOut or cx < cellRectOther.pos.x or cx > cr2)
                    {
                        RemoveItemFromCell(item, cx,cy);
                    }
                }
            }


            for (int cy = cellRectOther.pos.y; cy <= cb2; cy++)
            {
                cyOut = cy < cellRect.pos.y or cy > cb1;

                for (int cx = cellRectOther.pos.x; cx <= cr2; cx++)
                {
                    if (cyOut or cx < cellRectOther.pos.x or cx > cr2)
                    {
                        AddItemToCell(item, cx,cy);
                    }
                }
            }
        }

        item->rect = Rectangle(rectOther);
    }
}

void World::Move(CollisionResolution& finalRes, Item* item, math::vec2 goal,Filter filter)
{
    Check(finalRes, item, goal, filter);
    auto rect = item->GetRectangle();
    rect.pos = finalRes.pos;
    Update(item,rect);

}
void World::Check(CollisionResolution& finalRes, Item* item, math::vec2 goal, Filter filter)
{
    std::set<Item*> visited;
    visited.insert(item);

    auto visitedFilter = [filter,&visited](auto item, auto other, auto response){
        if (visited.find(other) != visited.end())
        {
            return false;
        }
        else
        {
            return filter(item, other, response);
        }
    };

    auto rect = item->rect;

    Collisions projectedCollisions;
    Project(projectedCollisions,item, rect, goal, filter);

    int projected_len = projectedCollisions.size();


    Collisions collisions;
    finalRes.pos = goal;

    while(projected_len > 0)
    {
        auto col = projectedCollisions[0];
        collisions.push_back(col);
        visited.insert(col->other);

        CollisionResolution resolution;
        col->response->Resolve(resolution, col.get(), rect, goal, visitedFilter);
        projectedCollisions = resolution.collisions;
        projected_len = projectedCollisions.size();
        finalRes.pos = resolution.pos;
        goal = resolution.pos;

    }
    finalRes.collisions = collisions;

}


void World::AddItemToCell(Item* item, int cx,int cy)
{

    if (rows.find(cy) == rows.end())
    {
        rows[cy] = {};
    }

    auto& row = rows[cy];
    if (row.find(cx) == row.end())
    {
        row[cx] = Cell();
    }

    auto& cell = row[cx];
    if (cell.items.find(item) == cell.items.end())
    {
        cell.items.insert(item);
    }
}

bool World::RemoveItemFromCell(Item* item, int cx, int cy)
{
    if (rows.find(cy) == rows.end())
    {
        return false;
    }
    else
    {
        auto& row = rows[cy];

        if (row.find(cx) == row.end())
        {
            return false;
        }

        auto& cell = row[cx];

        auto itemEntry = cell.items.find(item);
        if (itemEntry != cell.items.end())
        {
            cell.items.erase(itemEntry);
            //You need to clean the cells here, eg remove the entry both in row and col.
            if (cell.items.size() == 0)
            {
                row.erase(cx);
                if (row.size() == 0)
                {
                    rows.erase(cy);
                }
            }
            return true;
        }
        else
        {
            return false;
        }
    }


}
void World::GetDictItemsInCellRect(std::set<Item*>& items, const Rectangle& rect)
{
    int cl = rect.pos.x;
    int ct = rect.pos.y;
    int cw = rect.scale.x;
    int ch = rect.scale.y;

    for (int cy=ct;cy < ct + ch; cy++)
    {
        auto row = rows.find(cy);
        if (row != rows.end())
        {
            for (int cx=cl; cx < cl + cw; cx++)
            {
                auto rRow = (*row).second;
                if (rRow.find(cx) != rRow.end())
                {
                    auto& cell = rRow[cx];
                    if (cell.items.size() > 0)
                    {
                        for (auto item: cell.items)
                        {
                            items.insert(item);
                        }
                    }
                }
            }
        }
    }
}

void World::GetCellsTouchedBySegment(std::map<int, Cell*>& cells, math::vec2 top,math::vec2 bottom)
{
    std::set<Cell*> visited;

    int cellsLen =0;
    grid.Traverse(top, bottom, [&cells, &cellsLen, &visited, this](int x, int y)
    {
        auto row = rows.find(y);
        if (row == rows.end()){return;}

        auto rRow = &(*row).second;
        auto cell = rRow->find(y);

        if (cell == rRow->end() or (visited.find(&(*cell).second) != visited.end()))
        {
            return;
        }
        auto rCell = &(*cell).second;
        visited.insert(rCell);
        cellsLen++;
        cells[cellsLen] = rCell;
    });
}
void World::GetInfoAboutItemsTouchedBySegment(std::vector<ItemInfo>& itemInfo, math::vec2 top,math::vec2 bottom, FilterSingle filter)
{
    std::map<int, Cell*> cells;
    GetCellsTouchedBySegment(cells,top, bottom);

    std::set<Item*> visited;



    for(auto entry: cells)
    {
        auto cell = entry.second;
        for (auto item : cell->items)
        {
            if (item)
            {
                if (visited.find(item) == visited.end())
                {
                    visited.insert(item);
                    if (not filter or filter(item))
                    {

                        IntersectionIndicie indicie;
                        indicie.ti1 = 0;
                        indicie.ti2 = 1;
                        if ( item->rect.GetSegmentIntersectionIndices(top, bottom, indicie)
                        and ((0.0f < indicie.ti1 and indicie.ti1 < 1.0f) or (0.0f < indicie.ti2 and indicie.ti2 < 1.0f))
                        )
                        {
                            IntersectionIndicie indicie2;
                            indicie2.ti1 = -std::numeric_limits<float>::max();
                            indicie2.ti2 = std::numeric_limits<float>::max();
                            //the sorting is according to the t of an infinite line, not the segment
                            item->rect.GetSegmentIntersectionIndices( top, bottom, indicie2);


                            ItemInfo info;
                            info.item = item;
                            info.ti1 = indicie.ti1;
                            info.ti2 = indicie.ti2;
                            info.top = top;
                            info.bottom = bottom;
                            info.weight = std::min(indicie2.ti1, indicie2.ti2);
                            itemInfo.push_back(info);

                        }

                    }
                }
            }
        }
    }
    std::sort(itemInfo.begin(), itemInfo.end(), [](const ItemInfo& a, const ItemInfo& b){
        return a.weight < b.weight;
    });
}
