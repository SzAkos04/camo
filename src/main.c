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

void help(void);

int main(int argc, char **argv) {
    if (argc < 2) {
        error("incorrect usage, for help run `camo --help`");
        return EXIT_FAILURE;
    }

    char *path;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'h':
                help();
                return EXIT_SUCCESS;
            case '-':
                if (strcmp(argv[i], "--help") == 0) {
                    help();
                    return EXIT_SUCCESS;
                } else {
                    char msg[128];
                    snprintf(msg, sizeof(msg),
                             "unknown argument `%s`\n"
                             "for more info, run `camo --help`",
                             argv[i]);
                    error(msg);
                    return EXIT_FAILURE;
                }
            default: {
                char msg[128];
                snprintf(msg, sizeof(msg),
                         "unknown argument `%s`\n"
                         "for more info, run `camo --help`",
                         argv[i]);
                error(msg);
                return EXIT_FAILURE;
            }
            }
        } else {
            path = argv[i];
        }
    }

    long file_size = 0;
    char *buf;

    if (access(path, F_OK | R_OK) == 0) {
        FILE *infile = fopen(path, "r");
        if (!infile) {
            perr("failed to open file");
            return EXIT_FAILURE;
        }

        // get the size of the file
        fseek(infile, 0, SEEK_END);
        file_size = ftell(infile);
        fseek(infile, 0, SEEK_SET);

        // +1 for the null terminator
        buf = (char *)malloc(sizeof(char) * (file_size + 1));
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
    } else {
        // create file
        FILE *outfile = fopen(path, "w");
        if (!outfile) {
            perr("failed to open file");
            return EXIT_FAILURE;
        }
        fclose(outfile);
        // initialize buffer
        buf = (char *)malloc(sizeof(char));
        if (!buf) {
            perr("failed to allocate memory");
            return EXIT_FAILURE;
        }
    }

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

void help(void) {
    printf("Usage: camo [file]\n");
    printf("\n");
    printf("Arguments:\n");
    printf("  file    The path of the file intended to work on\n");
    printf("\n");
    printf("For more information, visit: \n");
    printf("  https://github.com/SzAkos04/camo\n");
}
