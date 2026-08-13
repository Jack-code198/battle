# Persona Framework — Game Overview (BMCS2224)

## Title
**Persona Framework** — a 2D side-view fighting / training game built with DirectX 9, DirectInput, and FMOD.

## Genre & Goal
Players pick a Phantom Thief / Persona series fighter, choose a stage, and battle a sandbag opponent. The goal of a match is to reduce the opponent’s HP to zero. The project emphasises an object-oriented game framework: reusable fighter base class, modular render / input / physics / sound systems, and a stack-based screen flow.

## Core Gameplay
- **Roster:** Makoto Yuki, Joker (with Arsene / Mona), Yu Narukami (with Izanagi)
- **Controls:** keyboard + mouse (movement, jumps, melee, persona skills, stamina moves)
- **Resources:** HP, SP (persona skills), Stamina (3 bars — dash / heavy moves / sprint drain + regen)
- **Modes:** Main Menu → Player Select → Stage Select → Battle → Game Over (Retry / Main Menu)

## Technical Stack
| Module | Technology |
|--------|------------|
| Framework | C++20, OO (`Fighter` hierarchy, factories) |
| Rendering | Direct3D 9 + `ID3DXSprite` (formula-based sprite RECTs) |
| Input | DirectInput 8 (`InputManager` class) |
| Physics | `PhysicsBody` / `PhysicsWorld` (force, gravity, integrate, swept AABB, OBB, overlap resolve) |
| Sound | FMOD (`SoundManager`) |
| Font | `FontRenderer` (`ID3DXFont`) |
| Game state | `GameStateStack` (push/pop, Game Over retry) |

## What Makes This Submission Distinct
1. Three fully animated fighters with persona pairing and skill VFX on the opponent  
2. Shared stamina system under the HUD SP bar  
3. Body collision push for all characters (cannot walk through opponents)  
4. Clear separation of modules matching the BMCS2224 rubric (Framework, 2D, Input, Physics, Sound)

## Document Map
| Doc | File |
|-----|------|
| Overview | `docs/01_Overview.md` (this file) |
| Class Diagram | `docs/02_Class_Diagram.md` |
| Program Flow | `docs/03_Program_Flow.md` |
| Program Structure | `docs/04_Program_Structure.md` |
| References | `docs/05_References.md` |
| Personal Learning Review | `docs/06_Personal_Learning_Review.md` |
