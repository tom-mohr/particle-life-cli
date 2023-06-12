#define DEFAULT_W 80
#define DEFAULT_H 24

#define DEFAULT_N 400
#define DEFAULT_M 6
#define DEFAULT_RADIUS 0.04
#define DEFAULT_DT 0.02
#define DEFAULT_POSITION_MODE 2
#define DEFAULT_MATRIX_MODE 1
#define DEFAULT_COLOR_MODE 1
#define DEFAULT_DENSITY_CHARS ".:oO80@"
#define DEFAULT_STEPS_PER_FRAME 10

#define MAX_WAIT_ARG_LEN 10
#define NUM_POSITION_MODES 4
#define NUM_MATRIX_MODES 2
#define NUM_COLOR_MODES 1  // not counting "0" mode

#define CHAR_RATIO 2.0f

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#ifdef _WIN32
    #include "curses.h"
    #include "getopt.h"
#elif __unix__
    #include <curses.h>
#endif

#include <unistd.h>
#include <string.h>


void print_usage() {
    printf("Usage:\n");
    printf("  particle-life [options]\n");
}

void print_help() {
    print_usage();
    printf("\nby Tom Mohr (github.com/tom-mohr)\n");
    printf("\nOptions:\n");
    printf("  -n <n>              number of particles (default: %d)\n", DEFAULT_N);
    printf("  -m <m>              number of colors (default: %d)\n", DEFAULT_M);
    printf("  -a <mode>           attraction mode (default: %d)\n", DEFAULT_MATRIX_MODE);
    printf("                          1: random\n");
    printf("                          2: snakes\n");
    printf("  -r <distance>       maximum interaction radius (default: %.2f)\n", DEFAULT_RADIUS);
    printf("  -t <seconds>        delta time in seconds (default: %.2f)\n", DEFAULT_DT);
    printf("  -p <mode>           position mode (default: %d)\n", DEFAULT_POSITION_MODE);
    printf("                          1: uniform\n");
    printf("                          2: centered\n");
    printf("                          3: horizontal\n");
    printf("  -z <zoom>           set zoom to float value\n");
    printf("  -i                  show interface\n");
    printf("  -x <string>         characters to represent density (default: \"%s\")\n", DEFAULT_DENSITY_CHARS);
    printf("  -s <seed>           random seed (non-negative integer)\n");
    printf("  -c <mode>           color mode (default: %d)\n", DEFAULT_COLOR_MODE);
    printf("                          0: black and white\n");
    printf("                          1: color\n");
    printf("  -d                  disables clearing of the render buffer\n");
    printf("  -o                  output to stdout (disables GUI)\n");
    printf("  -O                  like -o, but draws the frames in-place\n");
    printf("  -q                  quit after the first rendered frame\n");
    printf("  -W <width>          set the height of the text output\n");
    printf("  -H <height>         set the width of the text output\n");
    printf("  -P                  launch paused\n");
    printf("  -k <int>            steps per frame (default: %d)\n", DEFAULT_STEPS_PER_FRAME);
    printf("  -K <int>            frames to render silently before start (default: 0)\n");
    printf("  -h                  print this help message\n");
}


typedef struct {
    int type;
    float x;
    float y;
    float vx;
    float vy;
} Particle;

typedef struct {
    float rMax;
    float frictionHalfLife;
    float forceFactor;
    float dt;
    int n;
    Particle *particles;
    int gridSize;
    int *grid;
    int *gridMap;
    int m;
    float *matrix;
} ParticleSystem;

typedef struct {
    bool printInPlace;
    bool printInPlaceIsFirstFrame;
    int w;
    int h;
    float zoom;
    float shiftX;
    float shiftY;
    bool pause;
    char* densityChars;
    bool showInfo;
    bool showDebug;
    bool clear;
    int colorMode;
} UiSettings;


float randFloat() {
    return (float) rand() / (float) RAND_MAX;
}


float force(float r, float a) {
    const float beta = 0.3;
    if (r < beta) {
        return r / beta - 1;
    } else if (beta < r && r < 1.0f) {
        return a * (1 - fabsf(2 * r - 1 - beta) / (1 - beta));
    } else {
        return 0;
    }
}


