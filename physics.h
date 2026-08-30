#pragma once
#include <d3dx9.h>
#include "Collision.h"
#include "Config.h"

// =============================================================================
// Physics module (BMCS2224)
// OO force-based integration + advanced collision helpers used in battle.
// Techniques: gravity/force/acceleration/velocity, Euler integrate,
//             swept AABB (frame-miss), OBB (non-axis-aligned), overlap resolve.
// =============================================================================

// Rigid-body state driven by F = m * a.
struct PhysicsBody {
    D3DXVECTOR3 position;
    D3DXVECTOR3 velocity;
    D3DXVECTOR3 acceleration;
    D3DXVECTOR3 force;
    float mass;

    PhysicsBody()
        : position(0, 0, 0)
        , velocity(0, 0, 0)
        , acceleration(0, 0, 0)
        , force(0, 0, 0)
        , mass(1.0f) {
    }

    void ClearForce() { force = D3DXVECTOR3(0, 0, 0); }

    void ApplyForce(const D3DXVECTOR3& appliedForce) { force += appliedForce; }

    // F = m * g  (screen Y increases downward in this project).
    void ApplyGravity(float gravityAccelerationY) {
        ApplyForce(D3DXVECTOR3(0.0f, mass * gravityAccelerationY, 0.0f));
    }

    // Semi-implicit Euler: a = F/m, v += a*dt, p += v*dt.
    void Integrate(float deltaTime) {
        if (mass <= 0.0f || deltaTime <= 0.0f) return;
        acceleration = force * (1.0f / mass);
        velocity += acceleration * deltaTime;
        position += velocity * deltaTime;
        ClearForce();
    }

    void SetVerticalVelocity(float verticalVelocity) { velocity.y = verticalVelocity; }
    float GetVerticalVelocity() const { return velocity.y; }
};

// Facade for reusable physics operations (keeps fighters thin / maintainable).
class PhysicsWorld {
public:
    // One gameplay tick of gravity for a body standing on groundY.
    // Returns true if the body landed this tick.
    static bool IntegrateGravityOnGround(
        PhysicsBody& body,
        float groundY,
        float gravityAccelerationY,
        float deltaTime)
    {
        const bool onGround = body.position.y >= groundY - GROUND_CONTACT_EPSILON;
        if (onGround && body.velocity.y >= 0.0f) {
            body.position.y = groundY;
            body.velocity.y = 0.0f;
            body.ClearForce();
            return true;
        }

        body.ApplyGravity(gravityAccelerationY);
        body.Integrate(deltaTime);

        if (body.position.y >= groundY) {
            body.position.y = groundY;
            body.velocity.y = 0.0f;
            return true;
        }
        return false;
    }

    // Advanced: swept AABB test for fast movers (solves frame-miss tunneling).
    static bool DetectSweptCollision(const AABB& moving, const AABB& solid, float velocityX, float velocityY) {
        return CollisionHelper::SweptAABBIntersects(moving, solid, velocityX, velocityY);
    }

    // Advanced: non-axis-aligned OBB overlap.
    static bool DetectOrientedCollision(const OBB& a, const OBB& b) {
        return CollisionHelper::OBBIntersect(a, b);
    }

    // Advanced: resolve AABB overlap along shallowest axis.
    static bool ResolveOverlap(AABB& moving, const AABB& solid, float& outPushX, float& outPushY) {
        return CollisionHelper::ResolveAABBOverlap(moving, solid, outPushX, outPushY);
    }
};
