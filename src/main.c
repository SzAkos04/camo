#include "debug.h"

#include <ctype.h>
#include <curses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAYBE_UNUSED __attribute__((unused))

#define CTRL(ch) (ch & 0x1F)

int main(int argc, char **argv) {
    if (argc != 2) {
        error("incorrect usage, for help run `camo --help`");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "--help") == 0) {
        printf("Usage: camo [file]\n");
        printf("\n");
        printf("Arguments:\n");
        printf("  file    The path of the file intended to work on\n");
        printf("\n");
        printf("For more information, visit: \n");
        printf("  https://github.com/SzAkos04/camo\n");
        return EXIT_SUCCESS;
    }

    char *path = argv[1];

    if (access(path, F_OK | R_OK) != 0) {
        perr("file does not exist");
        return EXIT_FAILURE;
    }

    FILE *infile = fopen(path, "r");
    if (!infile) {
        perr("failed to open file");
        return EXIT_FAILURE;
    }

    // get the size of the file
    fseek(infile, 0, SEEK_END);
    long file_size = ftell(infile);
    fseek(infile, 0, SEEK_SET);

    char *buf = (char *)malloc(sizeof(char) * (file_size + 1));
    if (!buf) {
        perr("failed to allocate memory");
        fclose(infile);
        return EXIT_FAILURE;
    }

    size_t files_read = fread(buf, sizeof(char), file_size, infile);
    if (files_read != (size_t)file_size) {
        perr("failed to read file");
        fclose(infile);
        free(buf);
        return EXIT_FAILURE;
    }
    buf[file_size] = '\0';

    fclose(infile);

    bool running = true;
    initscr();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    raw();
    noecho();

    printw("%s", buf);
    refresh();

    while (running) {
        int cury = getcury(stdscr);
        int curx = getcurx(stdscr);
        int maxy MAYBE_UNUSED = getmaxy(stdscr);
        int maxx = getmaxx(stdscr);
        int ch = getch();
        switch (ch) {
        case CTRL('c'):
            running = false;
            break;
        case CTRL('s'): {
            FILE *outfile = fopen(path, "w");
            if (!outfile) {
                perr("failed to open file");
                return EXIT_FAILURE;
            }
            fprintf(outfile, "%s", buf);
            fclose(outfile);
            break;
        }
        case KEY_BACKSPACE:
            if (file_size > 0) {
                // move the cursor back and delete the character
                if (curx > 0) {
                    mvdelch(cury, curx - 1);
                } else {
                    move(cury - 1, maxx - 1);
                    addch(' '); // overwrite the character with a space
                    move(cury - 1, maxx - 1);
                }
                // remove the last character from the buffer
                buf[file_size - 1] = '\0';
                file_size--;
            }
            break;
            // TODO: Make it so it doesn't override the existing text
        case KEY_LEFT:
            move(cury, curx - 1);
            break;
        case KEY_RIGHT:
            move(cury, curx + 1);
            break;
        case KEY_UP:
            move(cury - 1, curx);
            break;
        case KEY_DOWN:
            move(cury + 1, curx);
            break;
        default: {
            if (isprint(ch) || isspace(ch)) {
                char *tmp = (char *)malloc(file_size + 1);
                if (!tmp) {
                    free(buf);
                    perr("failed to allocate memory");
                    return EXIT_FAILURE;
                }
                strcpy(tmp, buf);
                file_size++;
                buf = (char *)realloc(buf, file_size + 1);
                if (!buf) {
                    free(buf);
                    perr("failed to allocate memory");
                    return EXIT_FAILURE;
                }
                sprintf(buf, "%s%c", tmp, ch);
                free(tmp);
                addch(ch);
            }
            break;
        }
        }
        refresh();
    }

    endwin();

    free(buf);

    return EXIT_SUCCESS;
}
