#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <random>

#include <unistd.h>
#include <limits.h>
#if __APPLE__
#include <mach-o/dyld.h>
#endif

using namespace std;

constexpr int BOARD_HEIGHT     = 20;
constexpr int BOARD_WIDTH      = 15;
constexpr int BLOCK_SIZE       = 4;
constexpr int NUM_BLOCK_TYPES  = 7;

// gameplay tuning
constexpr long BASE_DROP_SPEED_US   = 500000; // base drop speed (5s)
constexpr int  DROP_INTERVAL_TICKS  = 5;      // logic steps per drop

struct Position {
    int x{}, y{};
    Position() = default;
    Position(int _x, int _y) : x(_x), y(_y) {}
};

struct GameState {
    bool running{true};
};

struct Piece {
    int type{0};
    int rotation{0};
    Position pos{5, 0};
};

struct SoundManager {
    // Get the execute file path
    static std::string getExecutableDirectory() {
        char buffer[PATH_MAX];

    #if __APPLE__
        uint32_t size = sizeof(buffer);
        if (_NSGetExecutablePath(buffer, &size) == 0) {
            std::string path(buffer);
            return path.substr(0, path.find_last_of('/'));
        }
        return "";
    #else
        ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (len != -1) {
            buffer[len] = '\0';
            std::string path(buffer);
            return path.substr(0, path.find_last_of('/'));
        }
        return "";
    #endif
    }

    // Helper
    static std::string soundPath(const std::string& filename) {
        return getExecutableDirectory() + "/sounds/" + filename;
    }

    // File names
    static inline std::string backgroundSoundFile = "background_sound_01.mp3";

    // Background sound
    static void playBackgroundSound() {
        std::string path = soundPath(backgroundSoundFile);

    #if __APPLE__
        std::string cmd =
            "while true; do afplay \"" + path + "\"; done &";
    #else
        std::string cmd =
            "while true; do aplay \"" + path + "\"; done &";
    #endif

        system(cmd.c_str());
    }
    
    static void stopBackgroundSound() {
        // Kills all "play" processes running in loop
        system("pkill -f \"play .*background_sound_01.mp3\" >/dev/null 2>&1");
    }
    
    // Sound effects
    static void playSFX(const std::string& filename) {
        std::string path = soundPath(filename);
        
    #if __APPLE__
        std::string cmd = "afplay \"" + path + "\" &";
    #else
        std::string cmd = "aplay \"" + path + "\" &";
    #endif
        system(cmd.c_str());
    }
    
    // Soft drop
    static inline std::string softDropSoundFile = "soft_drop.mp3";
    static void playSoftDropSound() {
        playSFX(softDropSoundFile);
    }
    
    // Hard drop
    static inline std::string hardDropSoundFile = "hard_drop.wav";
    static void playHardDropSound() {
        playSFX(hardDropSoundFile);
    }
    
    // Lock piece
    static inline std::string lockPieceSoundFile = "lock_piece.wav";
    static void playLockPieceSound() {
        playSFX(lockPieceSoundFile);
    }
};

struct Board {
    char grid[BOARD_HEIGHT][BOARD_WIDTH]{};

