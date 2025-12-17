#pragma once
#include <string>
#include "GameState.h"
#include "BlockTemplate.h"

// Terminal color escape sequences (ANSI).
extern const char* COLOR_RESET;
extern const char* COLOR_CYAN;
extern const char* COLOR_YELLOW;
extern const char* COLOR_PURPLE;
extern const char* COLOR_GREEN;
extern const char* COLOR_RED;
extern const char* COLOR_BLUE;
extern const char* COLOR_ORANGE;
extern const char* COLOR_WHITE;

// Piece color mapping array
extern const char* PIECE_COLORS[BlockTemplate::NUM_BLOCK_TYPES];

// Helper that maps a block character to a color code.
const char* getColorForPiece(char cell);

constexpr int BOARD_HEIGHT    = 20;
constexpr int BOARD_WIDTH     = 15;

class Board {
public:
    // 2D char grid representing the playfield
    char grid[BOARD_HEIGHT][BOARD_WIDTH]{};

    // Reset the board to all empty spaces
    void init();

    // Render the board and right-side panel to the terminal
    void draw(
        const GameState& state,
        const std::string nextPieceLines[4]
    ) const;
    void draw(
        const GameState& state,
        const std::string nextPieceLines[4]
    ) const;

    // Number of lines cleared.
    int clearLines();
};
