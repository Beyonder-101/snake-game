/*
 * snake.c
 * A simple terminal Snake game in C
 * 
 * Created by Miguel Marafonas
 * License: MIT
 * 
 * Description:
 *   This is a simple implementation of the Snake game using
 *   standard C (C99). The snake moves around collecting food
 *   and grows in length each time food is eaten.
 */

// Include standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Include Platform-specific libraries
#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
    #define CLEAR "cls"
#else
    #include <unistd.h>
    #include <termios.h>
    #include <fcntl.h>
    #define CLEAR "clear"
#endif

// Game board dimensions
#define WIDTH 40
#define HEIGHT 20
#define MAX_LENGTH 800

// Game board coordinates
typedef struct {
    int x, y;
} Point;

// Game state variables
Point snake[MAX_LENGTH];
int snake_length = 3;
Point food;
int score = 0;
char direction = '\0';
int game_over = 0;

// For non-Windows systems
#ifndef _WIN32
int kbhit(void) {
    struct termios oldt, newt;
    int ch, oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

int getch(void) {
    struct termios oldt, newt;
    int ch;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif

// Prototypes
void sleep_ms(int ms);
void hide_cursor();
void show_cursor();
void init_game();
void draw();
void input();
void logic();

// Main function
int main() {
    init_game();
    hide_cursor();

    // Show start screen
    system(CLEAR);
    printf("\n\n");
    printf("  --------------------------------------\n");
    printf("  |         SNAKE GAME                 |\n");
    printf("  |                                    |\n");
    printf("  |  Press W/A/S/D to start...         |\n");
    printf("  |                                    |\n");
    printf("  |  Controls:                         |\n");
    printf("  |  W   = Up                          |\n");
    printf("  |  A   = Left                        |\n");
    printf("  |  S   = Down                        |\n");
    printf("  |  D   = Right                       |\n");
    printf("  |  Q = Quit                          |\n");
    printf("  --------------------------------------\n");
    printf("\n\n");

    // Wait for first keypress
    while (direction == '\0' && !game_over) {
        if (kbhit()) {
            char key = getch();

            // Set direction on first key press
            if (key == 'w' || key == 'W') {
                direction = 'w';
            }
            else if (key == 's' || key == 'S') {
                direction = 's';
            }
            else if (key == 'a' || key == 'A') {
                direction = 'a';
            }
            else if (key == 'd' || key == 'D') {
                direction = 'd';
            }
            else if (key == 'q' || key == 'Q') {
                game_over = 1;
            }
        }
        sleep_ms(50);
    }

    // Game Loop
    while (!game_over) {
        draw();
        input();
        logic();
        sleep_ms(100);
    }

    // Game over
    system(CLEAR);
    show_cursor();
    printf("\n\n");
    printf("  ------------------------------\n");
    printf("  |      GAME OVER!            |\n");
    printf("  |                            |\n");
    printf("  |   Final Score: %-4d        |\n", score);
    printf("  |                            |\n");
    printf("  |  Press Q to exit           |\n");
    printf("  ------------------------------\n");
    printf("\n\n");

    // Clear any buffered input from the game
    while (kbhit()) {
        getch();
    }
    sleep_ms(200);

    // Wait for 'Q' input to quit game
    char exit_key = '\0';
    while (exit_key != 'q' && exit_key != 'Q') {
        if (kbhit()) {
            exit_key = getch();
        }
        sleep_ms(50);
    }
    return 0;
}

// Sleep function
void sleep_ms(int ms) {
    #ifdef _WIN32
        Sleep(ms);
    #else
        usleep(ms * 1000);
    #endif
}

// Hide cursor
void hide_cursor() {
    #ifdef _WIN32
        HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO info;
        info.dwSize = 100;
        info.bVisible = FALSE;
        SetConsoleCursorInfo(consoleHandle, &info);
    #else
        printf("\e[?25l");
    #endif
}

// Show cursor (game over)
void show_cursor() {
    #ifdef _WIN32
        HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO info;
        info.dwSize = 100;
        info.bVisible = TRUE;
        SetConsoleCursorInfo(consoleHandle, &info);
    #else
        printf("\e[?25h");
        fflush(stdout);
    #endif
}

// Initialize game state
void init_game() {
    // Place snake in the middle, facing right
    snake[0].x = WIDTH / 2;
    snake[0].y = HEIGHT / 2;
    snake[1].x = WIDTH / 2 - 1;
    snake[1].y = HEIGHT / 2;
    snake[2].x = WIDTH / 2 - 2;
    snake[2].y = HEIGHT / 2;

    // Place food at random position
    srand(time(NULL));
    food.x = rand() % (WIDTH - 2) + 1;
    food.y = rand() % (HEIGHT - 2) + 1;
}

// Draw game board, snake, food and score
void draw() {
    system(CLEAR);
    fflush(stdout);

    // Draw top border
    for (int i = 0; i < WIDTH + 2; i++) {
        printf("#");
    }
    printf("\n");

    // Draw game field row by row
    for (int y = 0; y < HEIGHT; y++) {
        printf("#");

        // Draw each column in this row
        for (int x = 0; x < WIDTH; x++) {
            int is_snake = 0;

            // Check if part of snake in this position
            for (int segment = 0; segment < snake_length; segment++) {
                if (snake[segment].x == x && snake[segment].y == y) {
                    printf(segment == 0 ? "0" : "o");
                    is_snake = 1;
                    break;
                }
            }

            // If not snake, check if food or empty
            if (!is_snake) {
                if (food.x == x && food.y == y) {
                    printf("*");
                }
                else {
                    printf(" ");
                }
            }
        }

        // Print right border and move to next line
        printf("#\n");
    }

    // Draw bottom border
    for (int i = 0; i < WIDTH + 2; i++) {
        printf("#");
    }

    // Display score and control keys
    printf("\nScore: %d\n", score);
    printf("Controls : W=Up, A=Left, S=Down, D=Right, Q=Quit\n");
}

// Handle keyboard input
void input() {
    if (kbhit()) {
        char key = getch();

        // Update direction without moving into itself
        if ((key == 'w' || key == 'W') && direction != 's') {
            direction = 'w';
        }
        else if ((key == 's' || key == 'S') && direction != 'w') {
            direction = 's';
        }
        else if ((key == 'a' || key == 'A') && direction != 'd') {
            direction = 'a';
        }
        else if ((key == 'd' || key == 'D') && direction != 'a') {
            direction = 'd';
        }
        else if (key == 'q' || key == 'Q') {
            game_over = 1;
        }
    }
}

// Update game - move snake, check collisions, food
void logic() {
    if (direction == '\0') {
        return;
    }

    Point prev = snake[0];
    Point prev2;

    // Move head in current direction
    switch(direction) {
        case 'w': snake[0].y--; break;
        case 's': snake[0].y++; break;
        case 'a': snake[0].x--; break;
        case 'd': snake[0].x++; break;
    }

    // Check for wall collision
    if (snake[0].x < 0 || snake[0].x >= WIDTH || snake[0].y < 0 || snake[0].y >= HEIGHT) {
        game_over = 1;
        return;
    }

    // Check for collision with itself
    for (int i = 1; i < snake_length; i++) {
        if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
            game_over = 1;
            return;
        }
    }

    // Move each body part to the previous position of the body part in front of it
    for (int i = 1; i < snake_length; i++) {
        prev2 = snake[i];
        snake[i] = prev;
        prev = prev2;
    }

    // Check food collision
    if (snake[0].x == food.x && snake[0].y == food.y) {
        score += 10;
        snake_length++;
        snake[snake_length - 1] = prev;

        // Spawn new food
        food.x = rand() % (WIDTH - 2) + 1;
        food.y = rand() % (HEIGHT - 2) + 1;
    }
}
