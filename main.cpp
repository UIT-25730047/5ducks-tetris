#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <random>

using namespace std;

// --- Constants ---
constexpr int BOARD_HEIGHT      = 20;
constexpr int BOARD_WIDTH       = 15;
constexpr int BLOCK_SIZE        = 4;
constexpr int NUM_BLOCK_TYPES   = 7;

// Gameplay settings
constexpr long BASE_DROP_SPEED_US   = 500000; // 0.5 seconds per drop
constexpr int  DROP_INTERVAL_TICKS  = 5;      // Ticks per gravity update

struct Position {
    int x{}, y{};
    Position() = default;
    Position(int _x, int _y) : x(_x), y(_y) {}
};

struct GameState {
    bool running{true};
    bool paused{false};
};

struct Piece {
    int type{0};
    int rotation{0};
    Position pos{5, 0};
};

struct Board {
    char grid[BOARD_HEIGHT][BOARD_WIDTH]{};

    // Initialize the game board with walls and a "ceiling hole"
    void init() {
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                
                // 1. Draw Bottom, Left, and Right Walls
                if (i == BOARD_HEIGHT - 1 || j == 0 || j == BOARD_WIDTH - 1) {
                    grid[i][j] = '#';
                }
                // 2. Handle the Ceiling (Top Row)
                // We must leave a gap for new pieces to enter the board.
                else if (i == 0) {
                    // Create an opening from column 5 to 9
                    if (j >= 5 && j <= 9) {
                        grid[i][j] = ' '; 
                    } else {
                        grid[i][j] = '#'; // Ceiling blocks
                    }
                }
                // 3. The playable area inside is empty
                else {
                    grid[i][j] = ' ';
                }
            }
        }
    }

    void draw(const GameState& state) const {
        // Clear screen and reset cursor to top-left
        cout << "\033[2J\033[1;1H";
        cout << "Controls: A/D=Move  W=Rotate  X=Soft-drop  SPACE=Hard-drop  P=Pause  Q=Quit\n\n";

        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                cout << grid[i][j];
            }
            cout << "\n";
        }
        cout.flush();
    }

    void drawPause() const {
        cout << "\033[2J\033[1;1H"; 
        cout << "\n\n\n";
        cout << "      ======================\n";
        cout << "      =    GAME PAUSED     =\n";
        cout << "      ======================\n\n";
        cout << "        Press P to Resume   \n";
        cout << "        Press Q to Quit     \n";
        cout.flush();
    }

    // [FEATURE] Game Over Animation
    // Creates a "rising wave" effect by filling the board with '#' from bottom to top.
    void animateGameOver(const GameState& state) {
        // Iterate from the bottom playable row up to the top
        for (int y = BOARD_HEIGHT - 2; y >= 1; --y) {
            
            // Fill the current row with '#' blocks
            for (int x = 1; x < BOARD_WIDTH - 1; ++x) {
                grid[y][x] = '#'; 
            }

            // Redraw immediately to show the frame
            draw(state);

            // Sleep 50ms to create the visual "wave" effect
            usleep(50000); 
        }
    }

    void clearLines() {
        int writeRow = BOARD_HEIGHT - 2;

        // Scan from bottom to top
        for (int readRow = BOARD_HEIGHT - 2; readRow > 0; --readRow) {
            bool full = true;
            for (int j = 1; j < BOARD_WIDTH - 1; ++j) {
                if (grid[readRow][j] == ' ') {
                    full = false;
                    break;
                }
            }

            // If row is not full, copy it to the write position
            if (!full) {
                if (writeRow != readRow) {
                    for (int j = 1; j < BOARD_WIDTH - 1; ++j) {
                        grid[writeRow][j] = grid[readRow][j];
                    }
                }
                --writeRow;
            }
            // If row IS full, we skip incrementing writeRow (effectively deleting the line)
        }

        // Fill the remaining top rows with empty space
        while (writeRow > 0) {
            for (int j = 1; j < BOARD_WIDTH - 1; ++j) {
                grid[writeRow][j] = ' ';
            }
            --writeRow;
        }
    }
};

