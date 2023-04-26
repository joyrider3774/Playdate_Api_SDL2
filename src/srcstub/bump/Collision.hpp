#pragma once
#include <plugin/math/vec2.hpp>
#include "Rectangle.hpp"
#include<functional>
#include <typeinfo>
#include <memory>


namespace plugin::physics::bump
{
    class CollisionResolution;
    class CollisionResponse;
    class World;
    class Item;

    typedef std::function<bool(Item*, Item*, Collision*)> Filter;
    typedef std::function<bool(Item* a)> FilterSingle;
    typedef std::vector<std::shared_ptr<Collision>> Collisions;

    class CollisionResponse
    {
        friend class World;

    public:
        CollisionResponse(){};
        virtual void Resolve(
            CollisionResolution& resolution,
            Collision* col,
            const Rectangle& rect,
            math::vec2 goal,
            Filter filter
        ){};

        void Resolve(
            CollisionResolution& resolution,
            Collision* col,
            const Rectangle& rect,
            math::vec2 goal)
        {
            return Resolve(resolution,col,rect,goal, nullptr);
        }

        void SetWorld(World* world){this->world = world;}
    protected:
        World* world=nullptr;

    };

    class Collision
    {
    public:
        friend class World;

        Collision(){}

        template<typename T>
        bool Is()
        {
            if (dynamic_cast<T*>(response.get()))
            {
                return true;
            }
            return false;
        }

        template<typename T>
        void Respond(){
            response = std::make_shared<T>();
        }

        CollisionResponse* Response()
        {
            return response.get();
        }

    public:
        bool overlaps;
        float ti;
        math::vec2 move;
        math::vec2 normal;
        math::vec2 touch;
        Rectangle itemRect;
        Rectangle otherRect;

        Item* item=nullptr;
        Item* other=nullptr;

    protected:
        std::shared_ptr<CollisionResponse> response;


    };



    class CollisionResolution
    {
    public:
        CollisionResolution(math::vec2 pos, Collisions cols)
        {
            this->pos = pos;
            this->collisions = cols;
        }
        CollisionResolution()
        {
            this->pos = math::vec2(0,0);
        }
        math::vec2 pos;
        Collisions collisions;
    };
}
