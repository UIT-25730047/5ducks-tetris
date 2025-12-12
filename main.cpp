// [VERSION 5: INFINITE LOOP]
// Description: The game automatically restarts immediately after Game Over.
// It fixes the crash from Version 4 but introduces a bad User Experience (UX).

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <random>

using namespace std;

// Constants (kept same as previous versions)
constexpr int BOARD_HEIGHT = 20; 
constexpr int BOARD_WIDTH = 15; 
constexpr int BLOCK_SIZE = 4; 
constexpr int NUM_BLOCK_TYPES = 7;
constexpr long BASE_DROP_SPEED_US = 500000; 
constexpr int DROP_INTERVAL_TICKS = 5;

struct Position { int x{}, y{}; Position() = default; Position(int _x, int _y) : x(_x), y(_y) {} };
struct GameState { bool running{true}; bool paused{false}; int score{0}; }; 
struct Piece { int type{0}; int rotation{0}; Position pos{5, 0}; };

// Board and Template structs (Identical to previous versions)
struct Board {
    char grid[BOARD_HEIGHT][BOARD_WIDTH]{};
    void init() { for(int i=0;i<BOARD_HEIGHT;++i) for(int j=0;j<BOARD_WIDTH;++j) grid[i][j]=(i==0||i==BOARD_HEIGHT-1||j==0||j==BOARD_WIDTH-1)?'#':' '; }
    void draw(const GameState& s) const { 
        cout << "\033[2J\033[1;1H === VERSION 5: INFINITE LOOP ===\n(Status: Die -> Auto Restart)\n"; 
        for(int i=0;i<BOARD_HEIGHT;++i){for(int j=0;j<BOARD_WIDTH;++j)cout<<grid[i][j];cout<<"\n";} 
        cout.flush(); 
    }
    void clearLines() { int w=BOARD_HEIGHT-2; for(int r=BOARD_HEIGHT-2;r>0;--r){bool f=true;for(int j=1;j<BOARD_WIDTH-1;++j)if(grid[r][j]==' ')f=false; if(!f){if(w!=r)for(int j=1;j<BOARD_WIDTH-1;++j)grid[w][j]=grid[r][j]; --w;} } while(w>0){for(int j=1;j<BOARD_WIDTH-1;++j)grid[w][j]=' '; --w;} }
};