struct BlockTemplate {
    static char templates[NUM_BLOCK_TYPES][BLOCK_SIZE][BLOCK_SIZE];

    static void setBlockTemplate(int type, char symbol, const int shape[BLOCK_SIZE][BLOCK_SIZE]) {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                templates[type][i][j] = shape[i][j] ? symbol : ' ';
            }
        }
    }

    static void initializeTemplates() {
        // Tetromino definitions (I, O, T, S, Z, J, L)
        static const int TETROMINOES[7][4][4] = {
            { {0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0} }, // I
            { {0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0} }, // O
            { {0,0,0,0}, {0,1,0,0}, {1,1,1,0}, {0,0,0,0} }, // T
            { {0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0} }, // S
            { {0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0} }, // Z
            { {0,0,0,0}, {1,0,0,0}, {1,1,1,0}, {0,0,0,0} }, // J
            { {0,0,0,0}, {0,0,1,0}, {1,1,1,0}, {0,0,0,0} }  // L
        };

        static const char NAMES[7] = {'I','O','T','S','Z','J','L'};

        for (int i = 0; i < 7; i++) {
            setBlockTemplate(i, NAMES[i], TETROMINOES[i]);
        }
    }

    static char getCell(int type, int rotation, int row, int col) {
        int r = row;
        int c = col;
        // Simple 90-degree clockwise rotation logic
        for (int i = 0; i < rotation; ++i) {
            int temp = 3 - c;
            c = r;
            r = temp;
        }
        return templates[type][r][c];
    }
};

char BlockTemplate::templates[NUM_BLOCK_TYPES][BLOCK_SIZE][BLOCK_SIZE];

struct TetrisGame {
    Board board;
    GameState state;
    Piece currentPiece{};

    termios origTermios{};
    long dropSpeedUs{BASE_DROP_SPEED_US};
    int dropCounter{0};
    std::mt19937 rng;

    TetrisGame() {
        std::random_device rd;
        rng.seed(rd());
    }

    // --- POSIX Terminal Handling ---

