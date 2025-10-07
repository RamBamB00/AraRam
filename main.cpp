#include <GLFW/glfw3.h>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <iostream>

// Simple 2D Tetris implemented in a single file using OpenGL immediate mode.
// No right-side bar; score is shown in the window title. Press R to restart.

struct Color { float r, g, b; };

struct Vec2I { int x, y; };

// Board size
static const int BOARD_COLS = 10;
static const int BOARD_ROWS = 20;

// Cell size in pixels
static const int CELL_SIZE = 24;

// Active piece data
struct Piece {
    // 4 rotation states, each state has 4 blocks (x, y) in local coords
    std::array<std::array<Vec2I, 4>, 4> rotations;
    Color color;
};

// Define the 7 Tetrominoes (I, O, T, S, Z, J, L)
static const std::array<Piece, 7> TETROMINOES = {{
    // I
    Piece{
        std::array<std::array<Vec2I, 4>, 4>{
            std::array<Vec2I, 4>{{ {0,1}, {1,1}, {2,1}, {3,1} }},
            std::array<Vec2I, 4>{{ {2,0}, {2,1}, {2,2}, {2,3} }},
            std::array<Vec2I, 4>{{ {0,2}, {1,2}, {2,2}, {3,2} }},
            std::array<Vec2I, 4>{{ {1,0}, {1,1}, {1,2}, {1,3} }}
        },
        Color{0.0f, 1.0f, 1.0f}
    },
    // O
    Piece{
        std::array<std::array<Vec2I, 4>, 4>{
            std::array<Vec2I, 4>{{ {1,1}, {2,1}, {1,2}, {2,2} }} ,
            std::array<Vec2I, 4>{{ {1,1}, {2,1}, {1,2}, {2,2} }} ,
            std::array<Vec2I, 4>{{ {1,1}, {2,1}, {1,2}, {2,2} }} ,
            std::array<Vec2I, 4>{{ {1,1}, {2,1}, {1,2}, {2,2} }}
        },
        Color{1.0f, 1.0f, 0.0f}
    },
    // T
    Piece{
        std::array<std::array<Vec2I, 4>, 4>{
            std::array<Vec2I, 4>{{ {1,1}, {0,1}, {2,1}, {1,2} }},
            std::array<Vec2I, 4>{{ {1,1}, {1,0}, {1,2}, {2,1} }},
            std::array<Vec2I, 4>{{ {1,1}, {0,1}, {2,1}, {1,0} }},
            std::array<Vec2I, 4>{{ {1,1}, {1,0}, {1,2}, {0,1} }}
        },
        Color{0.7f, 0.0f, 1.0f}
    },
    // S
    Piece{
        std::array<std::array<Vec2I, 4>, 4>{
            std::array<Vec2I, 4>{{ {1,1}, {2,1}, {0,2}, {1,2} }} ,
            std::array<Vec2I, 4>{{ {1,0}, {1,1}, {2,1}, {2,2} }} ,
            std::array<Vec2I, 4>{{ {1,1}, {2,1}, {0,2}, {1,2} }} ,
            std::array<Vec2I, 4>{{ {1,0}, {1,1}, {2,1}, {2,2} }}
        },
        Color{0.0f, 1.0f, 0.0f}
    },
    // Z
    Piece{
        std::array<std::array<Vec2I, 4>, 4>{
            std::array<Vec2I, 4>{{ {0,1}, {1,1}, {1,2}, {2,2} }},
            std::array<Vec2I, 4>{{ {2,0}, {1,1}, {2,1}, {1,2} }},
            std::array<Vec2I, 4>{{ {0,1}, {1,1}, {1,2}, {2,2} }},
            std::array<Vec2I, 4>{{ {2,0}, {1,1}, {2,1}, {1,2} }}
        },
        Color{1.0f, 0.0f, 0.0f}
    },
    // J
    Piece{
        std::array<std::array<Vec2I, 4>, 4>{
            std::array<Vec2I, 4>{{ {0,1}, {0,2}, {1,1}, {2,1} }},
            std::array<Vec2I, 4>{{ {1,0}, {2,0}, {1,1}, {1,2} }},
            std::array<Vec2I, 4>{{ {0,1}, {1,1}, {2,1}, {2,0} }},
            std::array<Vec2I, 4>{{ {1,0}, {1,1}, {1,2}, {0,2} }}
        },
        Color{0.0f, 0.0f, 1.0f}
    },
    // L
    Piece{
        std::array<std::array<Vec2I, 4>, 4>{
            std::array<Vec2I, 4>{{ {2,1}, {0,1}, {1,1}, {2,2} }},
            std::array<Vec2I, 4>{{ {1,0}, {1,1}, {1,2}, {2,2} }},
            std::array<Vec2I, 4>{{ {0,0}, {1,1}, {2,1}, {0,1} }},
            std::array<Vec2I, 4>{{ {0,0}, {1,0}, {1,1}, {1,2} }}
        },
        Color{1.0f, 0.5f, 0.0f}
    }
}};