struct BlockTemplate {
    static char templates[NUM_BLOCK_TYPES][BLOCK_SIZE][BLOCK_SIZE];
    static void initializeTemplates() { 
        static const int TET[7][4][4]={{{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}},{{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},{{0,0,0,0},{0,1,0,0},{1,1,1,0},{0,0,0,0}},{{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},{{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},{{0,0,0,0},{1,0,0,0},{1,1,1,0},{0,0,0,0}},{{0,0,0,0},{0,0,1,0},{1,1,1,0},{0,0,0,0}}};
        static const char N[7]={'I','O','T','S','Z','J','L'};
        for(int t=0;t<7;t++) for(int i=0;i<4;i++) for(int j=0;j<4;j++) templates[t][i][j]=TET[t][i][j]?N[t]:' ';
    }
    static char getCell(int t, int r, int row, int col) { int rw=row, cl=col; for(int i=0;i<r;i++){int tm=3-cl;cl=rw;rw=tm;} return templates[t][rw][cl]; }
};
char BlockTemplate::templates[NUM_BLOCK_TYPES][BLOCK_SIZE][BLOCK_SIZE];

struct TetrisGame {
    Board board; GameState state; Piece currentPiece{};
    termios origTermios{}; long dropSpeedUs{BASE_DROP_SPEED_US}; int dropCounter{0}; std::mt19937 rng;
    TetrisGame() { std::random_device rd; rng.seed(rd()); }

    // Terminal handling
    void enableRawMode() { tcgetattr(STDIN_FILENO, &origTermios); termios r=origTermios; r.c_lflag&=~(ICANON|ECHO); r.c_cc[VMIN]=0; r.c_cc[VTIME]=0; tcsetattr(STDIN_FILENO, TCSAFLUSH, &r); int f=fcntl(STDIN_FILENO,F_GETFL,0); fcntl(STDIN_FILENO,F_SETFL,f|O_NONBLOCK); }
    void disableRawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios); }
    char getInput() const { char c=0; if(read(STDIN_FILENO,&c,1)>0) return c; return 0; }
    void flushInput() const { tcflush(STDIN_FILENO, TCIFLUSH); }

    bool canMove(int dx, int dy, int rot) const { for(int i=0;i<4;i++) for(int j=0;j<4;j++) if(BlockTemplate::getCell(currentPiece.type, rot, i, j)!=' ') { int nx=currentPiece.pos.x+j+dx, ny=currentPiece.pos.y+i+dy; if(nx<1||nx>=BOARD_WIDTH-1||ny>=BOARD_HEIGHT-1) return false; if(ny>=0 && board.grid[ny][nx]!=' ') return false; } return true; }
    void placePiece(const Piece& p, bool place) { for(int i=0;i<4;i++) for(int j=0;j<4;j++) { char c=BlockTemplate::getCell(p.type,p.rotation,i,j); if(c!=' ' && p.pos.y+i>=0) board.grid[p.pos.y+i][p.pos.x+j]=place?c:' '; } }
    void spawnNewPiece() { std::uniform_int_distribution<int> d(0,6); currentPiece={d(rng),0,{5,-1}}; if(!canMove(0,0,0)) state.running=false; }

    // [VER 5 Logic] - The Logic is now correct (Version 3 bug fixed)
    // This correctly resets the board, score, and speed.
    void resetGame() {
        board.init();
        state.running = true;
        state.score = 0; // Fix: Score is now reset properly
        dropSpeedUs = BASE_DROP_SPEED_US;
        spawnNewPiece();
        cout << "\033[2J\033[1;1H RESTARTING..."; usleep(500000);
    }

    void handleInput() {
        char c = getInput(); 
        // Logic: Press R to restart mid-game
        if(c=='r') { resetGame(); return true; } 
        // Force exit because we are inside a while(true) loop
        if(c=='q') exit(0); 
        
        switch(c) {
            case 'a': if(canMove(-1,0,currentPiece.rotation)) currentPiece.pos.x--; break;
            case 'd': if(canMove(1,0,currentPiece.rotation)) currentPiece.pos.x++; break;
            case ' ': while(canMove(0,1,currentPiece.rotation)) currentPiece.pos.y++; placePiece(currentPiece,true); board.clearLines(); spawnNewPiece(); break;
        }
    }

    void run() {
        BlockTemplate::initializeTemplates(); board.init(); enableRawMode(); spawnNewPiece();
        
        // [VER 5 Loop] - The Infinite Outer Loop
        // Used to prevent the application from closing when state.running becomes false.
        while (true) {
            
            // Inner Game Loop
            while (state.running) {
                handleInput();
                dropCounter++;
                if (dropCounter >= DROP_INTERVAL_TICKS) {
                    dropCounter = 0;
                    if (canMove(0,1,currentPiece.rotation)) currentPiece.pos.y++;
                    else { placePiece(currentPiece,true); board.clearLines(); spawnNewPiece(); }
                }
                placePiece(currentPiece,true); board.draw(state); placePiece(currentPiece,false);
                usleep(dropSpeedUs/DROP_INTERVAL_TICKS);
            }

            // [CRITICAL UX ISSUE] 
            // The code reaches here when the game is over (state.running == false).
            // BUG: It does NOT wait for user input (like 'Press R to Restart').
            // Result: The game restarts immediately. The user never sees the "Game Over" state.
            resetGame(); 
        }
        disableRawMode();
    }
};

int main() { TetrisGame game; game.run(); return 0; }
