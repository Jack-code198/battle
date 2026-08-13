#pragma once
#include "config.h"
#include "FrameTimer.h"
#include "player/Fighter.h"
#include "player/CharacterId.h"

// Shared battle objects: frame timer and the two fighter slots.

extern FrameTimer g_GameTimer;
extern Fighter* g_Player1;
extern Fighter* g_Player2;
extern bool g_ShowDebugHitboxes;

extern CharacterId g_SelectedP1;
extern CharacterId g_SelectedP2;

Fighter* CreateFighter(CharacterId id, int slot, bool humanControlled);
void DestroyFighters();
void SetupBattleFighters(CharacterId p1Id, CharacterId p2Id);

// Opposite fighter for collision / persona targeting.
Fighter* GetOpponent(const Fighter& self);

// Body push: keep fighters from walking through each other (all characters).
void ResolveFighterBodyOverlap(Fighter& a, Fighter& b);
