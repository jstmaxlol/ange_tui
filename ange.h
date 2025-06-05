// ange_tui (ANGE)
// repo: github.com/jstmaxlol/ange_tui
// version: 1.2

#ifndef ANGE
#define ANGE

#ifdef __cplusplus
extern "C" {
#endif

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

// Internal utilities
/**
 * @brief (Development use) Saves the current cursor position in the terminal.
 */
void SaveCurPos() {
    printf("\033[s");
}
/**
 * @brief (Development use) Restores the cursor position previously saved.
 */
void RestoreCurPos() {
    printf("\033[u");
}

// Internal state
static struct termios tui_orig_termios;
static int ange_initialized = 0;

// ANGE_KEYBOARD_INPUT_API
/**
 * @enum ange_key_t
 * @brief Enum representing supported key input types.
 */
typedef enum {
    ANGE_KEY_UNKNOWN = 0,   /**< Unknown key */
    ANGE_KEY_ENTER,         /**< Enter key */
    ANGE_KEY_BACKSPACE,     /**< Backspace key */
    ANGE_KEY_ARROW_UP,      /**< Up arrow key */
    ANGE_KEY_ARROW_DOWN,    /**< Down arrow key */
    ANGE_KEY_ARROW_LEFT,    /**< Left arrow key */
    ANGE_KEY_ARROW_RIGHT,   /**< Right arrow key */
    ANGE_KEY_ESCAPE,        /**< Escape key */
    ANGE_KEY_CHAR           /**< Printable character */
} ange_key_t;

/**
 * @struct ange_input_t
 * @brief Represents a keyboard input event.
 */
typedef struct {
    ange_key_t type; /**< Type of the key event */
    char ch;        /**< Character pressed (valid only if type == ANGE_KEY_CHAR) */
} ange_input_t;

/**
 * @brief Reads a key press from standard input and returns it as an ange_input_t structure.
 * 
 * This function supports arrow keys, Enter, Escape, and printable characters.
 * 
 * @return ange_input_t representing the pressed key.
 */
static ange_input_t angeReadKey(void) {
    char c;
    ange_input_t input = { ANGE_KEY_UNKNOWN, 0 };

    if (read(STDIN_FILENO, &c, 1) != 1) return input;

    if (c == '\033') {
        char seq[3] = {0};

        fd_set set;
        struct timeval timeout;

        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);

        timeout.tv_sec = 0;
        timeout.tv_usec = 50000;

        int rv = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
        if (rv == 0) {
            input.type = ANGE_KEY_ESCAPE;
            input.ch = 27;
            return input;
        }

        if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            input.type = ANGE_KEY_ESCAPE;
            input.ch = 27;
            return input;
        }
        if (read(STDIN_FILENO, &seq[1], 1) != 1) {
            input.type = ANGE_KEY_ESCAPE;
            input.ch = 27;
            return input;
        }

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return input;
            } else {
                switch (seq[1]) {
                    case 'A': input.type = ANGE_KEY_ARROW_UP; break;
                    case 'B': input.type = ANGE_KEY_ARROW_DOWN; break;
                    case 'C': input.type = ANGE_KEY_ARROW_RIGHT; break;
                    case 'D': input.type = ANGE_KEY_ARROW_LEFT; break;
                    default: break;
                }
            }
        }

        return input;
    } else if (c == '\177') {
        input.type = ANGE_KEY_BACKSPACE;
        input.ch = 127;
        return input;
    } else if (c == '\n' || c == '\r') {
        input.type = ANGE_KEY_ENTER;
        return input;
    } else if (c >= 32 && c <= 126) {
        input.type = ANGE_KEY_CHAR;
        input.ch = c;
        return input;
    }

    return input;
}

// ANGE_API
//

// Internal state modifiers
// ANGE_INITIALIZER
/**
 * @brief Initializes the TUI by setting the terminal to raw mode and clearing the screen.
 * 
 * Disables canonical mode and echo, hides the cursor, and clears the terminal.
 * Safe to call multiple times; only initializes once.
 */
static void angeInit(void) {
    if (ange_initialized) return;

    tcgetattr(STDIN_FILENO, &tui_orig_termios);

    struct termios raw = tui_orig_termios;

    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    printf("\033[?25l\033[2J\033[H");
    fflush(stdout);

    ange_initialized = 1;
}

// ANGE_KILLER
/**
 * @brief Restores the terminal to its original settings and shows the cursor.
 * 
 * Safe to call multiple times; only deinitializes if previously initialized.
 */
static void angeKill(void) {
    if (!ange_initialized) return;

    tcsetattr(STDIN_FILENO, TCSANOW, &tui_orig_termios);

    printf("\033[?25h\n");
    fflush(stdout);

    ange_initialized = 0;
}

// Layout alignment
/**
 * @enum ange_align_t
 * @brief Text alignment options for printing.
 */
typedef enum {
    ANGE_ALIGN_LEFT,    /**< Left aligned text */
    ANGE_ALIGN_CENTER,  /**< Center aligned text */
    ANGE_ALIGN_RIGHT    /**< Right aligned text */
} ange_align_t;

