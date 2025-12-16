#pragma once

// \brief Static container for the 7 tetromino templates and rotation logic.
// The templates are 4x4 matrices for each piece type.
class BlockTemplate {
public:
    static constexpr int BLOCK_SIZE      = 4;
    static constexpr int NUM_BLOCK_TYPES = 7;

    // \brief Initialize all tetromino templates.
    // Must be called once before using \getCell\.
    static void initializeTemplates();

    // \brief Return the character for a given piece cell after rotation.
    // \param type     Piece type [0..6]
    // \param rotation Rotation [0..3], clockwise 90° steps
    // \param row      Row in the 4x4 template [0..3]
    // \param col      Column in the 4x4 template [0..3]
    // \return ' ' for empty, otherwise one of \I,O,T,S,Z,J,L\.
    static char getCell(int type, int rotation, int row, int col);

private:
    // [type][row][col]
    static char templates[NUM_BLOCK_TYPES][BLOCK_SIZE][BLOCK_SIZE];

    // Helper to fill one template entry.
    static void setBlockTemplate(int type,
                                 char symbol,
                                 const int shape[BLOCK_SIZE][BLOCK_SIZE]);
};
