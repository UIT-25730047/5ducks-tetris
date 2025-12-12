// [VERSION 2: NAIVE RESET]
// Copy toàn bộ code này vào file main.cpp
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <random>

using namespace std;

constexpr int BOARD_HEIGHT      = 20;
constexpr int BOARD_WIDTH       = 15;
constexpr int BLOCK_SIZE        = 4;
constexpr int NUM_BLOCK_TYPES   = 7;
constexpr long BASE_DROP_SPEED_US   = 500000;
constexpr int  DROP_INTERVAL_TICKS  = 5;

struct Position { int x{}, y{}; Position() = default; Position(int _x, int _y) : x(_x), y(_y) {} };

struct GameState {
    bool running{true};
    bool paused{false};
};

struct Piece { int type{0}; int rotation{0}; Position pos{5, 0}; };

struct Board {
    char grid[BOARD_HEIGHT][BOARD_WIDTH]{};
    void init() {
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                grid[i][j] = (i == 0 || i == BOARD_HEIGHT - 1 || j == 0 || j == BOARD_WIDTH - 1) ? '#' : ' ';
            }
        }
    }
    void draw(const GameState& state) const {
        cout << "\033[2J\033[1;1H";
        cout << "=== VERSION 2: NAIVE RESET ===\n"; // Marker
        cout << "Controls: A/D=Move W=Rotate SPACE=Drop P=Pause R=Reset Q=Quit\n\n";
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH; ++j) cout << grid[i][j];
            cout << "\n";
        }
        cout.flush();
    }
    void drawPause() const {
        cout << "\033[2J\033[1;1H\n\n   === PAUSED ===\n   Press P to Resume\n"; cout.flush();
    }
    void clearLines() { /* Logic xóa dòng cơ bản */
        int writeRow = BOARD_HEIGHT - 2;
        for (int readRow = BOARD_HEIGHT - 2; readRow > 0; --readRow) {
            bool full = true;
            for (int j = 1; j < BOARD_WIDTH - 1; ++j) if (grid[readRow][j] == ' ') { full = false; break; }
            if (!full) {
                if (writeRow != readRow) for (int j = 1; j < BOARD_WIDTH - 1; ++j) grid[writeRow][j] = grid[readRow][j];
                --writeRow;
            }
        }
        while (writeRow > 0) { for (int j = 1; j < BOARD_WIDTH - 1; ++j) grid[writeRow][j] = ' '; --writeRow; }
    }
};

struct BlockTemplate {
    static char templates[NUM_BLOCK_TYPES][BLOCK_SIZE][BLOCK_SIZE];
    static void initializeTemplates() {
         static const int TETROMINOES[7][4][4] = {
            {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}, // I
            {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}}, // O
            {{0,0,0,0},{0,1,0,0},{1,1,1,0},{0,0,0,0}}, // T
            {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}}, // S
            {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}}, // Z
            {{0,0,0,0},{1,0,0,0},{1,1,1,0},{0,0,0,0}}, // J
            {{0,0,0,0},{0,0,1,0},{1,1,1,0},{0,0,0,0}}  // L
        };
        static const char NAMES[7] = {'I','O','T','S','Z','J','L'};
        for(int t=0; t<7; t++) for(int i=0; i<4; i++) for(int j=0; j<4; j++) templates[t][i][j] = TETROMINOES[t][i][j] ? NAMES[t] : ' ';
    }
    static char getCell(int type, int rotation, int row, int col) {
        int r = row, c = col;
        for (int i = 0; i < rotation; ++i) { int temp = 3 - c; c = r; r = temp; }
        return templates[type][r][c];
    }
};
char BlockTemplate::templates[NUM_BLOCK_TYPES][BLOCK_SIZE][BLOCK_SIZE];

struct TetrisGame {
    Board board; GameState state; Piece currentPiece{};
    termios origTermios{}; long dropSpeedUs{BASE_DROP_SPEED_US}; int dropCounter{0}; std::mt19937 rng;

