#pragma once
#include "Config.h"

// Practical-style physics ball: rotation + thrust, friction, wall bounce, elastic collision.
class Ball {
public:
    explicit Ball(float ballMass = BALL_1_MASS);

    void SetTexture(LPDIRECT3DTEXTURE9 texture) { icon = texture; }
    float GetMass() const { return mass; }

    void Reset(float spawnX, float spawnY);
    void BeginFrame();
    void SetThrustActive(bool active);
    void SetRotateLeft(bool active);
    void SetRotateRight(bool active);
    bool IntegrateStep();
    void Render(LPD3DXSPRITE sprite) const;
    float GetCenterX() const { return position.x; }

    static bool ResolvePairCollision(Ball& a, Ball& b, float* outCollisionX = nullptr);

private:
    D3DXVECTOR2 position;
    D3DXVECTOR2 velocity;
    D3DXVECTOR2 engineForce;
    float rotation;
    float mass;
    float collisionRadius;
    float displaySize;
    LPDIRECT3DTEXTURE9 icon;

    bool ResolveWallCollision();
};

bool LoadBallTexture(LPDIRECT3DTEXTURE9* outTexture, const char* path);
void CleanUpBallTexture(LPDIRECT3DTEXTURE9& texture);