float boundary(float x) {
    if (x < -1.0f) {
        do {
            x += 2.0f;
        } while (x < -1.0f);
    } else if (x >= 1.0f) {
        do {
            x -= 2.0f;
        } while (x >= 1.0f);
    }
    return x;
}


void update(ParticleSystem *system) {
    // pre-compute
    float frictionFactor = pow(0.5, system->dt / system->frictionHalfLife);

    int gridSize = (int) floor(2.0f / system->rMax);
    // ensure grid memory size (changes if rMax changes)
    if (gridSize != system->gridSize) {
        system->grid = (int*) realloc(system->grid,
                sizeof(int) * (gridSize * gridSize + 1));
        system->gridSize = gridSize;
    }
    if (gridSize < 3) {
        // todo: throw error
        return;
    }

    // shorthands
    int *grid = system->grid;
    int *gridMap = system->gridMap;
    Particle *particles = system->particles;

    // clear grid
    for (int i = 0; i < gridSize * gridSize; i++) {
        grid[i] = 0;
    }
    // count particles in cells
    for (int i = 0; i < system->n; i++) {
        Particle *p = &particles[i];
        int cx = (int) floor((p->x + 1.0f) * 0.5f * (float) gridSize);
        int cy = (int) floor((p->y + 1.0f) * 0.5f * (float) gridSize);
        int gridIndex = cx + cy * gridSize;
        grid[gridIndex]++;
    }
    // cumsum
    int sum = 0;
    for (int i = 0; i < gridSize * gridSize; i++) {
        int temp = grid[i];
        grid[i] = sum;
        sum += temp;
    }
    // pointers to cell index
    for (int i = 0; i < system->n; i++) {
        Particle *p = &particles[i];
        int cx = (int) floor((p->x + 1.0f) * 0.5f * (float) gridSize);
        int cy = (int) floor((p->y + 1.0f) * 0.5f * (float) gridSize);
        int gridIndex = cx + cy * gridSize;
        int particleIndex = grid[gridIndex];
        grid[gridIndex]++;
        gridMap[particleIndex] = i;
    }
    // undo changes to grid
    for (int i = gridSize * gridSize; i > 0; i--) {
        grid[i] = grid[i - 1];
    }
    grid[0] = 0;
    // todo: copy actual particle structs to avoid additional lookups

    // velocities
    for (int cy = 0; cy < gridSize; cy++) {
        for (int cx = 0; cx < gridSize; cx++) {

            int gridIndex = cx + cy * gridSize;
            int start = grid[gridIndex];
            int stop = grid[gridIndex + 1];
            for (int k = start; k < stop; k++) {
                int i = gridMap[k];
                Particle *p = &particles[i];

                float totalForceX = 0.0f;
                float totalForceY = 0.0f;

                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int cx_ = cx + dx;
                        int cy_ = cy + dy;

                        // wrap around
                        if (cx_ < 0) cx_ += gridSize;
                        if (cx_ >= gridSize) cx_ -= gridSize;
                        if (cy_ < 0) cy_ += gridSize;
                        if (cy_ >= gridSize) cy_ -= gridSize;

                        int c_ = cx_ + cy_ * gridSize;

                        int start_ = grid[c_];
                        int stop_ = grid[c_ + 1];
                        for (int k_ = start_; k_ < stop_; k_++) {
                            int i_ = gridMap[k_];
                            if (i_ == i) continue;
                            Particle *p_ = &particles[i_];

                            float rx = boundary(p_->x - p->x);
                            float ry = boundary(p_->y - p->y);
                            float rSquared = rx * rx + ry * ry;
                            float r = sqrtf(rSquared);
                            if (r > 0.0f && r < system->rMax) {
                                float a = system->matrix[p->type * system->m + p_->type];
                                float f = force(r / system->rMax, a);
                                totalForceX += rx / r * f;
                                totalForceY += ry / r * f;
                            }
                        }
                    }
                }

                totalForceX *= system->rMax * system->forceFactor;
                totalForceY *= system->rMax * system->forceFactor;

                p->vx *= frictionFactor;
                p->vy *= frictionFactor;

                p->vx += totalForceX * system->dt;
                p->vy += totalForceY * system->dt;
            }
        }
    }

    // positions
    for (int i = 0; i < system->n; i++) {
        Particle *particle = &particles[i];
        particle->x = boundary(particle->x + particle->vx * system->dt);
        particle->y = boundary(particle->y + particle->vy * system->dt);
    }
}

