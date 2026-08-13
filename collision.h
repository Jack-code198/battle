#pragma once
#include <cmath>

// Physics / collision helpers (BMCS2224).
// Axis-aligned boxes used for hurtboxes, hitboxes, and body push resolution.
// Also covers non-axis-aligned OBB tests, swept AABB (frame-miss), and overlap response.

struct AABB {
    float x, y;
    float width, height;

    bool Intersects(const AABB& other) const {
        return x < other.x + other.width &&
            x + width > other.x &&
            y < other.y + other.height &&
            y + height > other.y;
    }

    float CenterX() const { return x + width * 0.5f; }
    float CenterY() const { return y + height * 0.5f; }
};

// Non-axis-aligned oriented bounding box (rotation around center).
struct OBB {
    float centerX;
    float centerY;
    float halfW;
    float halfH;
    float rotationRad;
};

class CollisionHelper {
public:
    // --- Collision detection (AABB) ---
    static bool AABBIntersect(const AABB& a, const AABB& b) {
        return a.x < b.x + b.width &&
            a.x + a.width > b.x &&
            a.y < b.y + b.height &&
            a.y + a.height > b.y;
    }

    // Frame-miss issue: fast movers can tunnel through thin boxes between frames.
    // SweptAABB expands the source by velocity so we still detect the pass-through.
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

    // --- Non-axis-aligned (OBB vs OBB via separating axis theorem) ---
    static bool OBBIntersect(const OBB& a, const OBB& b) {
        const float cosA = cosf(a.rotationRad);
        const float sinA = sinf(a.rotationRad);
        const float cosB = cosf(b.rotationRad);
        const float sinB = sinf(b.rotationRad);

        // Axes: A's local X/Y and B's local X/Y.
        float axes[4][2] = {
            { cosA, sinA },
            { -sinA, cosA },
            { cosB, sinB },
            { -sinB, cosB }
        };

        for (int i = 0; i < 4; ++i) {
            float minA = 0.0f, maxA = 0.0f, minB = 0.0f, maxB = 0.0f;
            ProjectOBB(a, axes[i][0], axes[i][1], minA, maxA);
            ProjectOBB(b, axes[i][0], axes[i][1], minB, maxB);
            if (maxA < minB || maxB < minA) {
                return false;
            }
        }
        return true;
    }

    // --- Collision response / overlap resolution ---
    // Separates two overlapping AABBs along the shallowest axis (push A out of B).
    // Returns true if an overlap was resolved (addresses overlap resolution issue).
    static bool ResolveAABBOverlap(AABB& moving, const AABB& solid, float& outPushX, float& outPushY) {
        outPushX = 0.0f;
        outPushY = 0.0f;
        if (!AABBIntersect(moving, solid)) return false;

        const float overlapLeft = (moving.x + moving.width) - solid.x;
        const float overlapRight = (solid.x + solid.width) - moving.x;
        const float overlapTop = (moving.y + moving.height) - solid.y;
        const float overlapBottom = (solid.y + solid.height) - moving.y;

        const float pushX = (overlapLeft < overlapRight) ? -overlapLeft : overlapRight;
        const float pushY = (overlapTop < overlapBottom) ? -overlapTop : overlapBottom;

        if (fabs(pushX) < fabs(pushY)) {
            outPushX = pushX;
            moving.x += pushX;
        }
        else {
            outPushY = pushY;
            moving.y += pushY;
        }
        return true;
    }

private:
    static void ProjectOBB(const OBB& box, float axisX, float axisY, float& outMin, float& outMax) {
        const float cosR = cosf(box.rotationRad);
        const float sinR = sinf(box.rotationRad);
        const float corners[4][2] = {
            { -box.halfW, -box.halfH },
            {  box.halfW, -box.halfH },
            {  box.halfW,  box.halfH },
            { -box.halfW,  box.halfH }
        };

        outMin = 1e9f;
        outMax = -1e9f;
        for (int i = 0; i < 4; ++i) {
            const float wx = box.centerX + corners[i][0] * cosR - corners[i][1] * sinR;
            const float wy = box.centerY + corners[i][0] * sinR + corners[i][1] * cosR;
            const float proj = wx * axisX + wy * axisY;
            if (proj < outMin) outMin = proj;
            if (proj > outMax) outMax = proj;
        }
    }
};
