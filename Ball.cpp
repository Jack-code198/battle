#include "Ball.h"
#include "renderer.h"
#include <cmath>

// =============================================================================
// Mini Game physics (BMCS2224) — force-based 2D motion.
// Each step: engineForce -> acceleration (F/m) -> velocity -> position.
// Thrust uses sin/cos(rotation); friction and wall/ball collision apply response.
// =============================================================================

Ball::Ball(float ballMass)
    : position((float)SCREEN_WIDTH * 0.5f, (float)SCREEN_HEIGHT * 0.5f)
    , velocity(0.0f, 0.0f)
    , engineForce(0.0f, 0.0f)
    , rotation(0.0f)
    , mass(ballMass)
    , collisionRadius(BALL_RADIUS)
    , displaySize(BALL_DISPLAY_SIZE)
    , icon(nullptr) {
    if (mass <= 0.0f) {
        mass = 1.0f;
    }
}

void Ball::Reset(float spawnX, float spawnY) {
    position = D3DXVECTOR2(spawnX, spawnY);
    velocity = D3DXVECTOR2(0.0f, 0.0f);
    engineForce = D3DXVECTOR2(0.0f, 0.0f);
    rotation = 0.0f;
}

void Ball::BeginFrame() {
    engineForce = D3DXVECTOR2(0.0f, 0.0f);
}

void Ball::SetThrustActive(bool active) {
    if (!active) {
        return;
    }
    engineForce.x = BALL_ENGINE_POWER * sinf(rotation);
    engineForce.y = BALL_ENGINE_POWER * -cosf(rotation);
}

void Ball::SetRotateLeft(bool active) {
    if (active) {
        rotation -= BALL_ROTATION_SPEED;
    }
}

void Ball::SetRotateRight(bool active) {
    if (active) {
        rotation += BALL_ROTATION_SPEED;
    }
}

bool Ball::ResolveWallCollision() {
    bool hit = false;
    const float minX = collisionRadius;
    const float minY = collisionRadius;
    const float maxX = (float)SCREEN_WIDTH - collisionRadius;
    const float maxY = (float)SCREEN_HEIGHT - collisionRadius;

    if (position.x < minX) {
        position.x = minX;
        if (velocity.x < 0.0f) {
            velocity.x *= -1.0f;
            hit = true;
        }
    }
    else if (position.x > maxX) {
        position.x = maxX;
        if (velocity.x > 0.0f) {
            velocity.x *= -1.0f;
            hit = true;
        }
    }

    if (position.y < minY) {
        position.y = minY;
        if (velocity.y < 0.0f) {
            velocity.y *= -1.0f;
            hit = true;
        }
    }
    else if (position.y > maxY) {
        position.y = maxY;
        if (velocity.y > 0.0f) {
            velocity.y *= -1.0f;
            hit = true;
        }
    }

    return hit;
}

bool Ball::IntegrateStep() {
    const D3DXVECTOR2 acceleration(engineForce.x / mass, engineForce.y / mass);
    velocity += acceleration;
    velocity *= BALL_FRICTION;
    position += velocity;
    return ResolveWallCollision();
}

bool Ball::ResolvePairCollision(Ball& a, Ball& b, float* outCollisionX) {
    D3DXVECTOR2 delta = b.position - a.position;
    const float minDist = a.collisionRadius + b.collisionRadius;
    const float distSq = D3DXVec2LengthSq(&delta);

    if (distSq >= minDist * minDist || distSq <= 0.001f) {
        return false;
    }

    const float dist = sqrtf(distSq);
    D3DXVECTOR2 normal = delta / dist;
    bool hit = false;

    D3DXVECTOR2 relVel = b.velocity - a.velocity;
    const float velAlongNormal = D3DXVec2Dot(&relVel, &normal);

    if (velAlongNormal < 0.0f) {
        const float invMassA = 1.0f / a.mass;
        const float invMassB = 1.0f / b.mass;
        const float totalInvMass = invMassA + invMassB;
        const float impulseScaler = -(1.0f + BALL_RESTITUTION) * velAlongNormal / totalInvMass;
        const D3DXVECTOR2 impulse = normal * impulseScaler;

        a.velocity -= impulse * invMassA;
        b.velocity += impulse * invMassB;
        hit = true;
    }

    const float overlap = minDist - dist;
    const float totalInvMass = (1.0f / a.mass) + (1.0f / b.mass);
    const D3DXVECTOR2 correction = normal * (overlap / totalInvMass);
    a.position -= correction * (1.0f / a.mass);
    b.position += correction * (1.0f / b.mass);

    if (outCollisionX) {
        *outCollisionX = (a.position.x + b.position.x) * 0.5f;
    }

    return hit;
}

void Ball::Render(LPD3DXSPRITE sprite) const {
    if (!sprite) {
        return;
    }

    const float drawX = position.x - displaySize * 0.5f;
    const float drawY = position.y - displaySize * 0.5f;

    if (icon) {
        D3DSURFACE_DESC desc;
        icon->GetLevelDesc(0, &desc);
        if (desc.Width == 0 || desc.Height == 0) {
            return;
        }

        const D3DXVECTOR2 scalingCenter(displaySize * 0.5f, displaySize * 0.5f);
        const D3DXVECTOR2 rotationCenter(displaySize * 0.5f, displaySize * 0.5f);
        const D3DXVECTOR2 translation(drawX, drawY);
        const D3DXVECTOR2 scaling(displaySize / (float)desc.Width, displaySize / (float)desc.Height);

        D3DXMATRIX matrix;
        D3DXMatrixTransformation2D(
            &matrix,
            &scalingCenter,
            0.0f,
            &scaling,
            &rotationCenter,
            rotation,
            &translation);

        sprite->SetTransform(&matrix);
        D3DXVECTOR3 zeroPos(0.0f, 0.0f, 0.0f);
        sprite->Draw(icon, NULL, NULL, &zeroPos, D3DCOLOR_XRGB(255, 255, 255));

        D3DXMATRIX identity;
        D3DXMatrixIdentity(&identity);
        sprite->SetTransform(&identity);
    }
    else {
        DrawDebugCircleRing(sprite, position.x, position.y, collisionRadius, BALL_FALLBACK_COLOR, 24);
    }
}

bool LoadBallTexture(LPDIRECT3DTEXTURE9* outTexture, const char* path) {
    if (!outTexture || !path) {
        return false;
    }
    if (*outTexture != nullptr) {
        return true;
    }

    HRESULT hr = D3DXCreateTextureFromFileEx(
        g_pD3DDevice,
        path,
        D3DX_DEFAULT_NONPOW2,
        D3DX_DEFAULT_NONPOW2,
        D3DX_DEFAULT,
        NULL,
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        D3DX_DEFAULT,
        D3DX_DEFAULT,
        0,
        NULL,
        NULL,
        outTexture);

    if (FAILED(hr)) {
        *outTexture = nullptr;
        return false;
    }
    return true;
}

void CleanUpBallTexture(LPDIRECT3DTEXTURE9& texture) {
    if (texture) {
        texture->Release();
        texture = nullptr;
    }
}