void renderDensity(int *grid, int w, int h,
        ParticleSystem *system,
        float zoom, float shiftX, float shiftY, bool clear) {
    int n = system->n;
    int m = system->m;

    if (clear) memset(grid, 0, w * h * m * sizeof(int));

    int w_cw = w;
    int h_cw = h * CHAR_RATIO;

    for (int i = 0; i < n; i++) {
        Particle *p = &(system->particles[i]);
        float x_cw = (p->x + shiftX) * zoom * h_cw / 2 + w_cw / 2;
        float y_cw = (p->y + shiftY) * zoom * h_cw / 2 + h_cw / 2;

        int x = (int) floor(x_cw);
        int y = (int) floor(y_cw / CHAR_RATIO);

        if (x >= 0 && x < w && y >= 0 && y < h) {
            grid[y * w * m + x * m + p->type]++;
        }
    }
}

void renderText(ParticleSystem *system, UiSettings *ui, int *densityGridBuf) {
    int w = ui->w;
    int h = ui->h;
    int colorMode = ui->colorMode;
    int m = system->m;
    char *densityChars = ui->densityChars;
    size_t densityCharsLen = strlen(densityChars);
    
    if (ui->printInPlace) {
        if (ui->printInPlaceIsFirstFrame) {
            ui->printInPlaceIsFirstFrame = false;
        } else {
            // move cursor up
            printf("\033[%dA", h);
        }
    }

    // draw grid
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // find most common particle type
            int maxType = 0;
            int maxCount = 0;
            int offset = y * w * m + x * m;
            for (int i = 0; i < m; i++) {
                int count = densityGridBuf[offset + i];
                if (count > maxCount) {
                    maxType = i;
                    maxCount = count;
                }
            }
            // draw character
            if (maxCount == 0) {
                putchar(' ');
            } else {
                size_t index = (size_t) maxCount - 1;
                if (index > densityCharsLen - 1) {
                    index = densityCharsLen - 1;
                }
                char c = densityChars[index];
                if (colorMode == 1) {
                    printf("\033[38;5;%dm", maxType + 1);
                }
                putchar(c);
            }
        }
        if (colorMode == 1) {
            printf("\033[0m"); // reset ANSI color code
        }
        putchar('\n');
    }
}

void renderTerminal(ParticleSystem *system, UiSettings *ui, WINDOW *win, int *densityGridBuf) {
    int w = ui->w;
    int h = ui->h;
    int colorMode = ui->colorMode;
    int m = system->m;
    char *densityChars = ui->densityChars;
    size_t densityCharsLen = strlen(densityChars);

    // draw grid
    chtype rowString[w];
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // find most common particle type
            int maxType = 0;
            int maxCount = 0;
            int offset = y * w * m + x * m;
            for (int i = 0; i < m; i++) {
                int count = densityGridBuf[offset + i];
                if (count > maxCount) {
                    maxType = i;
                    maxCount = count;
                }
            }
            // draw character
            if (maxCount == 0) {
                rowString[x] = ' ';
            } else {
                size_t index = (size_t) maxCount - 1;
                if (index > densityCharsLen - 1) {
                    index = densityCharsLen - 1;
                }
                chtype ch = densityChars[index];
                if (colorMode == 1) {
                    ch |= COLOR_PAIR(maxType + 1);
                }
                rowString[x] = ch;
            }
        }
        mvwaddchnstr(win, y, 0, rowString, w);
    }
}



