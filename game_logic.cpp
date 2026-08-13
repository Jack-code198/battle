#include "game_logic.h"
#include "collision.h"
#include "physics.h"
#include "player/makoto/Makoto.h"
#include "player/joker/Joker.h"
#include "player/narukami/Narukami.h"

FrameTimer g_GameTimer;
Fighter* g_Player1 = nullptr;
Fighter* g_Player2 = nullptr;
bool g_ShowDebugHitboxes = false;

CharacterId g_SelectedP1 = Char_Makoto;
CharacterId g_SelectedP2 = Char_Joker;

Fighter* CreateFighter(CharacterId id, int slot, bool humanControlled) {
    Fighter* fighter = nullptr;
    switch (id) {
    case Char_Makoto:
        fighter = new Makoto();
        break;
    case Char_Joker:
        fighter = new Joker();
        break;
    case Char_Narukami:
        fighter = new Narukami();
        break;
    default:
        fighter = new Makoto();
        id = Char_Makoto;
        break;
    }

    fighter->SetCharacterId(id);
    fighter->SetPlayerSlot(slot);
    fighter->SetHumanControlled(humanControlled);
    fighter->Reset();
    return fighter;
}

void DestroyFighters() {
    delete g_Player1;
    delete g_Player2;
    g_Player1 = nullptr;
    g_Player2 = nullptr;
}

void SetupBattleFighters(CharacterId p1Id, CharacterId p2Id) {
    DestroyFighters();
    g_SelectedP1 = p1Id;
    g_SelectedP2 = p2Id;
    g_Player1 = CreateFighter(p1Id, 0, true);
    g_Player2 = CreateFighter(p2Id, 1, false);
}

Fighter* GetOpponent(const Fighter& self) {
    if (&self == g_Player1) return g_Player2;
    if (&self == g_Player2) return g_Player1;
    return g_Player2;
}

static void ClampFighterPushX(Fighter& fighter) {
    const AABB box = fighter.GetBodyCollisionBox();
    ClampFighterCenterX(fighter.position.x, box.width * 0.5f);
}

// Body push (BMCS2224 Physics / collision response).
// Side-view fighters separate on X only. Uses AABB detection from CollisionHelper /
// PhysicsWorld; sandbag stays put while the human is pushed out.
void ResolveFighterBodyOverlap(Fighter& a, Fighter& b) {
    const AABB boxA = a.GetBodyCollisionBox();
    const AABB boxB = b.GetBodyCollisionBox();
    if (!CollisionHelper::AABBIntersect(boxA, boxB)) return;

    const float overlapLeft = (boxA.x + boxA.width) - boxB.x;
    const float overlapRight = (boxB.x + boxB.width) - boxA.x;
    if (overlapLeft <= 0.0f && overlapRight <= 0.0f) return;

    float deltaA = 0.0f;
    float deltaB = 0.0f;
    if (overlapLeft > 0.0f && overlapLeft <= overlapRight) {
        deltaA = -overlapLeft;
        deltaB = overlapLeft;
    }
    else if (overlapRight > 0.0f) {
        deltaA = overlapRight;
        deltaB = -overlapRight;
    }

    // Also exercise PhysicsWorld overlap resolver (horizontal component).
    float resolvedPushX = 0.0f;
    float resolvedPushY = 0.0f;
    AABB movingProbe = boxA;
    if (PhysicsWorld::ResolveOverlap(movingProbe, boxB, resolvedPushX, resolvedPushY) &&
        fabsf(resolvedPushX) > fabsf(resolvedPushY) &&
        fabsf(resolvedPushX) > 0.001f) {
        deltaA = resolvedPushX;
        deltaB = -resolvedPushX;
    }

    const bool aHuman = a.IsHumanControlled();
    const bool bHuman = b.IsHumanControlled();

    if (!aHuman && bHuman) {
        b.position.x += deltaB;
        ClampFighterPushX(b);
    }
    else if (aHuman && !bHuman) {
        a.position.x += deltaA;
        ClampFighterPushX(a);
    }
    else {
        a.position.x += deltaA * 0.5f;
        b.position.x += deltaB * 0.5f;
        ClampFighterPushX(a);
        ClampFighterPushX(b);
    }

    a.UpdateScaledHurtbox();
    b.UpdateScaledHurtbox();
}
