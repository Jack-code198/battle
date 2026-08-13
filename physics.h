#pragma once
#include <d3dx9.h>

// Physics primitives (BMCS2224): gravity, velocity, acceleration, force.
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

    void ApplyForce(const D3DXVECTOR3& f) { force += f; }

    void ApplyGravity(float gravityY) {
        // F = m * g (down positive in this project's screen Y).
        ApplyForce(D3DXVECTOR3(0.0f, mass * gravityY, 0.0f));
    }

    void Integrate(float dt) {
        if (mass <= 0.0f || dt <= 0.0f) return;
        // a = F / m
        acceleration = force / mass;
        // v += a * dt
        velocity += acceleration * dt;
        // p += v * dt
        position += velocity * dt;
        ClearForce();
    }
};
