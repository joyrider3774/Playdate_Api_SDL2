#pragma once
#include "Collision.hpp"

namespace plugin::physics::bump::response
{
    class Touch : public CollisionResponse
    {
    public:
        using CollisionResponse::CollisionResponse;

        void Resolve(
            CollisionResolution& resolution,
            Collision* col,
            const Rectangle& rect,
            math::vec2 goal,
            Filter filter
        ) override;
    };
    class Cross : public CollisionResponse
    {
    public:
        using CollisionResponse::CollisionResponse;

        void Resolve(
            CollisionResolution& resolution,
            Collision* col,
            const Rectangle& rect,
            math::vec2 goal,
            Filter filter
        ) override;

    };

    class Slide : public CollisionResponse
    {
    public:
        using CollisionResponse::CollisionResponse;

        void Resolve(
            CollisionResolution& resolution,
            Collision* col,
            const Rectangle& rect,
            math::vec2 goal,
            Filter filter
        ) override;

    public:
        math::vec2 slide;

    };

    class Bounce : public CollisionResponse
    {
    public:
        using CollisionResponse::CollisionResponse;

        void Resolve(
            CollisionResolution& resolution,
            Collision* col,
            const Rectangle& rect,
            math::vec2 goal,
            Filter filter
        ) override;
    public:
        math::vec2 bounce;

    };

    class DefaultCollisionResponse : public Slide
    {
        using Slide::Slide;
    };

}
