#pragma once
#include <vector>

// \brief Holds global game statistics and flags.
// This is a plain data holder used by TetrisGame and Board.
class GameState {
public:
    bool running{true};       // \true while the game loop is active
    bool quitByUser{false};   // \true if user pressed \`q\` to quit
    bool paused{false};       // \true if game is currently paused
    bool ghostEnabled{true};  // \true if ghost piece (shadow) is shown

    int score{0};             // Current score
    int level{1};             // Current level (affects speed)
    int linesCleared{0};      // Total lines cleared in this run

    std::vector<int> highScores; // Loaded high scores shown on game over
};
