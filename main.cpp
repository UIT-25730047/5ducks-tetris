#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <random>
#include <fstream>

#include <unistd.h>
#include <limits.h>
#if __APPLE__
#include <mach-o/dyld.h>
#endif

#include <thread>
#include <chrono>

using namespace std;

constexpr int BOARD_HEIGHT     = 20;
constexpr int BOARD_WIDTH      = 15;
constexpr int NEXT_PICE_WIDTH  = 14;

constexpr int BLOCK_SIZE       = 4;
constexpr int NUM_BLOCK_TYPES  = 7;

// gameplay tuning
constexpr long BASE_DROP_SPEED_US   = 500000; // base drop speed (5s)
constexpr int  DROP_INTERVAL_TICKS  = 5;      // logic steps per drop
constexpr int ANIM_DELAY_US = 15000; // 15ms per cell for smooth animation
const string HIGH_SCORE_FILE    = "highscores.txt";

// Color codes for terminal
const char* COLOR_RESET = "\033[0m";
const char* COLOR_CYAN = "\033[36m";
const char* COLOR_YELLOW = "\033[33m";
const char* COLOR_PURPLE = "\033[35m";
const char* COLOR_GREEN = "\033[32m";
const char* COLOR_RED = "\033[31m";
const char* COLOR_BLUE = "\033[34m";
const char* COLOR_ORANGE = "\033[38;5;208m";
const char* COLOR_WHITE = "\033[37m";

// Color mapping for each piece type (by index)
const char* PIECE_COLORS[7] = {
    COLOR_CYAN,   // I - type 0
    COLOR_YELLOW, // O - type 1
    COLOR_PURPLE, // T - type 2
    COLOR_GREEN,  // S - type 3
    COLOR_RED,    // Z - type 4
    COLOR_BLUE,   // J - type 5
    COLOR_ORANGE  // L - type 6
};

// Helper function to get color code for a piece character
static const char* getColorForPiece(char cell) {
    switch (cell) {
        case 'I': return PIECE_COLORS[0]; // Cyan
        case 'O': return PIECE_COLORS[1]; // Yellow
        case 'T': return PIECE_COLORS[2]; // Purple
        case 'S': return PIECE_COLORS[3]; // Green
        case 'Z': return PIECE_COLORS[4]; // Red
        case 'J': return PIECE_COLORS[5]; // Blue
        case 'L': return PIECE_COLORS[6]; // Orange
        case '.': return COLOR_WHITE;     // Ghost piece (dim white)
        case '#': return COLOR_WHITE;     // Game over animation
        default: return COLOR_RESET;
    }
}

struct Position {
    int x{}, y{};
    Position() = default;
    Position(int _x, int _y) : x(_x), y(_y) {}
};

struct GameState {
    bool running{true};
    bool quitByUser{false};   // Track if user quit manually vs. game over
    bool paused{false};       // Pause state tracking
    bool ghostEnabled{true};  // Ghost shadow enabled by default
    int score{0};
    int level{1};
    int linesCleared{0};
    vector<int> highScores;   // Stores the Top 5 High Scores

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

    // File names - using const char* for C++11 compatibility
    static const char* getBackgroundSoundFile() { return "background_sound_01.wav"; }

    // Background sound
    static void playBackgroundSound() {
        std::string path = soundPath(getBackgroundSoundFile());

    #if __APPLE__
        std::string cmd =
            "while true; do afplay \"" + path + "\"; done &";
    #else
        // Play wav files in Linux. (without mp3 files)
        std::string cmd = "while true; do aplay -q \"" + path + "\"; done &";
    #endif

        system(cmd.c_str());
    }

    static void stopBackgroundSound() {
        // Kills all background music processes
    #if __APPLE__
        system("pkill -f \"afplay.*background_sound_01.wav\" >/dev/null 2>&1");
    #else
        // Kill wav files processes playing background music (without mp3 files)
        system("pkill -f \"aplay.*background_sound_01.wav\" >/dev/null 2>&1");
    #endif
    }

    // Sound effects
    static void playSFX(const std::string& filename) {
        std::string path = soundPath(filename);

    #if __APPLE__
        std::string cmd = "afplay \"" + path + "\" &";
    #else
        // Linux: Determine player based on file extension
        std::string ext = filename.substr(filename.find_last_of('.'));
        std::string cmd;

        if (ext == ".mp3") {
            // For MP3: use mpg123 (fallback to ffplay)
            cmd = "(command -v mpg123 >/dev/null 2>&1 && mpg123 -q \"" + path + "\") || "
                  "(command -v ffplay >/dev/null 2>&1 && ffplay -nodisp -autoexit -loglevel quiet \"" + path + "\") &";
        } else {
            // For WAV: use aplay (fallback to ffplay)
            cmd = "(command -v aplay >/dev/null 2>&1 && aplay -q \"" + path + "\") || "
                  "(command -v ffplay >/dev/null 2>&1 && ffplay -nodisp -autoexit -loglevel quiet \"" + path + "\") &";
        }
    #endif
        system(cmd.c_str());
    }

