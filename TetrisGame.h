#pragma once

#include <string>
#include <vector>
#include <random>
#include <termios.h>

#include "Board.h"
#include "GameState.h"
#include "Piece.h"

using namespace std;

// Base drop speed and timing constants (microseconds / ticks).
constexpr long BASE_DROP_SPEED_US  = 500000; // Base tick group duration.
constexpr int  DROP_INTERVAL_TICKS = 5;      // Logic steps per drop.
constexpr int  ANIM_DELAY_US       = 15000;  // Game-over animation delay.


class TetrisGame {
private:
    Board      board;
    GameState  state;
    Piece      currentPiece;
    int        nextPieceType{0};

    termios    origTermios{};         // Saved terminal settings.
    long       dropSpeedUs{BASE_DROP_SPEED_US};
    int        dropCounter{0};

    // Track previous ghost locations to clear only those cells.
    vector<Position> lastGhostPositions;

    // Cache for "next piece" preview.
    string cachedNextPiecePreview[4];
    int         cachedNextPieceType{-1};

    mt19937 rng;                 // Random generator for piece types.

    // \=== High score handling ===
    void loadHighScores();
    int  saveAndGetRank();

    // \=== Drawing screens ===
    void drawStartScreen();
    void drawGameOverScreen(int rank);
    void drawPauseScreen() const;

    // \=== Terminal handling (POSIX raw mode) ===
    void enableRawMode();
    void disableRawMode();
    char getInput() const;
    void flushInput() const;
    char waitForKeyPress();

    // \=== Game logic helpers ===
    void resetGame();
    void animateGameOver();
    bool isInsidePlayfield(int x, int y) const;
    Piece calculateGhostPiece() const;
    bool  canSpawn(const Piece& piece) const;
    bool  canMove(int dx, int dy, int newRotation) const;

    void placePiece(const Piece& piece, bool place);
    void placePieceSafe(const Piece& piece);
    void clearAllGhostDots();
    void placeGhostPiece(const Piece& ghostPiece);

    void spawnNewPiece();
    bool lockPieceAndCheck(bool muteLockSound = false);
    void softDrop();
    void hardDrop();
    void handleInput();
    void handleGravity();

    void getNextPiecePreview(string lines[4]);

    // \=== Difficulty / speed ===
    long computeDropSpeedUs(int level) const;
    void updateDifficulty();

public:
    // Constructor
    TetrisGame();

    // Run the game
    void run();
};