    void enableRawMode() {
        tcgetattr(STDIN_FILENO, &origTermios);
        termios raw = origTermios;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

    void disableRawMode() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios);
    }

    char getInput() const {
        char ch = 0;
        if (read(STDIN_FILENO, &ch, 1) > 0) return ch;
        return 0;
    }

    void flushInput() const {
        tcflush(STDIN_FILENO, TCIFLUSH);
    }

    // --- Game Logic ---

    bool isInsidePlayfield(int x, int y) const {
        return x >= 1 && x < BOARD_WIDTH - 1 &&
               y >= 0 && y < BOARD_HEIGHT - 1;
    }

    // Check if a piece can exist at the current location
    bool canSpawn(const Piece& piece) const {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(piece.type, piece.rotation, i, j);
                if (cell == ' ') continue;

                int xt = piece.pos.x + j;
                int yt = piece.pos.y + i;

                // 1. Boundary Check: Ensure piece is within walls
                if (yt >= 0 && !isInsidePlayfield(xt, yt)) {
                    return false;
                }

                // 2. Collision Check: Ensure piece doesn't overlap existing blocks
                // This logic is crucial for "Touch Roof" detection.
                if (yt >= 0 && board.grid[yt][xt] != ' ') {
                    return false; 
                }
            }
        }
        return true;
    }

    bool canMove(int dx, int dy, int newRotation) const {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(currentPiece.type, newRotation, i, j);
                if (cell == ' ') continue;

                int xt = currentPiece.pos.x + j + dx;
                int yt = currentPiece.pos.y + i + dy;

                if (xt < 1 || xt >= BOARD_WIDTH - 1) return false;
                if (yt >= BOARD_HEIGHT - 1) return false;
                if (yt >= 0 && board.grid[yt][xt] != ' ') return false;
            }
        }
        return true;
    }

    void placePiece(const Piece& piece, bool place) {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(piece.type, piece.rotation, i, j);
                if (cell == ' ') continue;

                int xt = piece.pos.x + j;
                int yt = piece.pos.y + i;

                if (yt < 0 || yt >= BOARD_HEIGHT || xt < 0 || xt >= BOARD_WIDTH) continue;

                board.grid[yt][xt] = place ? cell : ' ';
            }
        }
    }

    void spawnNewPiece() {
        std::uniform_int_distribution<int> dist(0, NUM_BLOCK_TYPES - 1);
        currentPiece.type = dist(rng);
        currentPiece.rotation = 0;
        int spawnX = (BOARD_WIDTH / 2) - (BLOCK_SIZE / 2);
        currentPiece.pos = Position(spawnX, -1);

        // If the new piece collides immediately, it means Game Over.
        if (!canSpawn(currentPiece)) {
            state.running = false;
        }
    }

    bool lockPieceAndCheck() {
        placePiece(currentPiece, true);
        board.clearLines();

        spawnNewPiece();
        // If spawnNewPiece set running=false, we return false here.
        // We do NOT print Game Over here; we let the animation handle it.
        if (!state.running) {
            return false;
        }
        return true;
    }

    void softDrop() {
        if (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        } else {
            state.running = lockPieceAndCheck();
            dropCounter = 0;
        }
    }

    void hardDrop() {
        while (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        }
        state.running = lockPieceAndCheck();
        dropCounter = 0;
    }

    void handleInput() {
        char c = getInput();
        if (c == 0) return;

        // Toggle Pause
        if (c == 'p') {
            state.paused = !state.paused;
            if (state.paused) {
                board.drawPause(); 
            }
            return;
        }

        if (state.paused) {
            if (c == 'q') state.running = false;
            return;
        }

        switch (c) {
            case 'a': if (canMove(-1, 0, currentPiece.rotation)) currentPiece.pos.x--; break;
            case 'd': if (canMove(1, 0, currentPiece.rotation)) currentPiece.pos.x++; break;
            case 'x': softDrop(); break;
            case ' ': hardDrop(); flushInput(); break;
            case 'w': { 
                int newRot = (currentPiece.rotation + 1) % 4;
                int kicks[] = {0, -1, 1, -2, 2};
                for (int dx : kicks) {
                    if (canMove(dx, 0, newRot)) {
                        currentPiece.pos.x += dx;
                        currentPiece.rotation = newRot;
                        break;
                    }
                }
                break;
            }
            case 'q': state.running = false; break;
        }
    }

    void handleGravity() {
        if (!state.running || state.paused) return;

        ++dropCounter;
        if (dropCounter < DROP_INTERVAL_TICKS) return;
        dropCounter = 0;

        if (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        } else {
            state.running = lockPieceAndCheck();
        }
    }

    void run() {
        BlockTemplate::initializeTemplates();
        board.init();

        // Ensure playfield is clean (except walls)
        for (int y = 1; y < BOARD_HEIGHT - 1; ++y) {
            for (int x = 1; x < BOARD_WIDTH - 1; ++x) {
                board.grid[y][x] = ' ';
            }
        }

        enableRawMode();
        spawnNewPiece();

        cout << "Tetris Game - Starting...\n";
        usleep(500000);

        // --- MAIN GAME LOOP ---
        while (state.running) {
            handleInput();

            if (state.paused) {
                usleep(100000);
                continue;
            }

            handleGravity();
            
            placePiece(currentPiece, true);
            board.draw(state);
            placePiece(currentPiece, false);

            usleep(dropSpeedUs / DROP_INTERVAL_TICKS);
        }
        
        // --- GAME OVER SEQUENCE ---
        
        // 1. Play the rising wave animation
        board.animateGameOver(state);

        // 2. Restore terminal and show final message
        disableRawMode();
        usleep(2000000); 
    }
};

int main() {
    TetrisGame game;
    game.run();
    return 0;
}
