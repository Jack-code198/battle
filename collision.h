#pragma once
#include <cmath>

struct AABB {
    float x, y;
    float width, height;

    bool Intersects(const AABB& other) const {
        return x < other.x + other.width &&
            x + width > other.x &&
            y < other.y + other.height &&
            y + height > other.y;
    }
};

class CollisionHelper {
public:
    static bool AABBIntersect(const AABB& a, const AABB& b) {
        return a.x < b.x + b.width &&
            a.x + a.width > b.x &&
            a.y < b.y + b.height &&
            a.y + a.height > b.y;
    }

    static bool SweptAABBIntersects(const AABB& sourceBox, const AABB& targetBox, float vx, float vy) {
        if (AABBIntersect(sourceBox, targetBox)) return true;
        if (vx == 0.0f && vy == 0.0f) return false;

        AABB sweptBox = sourceBox;
        if (vx > 0) sweptBox.width = sourceBox.width + fabs(vx);
        else if (vx < 0) {
            sweptBox.x = sourceBox.x + vx;
            sweptBox.width = sourceBox.width + fabs(vx);
        }
        if (vy > 0) sweptBox.height = sourceBox.height + fabs(vy);
        else if (vy < 0) {
            sweptBox.y = sourceBox.y + vy;
            sweptBox.height = sourceBox.height + fabs(vy);
        }
        return AABBIntersect(sweptBox, targetBox);
    }
};
