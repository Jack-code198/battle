#pragma once
#include <vector>

// High-level screens pushed/popped like a stack (game over retry / back).
enum class AppScreen {
    MainMenu,
    PlayerSelect,
    StageSelect,
    Battle,
    GameOver
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

    AppScreen Current() const {
        if (screens.empty()) return AppScreen::MainMenu;
        return screens.back();
    }

    bool Empty() const { return screens.empty(); }
    size_t Size() const { return screens.size(); }

private:
    std::vector<AppScreen> screens;
};