// Elements
/**
 * @struct ange_button_t
 * @brief Represents a button element.
 */
typedef struct {
    const char* label; /**< Text label of the button */
    int selected;      /**< Whether the button is currently selected */
} ange_button_t;
/**
 * @struct ange_radio_option_t
 * @brief Represents a single radio option.
 */
typedef struct {
    const char* label; /**< Text label of the radio option */
} ange_radio_option_t;
/**
 * @struct ange_radio_group_t
 * @brief Represents a group of radio options.
 */
typedef struct {
    ange_radio_option_t* options; /**< Array of radio options */
    int count;                    /**< Number of radio options */
    int selected_index;           /**< Currently selected option index */
} ange_radio_group_t;
/**
 * @struct ange_navitem_t
 * @brief Represents an item in a navigation bar.
 */
typedef struct {
    const char* label; /**< Text label of the navigation item */
    char hotkey;       /**< Single character shortcut key */
} ange_navitem_t;

// Utilities
// ANGE_CLEAR_SCREEN
/**
 * @brief Clears the screen
*/
void angeClear() {
    printf("\033[2J\033[H");
}

// Draw functions
// ANGE_PRINT_PARAGRAPH
/**
 * @brief Prints a paragraph of text aligned within the terminal width.
 * 
 * Initializes the TUI if not already initialized.
 * 
 * @param text The null-terminated string to print.
 * @param align The alignment mode (left, center, right).
 * @param term_width The width of the terminal in characters.
 */
static void angePrintP(const char* text, ange_align_t align, int term_width) {
    if (!ange_initialized) angeInit();

    int len = strlen(text);
    int pad = 0;

    switch (align) {
        case ANGE_ALIGN_CENTER: pad = (term_width - len) / 2; break;
        case ANGE_ALIGN_RIGHT: pad = term_width - len; break;
        case ANGE_ALIGN_LEFT: default: pad = 0;
    }

    if (pad < 0) pad = 0;
    printf("\n%*s%s\n", pad, "", text);
    fflush(stdout);
}

// ANGE_PRINT_PARAGRAPH w/o Alignment
/**
 * @brief Prints a paragraph of text without any alignment (left aligned by default).
 * 
 * Initializes the TUI if not already initialized.
 * 
 * @param text The null-terminated string to print.
 */
static void angePrintP_Unaligned(const char* text) {
    if (!ange_initialized) angeInit();

    printf("\033[H%s\n", text);
    fflush(stdout);
}

// ANGE_PRINT_PARAGRAPH_Y_ALIGN
/**
 * @brief Prints a paragraph of text with vertical alignment
 * 
 * Initializes the TUI if not already initialized (through angePrintP)
 * 
 * @param text The null-terminated string to print.
 * @param y_padding_amount The amount of vertical (y-axis) padding to insert
 */
static void angePrintP_yAlign(const char* text, int y_padding_amount) {
    for (int i = 0; i < y_padding_amount; ++i) {
        printf("\n");
    }
    angePrintP_Unaligned(text);
}

// ANGE_PRINT_HEADER
/**
 * @brief Prints a header line with larger size.
 * 
 * The header is printed in a larger size.
 * 
 * @param text The null-terminated string to print as header.
 */
static void angePrintHeader(const char* text) {
    int hasFiglet = system("figlet -v > /dev/null 2>&1");
    char command[512];
    if (hasFiglet == 0) {
        snprintf(command, sizeof(command), "figlet \"%s\"", text);
        system(command);
    } else {
        printf(text); // Fallback to crappy visualizing
    }
    fflush(stdout);
}

// ANGE_DRAW_BUTTON w/o User input check
/**
 * @brief Draws buttons with one selected but does not handle user input.
 * 
 * @param buttons Array of ange_button_t buttons.
 * @param count Number of buttons.
 * @param selected_index Index of the currently selected button.
 */
static void angeDrawButton(ange_button_t* buttons, int count, int selected_index) {
    printf("\033[H");
    for (int i = 0; i < count; i++) {
        if (i == selected_index)
            printf("[ %s ]\n", buttons[i].label);
        else
            printf("[ %s ]\n", buttons[i].label);
    }
    fflush(stdout);
}

// ANGE_DRAW_BUTTON
/**
 * @brief Draws a list of buttons and handles user input to select one.
 * 
 * Uses arrow keys to navigate and Enter to select.
 * 
 * @param buttons Array of ange_button_t buttons.
 * @param count Number of buttons.
 * @return Index of the selected button.
 */
static int angeInputButton(ange_button_t* buttons, int count) {
    int current = 0;

    while (1) {
        angeDrawButton(buttons, count, current);

        ange_input_t input = angeReadKey();
        switch (input.type) {
            case ANGE_KEY_ARROW_UP:
                current = (current - 1 + count) % count;
                break;
            case ANGE_KEY_ARROW_DOWN:
                current = (current + 1) % count;
                break;
            case ANGE_KEY_ENTER:
                return current;
            default:
                break;
        }
    }
}

