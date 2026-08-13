# Personal Learning Review (BMCS2224)

> Each team member should copy this template into their own short review (or write separate files). Aim for **clear, reflective writing** — not too short.

## What I Learned
- Object-oriented game structure: a shared `Fighter` base with character-specific overrides kept battle code reusable.  
- DirectX 9 sprite sheets work best when RECT frames are **calculated from texture size / cell size / frame index**, instead of hardcoding.  
- Physics is clearer when written as **force → acceleration → velocity → position** (`PhysicsBody`) rather than scattering `+= GRAVITY` everywhere.  
- DirectInput is easier to maintain behind an `InputManager` class.  
- FMOD needs careful init / update / shutdown order with the game loop.

## Challenges
- Keeping three large character state machines consistent (intro, damage, stamina costs, persona pairing).  
- Body collision: push must run **after both fighters update**, otherwise the second mover can walk through.  
- Small packed sprites (e.g. Mona taunt) need correct feet anchors measured from the art, not guessed.

## How Rubric Topics Map to My Work
| Rubric area | My contribution / evidence |
|-------------|----------------------------|
| Framework | Fighter hierarchy / factory |
| 2D rendering | Sprite sheets + formula RECTs |
| Input | DirectInput via `InputManager` |
| Physics | Gravity integrate + body overlap |
| Sound | Menu / battle music with FMOD |
| Clarity | Named constants in `config.h` |

## Future Improvements
- Simple AI for Player 2 instead of a sandbag.  
- More SFX on hits / skills.  
- Further comment pass on large character files.

*(Write 1–2 pages equivalent in your final Word/PDF submission if the lecturer requires a formal document.)*