void randomizeMatrix(ParticleSystem *system, int mode) {
    if (mode < 1 || mode > NUM_MATRIX_MODES) return;

    if (mode == 1) {
        for (int i = 0; i < system->m; i++) {
            for (int j = 0; j < system->m; j++) {
                system->matrix[i * system->m + j] = randFloat() * 2.0f - 1.0f;
            }
        }
    } else if (mode == 2) {
        for (int i = 0; i < system->m; i++) {
            for (int j = 0; j < system->m; j++) {
                float val = 0.0f;
                if (i == j) {
                    val = 1.0f;
                }
                if (i == (j + 1) % system->m) {
                    val = 0.5f;
                }
                system->matrix[i * system->m + j] = val;
            }
        }
    }
}

void initPositions(ParticleSystem *system, int mode) {
    if (mode < 1 || mode > NUM_POSITION_MODES) return;

    if (mode == 1) {
        for (int i = 0; i < system->n; i++) {
            Particle *particle = &system->particles[i];
            particle->x = randFloat() * 2.0f - 1.0f;
            particle->y = randFloat() * 2.0f - 1.0f;
        }
    } else if (mode == 2) {
        for (int i = 0; i < system->n; i++) {
            Particle *particle = &system->particles[i];
            float angle = randFloat() * 2.0f * M_PI;
            float radius = randFloat() * randFloat() * 0.3;
            particle->x = cos(angle) * radius;
            particle->y = sin(angle) * radius;
        }
    } else if (mode == 3) {
        for (int i = 0; i < system->n; i++) {
            Particle *particle = &system->particles[i];
            particle->x = randFloat() * 2.0f - 1.0f;
            particle->y = (randFloat() - 0.5) * 0.2 * randFloat();
        }
    } else if (mode == 4) {
        for (int i = 0; i < system->n; i++) {
            Particle *particle = &system->particles[i];
            float angle = randFloat() * 2.0f * M_PI;
            float radius = 0.1f + angle * 0.1f;
            particle->x = cos(angle) * radius;
            particle->y = sin(angle) * radius;
        }
    }
}

double diffMs(struct timespec *start, struct timespec *end) {
    // in ms
    return (((double) (end->tv_sec - start->tv_sec)) * 1000.0 +
            ((double) (end->tv_nsec - start->tv_nsec)) / 1000000.0);
}

void startTimer(struct timespec *t) {
    clock_gettime(CLOCK_MONOTONIC, t);
}

double stopTimer(struct timespec *t) {
    // after this call, t contains the current time
    struct timespec t0 = *t;
    clock_gettime(CLOCK_MONOTONIC, t);
    return diffMs(&t0, t);
}

void colorIf(bool val, WINDOW *win) {
    attr_t attr = COLOR_PAIR(0) | A_REVERSE;
    if (val) {
        wattron(win, attr);
    } else {
        wattroff(win, attr);
    }
}