// ANGE_RADIO
/**
 * @brief Displays a group of radio options and lets the user select one.
 * 
 * Supports navigation with arrow keys, selection with Enter, and cancellation with Escape.
 * 
 * @param group Pointer to an ange_radio_group_t structure.
 * @return Index of the selected radio option, or -1 if cancelled.
 */
static int angeInputRadio(ange_radio_group_t* group) {
    int current = group->selected_index;

    while (1) {
        printf("\033[H");
        for (int i = 0; i < group->count; i++) {
            const char* marker = (i == group->selected_index) ? "(o)" : "( )";
            if (i == current)
                printf("➤ %s %s\n", marker, group->options[i].label);
            else
                printf("  %s %s\n", marker, group->options[i].label);
        }
        fflush(stdout);

        ange_input_t input = angeReadKey();
        switch (input.type) {
            case ANGE_KEY_ARROW_UP:
                current = (current - 1 + group->count) % group->count;
                break;
            case ANGE_KEY_ARROW_DOWN:
                current = (current + 1) % group->count;
                break;
            case ANGE_KEY_ENTER:
                group->selected_index = current;
                return current;
            case ANGE_KEY_ESCAPE:
                return -1;
            default:
                break;
        }
    }
}

// ANGE_TEXTBOX
/**
 * @brief Draws a textbox allowing the user to input text.
 * 
 * Supports typing characters, backspace (ASCII 127), and finishing input with Enter.
 * 
 * @param buffer Character buffer to store input. Must be preallocated.
 * @param max_len Maximum length of the input buffer.
 */
static void angeDrawTextbox(char* buffer, int max_len) {
    int len = 0;
    buffer[0] = '\0';

    while (1) {
        printf("\033[H> %s ", buffer);
        fflush(stdout);

        ange_input_t input = angeReadKey();
        if (input.type == ANGE_KEY_BACKSPACE && len > 0) {
            len--;
            buffer[len] = '\0';
        } else if (input.type == ANGE_KEY_CHAR && len < max_len - 1) {
            buffer[len++] = input.ch;
            buffer[len] = '\0';
        } else if (input.type == ANGE_KEY_ENTER) {
            break;
        }
    }
}

// ANGE_DRAW_GROUPBOX
/**
 * @brief Draws a rectangular box (group box) with optional title at specified position.
 * 
 * Uses box-drawing characters for the frame.
 * 
 * @param x X coordinate of the top-left corner (1-based column).
 * @param y Y coordinate of the top-left corner (1-based row).
 * @param width Width of the box.
 * @param height Height of the box.
 * @param title Optional title to display in the top border.
 */
static void angeDrawBox(int x, int y, int width, int height, const char* title) {
    printf("\033[%d;%dH┌", y, x);
    for (int i = 0; i < width - 2; i++) printf("─");
    printf("┐");

    for (int i = 1; i < height - 1; i++) {
        printf("\033[%d;%dH│", y + i, x);
        printf("\033[%d;%dH│", y + i, x + width - 1);
    }

    printf("\033[%d;%dH└", y + height - 1, x);
    for (int i = 0; i < width - 2; i++) printf("─");
    printf("┘");

    if (title) {
        printf("\033[%d;%dH[%s]", y, x + 2, title);
    }

    fflush(stdout);
}

// ANGE_DRAW_NAVBAR
/**
 * @brief Draws a navigation bar with items and handles user input to select one.
 * 
 * Supports navigation with left/right arrow keys, selection with Enter, and hotkey activation.
 * 
 * @param items Array of ange_navitem_t navigation items.
 * @param count Number of navigation items.
 * @param selected_index Index of the initially selected item.
 * @return Index of the selected navigation item.
 */
static int angeDrawNavbar(ange_navitem_t* items, int count, int selected_index) {
    while (1) {
        printf("\033[H");
        for (int i = 0; i < count; i++) {
            if (i == selected_index)
                printf("[\033[7m%s\033[0m] ", items[i].label);
            else
                printf("[%s] ", items[i].label);
        }
        printf("\n");
        fflush(stdout);

        ange_input_t input = angeReadKey();

        if (input.type == ANGE_KEY_ARROW_LEFT) {
            selected_index = (selected_index - 1 + count) % count;
        } else if (input.type == ANGE_KEY_ARROW_RIGHT) {
            selected_index = (selected_index + 1) % count;
        } else if (input.type == ANGE_KEY_ENTER) {
            return selected_index;
        } else if (input.type == ANGE_KEY_CHAR) {
            for (int i = 0; i < count; i++) {
                if (items[i].hotkey == input.ch || items[i].hotkey == input.ch + 32) {
                    return i;
                }
            }
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif


/*
ANGE FUN NOTES!
-   About version 1, did you know that:
    1. I finished writing it (with full doxygen-ready comments and all) at 07:39 AM (05/06/2025) and started at about midnight of the same day
    2. It clocked in at just 495 lines of code, excluding this "FUN NOTES!"
    3. I was going CRAZY because of me not being used to writing C, but only C++
-   Did you know that the original name for ANGE (ange_tui) should have been "ANGER"
*/
