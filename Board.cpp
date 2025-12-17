#include "Board.h"
#include "BlockTemplate.h"
#include <iostream>
#include <algorithm>

// Define color escape constants once here.
const char* COLOR_RESET  = "\033[0m";
const char* COLOR_CYAN   = "\033[36m";
const char* COLOR_YELLOW = "\033[33m";
const char* COLOR_PURPLE = "\033[35m";
const char* COLOR_GREEN  = "\033[32m";
const char* COLOR_RED    = "\033[31m";
const char* COLOR_BLUE   = "\033[34m";
const char* COLOR_ORANGE = "\033[38;5;208m";
const char* COLOR_WHITE  = "\033[37m";

const char* PIECE_COLORS[BlockTemplate::NUM_BLOCK_TYPES] = {
    COLOR_CYAN,   // I
    COLOR_YELLOW, // O
    COLOR_PURPLE, // T
    COLOR_GREEN,  // S
    COLOR_RED,    // Z
    COLOR_BLUE,   // J
    COLOR_ORANGE  // L
};

const char* getColorForPiece(char cell) {
    switch (cell) {
        case 'I': return PIECE_COLORS[0];
        case 'O': return PIECE_COLORS[1];
        case 'T': return PIECE_COLORS[2];
        case 'S': return PIECE_COLORS[3];
        case 'Z': return PIECE_COLORS[4];
        case 'J': return PIECE_COLORS[5];
        case 'L': return PIECE_COLORS[6];
        case '.': return COLOR_WHITE; // ghost
        case '#': return COLOR_WHITE; // game over animation
        default:  return COLOR_RESET;
    }
}

void Board::init() {
    // Fill all cells with spaces (empty).
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            grid[y][x] = ' ';
        }
    }
}

void Board::draw(
    const GameState& state,
    const std::string nextPieceLines[4]
) const {
    using std::string;
    string frame;
    frame.reserve(12000); // Enough capacity for ANSI colors and full frame.

    // Clear screen and move cursor to top\-left (ANSI escapes).
    frame += "\033[2J\033[1;1H";

    const string title = "TETRIS GAME";
    int boardVisualWidth = BOARD_WIDTH * 2; // Each cell is drawn 2 chars wide.

    // Top border with box\-drawing characters.
    frame += "╔";
    for (int i = 0; i < boardVisualWidth; ++i) frame += "═";
    frame += "╦";
    for (int i = 0; i < 13; ++i) frame += "═";
    frame += "╗\n";

    // Title row.
    frame += "║";
    int totalPadding = boardVisualWidth - static_cast<int>(title.size());
    int leftPad      = totalPadding / 2;
    int rightPad     = totalPadding - leftPad;

    frame.append(leftPad, ' ');
    frame += title;
    frame.append(rightPad, ' ');
    frame += "║  NEXT PIECE ║\n";

    // Divider row.
    frame += "╠";
    for (int i = 0; i < boardVisualWidth; ++i) frame += "═";
    frame += "╬";
    for (int i = 0; i < 13; ++i) frame += "═";
    frame += "╣\n";

    // Main playfield rows.
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        frame += "║";

        // Left side: playfield cells.
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            char cell = grid[y][x];

            if (cell == '.') {
                // Ghost piece is drawn as "[ ]" style but here we keep 2 chars.
                frame.append("[]");
            } else if (cell != ' ') {
                // Locked piece cell, draw as colored "██".
                frame += getColorForPiece(cell);
                frame.append("██");
                frame += COLOR_RESET;
            } else {
                // Empty cell: 2 spaces.
                frame.append("  ");
            }
        }

        frame += "║";

        // Right side: next piece + stats panel.
        if (y == 0) {
            frame.append(13, ' ');
            frame += "║";
        } else if (y >= 1 && y <= 4) {
            frame += "  ";                      // Left padding (2 spaces)
            frame += nextPieceLines[y - 1];     // One row of preview
            frame += "   ║";                    // Right padding + border
        } else if (y == 5) {
            frame.append(13, ' ');
            frame += "║";
        } else if (y == 6) {
            for (int k = 0; k < 13; ++k) frame += "─";
            frame += "║";
        } else if (y == 7) {
            frame += " SCORE:      ║";
        } else if (y == 8) {
            string scoreStr = std::to_string(state.score);
            frame += " ";
            frame += scoreStr;
            int padding = 12 - static_cast<int>(scoreStr.length());
            if (padding > 0) frame.append(padding, ' ');
            frame += "║";
        } else if (y == 9) {
            frame += " LEVEL:      ║";
        } else if (y == 10) {
            string lvlStr = std::to_string(state.level);
            frame += " ";
            frame += lvlStr;
            int padding = 12 - static_cast<int>(lvlStr.length());
            if (padding > 0) frame.append(padding, ' ');
            frame += "║";
        } else if (y == 11) {
            frame += " LINES:      ║";
        } else if (y == 12) {
            string linesStr = std::to_string(state.linesCleared);
            frame += " ";
            frame += linesStr;
            int padding = 12 - static_cast<int>(linesStr.length());
            if (padding > 0) frame.append(padding, ' ');
            frame += "║";
        } else {
            frame.append(13, ' ');
            frame += "║";
        }

        frame += '\n';
    }

    // Bottom border.
    frame += "╚";
    for (int i = 0; i < boardVisualWidth; ++i) frame += "═";
    frame += "╩";
    for (int i = 0; i < 13; ++i) frame += "═";
    frame += "╝\n";

    frame +=
        "Controls: A/D (Move)  W (Rotate)  S (Soft Drop)  SPACE (Hard Drop)"
        "  G (Ghost)  P (Pause)  Q (Quit)\n";

    std::cout << frame;
    std::cout.flush();
}

int Board::clearLines() {
    int writeRow    = BOARD_HEIGHT - 1;
    int linesCleared = 0;

    // Start from bottom row and scan upwards.
    for (int readRow = BOARD_HEIGHT - 1; readRow >= 0; --readRow) {
        bool full = true;
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            if (grid[readRow][x] == ' ') {
                full = false;
                break;
            }
        }

        if (!full) {
            // If the row is NOT full, copy it down to writeRow.
            if (writeRow != readRow) {
                for (int x = 0; x < BOARD_WIDTH; ++x) {
                    grid[writeRow][x] = grid[readRow][x];
                }
            }
            --writeRow;
        } else {
            // Row is full: we simply skip it and count a cleared line.
            ++linesCleared;
        }
    }

    // Any rows above writeRow are cleared to empty.
    while (writeRow >= 0) {
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            grid[writeRow][x] = ' ';
        }
        --writeRow;
    }

    return linesCleared;
}
