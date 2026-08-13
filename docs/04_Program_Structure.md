# Program Structure (BMCS2224)

## Folder / Module Map

| Path | Role |
|------|------|
| `main.cpp` | Entry (`int main`), game loop, `GameStateStack` handling |
| `game_logic.cpp/.h` | Fighter factory, opponents, body overlap resolve |
| `physics.h` | `PhysicsBody`, `PhysicsWorld` (force / gravity / advanced collision) |
| `collision.h` | AABB / OBB / swept / overlap primitives |
| `input.cpp/.h` | Window + OO `InputManager` (DirectInput) |
| `renderer.cpp/.h` | Direct3D device, present, battle draw |
| `audio.cpp/.h` | FMOD `SoundManager` |
| `ui.cpp/.h` | HUD (HP / SP / Stamina), `Font` helpers |
| `FontRenderer.cpp/.h` | Reusable `ID3DXFont` wrapper |
| `GameStateStack.h` | Screen stack (menu / battle / game over) |
| `FrameTimer.cpp/.h` | Fixed-step animation timing |
| `config.h` | Named constants (no magic sprite sizes) |
| `menuMain.cpp` / `playerSelect.cpp` / `stageSelect.cpp` | Menu screens |
| `player/Fighter.*` | Abstract OO fighter base |
| `player/makoto/` | Makoto assets + logic |
| `player/joker/` | Joker + Arsene / Mona |
| `player/narukami/` | Narukami + Izanagi |
| `assets/` | Sprites, fonts, music, cursors |
| `docs/` | Assignment Game Doc (this folder) |

## Design Principles (Rubric + “Should NOT do”)
1. **OO:** behaviour lives in classes (`Fighter`, `InputManager`, `SoundManager`, `PhysicsWorld`).  
2. **Named constants:** gameplay numbers live in `config.h`.  
3. **Formula RECTs:** `frame % cols`, `frame / cols`, `cellSize` from loaded texture — not hand-typed per frame.  
4. **Reuse:** one gravity path, one body push, one HUD drawer for all characters.
