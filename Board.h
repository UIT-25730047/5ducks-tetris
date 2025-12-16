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

// Global board dimensions.
constexpr int BOARD_HEIGHT    = 20;
constexpr int BOARD_WIDTH     = 15;

// \brief Holds the playfield grid and handles drawing/line clearing.
class Board {
public:
    // 2D char grid representing the playfield.
    // ' '  : empty
    // I,O,T,S,Z,J,L : locked blocks
    // '.'  : ghost piece
    // '#'  : used for game\-over animation
    char grid[BOARD_HEIGHT][BOARD_WIDTH]{};

    // \brief Reset the board to all empty spaces.
    void init();

    // \brief Render the board and right\-side panel to the terminal.
    // \param state           Current game state (score, level, etc.).
    // \param nextPieceLines  Pre\-rendered 4 lines preview of the next piece.
    void draw(const GameState& state,
              const std::string nextPieceLines[4]) const;

    // \brief Clear any fully filled rows and compact the board.
    // \return Number of lines cleared.
    int clearLines();
};

// Helper that maps a block character to a color code.
const char* getColorForPiece(char cell);
