# Class Diagram (BMCS2224)

```mermaid
classDiagram
    class Fighter {
        <<abstract>>
        +position
        +health / sp / stamina
        #physicsBody : PhysicsBody
        +Update()*
        +Render()*
        +TakeDamage()*
        +Reset()*
        +GetBodyCollisionBox()
        #ApplyPhysicsGravitySteps()
    }

    class Makoto
    class Joker
    class Narukami
    Fighter <|-- Makoto
    Fighter <|-- Joker
    Fighter <|-- Narukami

    class PhysicsBody {
        +position / velocity / acceleration / force
        +mass
        +ApplyForce()
        +ApplyGravity()
        +Integrate()
    }
    class PhysicsWorld {
        <<utility>>
        +IntegrateGravityOnGround()
        +DetectSweptCollision()
        +DetectOrientedCollision()
        +ResolveOverlap()
    }
    Fighter *-- PhysicsBody
    PhysicsWorld ..> PhysicsBody
    PhysicsWorld ..> CollisionHelper

    class CollisionHelper {
        <<utility>>
        +AABBIntersect()
        +SweptAABBIntersects()
        +OBBIntersect()
        +ResolveAABBOverlap()
    }

    class InputManager {
        -directInput
        -keyboardDevice
        -keyState
        +Create()
        +Poll()
        +IsKeyDown()
        +Cleanup()
    }

    class SoundManager {
        +Initialise()
        +PlayMenuMusic()
        +PlayBattleMusic()
        +Update()
        +Shutdown()
    }

    class FontRenderer {
        -font : ID3DXFont
        +Create()
        +DrawTextA()
        +Release()
    }

    class GameStateStack {
        -screens : vector~AppScreen~
        +Push() / Pop()
        +ExecuteGameOver()
        +RetryFromGameOver()
        +Current()
    }

    class FrameTimer {
        +Init()
        +FramesToUpdate()
    }
```

## Design Notes
- **OO Framework:** all playable characters inherit `Fighter`; battle code talks to the base pointer (`g_Player1` / `g_Player2`).
- **Reuse:** sprite RECT formulas, physics gravity, body push, and HUD meters are shared.
- **Should NOT do compliance:** sprite RECTs are computed from sheet `cols/rows/cellSize` (no hardcoded per-frame RECT tables).