    void init() {
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                grid[i][j] = (i == 0 || i == BOARD_HEIGHT - 1 ||
                              j == 0 || j == BOARD_WIDTH - 1)
                              ? '#'
                              : ' ';
            }
        }
    }

    void draw(const GameState& state) const {
        // clear screen + move cursor to top-left
        cout << "\033[2J\033[1;1H";
        cout << "Controls: a=left d=right w=rotate x=soft-drop SPACE=hard-drop q=quit\n\n";

        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                cout << grid[i][j];
            }
            cout << "\n";
        }
        cout.flush();
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

            // Keep non-full rows, skip full ones
            if (!full) {
                if (writeRow != readRow) {
                    for (int j = 1; j < BOARD_WIDTH - 1; ++j) {
                        grid[writeRow][j] = grid[readRow][j];
                    }
                }
                --writeRow;
            }
        }

        // Clear remaining top rows
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

    static void setBlockTemplate(int type,
                                 char symbol,
                                 const int shape[BLOCK_SIZE][BLOCK_SIZE]) {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                templates[type][i][j] = shape[i][j] ? symbol : ' ';
            }
        }
    }

    static void initializeTemplates() {
        static const int TETROMINOES[7][4][4] = {
            // I
            {
                {0,1,0,0},
                {0,1,0,0},
                {0,1,0,0},
                {0,1,0,0}
            },
            // O
            {
                {0,0,0,0},
                {0,1,1,0},
                {0,1,1,0},
                {0,0,0,0}
            },
            // T
            {
                {0,0,0,0},
                {0,1,0,0},
                {1,1,1,0},
                {0,0,0,0}
            },
            // S
            {
                {0,0,0,0},
                {0,1,1,0},
                {1,1,0,0},
                {0,0,0,0}
            },
            // Z
            {
                {0,0,0,0},
                {1,1,0,0},
                {0,1,1,0},
                {0,0,0,0}
            },
            // J
            {
                {0,0,0,0},
                {1,0,0,0},
                {1,1,1,0},
                {0,0,0,0}
            },
            // L
            {
                {0,0,0,0},
                {0,0,1,0},
                {1,1,1,0},
                {0,0,0,0}
            }
        };

        static const char NAMES[7] = {'I','O','T','S','Z','J','L'};

        for (int i = 0; i < 7; i++) {
            setBlockTemplate(i, NAMES[i], TETROMINOES[i]);
        }
    }

    // rotation: 0-3 (90° steps clockwise)
    static char getCell(int type, int rotation, int row, int col) {
        int r = row;
        int c = col;

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

    // ---------- terminal handling (POSIX) ----------

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
        ssize_t result = read(STDIN_FILENO, &ch, 1);
        return (result > 0) ? ch : 0;
    }

    void flushInput() const {
        char ch;
        tcflush(STDIN_FILENO, TCIFLUSH);
    }

    // ---------- helpers for spawn / movement ----------

    bool isInsidePlayfield(int x, int y) const {
        return x >= 1 && x < BOARD_WIDTH - 1 &&
               y >= 0 && y < BOARD_HEIGHT - 1;
    }

    // used only when spawning: ignore existing blocks, check only bounds
    bool canSpawn(const Piece& piece) const {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(piece.type, piece.rotation, i, j);
                if (cell == ' ') continue;

                int xt = piece.pos.x + j;
                int yt = piece.pos.y + i;

                // blocks can start above the field (yt < 0), but must be in bounds once in field
                if (yt >= 0 && !isInsidePlayfield(xt, yt)) {
                    return false;
                }
            }
        }
        return true;
    }

    bool canMove(int dx, int dy, int newRotation) const {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(
                    currentPiece.type, newRotation, i, j
                );
                if (cell == ' ') continue;

                int xt = currentPiece.pos.x + j + dx;
                int yt = currentPiece.pos.y + i + dy;

                // horizontal bounds (inner playable area 1..BOARD_WIDTH-2)
                if (xt < 1 || xt >= BOARD_WIDTH - 1) return false;

                // bottom collision with floor or blocks
                if (yt >= BOARD_HEIGHT - 1) return false;

                // above top is allowed (yt < 0), but do not read grid there
                if (yt >= 0 && board.grid[yt][xt] != ' ') return false;
            }
        }
        return true;
    }

    void placePiece(const Piece& piece, bool place) {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(
                    piece.type, piece.rotation, i, j
                );
                if (cell == ' ') continue;

                int xt = piece.pos.x + j;
                int yt = piece.pos.y + i;

                if (yt < 0 || yt >= BOARD_HEIGHT ||
                    xt < 0 || xt >= BOARD_WIDTH) {
                    continue;
                }

                board.grid[yt][xt] = place ? cell : ' ';
            }
        }
    }

    void spawnNewPiece() {
        std::uniform_int_distribution<int> dist(0, NUM_BLOCK_TYPES - 1);
        currentPiece.type = dist(rng);
        currentPiece.rotation = 0;

        // spawn near horizontal center
        int spawnX = (BOARD_WIDTH / 2) - (BLOCK_SIZE / 2);
        currentPiece.pos = Position(spawnX, -1);

        // spawn-specific check: only bounds
        if (!canSpawn(currentPiece)) {
            state.running = false;
        }
    }

    bool lockPieceAndCheck() {
        // permanently place current piece
        placePiece(currentPiece, true);
        board.clearLines();

        spawnNewPiece();
        if (!state.running) {
            board.draw(state);
            cout << "\n*** GAME OVER ***\n";
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
        int distance = 0;
        while (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
            ++distance;
        }
        state.running = lockPieceAndCheck();
        dropCounter = 0;
    }

    void handleInput() {
        char c = getInput();
        if (c == 0) return;

        switch (c) {
            case 'a': // move left
                if (canMove(-1, 0, currentPiece.rotation)) {
                    currentPiece.pos.x--;
                }
                break;
            case 'd': // move right
                if (canMove(1, 0, currentPiece.rotation)) {
                    currentPiece.pos.x++;
                }
                break;
            case 'x': // soft drop one cell
                SoundManager::playSoftDropSound();
                softDrop();
                break;
            case ' ': // hard drop
                SoundManager::playHardDropSound();
                hardDrop();
                flushInput(); // flush repeated spaces
                break;
            case 'w': { // rotate with basic wall kicks
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
            case 'q':
                state.running = false;
                break;
            default:
                break;
        }
    }

    void handleGravity() {
        if (!state.running) return;

        ++dropCounter;
        if (dropCounter < DROP_INTERVAL_TICKS) return;

        dropCounter = 0;

        if (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        } else {
            state.running = lockPieceAndCheck();
            SoundManager::playLockPieceSound();
        }
    }

    void run() {
        BlockTemplate::initializeTemplates();
        board.init();

        // ensure the inner board is empty before starting
        for (int y = 1; y < BOARD_HEIGHT - 1; ++y) {
            for (int x = 1; x < BOARD_WIDTH - 1; ++x) {
                board.grid[y][x] = ' ';
            }
        }
        
        // Play background sound
        SoundManager::playBackgroundSound();

        enableRawMode();
        spawnNewPiece();

        cout << "Tetris Game - Starting...\n";
        usleep(500000);

        while (state.running) {
            handleInput();
            handleGravity();
            placePiece(currentPiece, true);
            board.draw(state);
            placePiece(currentPiece, false);

            usleep(dropSpeedUs / DROP_INTERVAL_TICKS);
        }

        disableRawMode();
        // Stop background sound
        SoundManager::stopBackgroundSound();
        cout << "Thanks for playing!\n";
        usleep(2000000);
    }
};

int main() {
    TetrisGame game;
    game.run();
    return 0;
}