    TetrisGame() { std::random_device rd; rng.seed(rd()); }
    void enableRawMode() { tcgetattr(STDIN_FILENO, &origTermios); termios raw = origTermios; raw.c_lflag &= ~(ICANON | ECHO); raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0; tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); int flags = fcntl(STDIN_FILENO, F_GETFL, 0); fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK); }
    void disableRawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios); }
    char getInput() const { char ch = 0; if(read(STDIN_FILENO, &ch, 1) > 0) return ch; return 0; }
    void flushInput() const { tcflush(STDIN_FILENO, TCIFLUSH); }

    bool isInsidePlayfield(int x, int y) const { return x >= 1 && x < BOARD_WIDTH - 1 && y >= 0 && y < BOARD_HEIGHT - 1; }
    bool canSpawn(const Piece& p) const {
        for(int i=0; i<4; i++) for(int j=0; j<4; j++) {
            if(BlockTemplate::getCell(p.type, p.rotation, i, j) != ' ') {
                if((p.pos.y+i) >= 0 && !isInsidePlayfield(p.pos.x+j, p.pos.y+i)) return false;
            }
        }
        return true;
    }
    bool canMove(int dx, int dy, int rot) const {
        for(int i=0; i<4; i++) for(int j=0; j<4; j++) {
            if(BlockTemplate::getCell(currentPiece.type, rot, i, j) != ' ') {
                int nx = currentPiece.pos.x+j+dx, ny = currentPiece.pos.y+i+dy;
                if(nx < 1 || nx >= BOARD_WIDTH-1 || ny >= BOARD_HEIGHT-1) return false;
                if(ny >= 0 && board.grid[ny][nx] != ' ') return false;
            }
        }
        return true;
    }
    void placePiece(const Piece& p, bool place) {
        for(int i=0; i<4; i++) for(int j=0; j<4; j++) {
            char c = BlockTemplate::getCell(p.type, p.rotation, i, j);
            if(c != ' ') {
                int nx = p.pos.x+j, ny = p.pos.y+i;
                if(nx>=0 && nx<BOARD_WIDTH && ny>=0 && ny<BOARD_HEIGHT) board.grid[ny][nx] = place ? c : ' ';
            }
        }
    }
    void spawnNewPiece() {
        std::uniform_int_distribution<int> dist(0, NUM_BLOCK_TYPES - 1);
        currentPiece.type = dist(rng); currentPiece.rotation = 0;
        currentPiece.pos = Position((BOARD_WIDTH/2)-(BLOCK_SIZE/2), -1);
        if (!canSpawn(currentPiece)) state.running = false;
    }

    void handleInput() {
        char c = getInput();
        if (c == 0) return;
        
        // [VER 2 Logic] - Ngây thơ: Chỉ set running = true
        // LỖI: Không xóa bàn cờ cũ -> Game over lại ngay lập tức
        if (c == 'r') {
            state.running = true; 
            return;
        }

        if (c == 'p') { state.paused = !state.paused; if (state.paused) board.drawPause(); return; }
        if (state.paused) { if (c == 'q') state.running = false; return; }

        switch (c) {
            case 'a': if(canMove(-1,0,currentPiece.rotation)) currentPiece.pos.x--; break;
            case 'd': if(canMove(1,0,currentPiece.rotation)) currentPiece.pos.x++; break;
            case 'x': if(canMove(0,1,currentPiece.rotation)) currentPiece.pos.y++; else { placePiece(currentPiece,true); board.clearLines(); spawnNewPiece(); } break;
            case ' ': while(canMove(0,1,currentPiece.rotation)) currentPiece.pos.y++; placePiece(currentPiece,true); board.clearLines(); spawnNewPiece(); flushInput(); break;
            case 'w': { int nr=(currentPiece.rotation+1)%4; if(canMove(0,0,nr)) currentPiece.rotation=nr; else if(canMove(-1,0,nr)){currentPiece.pos.x--; currentPiece.rotation=nr;} else if(canMove(1,0,nr)){currentPiece.pos.x++; currentPiece.rotation=nr;} break; }
            case 'q': state.running = false; break;
        }
    }

    void run() {
        BlockTemplate::initializeTemplates(); board.init(); enableRawMode(); spawnNewPiece();
        while (state.running) {
            handleInput();
            if (state.paused) { usleep(100000); continue; }
            dropCounter++;
            if (dropCounter >= DROP_INTERVAL_TICKS) {
                dropCounter=0;
                if(canMove(0,1,currentPiece.rotation)) currentPiece.pos.y++;
                else { placePiece(currentPiece,true); board.clearLines(); spawnNewPiece(); }
            }
            placePiece(currentPiece,true); board.draw(state); placePiece(currentPiece,false);
            usleep(dropSpeedUs/DROP_INTERVAL_TICKS);
        }
        disableRawMode(); cout << "Game Over. (Thu bam R o lan chay sau de thay loi Ver 2)\n";
    }
};

int main() { TetrisGame game; game.run(); return 0; }
