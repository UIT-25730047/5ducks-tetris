#pragma once

class BlockTemplate {
public:
    static constexpr int BLOCK_SIZE      = 4;
    static constexpr int NUM_BLOCK_TYPES = 7;

    // Initialize all tetromino templates
    static void initializeTemplates();

    // Return the character for a given piece cell after rotation.
    static char getCell(int type, int rotation, int row, int col);
private:
    static char templates[NUM_BLOCK_TYPES][BLOCK_SIZE][BLOCK_SIZE];

    // Hàm hỗ trợ để điền một template entry
    static void setBlockTemplate(
        int type,
        char symbol,
        const int shape[BLOCK_SIZE][BLOCK_SIZE]
    );
};
