#include "BlockTemplate.h"

char BlockTemplate::templates[NUM_BLOCK_TYPES][BLOCK_SIZE][BLOCK_SIZE];

void BlockTemplate::setBlockTemplate(
    int type,
    char symbol,
    const int shape[BLOCK_SIZE][BLOCK_SIZE]
) {
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        for (int j = 0; j < BLOCK_SIZE; ++j) {
            templates[type][i][j] = shape[i][j] ? symbol : ' ';
        }
    }
}

void BlockTemplate::initializeTemplates() {
    // 7 tetromino base shapes in 4x4 matrices.
    // 1 = filled cell, 0 = empty.
    static const int TETROMINOES[NUM_BLOCK_TYPES][BLOCK_SIZE][BLOCK_SIZE] = {
        // I
        { {0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0} },
        // O
        { {0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0} },
        // T
        { {0,0,0,0}, {0,1,0,0}, {1,1,1,0}, {0,0,0,0} },
        // S
        { {0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0} },
        // Z
        { {0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0} },
        // J
        { {0,0,0,0}, {1,0,0,0}, {1,1,1,0}, {0,0,0,0} },
        // L
        { {0,0,0,0}, {0,0,1,0}, {1,1,1,0}, {0,0,0,0} }
    };

    static const char NAMES[NUM_BLOCK_TYPES] = {'I','O','T','S','Z','J','L'};

    for (int i = 0; i < NUM_BLOCK_TYPES; ++i) {
        setBlockTemplate(i, NAMES[i], TETROMINOES[i]);
    }
}

char BlockTemplate::getCell(int type, int rotation, int row, int col) {
    int r = row;
    int c = col;

    // Apply rotation in 90° clockwise steps.
    for (int i = 0; i < rotation; ++i) {
        int temp = 3 - c;
        c = r;
        r = temp;
    }

    return templates[type][r][c];
}
