#pragma once
#include <vector>

class GameState {
public:
    bool running{true};
    bool quitByUser{false};
    bool paused{false};
    bool ghostEnabled{true};

    int score{0};
    int level{1};
    int linesCleared{0};

    std::vector<int> highScores;
};