struct GameState {
    // Board stores -1 for empty, or 0..6 for tetromino index
    std::array<int, BOARD_COLS * BOARD_ROWS> board;
    int currentPieceIdx;
    int rotation;
    int posX; // top-left origin of piece local grid
    int posY;
    int score;
    bool gameOver;

    // input edge detection
    bool prevLeft;
    bool prevRight;
    bool prevDown;
    bool prevUp;
    bool prevSpace;
    bool prevR;
};

static inline int idx(int x, int y) { return y * BOARD_COLS + x; }

static void resetGame(GameState &g) {
    std::fill(g.board.begin(), g.board.end(), -1);
    g.score = 0;
    g.gameOver = false;
    g.prevLeft = g.prevRight = g.prevDown = g.prevUp = g.prevSpace = g.prevR = false;
    g.currentPieceIdx = std::rand() % 7;
    g.rotation = 0;
    g.posX = 3;
    g.posY = 0;
}

static bool collides(const GameState &g, int pieceIdx, int rot, int ox, int oy) {
    const auto &shape = TETROMINOES[pieceIdx].rotations[rot];
    for (const auto &b : shape) {
        int x = ox + b.x;
        int y = oy + b.y;
        if (x < 0 || x >= BOARD_COLS || y < 0 || y >= BOARD_ROWS) return true;
        if (g.board[idx(x, y)] != -1) return true;
    }
    return false;
}

static void lockPiece(GameState &g) {
    const auto &shape = TETROMINOES[g.currentPieceIdx].rotations[g.rotation];
    for (const auto &b : shape) {
        int x = g.posX + b.x;
        int y = g.posY + b.y;
        if (y >= 0 && y < BOARD_ROWS && x >= 0 && x < BOARD_COLS) {
            g.board[idx(x, y)] = g.currentPieceIdx;
        }
    }
}

static int clearLines(GameState &g) {
    int cleared = 0;
    for (int y = 0; y < BOARD_ROWS; ++y) {
        bool full = true;
        for (int x = 0; x < BOARD_COLS; ++x) {
            if (g.board[idx(x, y)] == -1) { full = false; break; }
        }
        if (full) {
            // move all rows above down
            for (int yy = y; yy > 0; --yy) {
                for (int x = 0; x < BOARD_COLS; ++x) {
                    g.board[idx(x, yy)] = g.board[idx(x, yy - 1)];
                }
            }
            for (int x = 0; x < BOARD_COLS; ++x) g.board[idx(x, 0)] = -1;
            ++cleared;
        }
    }
    // scoring: 40/100/300/1200 like classic (single/double/triple/tetris)
    switch (cleared) {
        case 1: g.score += 40; break;
        case 2: g.score += 100; break;
        case 3: g.score += 300; break;
        case 4: g.score += 1200; break;
        default: break;
    }
    return cleared;
}

static void spawnPiece(GameState &g) {
    g.currentPieceIdx = std::rand() % 7;
    g.rotation = 0;
    g.posX = 3;
    g.posY = 0;
    if (collides(g, g.currentPieceIdx, g.rotation, g.posX, g.posY)) {
        g.gameOver = true;
    }
}

static void drawCell(float x, float y, float size, const Color &c) {
    glColor3f(c.r, c.g, c.b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + size, y);
    glVertex2f(x + size, y + size);
    glVertex2f(x, y + size);
    glEnd();

    // border
    glColor3f(0.05f, 0.05f, 0.05f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + size, y);
    glVertex2f(x + size, y + size);
    glVertex2f(x, y + size);
    glEnd();
}