int main(int argc, char *argv[]) {
    int positionMode = DEFAULT_POSITION_MODE;
    int matrixMode = DEFAULT_MATRIX_MODE;
    unsigned int seed;
    bool useSeed = false;
    bool quitAfterOneFrame = false;
    bool showGui = true;
    bool setZoomFit = true;
    int stepsPerFrame = DEFAULT_STEPS_PER_FRAME;
    int initialSkipFrames = 0;

    // ParticleSystem defaults
    ParticleSystem system;
    system.rMax = DEFAULT_RADIUS;
    system.frictionHalfLife = 0.040f;
    system.forceFactor = 10.0f;
    system.dt = DEFAULT_DT;
    system.n = DEFAULT_N;
    system.m = DEFAULT_M;

    // UiSettings defaults
    UiSettings ui;
    ui.printInPlace = false;
    ui.printInPlaceIsFirstFrame = true;
    ui.w = DEFAULT_W;
    ui.h = DEFAULT_H;
    ui.zoom = 1.0f;
    ui.shiftX = 0.0f;
    ui.shiftY = 0.0f;
    ui.pause = false;
    ui.densityChars = (char*) malloc(sizeof(char) * ((size_t) MAX_WAIT_ARG_LEN + 1));
    strcpy(ui.densityChars, DEFAULT_DENSITY_CHARS);
    ui.showInfo = false;
    ui.showDebug = false;
    ui.clear = true;
    ui.colorMode = DEFAULT_COLOR_MODE;

    // process command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "n:m:a:A:r:t:z:x:p:c:s:W:H:k:K:dqoOizPh")) != -1) {
        switch (opt) {
            case 'n':
                system.n = atoi(optarg);
                break;
            case 'm':
                system.m = atoi(optarg);
                break;
            case 'a':
                matrixMode = atoi(optarg);
                break;
            case 'A':
                printf("option -A <matrix> not implemented yet\n");
                break;
            case 'r':
                system.rMax = atof(optarg);
                break;
            case 't':
                system.dt = atof(optarg);
                break;
            case 'p':
                positionMode = atoi(optarg);
                break;
            case 'P':
                ui.pause = true;
                break;
            case 'x':
                strcpy(ui.densityChars, optarg);
                break;
            case 'c':
                ui.colorMode = atoi(optarg);
                break;
            case 'd':
                ui.clear = false;
                break;
            case 'O':
                ui.printInPlace = true;
            case 'o':
                showGui = false;
                break;
            case 'q':
                quitAfterOneFrame = true;
                ui.printInPlaceIsFirstFrame = true;
                break;
            case 'W':
                ui.w = atoi(optarg);
                break;
            case 'H':
                ui.h = atoi(optarg);
                break;
            case 'i':
                ui.showInfo = true;
                break;
            case 's':
                if (atoi(optarg) < 0) {
                    fprintf(stderr, "Seed must be a positive integer.\n");
                    return EXIT_FAILURE;
                }
                seed = (unsigned int) atoi(optarg);
                useSeed = true;
                break;
            case 'z':
                ui.zoom = atof(optarg);
                setZoomFit = false;
                break;
            case 'k':
                stepsPerFrame = atoi(optarg);
                break;
            case 'K':
                initialSkipFrames = atoi(optarg);
                break;
            case 'h':
                print_help();
                return EXIT_SUCCESS;
            default:
                print_usage();
                return EXIT_FAILURE;
        }
    }

    // sanity checks

    if (system.n <= 0) {
        printf("n must be positive\n");
        return 1;
    }
    if (system.m <= 0) {
        printf("m must be positive\n");
        return 1;
    }
    if (system.rMax <= 0) {
        printf("rmax must be positive\n");
        return 1;
    }
    if (system.dt < 0) {
        printf("dt must be non-negative\n");
        return 1;
    }
    if (positionMode < 1 || positionMode > NUM_POSITION_MODES) {
        printf("position mode must be an integer between 1 and %d\n", NUM_POSITION_MODES);
        return 1;
    }
    if (matrixMode < 1 || matrixMode > NUM_MATRIX_MODES) {
        printf("matrix mode must be an integer between 1 and %d\n", NUM_MATRIX_MODES);
        return 1;
    }
    if (ui.colorMode < 0 || ui.colorMode > NUM_COLOR_MODES) {
        printf("color mode must be an integer between 0 and %d\n", NUM_COLOR_MODES);
        return 1;
    }

    // ParticleSystem initialization

    srand(useSeed ? seed : time(NULL));

    system.particles = malloc(system.n * sizeof(Particle));
    system.gridSize = (int) floor(2.0f / system.rMax);
    system.grid = (int *) malloc((system.gridSize * system.gridSize + 1) * sizeof(int));
    system.gridMap = malloc(system.n * sizeof(int));
    system.matrix = malloc(system.m * system.m * sizeof(float));

    randomizeMatrix(&system, matrixMode);

    for (int i=0; i<system.n; i++) {
        Particle *particle = &system.particles[i];
        particle->type = rand() % system.m;
        particle->vx = 0.0f;
        particle->vy = 0.0f;
    }
    initPositions(&system, positionMode);

    // UI initialization

    char waitingCommand = 0;
    char waitingCommandArg[MAX_WAIT_ARG_LEN + 1] = "";
    
    WINDOW *win;          // for particle visualization
    WINDOW *infoWin;      // for info text
    WINDOW *debugWin;     // for debug info
    int *densityGridBuf;  // for density counting

    if (showGui) {
        initscr();
        if (!stdscr) {
            printf("Error initializing screen\n");
            return 1;
        }
        getmaxyx(stdscr, ui.h, ui.w);

        raw();
        curs_set(FALSE);  // hide cursor
        noecho();  // don't echo input
        nodelay(stdscr, TRUE);  // don't wait for input
        keypad(stdscr, TRUE);  // enable arrow & function keys

        if (has_colors()) {
            use_default_colors();
            assume_default_colors(-1, -1);
            start_color();
            for (int i = 0; i < system.m; i++) {
                init_pair(i + 1, COLOR_RED + i, -1);
            }
        }

        // allocate GUI buffers
        win = newwin(ui.h, ui.w, 0, 0);
        infoWin = newwin(12, 32, 0, 0);
        debugWin = newwin(8, 32, ui.h - 8, 0);
    }
    densityGridBuf = calloc(ui.w * ui.h * system.m, sizeof(int));

    if (setZoomFit) {
        ui.zoom = (float) ui.w / ((float) ui.h * CHAR_RATIO);
    }

    // timers for debugging
    struct timespec t, t0;
    double msPerRefresh = 0;
    double msPerFrame = 0;
    double msPerUpdate = 0;
    double msPerRender = 0;
    double msPerCopywin = 0;
    double msPerInputHandling = 0;

    renderDensity(densityGridBuf, ui.w, ui.h, &system, ui.zoom, ui.shiftX, ui.shiftY, ui.clear);

    for (int i=0; i<initialSkipFrames; i++) {
        for (int j=0; j<stepsPerFrame; j++) {
            update(&system);
        }
        renderDensity(densityGridBuf, ui.w, ui.h, &system, ui.zoom, ui.shiftX, ui.shiftY, ui.clear);
    }

    bool loop = !quitAfterOneFrame;

    // MAIN LOOP
    do {
        startTimer(&t0);

        // PHYSICS UPDATE
        if (!ui.pause) {
            startTimer(&t);
            for (int i=0; i<stepsPerFrame; i++) {
                update(&system);
            }
            msPerUpdate = stopTimer(&t) / (double) stepsPerFrame;
        }
        renderDensity(densityGridBuf, ui.w, ui.h, &system, ui.zoom, ui.shiftX, ui.shiftY, ui.clear);

        if (showGui) {
            startTimer(&t);
            renderTerminal(&system, &ui, win, densityGridBuf);
            msPerRender = stopTimer(&t);
            if (ui.showInfo) {
                box(infoWin, 0, 0);
                int y = 0;
                int x = 2;
                colorIf('i'==waitingCommand, infoWin);
                mvwprintw(infoWin, y, 14, "INFO [i]");
                y++;
                colorIf(' '==waitingCommand, infoWin);
                mvwprintw(infoWin, y, x, "%-16s %3s %7.0f", "FPS", "", 1000.0 / (msPerInputHandling + msPerUpdate + msPerRender));
                y++;
                colorIf('n'==waitingCommand, infoWin);
                mvwprintw(infoWin, y, x, "%-16s %3s %7d", "num. particles", "[n]", system.n);
                y++;
                colorIf('p'==waitingCommand, infoWin);
                mvwprintw(infoWin, y, x, "%-16s %3s %5d/%d", "position mode", "[p]", positionMode, NUM_POSITION_MODES);
                y++;
                colorIf('m'==waitingCommand, infoWin);
                mvwprintw(infoWin, y, x, "%-16s %3s %7d", "num. colors", "[m]", system.m);
                y++;
                colorIf('a'==waitingCommand, infoWin);
                mvwprintw(infoWin, y, x, "%-16s %3s %5d/%d", "attraction mode", "[a]", matrixMode, NUM_MATRIX_MODES);
                y++;
                colorIf('t'==waitingCommand, infoWin);
                mvwprintw(infoWin, y, x, "%-16s %3s %7.4f", "dt (seconds)", "[t]", system.dt);
                y++;
                colorIf('k'==waitingCommand, infoWin);
                mvwprintw(infoWin, y, x, "%-16s %3s %7d", "steps per frame", "[k]", stepsPerFrame);
                y++;
                colorIf('r'==waitingCommand, infoWin);
                mvwprintw(infoWin, y, x, "%-16s %3s %7.4f", "rmax", "[r]", system.rMax);
                y++;
                colorIf('x'==waitingCommand, infoWin);
                mvwprintw(infoWin, y, x, "%-16s %3s %7s", "chars", "[x]", ui.densityChars);
                y++;
                colorIf('c'==waitingCommand, infoWin);
                mvwprintw(infoWin, y, x, "%-16s %3s %5d/%d", "color mode", "[c]", ui.colorMode, NUM_COLOR_MODES);
                y++;
                colorIf(' '==waitingCommand, infoWin);
                mvwprintw(infoWin, y, 9, " github/tom-mohr ");
            }
            if (ui.showDebug) {
                box(debugWin, 0, 0);
                int y = 0;
                int x = 2;
                mvwprintw(debugWin, y, 13, "DEBUG [I]");
                y++;
                mvwprintw(debugWin, y, x, "%-16s     %7.2f", "frame", msPerInputHandling + msPerUpdate + msPerRender);
                y++;
                mvwprintw(debugWin, y, x, "%-16s     %7.2f", "input handling", msPerInputHandling);
                y++;
                mvwprintw(debugWin, y, x, "%-16s     %7.2f", "update", msPerUpdate);
                y++;
                mvwprintw(debugWin, y, x, "%-16s     %7.2f", "render", msPerRender);
                y++;
                mvwprintw(debugWin, y, x, "%-16s     %7.2f", "copywin", msPerCopywin);
                y++;
                mvwprintw(debugWin, y, x, "%-16s     %7.2f", "refresh", msPerRefresh);
                y++;
            }

            // draw all windows onto terminal screen
            startTimer(&t);
            overwrite(win, stdscr);
            if (ui.showInfo) overwrite(infoWin, stdscr);
            if (ui.showDebug) overwrite(debugWin, stdscr);
            if (waitingCommand != 0) {
                mvprintw(ui.h - 1, ui.w - 1 - (1 + 1 + MAX_WAIT_ARG_LEN + 1),
                        "%c %s%c", waitingCommand, waitingCommandArg, (t.tv_sec % 2) ? '_' : ' ');
            }
            msPerCopywin = stopTimer(&t);
            refresh();
            msPerRefresh = stopTimer(&t);

            // napms(10);

            startTimer(&t);
            int ch = getch();
            // control commands (should always be accessible)
            switch (ch) {
                case '\r':  // newline
                case '\n':
                    if (waitingCommand != 0) {
                        // execute current command
                        switch (waitingCommand) {
                            case 't':
                                if (strlen(waitingCommandArg) > 0) {
                                    // todo handle errors
                                    system.dt = atof(waitingCommandArg);
                                }
                                break;
                            case 'r':
                                if (strlen(waitingCommandArg) > 0) {
                                    // todo handle errors
                                    system.rMax = atof(waitingCommandArg);
                                }
                                break;
                            case 'z':
                                if (strlen(waitingCommandArg) > 0) {
                                    // todo handle errors
                                    ui.zoom = atof(waitingCommandArg);
                                }
                                break;
                            case 'k':
                                if (strlen(waitingCommandArg) > 0) {
                                    // todo handle errors
                                    stepsPerFrame = atoi(waitingCommandArg);
                                }
                                break;
                            case 'x':
                                ;
                                size_t len = strlen(waitingCommandArg);
                                if (len > 0) {
                                    ui.densityChars = (char *) realloc(ui.densityChars, sizeof(char) * (len + 1));
                                    strcpy(ui.densityChars, waitingCommandArg);
                                }
                                break;
                            default:
                                break;
                        }
                        waitingCommandArg[0] = '\0';
                        waitingCommand = 0;
                    }
                    break;
                case KEY_BACKSPACE:
                case '\b':
                    if (waitingCommandArg[0] != '\0') {
                        waitingCommandArg[strlen(waitingCommandArg) - 1] = '\0';
                    }
                    break;
                case 27:  // escape
                    waitingCommandArg[0] = '\0';
                    waitingCommand = 0;
                    break;
                default:
                    break;
            }
            // special treatment for 'x' command to allow for various character inputs
            if (waitingCommand == 'x') {
                if (ch >= ' ' && ch <= '~') {
                    // add the character
                    size_t len = strlen(waitingCommandArg);
                    if (len < MAX_WAIT_ARG_LEN) {
                        waitingCommandArg[len] = ch;
                        waitingCommandArg[len+1] = '\0';
                    }
                }
            } else {
                switch(ch) {
                    case 'q':
                        loop = false;
                        break;
                    case ' ':
                        ui.pause = !ui.pause;
                        break;
                    case 'Z':
                        ui.zoom = (float) ui.w / ((float) ui.h * CHAR_RATIO);
                        ui.shiftX = 0;
                        ui.shiftY = 0;
                        break;
                    case '+':
                    case '=':
                        ui.zoom *= 1.3f;
                        break;
                    case '-':
                        ui.zoom /= 1.3f;
                        break;
                    case KEY_LEFT:
                        ui.shiftX += 0.3f / ui.zoom;
                        break;
                    case KEY_RIGHT:
                        ui.shiftX -= 0.3f / ui.zoom;
                        break;
                    case KEY_UP:
                        ui.shiftY += 0.3f / ui.zoom;
                        break;
                    case KEY_DOWN:
                        ui.shiftY -= 0.3f / ui.zoom;
                        break;
                    case 't':
                    case 'r':
                    case 'z':
                    case 'k':
                    case 'x':
                    case 'p':
                    case 'a':
                    case 'c':
                        if (waitingCommand == ch) {
                            // handle double tap
                            switch (ch) {
                                case 'p':
                                    initPositions(&system, positionMode);
                                    break;
                                case 'a':
                                    randomizeMatrix(&system, matrixMode);
                                    break;
                                case 'z':
                                    ui.zoom = 1.0f;
                                    ui.shiftX = 0.0f;
                                    ui.shiftY = 0.0f;
                                    break;
                            }
                            // double tap always disables command
                            waitingCommand = 0;
                        } else {
                            waitingCommand = ch;
                        }
                        break;
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '.':
                        switch (waitingCommand) {
                            case 0:
                                // ignore
                                break;
                            case 'p':
                                if (ch == '.') break;
                                int val = ch - '0';
                                if (val >= 1 && val <= NUM_POSITION_MODES) {
                                    positionMode = val;
                                    initPositions(&system, val);
                                    waitingCommand = 0;  // success
                                }
                                break;
                            case 'a':
                                if (ch == '.') break;
                                val = ch - '0';
                                if (val >= 1 && val <= NUM_MATRIX_MODES) {
                                    matrixMode = val;
                                    randomizeMatrix(&system, matrixMode);
                                    waitingCommand = 0;  // success
                                }
                                break;
                            case 'c':
                                if (ch == '.') break;
                                val = ch - '0';
                                if (val >= 0 && val <= NUM_COLOR_MODES) {
                                    ui.colorMode = val;
                                    waitingCommand = 0;  // success
                                }
                                break;
                            case 'k':
                                if (ch == '.') break;
                            default:
                                ;
                                // for all other commands: simply collect chars
                                size_t len = strlen(waitingCommandArg);
                                if (len < MAX_WAIT_ARG_LEN) {
                                    waitingCommandArg[len] = ch;
                                    waitingCommandArg[len+1] = '\0';
                                }
                                break;
                        }
                        break;
                    case 'i':
                        ui.showInfo = !ui.showInfo;
                        break;
                    case 'I':
                        ui.showDebug = !ui.showDebug;
                        break;
                    case 'd':
                        ui.clear = !ui.clear;
                        break;
                    default:
                        break;
                }
            }
            msPerInputHandling = stopTimer(&t);
        } else {
            // no GUI -> print to stdout
            renderText(&system, &ui, densityGridBuf);
        }

        msPerFrame = stopTimer(&t0);

    } while (loop);

    if (showGui) {
        // restore original terminal state
        endwin();

        // free buffers
        delwin(win);
        delwin(infoWin);
        delwin(debugWin);
    }
    free(densityGridBuf);

    return 0;
}


