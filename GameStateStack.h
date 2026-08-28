#pragma once
#include <vector>

// High-level screens pushed/popped like a stack (game over retry / back).
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

    // Game over → retry: pop GameOver and keep Battle, or replace top.
    void ExecuteGameOver() {
        if (Current() != AppScreen::GameOver) {
            Push(AppScreen::GameOver);
        }
    }

    void RetryFromGameOver() {
        if (Current() == AppScreen::GameOver) {
            Pop(); // remove GameOver → return to Battle
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
