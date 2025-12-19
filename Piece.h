#pragma once

struct Position {
    int x{0};
    int y{0};

    Position() = default;
    Position(int px, int py) : x(px), y(py) {}
};

class Piece {
public:
    int type{0};              // 0..6 index into tetromino shapes
    int rotation{0};          // 0..3 rotation state (90° steps)
    Position pos{5, 0};       // Top-left corner of 4x4 template on board

    Piece() = default;
};
