#pragma once
#include <vector>

// =============================================================================
// Game state stack (BMCS2224 framework)
// Screens are pushed/popped like a stack: MainMenu -> PlayerSelect -> Battle ->
// Pause (overlay) -> GameOver. Pop returns to the previous screen without rewiring
// navigation in main.cpp for every transition. RetryFromGameOver pops GameOver
// and resumes Battle; ReturnToMainMenu clears the stack to the menu root.
// =============================================================================

enum class AppScreen {
    MainMenu,
    Options,
    PlayerSelect,
    StageSelect,
    Battle,
    Pause,
    MoveList,
    GameOver,
    Credits,
    MiniGame
};

class GameStateStack {
public:
    void Clear() { screens.clear(); }

    void Push(AppScreen screen) {
        screens.push_back(screen);
    }

    void Pop() {
        if (!screens.empty()) {
            screens.pop_back();
        }
    }

    // Push GameOver on top of Battle when a round ends (R = retry, ESC = menu).
    void ExecuteGameOver() {
        if (Current() != AppScreen::GameOver) {
            Push(AppScreen::GameOver);
        }
    }

    void RetryFromGameOver() {
        if (Current() == AppScreen::GameOver) {
            Pop(); // remove GameOver -> return to Battle
        }
        if (Current() != AppScreen::Battle) {
            Push(AppScreen::Battle);
        }
    }

    void ReturnToMainMenu() {
        Clear();
        Push(AppScreen::MainMenu);
    }

    // After round fade-out: pop Battle / StageSelect / GameOver, land on PlayerSelect.
    void ReturnToPlayerSelectAfterBattle() {
        ReturnToPlayerSelect();
    }

    // Pause / game-over shortcut: drop back to character select (keep MainMenu underneath).
    void ReturnToPlayerSelect() {
        while (!Empty()) {
            const AppScreen top = Current();
            if (top == AppScreen::PlayerSelect) {
                return;
            }
            if (top == AppScreen::MainMenu) {
                Push(AppScreen::PlayerSelect);
                return;
            }
            Pop();
        }
        Push(AppScreen::MainMenu);
        Push(AppScreen::PlayerSelect);
    }

    AppScreen Current() const {
        if (screens.empty()) return AppScreen::MainMenu;
        return screens.back();
    }

    bool Empty() const { return screens.empty(); }
    size_t Size() const { return screens.size(); }

private:
    std::vector<AppScreen> screens;
};