    static void playSoundAfterDelay(const std::string& file, int delayMs) {
        std::thread([file, delayMs]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            playSFX(file);
        }).detach();
    }

    // Soft drop
    static const char* getSoftDropSoundFile() { return "soft_drop_2.wav"; }
    static void playSoftDropSound() {
        playSFX(getSoftDropSoundFile());
    }

    // Hard drop
    static const char* getHardDropSoundFile() { return "hard_drop.wav"; }
    static void playHardDropSound() {
        playSFX(getHardDropSoundFile());
    }

    // Lock piece
    static const char* getLockPieceSoundFile() { return "lock_piece.wav"; }
    static void playLockPieceSound() {
        playSFX(getLockPieceSoundFile());
    }

    // Line clear
    static const char* getLineClearSoundFile() { return "line_clear.wav"; }
    static void playLineClearSound() {
        playSFX(getLineClearSoundFile());
    }

    // Tetris (4 lines)
    static const char* getFourLinesClearSoundFile() { return "4lines_clear.wav"; }
    static void play4LinesClearSound() {
        playSFX(getFourLinesClearSoundFile());
    }

    // Level up
    static const char* getLevelUpSoundFile() { return "level_up.wav"; }
    static void playLevelUpSound() {
        playSoundAfterDelay(getLevelUpSoundFile(), 1000);
    }

    // Game over
    static const char* getGameOverSoundFile() { return "game_over.wav"; }
    static void playGameOverSound() {
        playSFX(getGameOverSoundFile());
    }
};

struct Board {
    char grid[BOARD_HEIGHT][BOARD_WIDTH]{};

