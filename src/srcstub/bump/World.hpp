#pragma once
#include <plugin/util/UserData.hpp>
#include "Rectangle.hpp"
#include "Grid.hpp"
#include "Collision.hpp"
#include <set>
#include <map>

namespace plugin::physics::bump
{


    class Item
    {
    friend class World;

    public:
        Item(util::UserData data, Rectangle rect)
        {
            this->UserData = data;
            this->rect = rect;
        }
    public:
        util::UserData UserData;
        Rectangle GetRectangle(){return rect;}

    protected:
        Rectangle rect;
    };

    class Cell
    {
    public:
        Cell(){};
        std::set<Item*> items;
    };


    class World
    {

    private:
        struct ItemInfo
        {
            Item* item;
            float ti1;
            float ti2;
            math::vec2 top;
            math::vec2 bottom;
            float weight;
        };

    public:
        World() : World(64){};
        World(int cellsize);
        void Project(Collisions& collisions, Item* item, const Rectangle& rect, math::vec2 goal, Filter filter);
        void Project(Collisions& collisions, Item* item, const Rectangle& rect, math::vec2 goal)
        {
            Project(collisions, item, rect, goal, nullptr);
        };
        void Project(Collisions& collisions, Item* item, const Rectangle& rect)
        {
            Project(collisions, item, rect, rect.pos, nullptr);
        };

        int CountCells();
        int CountCellItems();
        int CountItems();

        template<typename T>
        bool HasItem(T* itemPtr)
        {
            for (auto& item: items)
            {
                if (item->UserData.GetRaw() == itemPtr)
                {
                    return true;
                }
            }
            return false;
        }

        template<typename T>
        bool HasItem(std::shared_ptr<T> itemPtr)
        {
            return HasItem(itemPtr.get());
        }


        std::vector<Item*> GetItems();
        math::vec2 ToWorld(const math::vec2& p){
            return grid.ToWorld(p);
        }
        math::vec2 ToCell(const math::vec2& p)
        {
            return grid.ToCell(p);
        }

        void QueryRect(std::set<Item*>& items,const Rectangle& rect, FilterSingle filter);
        void QueryRect(std::set<Item*>& items, const Rectangle& rect){return QueryRect(items, rect, nullptr);};
        void QueryPoint(std::set<Item*>& items, math::vec2 point, FilterSingle filter);
        void QueryPoint(std::set<Item*>& items, math::vec2 point){return QueryPoint(items, point, nullptr);};

        void QuerySegment(std::set<Item*>& items, math::vec2 top, math::vec2 bottom, FilterSingle filter);
        void QuerySegment(std::set<Item*>& items, math::vec2 top, math::vec2 bottom){QuerySegment(items, top,bottom, nullptr);};

        void QuerySegmentWithCoords(std::vector<ItemInfo>& items, math::vec2 top, math::vec2 bottom, FilterSingle filter);
        void QuerySegmentWithCoords(std::vector<ItemInfo>& items, math::vec2 top, math::vec2 bottom){QuerySegmentWithCoords(items, top, bottom, nullptr);};

        template<typename T>
        Item* Add(T* data, Rectangle rect){return Add(util::UserData(data), rect);}
        template<typename T>
        Item* Add(std::shared_ptr<T> data, Rectangle rect){return Add(util::UserData(data), rect);}
        Item* Add(util::UserData data, Rectangle rect);

        bool Remove(Item* item);
        bool Remove(void* itemPtr);

        void Update(Item* item, const Rectangle& rect);

        void Move(CollisionResolution& finalRes, Item* item, math::vec2 goal,Filter filter);
        void Move(CollisionResolution& finalRes, Item* item, math::vec2 goal){Move(finalRes, item, goal, nullptr);}

        void Check(CollisionResolution& finalRes, Item* item, math::vec2 goal,Filter filter);
        void Check(CollisionResolution& finalRes, Item* item, math::vec2 goal){Check(finalRes, item, goal, nullptr);};


    private:
        void AddItemToCell(Item* item, int x, int y);
        bool RemoveItemFromCell(Item* item, int x, int y);
        void GetDictItemsInCellRect(std::set<Item*>& items, const Rectangle& rect);
        void GetCellsTouchedBySegment(std::map<int, Cell*>& cells, math::vec2 top,math::vec2 bottom);
        void GetInfoAboutItemsTouchedBySegment(std::vector<ItemInfo>& info, math::vec2 top,math::vec2 bottom, FilterSingle filter);

    private:
        Grid grid;
        std::vector<std::unique_ptr<Item>> items;
        std::map<int, std::map<int, Cell>> rows;

    };
}
