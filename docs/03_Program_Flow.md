# Program Flow (BMCS2224)

## High-Level Screen Flow (GameStateStack)

```mermaid
flowchart TD
    A[int main] --> B[Init Window / D3D / InputManager / Assets / FMOD]
    B --> C[Push MainMenu]
    C --> D{Game Loop}
    D --> E[GetInput / InputManager.Poll]
    E --> F[Current AppScreen]
    F -->|MainMenu| G[Render menu]
    G -->|Start| H[Push PlayerSelect]
    F -->|PlayerSelect| I[Pick P1 + P2]
    I -->|Confirm| J[Push StageSelect]
    I -->|Back| C
    F -->|StageSelect| K[Pick stage]
    K -->|Confirm| L[SetupBattleFighters + Push Battle]
    K -->|Back| H
    F -->|Battle| M[P1.Update + P2.Update]
    M --> N[ResolveFighterBodyOverlap]
    N --> O[Render + HUD]
    O -->|Someone dead| P[Push GameOver]
    O -->|ESC| C
    F -->|GameOver| Q[FontRenderer text]
    Q -->|R Retry| L2[Reset fighters / Pop to Battle]
    Q -->|ESC| C
    D -->|WM_QUIT| R[Cleanup + exit]
```

## Battle Frame Flow
1. `InputManager::Poll` updates DirectInput key state  
2. Human fighter reads keys → state machine (walk / jump / attack / skill)  
3. `ApplyPhysicsGravitySteps` integrates force-based gravity when airborne  
4. Hitboxes / skill effects apply damage  
5. `ResolveFighterBodyOverlap` prevents walking through the opponent  
6. `Render` draws stage, fighters (formula sprite RECTs), HUD / fonts  

## Entry Point
`int main` in `main.cpp` (Windows subsystem entry `mainCRTStartup`) owns the game loop and state stack.