    // Initialize the game board with walls and a "ceiling hole"
    void init() {
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                grid[i][j] = ' ';
            }
        }
    }

    void draw(const GameState& state, const string nextPieceLines[4]) const {
        string frame;
        frame.reserve(12000); // Increased for colored blocks + ANSI codes

        // clear screen + move cursor to top-left
        frame += "\033[2J\033[1;1H";

        const string title = "TETRIS GAME";
        int boardVisualWidth = BOARD_WIDTH * 2; // Each cell is 2 chars wide (██)

        // Top border with Unicode box-drawing characters
        frame += "╔";
        for (int i = 0; i < boardVisualWidth; i++) frame += "═";
        frame += "╦";
        for (int i = 0; i < 13; i++) frame += "═";
        frame += "╗\n";

        // Title row
        frame += "║";
        int totalPadding = boardVisualWidth - title.size();
        int leftPad = totalPadding / 2;
        int rightPad = totalPadding - leftPad;

        frame.append(leftPad, ' ');
        frame += title;
        frame.append(rightPad, ' ');
        frame += "║  NEXT PIECE ║\n";

        // Divider
        frame += "╠";
        for (int i = 0; i < boardVisualWidth; i++) frame += "═";
        frame += "╬";
        for (int i = 0; i < 13; i++) frame += "═";
        frame += "╣\n";

        // Board rows with colored blocks
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            frame += "║";

            // Draw board cells WITH colors
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                char cell = grid[i][j];

                // Render based on cell content with colors
                if (cell == '.') {
                    // Ghost piece: show as [] (no color needed for outline)
                    frame.append("[]");
                } else if (cell != ' ') {
                    // Non-empty cell: show as ██ with color
                    frame += getColorForPiece(cell);
                    frame.append("██");
                    frame += COLOR_RESET;
                } else {
                    // Empty cell: show as two spaces
                    frame.append("  ");
                }
            }

            frame += "║";

            // Right side panel
            if (i == 0) {
                frame.append(13, ' ');
                frame += "║";
            } else if (i >= 1 && i <= 4) {
                frame += "  ";  // 2 spaces left padding
                frame += nextPieceLines[i - 1];
                frame += "   ║";  // 3 spaces right padding + border
            } else if (i == 5) {
                frame.append(13, ' ');
                frame += "║";
            } else if (i == 6) {
                for (int k = 0; k < 13; k++) frame += "─";
                frame += "║";
            } else if (i == 7) {
                frame += " SCORE:      ║";
            } else if (i == 8) {
                string scoreStr = to_string(state.score);
                frame += " ";
                frame += scoreStr;
                int padding = 12 - scoreStr.length();
                if (padding > 0) frame.append(padding, ' ');
                frame += "║";
            } else if (i == 9) {
                frame += " LEVEL:      ║";
            } else if (i == 10) {
                string levelStr = to_string(state.level);
                frame += " ";
                frame += levelStr;
                int padding = 12 - levelStr.length();
                if (padding > 0) frame.append(padding, ' ');
                frame += "║";
            } else if (i == 11) {
                frame += " LINES:      ║";
            } else if (i == 12) {
                string linesStr = to_string(state.linesCleared);
                frame += " ";
                frame += linesStr;
                int padding = 12 - linesStr.length();
                if (padding > 0) frame.append(padding, ' ');
                frame += "║";
            } else {
                frame.append(13, ' ');
                frame += "║";
            }

            frame += '\n';
        }

        // Bottom border
        frame += "╚";
        for (int i = 0; i < boardVisualWidth; i++) frame += "═";
        frame += "╩";
        for (int i = 0; i < 13; i++) frame += "═";
        frame += "╝\n";

        frame += "Controls: A/D (Move)  W (Rotate)  S (Soft Drop)  SPACE (Hard Drop)  G (Ghost)  P (Pause)  Q (Quit)\n";

        cout << frame;
        cout.flush();
    }

    int clearLines() {
        int writeRow = BOARD_HEIGHT - 1;
        int linesCleared = 0;

        for (int readRow = BOARD_HEIGHT - 1; readRow >= 0; --readRow) {
            bool full = true;
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                if (grid[readRow][j] == ' ') {
                    full = false;
                    break;
                }
            }

            if (!full) {
                if (writeRow != readRow) {
                    for (int j = 0; j < BOARD_WIDTH; ++j) {
                        grid[writeRow][j] = grid[readRow][j];
                    }
                }
                --writeRow;
            } else {
                ++linesCleared;
            }
            // If row IS full, we skip incrementing writeRow (effectively deleting the line)
        }

        while (writeRow >= 0) {
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                grid[writeRow][j] = ' ';
            }
            --writeRow;
        }

        return linesCleared;
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
    int nextPieceType{0};

    termios origTermios{};
    long dropSpeedUs{BASE_DROP_SPEED_US};
    int dropCounter{0};
    bool softDropActive{false};

    // Track ghost piece positions for efficient clearing
    vector<Position> lastGhostPositions;

    // Cache next piece preview to avoid regenerating every frame
    string cachedNextPiecePreview[4];
    int cachedNextPieceType{-1};

    mt19937 rng;

    TetrisGame() {
        random_device rd;
        rng.seed(rd());
        loadHighScores();
    }

    // Load multiple scores from file
    void loadHighScores() {
        state.highScores.clear();
        ifstream file(HIGH_SCORE_FILE);
        int scoreVal;
        if (file.is_open()) {
            while (file >> scoreVal) {
                state.highScores.push_back(scoreVal);
            }
            file.close();

            // Sort just in case the file was messed up
            sort(state.highScores.begin(), state.highScores.end(), greater<int>());
        }
    }

    void drawStartScreen() {
        string screen;
        screen.reserve(512);

        // Clear screen + move cursor to top-left
        screen += "\033[2J\033[1;1H";

        // Match the visual width of the game board
        int totalWidth = (BOARD_WIDTH * 2) + 13; // Match board visual width

        // Top border
        screen += "╔";
        for (int i = 0; i < totalWidth; i++) screen += "═";
        screen += "╗\n";

        // Empty row
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";

        // Title row
        const string title = "TETRIS GAME";
        int titlePadding = totalWidth - title.length();
        int titleLeft = titlePadding / 2;
        int titleRight = titlePadding - titleLeft;

        screen += "║";
        screen.append(titleLeft, ' ');
        screen += title;
        screen.append(titleRight, ' ');
        screen += "║\n";

        // Empty row
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";

        // Prompt row
        const string prompt = "Press any key to start...";
        int promptPadding = totalWidth - prompt.length();
        int promptLeft = promptPadding / 2;
        int promptRight = promptPadding - promptLeft;

        screen += "║";
        screen.append(promptLeft, ' ');
        screen += prompt;
        screen.append(promptRight, ' ');
        screen += "║\n";

        // Empty row
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";

        // Bottom border
        screen += "╚";
        for (int i = 0; i < totalWidth; i++) screen += "═";
        screen += "╝\n";

        cout << screen;
        cout.flush();
    }

    char waitForKeyPress() {
        enableRawMode();

        char key = 0;
        while ((key = getInput()) == 0) {
            usleep(50000);
        }

        flushInput();

        return key;
    }

    int saveAndGetRank() {
        // Read existing scores
        vector<int> scores;
        ifstream inFile(HIGH_SCORE_FILE);
        if (inFile.is_open()) {
            int score;
            while (inFile >> score) {
                scores.push_back(score);
            }
            inFile.close();
        }

        // Add current score
        scores.push_back(state.score);

        // Sort in descending order
        sort(scores.begin(), scores.end(), greater<int>());

        // Keep only top 10 scores
        if (scores.size() > 10) {
            scores.resize(10);
        }

        // Write back to file
        ofstream outFile(HIGH_SCORE_FILE);
        if (outFile.is_open()) {
            for (int score : scores) {
                outFile << score << "\n";
            }
            outFile.close();
        }

        // Find rank of current score
        int rank = 1;
        for (int score : scores) {
            if (score == state.score) {
                break;
            }
            rank++;
        }

        return rank;
    }

    void drawGameOverScreen(int rank) {
        // Build entire game over screen in a string buffer for single output
        string screen;
        screen.reserve(1024); // Pre-allocate to avoid reallocation

        // Clear screen + move cursor to top-left
        screen += "\033[2J\033[1;1H";

        // Match the visual width of the game board
        int totalWidth = (BOARD_WIDTH * 2) + 13;

        // Top border
        screen += "╔";
        for (int i = 0; i < totalWidth; i++) screen += "═";
        screen += "╗\n";

        // Empty row
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";

        // "GAME OVER" title
        const string title = "GAME OVER";
        int titlePadding = totalWidth - title.length();
        int titleLeft = titlePadding / 2;
        int titleRight = titlePadding - titleLeft;

        screen += "║";
        screen.append(titleLeft, ' ');
        screen += title;
        screen.append(titleRight, ' ');
        screen += "║\n";

        // Empty row
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";

        // Score display - label left aligned, number right aligned
        string scoreLabel = "Final Score:";
        string scoreNum = to_string(state.score);
        int scoreSpacing = totalWidth - scoreLabel.length() - scoreNum.length();

        screen += "║ ";
        screen += scoreLabel;
        screen.append(scoreSpacing - 2, ' '); // -2 for the left and right padding
        screen += scoreNum;
        screen += " ║\n";

        // Level display
        string levelLabel = "Level:";
        string levelNum = to_string(state.level);
        int levelSpacing = totalWidth - levelLabel.length() - levelNum.length();

        screen += "║ ";
        screen += levelLabel;
        screen.append(levelSpacing - 2, ' ');
        screen += levelNum;
        screen += " ║\n";

        // Lines display
        string linesLabel = "Lines Cleared:";
        string linesNum = to_string(state.linesCleared);
        int linesSpacing = totalWidth - linesLabel.length() - linesNum.length();

        screen += "║ ";
        screen += linesLabel;
        screen.append(linesSpacing - 2, ' ');
        screen += linesNum;
        screen += " ║\n";

        // Empty row
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";

        // Rank display with ordinal suffix
        char rankBuf[64];
        const char* suffix = "th";
        if (rank == 1) suffix = "st";
        else if (rank == 2) suffix = "nd";
        else if (rank == 3) suffix = "rd";
        snprintf(rankBuf, sizeof(rankBuf), "Your Rank: %d%s", rank, suffix);
        string rankStr(rankBuf);
        int rankPadding = totalWidth - rankStr.length();
        int rankLeft = rankPadding / 2;
        int rankRight = rankPadding - rankLeft;

        screen += "║";
        screen.append(rankLeft, ' ');
        screen += rankStr;
        screen.append(rankRight, ' ');
        screen += "║\n";

        // Empty row
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";

        // Display each high score with rank left-aligned and score right-aligned
        for (size_t i = 0; i < state.highScores.size(); ++i) {
            // Determine suffix (st, nd, rd, th)
            string suffix = "th";
            if (i == 0) suffix = "st";
            else if (i == 1) suffix = "nd";
            else if (i == 2) suffix = "rd";

            // Build rank string (e.g., "1st", "2nd", etc.)
            string rankStr = to_string(i + 1) + suffix;

            // Build score string
            string scoreStr = to_string(state.highScores[i]);

            // Add "NEW!" indicator if this is the current score
            bool isNew = (state.score > 0 && state.score == state.highScores[i]);
            if (isNew) {
                scoreStr += " NEW!";
            }

            // Calculate spacing to align rank left and score right
            int contentWidth = rankStr.length() + scoreStr.length();
            int spacing = totalWidth - contentWidth - 2; // -2 for left/right padding
            if (spacing < 1) spacing = 1; // Ensure at least 1 space

            screen += "║ ";
            screen += rankStr;
            screen.append(spacing, ' ');
            screen += scoreStr;
            screen += " ║\n";
        }

        // Empty row
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";

        // "Press R to Restart or Q to Quit" prompt
        const string prompt = "Press R to Restart or Q to Quit";
        int promptPadding = totalWidth - prompt.length();
        int promptLeft = promptPadding / 2;
        int promptRight = promptPadding - promptLeft;

        screen += "║";
        screen.append(promptLeft, ' ');
        screen += prompt;
        screen.append(promptRight, ' ');
        screen += "║\n";

        // Empty row
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";

        // Bottom border
        screen += "╚";
        for (int i = 0; i < totalWidth; i++) screen += "═";
        screen += "╝\n";

        // Single output call
        cout << screen;
        cout.flush();
    }

    void resetGame() {
        // Reset game state
        state.running = true;
        state.paused = false;
        state.quitByUser = false;
        state.score = 0;
        state.level = 1;
        state.linesCleared = 0;

        // Reset board
        board.init();

        // Reset timing
        dropCounter = 0;
        softDropActive = false;

        // Generate new next piece
        uniform_int_distribution<int> dist(0, NUM_BLOCK_TYPES - 1);
        nextPieceType = dist(rng);

        // Spawn first piece
        spawnNewPiece();
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
        if (result <= 0) return 0;

        if (ch == 27) {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) return 27;
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) return 27;

            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': return 'w';
                    case 'B': return 's';
                    case 'C': return 'd';
                    case 'D': return 'a';
                }
            }
            return 27;
        }

        return ch;
    }

    void flushInput() const {
        tcflush(STDIN_FILENO, TCIFLUSH);
    }

    // --- Game Logic ---
    void animateGameOver() {
        // Transform all locked pieces to '#' one by one from bottom to top
        // This creates a cascade effect showing the game is ending
        // Scan from bottom to top, left to right
        for (int i = BOARD_HEIGHT - 1; i >= 0; --i) {
            bool hasBlock = false;
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                if (board.grid[i][j] != ' ') {
                    hasBlock = true;
                    board.grid[i][j] = '#';

                    // Draw immediately for smooth animation
                    string preview[4];
                    getNextPiecePreview(preview);
                    board.draw(state, preview);

                    usleep(ANIM_DELAY_US);
                }
            }

            // Skip empty rows to speed up animation
            if (!hasBlock) continue;
        }

        // Final pause to see completed effect
        flushInput();
        usleep(500000); // 300ms final pause
        flushInput();
    }

    bool isInsidePlayfield(int x, int y) const {
        return x >= 0 && x < BOARD_WIDTH &&
               y >= 0 && y < BOARD_HEIGHT;
    }

    // Calculate where the current piece would land if dropped straight down
    Piece calculateGhostPiece() const {
        Piece ghost = currentPiece;

        // Keep moving down until we hit something
        // Check collision only with locked pieces (not dots or current piece)
        bool canMoveDown = true;
        while (canMoveDown) {
            canMoveDown = false;

            // Check if ghost can move down one more position
            for (int i = 0; i < BLOCK_SIZE; ++i) {
                for (int j = 0; j < BLOCK_SIZE; ++j) {
                    char cell = BlockTemplate::getCell(ghost.type, ghost.rotation, i, j);
                    if (cell == ' ') continue;

                    int xt = ghost.pos.x + j;
                    int yt = ghost.pos.y + i + 1;  // +1 for next position

                    // Check bounds
                    if (yt >= BOARD_HEIGHT) {
                        canMoveDown = false;
                        goto done_checking;
                    }

                    // Check collision with LOCKED pieces only (not dots or spaces)
                    // Locked pieces are letters (I, O, T, S, Z, J, L) or '#'
                    if (yt >= 0) {
                        char gridCell = board.grid[yt][xt];
                        if (gridCell != ' ' && gridCell != '.') {
                            canMoveDown = false;
                            goto done_checking;
                        }
                    }
                }
            }

            canMoveDown = true;
            done_checking:

            if (canMoveDown) {
                ghost.pos.y++;
            }
        }

        return ghost;
    }

    bool canSpawn(const Piece& piece) const {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(piece.type, piece.rotation, i, j);
                if (cell == ' ') continue;

                int xt = piece.pos.x + j;
                int yt = piece.pos.y + i;

                if (xt < 0 || xt >= BOARD_WIDTH) return false;
                if (yt >= BOARD_HEIGHT) return false;

                // Check collision with LOCKED blocks only (ignore ghost dots)
                if (yt >= 0) {
                    char gridCell = board.grid[yt][xt];
                    if (gridCell != ' ' && gridCell != '.') {
                        return false;
                    }
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

                if (xt < 0 || xt >= BOARD_WIDTH) return false;
                if (yt >= BOARD_HEIGHT) return false;

                // Check collision with LOCKED pieces only (ignore ghost dots)
                if (yt >= 0) {
                    char gridCell = board.grid[yt][xt];
                    if (gridCell != ' ' && gridCell != '.') {
                        return false;
                    }
                }
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

    void clearAllGhostDots() {
        // Clear only previously tracked ghost positions (O(n) instead of O(width*height))
        for (const Position& pos : lastGhostPositions) {
            if (board.grid[pos.y][pos.x] == '.') {
                board.grid[pos.y][pos.x] = ' ';
            }
        }
        lastGhostPositions.clear();
    }

    void placeGhostPiece(const Piece& ghostPiece) {
        // Place ghost piece using '.' character for outline effect
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(
                    ghostPiece.type, ghostPiece.rotation, i, j
                );
                if (cell == ' ') continue;

                int xt = ghostPiece.pos.x + j;
                int yt = ghostPiece.pos.y + i;

                if (yt < 0 || yt >= BOARD_HEIGHT ||
                    xt < 0 || xt >= BOARD_WIDTH) {
                    continue;
                }

                // Only draw ghost where there's empty space (don't overwrite actual pieces)
                if (board.grid[yt][xt] == ' ') {
                    board.grid[yt][xt] = '.';
                    lastGhostPositions.push_back(Position(xt, yt));
                }
            }
        }
    }

    void placePieceSafe(const Piece& piece) {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(
                    piece.type, piece.rotation, i, j
                );
                if (cell == ' ') continue;

                int xt = piece.pos.x + j;
                int yt = piece.pos.y + i;

                if (yt < 0 || yt >= BOARD_HEIGHT || xt < 0 || xt >= BOARD_WIDTH) continue;

                if (board.grid[yt][xt] == ' ') {
                    board.grid[yt][xt] = cell;
                }
            }
        }
    }

    void spawnNewPiece() {
        uniform_int_distribution<int> dist(0, NUM_BLOCK_TYPES - 1);

        Piece testPiece;
        testPiece.type = nextPieceType;
        testPiece.rotation = 0;

        int spawnX = (BOARD_WIDTH / 2) - (BLOCK_SIZE / 2);
        testPiece.pos = Position(spawnX, -1);

        currentPiece = testPiece;

        if (!canSpawn(testPiece)) {
            state.running = false;
            return;
        }

        nextPieceType = dist(rng);
    }

    bool lockPieceAndCheck(bool muteLockSound = false) {
        placePiece(currentPiece, true);

        int lines = board.clearLines();
        if (lines > 0) {
            if (lines == 4) {
                SoundManager::play4LinesClearSound();
            } else {
                SoundManager::playLineClearSound();
            }

            state.linesCleared += lines;

            // Scoring rules
            const int scores[] = {0, 100, 300, 500, 800};
            state.score += scores[lines] * state.level;

            int oldLevel = state.level;

            // Level progression: +1 level per 10 lines
            state.level = 1 + (state.linesCleared / 10);

            if (state.level > oldLevel) {
                SoundManager::playLevelUpSound();
            }

            // Update falling speed based on new level
            updateDifficulty();
        } else {
            if (!muteLockSound) { // Only play lock piece sound in gravity case.
                SoundManager::playLockPieceSound();
            }
        }

        spawnNewPiece();
        return state.running;
    }

    void softDrop() {
        if (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        } else {
            if (currentPiece.pos.y < 0) {
                state.running = false;
                return;
            }
            state.running = lockPieceAndCheck(true);
            dropCounter = 0;
        }
    }

    void hardDrop() {
        while (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        }
        if (currentPiece.pos.y < 0) {
            state.running = false;
            return;
        }
        state.running = lockPieceAndCheck(true);
        dropCounter = 0;
    }

    void handleInput() {
        char c = getInput();

        if (c == 's') {
            softDropActive = true;
        } else {
            softDropActive = false;
        }

        if (c == 0) return;

        // Toggle Pause
        if (c == 'p') {
            state.paused = !state.paused;
            flushInput(); // Clear input buffer when toggling pause
            if (state.paused) {
                drawPauseScreen();
            }
            return;
        }

        // Handle ghost toggle (can toggle even when paused)
        if (c == 'g') {
            state.ghostEnabled = !state.ghostEnabled;
            return;
        }

        if (state.paused) {
            if (c == 'q') {
                state.running = false;
                state.quitByUser = true;
                SoundManager::stopBackgroundSound();
            }
            return;
        }

        if (state.paused) {
            if (c == 'q') state.running = false;
            return;
        }

        // Gameplay controls
        switch (c) {
            case 'a':
                if (canMove(-1, 0, currentPiece.rotation)) {
                    currentPiece.pos.x--;
                }
                break;
            case 'd':
                if (canMove(1, 0, currentPiece.rotation)) {
                    currentPiece.pos.x++;
                }
                break;
            case 's':
                // `s` enables soft drop via softDropActive above
                break;
            case 'x': // soft drop one cell
                SoundManager::playSoftDropSound();
                softDrop();
                break;
            case ' ': // hard drop
                SoundManager::playHardDropSound();
                hardDrop();
                flushInput();
                break;
            case 'w': {
                int newRot = (currentPiece.rotation + 1) % 4;
                int kicks[] = {0, -1, 1, -2, 2, -3, 3};
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
                state.quitByUser = true;
                SoundManager::stopBackgroundSound();
                break;
            default:
                break;
        }
    }

    void handleGravity() {
        if (!state.running || state.paused) return;

        ++dropCounter;

        int effectiveInterval = softDropActive ? 1 : DROP_INTERVAL_TICKS;

        if (dropCounter < effectiveInterval) return;

        dropCounter = 0;
        if (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        } else {
            if (currentPiece.pos.y < 0) {
                state.running = false;
                return;
            }
            state.running = lockPieceAndCheck();
        }
    }

    void getNextPiecePreview(string lines[4]) {
        // Use cached preview if next piece hasn't changed
        if (cachedNextPieceType == nextPieceType) {
            for (int i = 0; i < 4; ++i) {
                lines[i] = cachedNextPiecePreview[i];
            }
            return;
        }

        // Render the next piece as 4 lines WITH colors (8 chars wide)
        for (int row = 0; row < 4; ++row) {
            cachedNextPiecePreview[row].clear();
            cachedNextPiecePreview[row].reserve(64); // Pre-allocate for colors + blocks

            // Render the piece blocks
            for (int col = 0; col < 4; ++col) {
                char cell = BlockTemplate::getCell(nextPieceType, 0, row, col);

                if (cell != ' ') {
                    // Show piece block with color
                    cachedNextPiecePreview[row] += PIECE_COLORS[nextPieceType];
                    cachedNextPiecePreview[row].append("██");
                    cachedNextPiecePreview[row] += COLOR_RESET;
                } else {
                    // Empty space
                    cachedNextPiecePreview[row].append("  ");
                }
            }
            lines[row] = cachedNextPiecePreview[row];
        }

        cachedNextPieceType = nextPieceType;
    }

    void run() {
        BlockTemplate::initializeTemplates();

        // Main game loop with restart support
        bool shouldRestart = true;

        while (shouldRestart) {
            board.init();

            uniform_int_distribution<int> dist(0, NUM_BLOCK_TYPES - 1);
            nextPieceType = dist(rng);

            drawStartScreen();
            waitForKeyPress();

            // Play background sound
            SoundManager::playBackgroundSound();

            // initialize speed for starting level
            updateDifficulty();
            spawnNewPiece();

            // --- MAIN GAME LOOP ---
            while (state.running) {
                handleInput();

                if (state.paused) {
                    usleep(100000);
                    continue;
                }

                if (!state.running) break;

                handleGravity();

                // Clear all ghost dots from previous frame
                clearAllGhostDots();

                // Calculate and draw ghost position (if enabled)
                if (state.ghostEnabled) {
                    Piece ghostPiece = calculateGhostPiece();
                    // Only draw ghost if it's different from current piece position
                    if (ghostPiece.pos.y != currentPiece.pos.y) {
                        placeGhostPiece(ghostPiece);
                    }
                }

                placePiece(currentPiece, true);

                string preview[4];
                getNextPiecePreview(preview);
                board.draw(state, preview);

                placePiece(currentPiece, false);

                usleep(dropSpeedUs / DROP_INTERVAL_TICKS);
            }

            if (!state.quitByUser) {
                placePieceSafe(currentPiece);

                string preview[4];
                getNextPiecePreview(preview);
                board.draw(state, preview);

                flushInput();
                usleep(800000);
                flushInput();

                animateGameOver();
            }

            // Stop background sound
            SoundManager::stopBackgroundSound();

            // Show game over screen and wait for user choice
            int rank = saveAndGetRank();
            loadHighScores(); // Reload scores to display updated leaderboard
            drawGameOverScreen(rank);

            char choice = waitForKeyPress();

            if (choice == 'r' || choice == 'R') {
                // Restart the game
                resetGame();
                shouldRestart = true;
            } else {
                // Quit the game
                shouldRestart = false;
            }

            disableRawMode();
        }
    }

    long computeDropSpeedUs(int level) const {
        if (level <= 3) {            // Levels 1–3: Slow
            return 500000;           // 0.50s per tick group
        } else if (level <= 6) {     // Levels 4–6: Medium
            return 300000;           // 0.30s
        } else if (level <= 9) {     // Levels 7–9: Fast
            return 150000;           // 0.15s
        } else {                     // Level 10+
            return 80000;            // 0.08s
        }
    }

    void updateDifficulty() {
        dropSpeedUs = computeDropSpeedUs(state.level);
    }

    void drawPauseScreen() const {
        // Build pause overlay in a string buffer
        string screen;
        screen.reserve(1024);

        // Clear screen + move cursor to top-left
        screen += "\033[2J\033[1;1H";

        // Match the visual width of the game board
        int totalWidth = (BOARD_WIDTH * 2) + 13;

        // Top border
        screen += "╔";
        for (int i = 0; i < totalWidth; i++) screen += "═";
        screen += "╗\n";

        // Empty rows for spacing
        for (int i = 0; i < 3; ++i) {
            screen += "║";
            screen.append(totalWidth, ' ');
            screen += "║\n";
        }

        // "GAME PAUSED" title
        const string title = "GAME PAUSED";
        int titlePadding = totalWidth - title.length();
        int titleLeft = titlePadding / 2;
        int titleRight = titlePadding - titleLeft;

        screen += "║";
        screen.append(titleLeft, ' ');
        screen += title;
        screen.append(titleRight, ' ');
        screen += "║\n";

        // Empty row
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";

        // Current stats - Score
        char scoreBuf[64];
        snprintf(scoreBuf, sizeof(scoreBuf), "Score: %d", state.score);
        string scoreStr(scoreBuf);
        int scorePadding = totalWidth - scoreStr.length();
        int scoreLeft = scorePadding / 2;
        int scoreRight = scorePadding - scoreLeft;

        screen += "║";
        screen.append(scoreLeft, ' ');
        screen += scoreStr;
        screen.append(scoreRight, ' ');
        screen += "║\n";

        // Level
        char levelBuf[64];
        snprintf(levelBuf, sizeof(levelBuf), "Level: %d", state.level);
        string levelStr(levelBuf);
        int levelPadding = totalWidth - levelStr.length();
        int levelLeft = levelPadding / 2;
        int levelRight = levelPadding - levelLeft;

        screen += "║";
        screen.append(levelLeft, ' ');
        screen += levelStr;
        screen.append(levelRight, ' ');
        screen += "║\n";

        // Lines
        char linesBuf[64];
        snprintf(linesBuf, sizeof(linesBuf), "Lines: %d", state.linesCleared);
        string linesStr(linesBuf);
        int linesPadding = totalWidth - linesStr.length();
        int linesLeft = linesPadding / 2;
        int linesRight = linesPadding - linesLeft;

        screen += "║";
        screen.append(linesLeft, ' ');
        screen += linesStr;
        screen.append(linesRight, ' ');
        screen += "║\n";

        // Empty row
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";

        // Menu options
        const string resumeOption = "P - Resume";
        int resumePadding = totalWidth - resumeOption.length();
        int resumeLeft = resumePadding / 2;
        int resumeRight = resumePadding - resumeLeft;

        screen += "║";
        screen.append(resumeLeft, ' ');
        screen += resumeOption;
        screen.append(resumeRight, ' ');
        screen += "║\n";

        const string quitOption = "Q - Quit";
        int quitPadding = totalWidth - quitOption.length();
        int quitLeft = quitPadding / 2;
        int quitRight = quitPadding - quitLeft;

        screen += "║";
        screen.append(quitLeft, ' ');
        screen += quitOption;
        screen.append(quitRight, ' ');
        screen += "║\n";

        // Empty rows for spacing
        for (int i = 0; i < 3; ++i) {
            screen += "║";
            screen.append(totalWidth, ' ');
            screen += "║\n";
        }

        // Bottom border
        screen += "╚";
        for (int i = 0; i < totalWidth; i++) screen += "═";
        screen += "╝\n";

        // Single output call
        cout << screen;
        cout.flush();
    }
};

int main() {
    TetrisGame game;
    game.run();
    return 0;
}