static void renderGame(const GameState &g, int winW, int winH) {
    glViewport(0, 0, winW, winH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, winW, winH, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const int boardPixelW = BOARD_COLS * CELL_SIZE;
    const int boardPixelH = BOARD_ROWS * CELL_SIZE;
    const float originX = (winW - boardPixelW) * 0.5f;
    const float originY = (winH - boardPixelH) * 0.5f;

    // draw board cells
    for (int y = 0; y < BOARD_ROWS; ++y) {
        for (int x = 0; x < BOARD_COLS; ++x) {
            int v = g.board[idx(x, y)];
            Color c = v == -1 ? Color{0.16f, 0.16f, 0.20f} : TETROMINOES[static_cast<size_t>(v)].color;
            drawCell(originX + x * CELL_SIZE, originY + y * CELL_SIZE, (float)CELL_SIZE, c);
        }
    }

    // draw active piece
    if (!g.gameOver) {
        const auto &shape = TETROMINOES[g.currentPieceIdx].rotations[g.rotation];
        const Color c = TETROMINOES[g.currentPieceIdx].color;
        for (const auto &b : shape) {
            float px = originX + (g.posX + b.x) * CELL_SIZE;
            float py = originY + (g.posY + b.y) * CELL_SIZE;
            drawCell(px, py, (float)CELL_SIZE, c);
        }
    }
}

static void updateWindowTitle(GLFWwindow *window, int score, bool gameOver) {
    std::string title = std::string("TetrisGL - Score: ") + std::to_string(score);
    if (gameOver) title += "  [Game Over - Press R to Restart]";
    glfwSetWindowTitle(window, title.c_str());
}

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "TetrisGL", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GameState g;
    g.board.fill(-1);
    g.score = 0;
    g.gameOver = false;
    g.prevLeft = g.prevRight = g.prevDown = g.prevUp = g.prevSpace = g.prevR = false;
    g.currentPieceIdx = std::rand() % 7;
    g.rotation = 0;
    g.posX = 3; g.posY = 0;

    double lastTime = glfwGetTime();
    double fallAccumulator = 0.0;
    const double baseFallInterval = 0.6; // seconds per cell at level 1
    const double softDropMultiplier = 0.08; // faster when holding down

    updateWindowTitle(window, g.score, g.gameOver);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int winW, winH;
        glfwGetFramebufferSize(window, &winW, &winH);

        // Input edge detection
        bool left  = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
        bool right = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
        bool up    = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;      // rotate
        bool down  = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;    // soft drop
        bool space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;   // hard drop
        bool keyR  = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;       // restart

        if (g.gameOver) {
            if (keyR && !g.prevR) {
                resetGame(g);
                updateWindowTitle(window, g.score, g.gameOver);
            }
            // still render background grid
            renderGame(g, winW, winH);
            glfwSwapBuffers(window);
            g.prevR = keyR;
            continue;
        }

        // Movement left/right with edge detection
        if (left && !g.prevLeft) {
            if (!collides(g, g.currentPieceIdx, g.rotation, g.posX - 1, g.posY)) g.posX -= 1;
        }
        if (right && !g.prevRight) {
            if (!collides(g, g.currentPieceIdx, g.rotation, g.posX + 1, g.posY)) g.posX += 1;
        }

        // Rotation (simple wall-kick: try shift left/right if needed)
        if (up && !g.prevUp) {
            int nextRot = (g.rotation + 1) % 4;
            if (!collides(g, g.currentPieceIdx, nextRot, g.posX, g.posY)) {
                g.rotation = nextRot;
            } else if (!collides(g, g.currentPieceIdx, nextRot, g.posX - 1, g.posY)) {
                g.posX -= 1; g.rotation = nextRot;
            } else if (!collides(g, g.currentPieceIdx, nextRot, g.posX + 1, g.posY)) {
                g.posX += 1; g.rotation = nextRot;
            }
        }

        // Time step
        double now = glfwGetTime();
        double dt = now - lastTime;
        lastTime = now;
        fallAccumulator += dt;

        double interval = baseFallInterval;
        if (down) interval *= softDropMultiplier;

        // Hard drop
        if (space && !g.prevSpace) {
            int steps = 0;
            while (!collides(g, g.currentPieceIdx, g.rotation, g.posX, g.posY + 1)) {
                g.posY += 1; ++steps;
            }
            // small reward per cell hard-dropped
            g.score += steps;
            lockPiece(g);
            clearLines(g);
            spawnPiece(g);
            updateWindowTitle(window, g.score, g.gameOver);
        }

        // Gravity
        while (fallAccumulator >= interval) {
            fallAccumulator -= interval;
            if (!collides(g, g.currentPieceIdx, g.rotation, g.posX, g.posY + 1)) {
                g.posY += 1;
            } else {
                lockPiece(g);
                clearLines(g);
                spawnPiece(g);
                updateWindowTitle(window, g.score, g.gameOver);
            }
        }

        // Render
        renderGame(g, winW, winH);
        glfwSwapBuffers(window);

        // Update previous input states
        g.prevLeft = left; g.prevRight = right; g.prevDown = down; g.prevUp = up; g.prevSpace = space; g.prevR = keyR;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
