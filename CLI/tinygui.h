
#ifndef TINYGUI_H
#define TINYGUI_H

/*
 * TinyGUI v0.1
 * Pure C terminal GUI
 * macOS / Linux
 *
 * Define TINYGUI_IMPLEMENTATION in ONE C file.
 */

#ifdef TINYGUI_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#define TG_WIDTH  80
#define TG_HEIGHT 24

typedef struct {
    char cells[TG_HEIGHT][TG_WIDTH];
    int focus;
    int previous_count;
    int widget_count;
    int key;
    bool running;
    struct termios original;
} TG_Context;

enum {
    TG_KEY_NONE = 0,
    TG_KEY_UP,
    TG_KEY_DOWN,
    TG_KEY_ENTER,
    TG_KEY_SPACE,
    TG_KEY_QUIT
};

static TG_Context tg_ctx;

static int tg_read_key(void)
{
    fd_set set;
    struct timeval timeout = {0, 50000};

    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    if (select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout) <= 0)
        return TG_KEY_NONE;

    char c;
    if (read(STDIN_FILENO, &c, 1) != 1)
        return TG_KEY_NONE;

    if (c == 'q' || c == 'Q')
        return TG_KEY_QUIT;

    if (c == '\n' || c == '\r')
        return TG_KEY_ENTER;

    if (c == ' ')
        return TG_KEY_SPACE;

    if (c == 'w' || c == 'W')
        return TG_KEY_UP;

    if (c == 's' || c == 'S')
        return TG_KEY_DOWN;

    if (c == 27) {
        char seq[2];

        struct timeval short_timeout = {0, 10000};

        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);

        if (select(STDIN_FILENO + 1, &set, NULL, NULL,
                   &short_timeout) > 0 &&
            read(STDIN_FILENO, &seq[0], 1) == 1 &&
            seq[0] == '[') {

            if (select(STDIN_FILENO + 1, &set, NULL, NULL,
                       &short_timeout) > 0 &&
                read(STDIN_FILENO, &seq[1], 1) == 1) {

                if (seq[1] == 'A') return TG_KEY_UP;
                if (seq[1] == 'B') return TG_KEY_DOWN;
            }
        }
    }

    return TG_KEY_NONE;
}

static void tg_put(int x, int y, char c)
{
    if (x >= 0 && x < TG_WIDTH &&
        y >= 0 && y < TG_HEIGHT) {
        tg_ctx.cells[y][x] = c;
    }
}

void tg_init(void)
{
    memset(&tg_ctx, 0, sizeof(tg_ctx));

    tcgetattr(STDIN_FILENO, &tg_ctx.original);

    struct termios raw = tg_ctx.original;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    printf("\033[?1049h");  // Alternate screen
    printf("\033[?25l");    // Hide cursor
    printf("\033[2J");      // Clear screen
    fflush(stdout);

    tg_ctx.running = true;
    tg_ctx.focus = 0;
}

void tg_shutdown(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tg_ctx.original);

    printf("\033[?25h");    // Show cursor
    printf("\033[?1049l");  // Restore screen
    fflush(stdout);
}

void tg_begin_frame(void)
{
    tg_ctx.key = tg_read_key();

    if (tg_ctx.key == TG_KEY_QUIT)
        tg_ctx.running = false;

    if (tg_ctx.previous_count > 0) {
        if (tg_ctx.key == TG_KEY_DOWN) {
            tg_ctx.focus++;

            if (tg_ctx.focus >= tg_ctx.previous_count)
                tg_ctx.focus = 0;
        }

        if (tg_ctx.key == TG_KEY_UP) {
            tg_ctx.focus--;

            if (tg_ctx.focus < 0)
                tg_ctx.focus = tg_ctx.previous_count - 1;
        }
    }

    tg_ctx.widget_count = 0;

    memset(tg_ctx.cells, ' ', sizeof(tg_ctx.cells));
}

void tg_end_frame(void)
{
    printf("\033[H");

    for (int y = 0; y < TG_HEIGHT; y++) {
        fwrite(tg_ctx.cells[y], 1, TG_WIDTH, stdout);
        putchar('\n');
    }

    fflush(stdout);

    tg_ctx.previous_count = tg_ctx.widget_count;
}

bool tg_running(void)
{
    return tg_ctx.running;
}

void tg_text(int x, int y, const char *text)
{
    if (!text) return;

    for (int i = 0; text[i] != '\0'; i++)
        tg_put(x + i, y, text[i]);
}

void tg_panel(int x, int y, int width, int height,
              const char *title)
{
    if (width < 2 || height < 2) return;

    for (int i = 0; i < width; i++) {
        tg_put(x + i, y, '-');
        tg_put(x + i, y + height - 1, '-');
    }

    for (int i = 0; i < height; i++) {
        tg_put(x, y + i, '|');
        tg_put(x + width - 1, y + i, '|');
    }

    tg_put(x, y, '+');
    tg_put(x + width - 1, y, '+');
    tg_put(x, y + height - 1, '+');
    tg_put(x + width - 1, y + height - 1, '+');

    if (title)
        tg_text(x + 2, y, title);
}

bool tg_button(int x, int y, int width, const char *label)
{
    int id = tg_ctx.widget_count++;
    bool focused = (id == tg_ctx.focus);

    if (width < 4) width = 4;

    tg_put(x, y, '[');

    for (int i = 1; i < width - 1; i++)
        tg_put(x + i, y, ' ');

    tg_put(x + width - 1, y, ']');

    if (label) {
        int len = (int)strlen(label);
        int max = width - 2;

        for (int i = 0; i < len && i < max; i++)
            tg_put(x + 1 + i, y, label[i]);
    }

    if (focused)
        tg_put(x - 2, y, '>');

    return focused &&
        (tg_ctx.key == TG_KEY_ENTER ||
         tg_ctx.key == TG_KEY_SPACE);
}

bool tg_checkbox(int x, int y, const char *label,
                 bool *checked)
{
    if (!checked) return false;

    int id = tg_ctx.widget_count++;
    bool focused = (id == tg_ctx.focus);

    tg_put(x, y, '[');
    tg_put(x + 1, y, *checked ? 'X' : ' ');
    tg_put(x + 2, y, ']');

    if (label)
        tg_text(x + 4, y, label);

    if (focused)
        tg_put(x - 2, y, '>');

    bool clicked = focused &&
        (tg_ctx.key == TG_KEY_ENTER ||
         tg_ctx.key == TG_KEY_SPACE);

    if (clicked)
        *checked = !*checked;

    return clicked;
}

#endif // TINYGUI_IMPLEMENTATION

#endif // TINYGUI_H

