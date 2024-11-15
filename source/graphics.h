#include "characters.h"

#define MEM_VRAM 0x06000000

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

#define CLR_BLACK 0x0000
#define CLR_RED 0x001F
#define CLR_LIME 0x03E0
#define CLR_YELLOW 0x03FF
#define CLR_BLUE 0x7C00
#define CLR_MAG 0x7C1F
#define CLR_CYAN 0x7FE0
#define CLR_WHITE 0x7FFF

const int CHAR_PIX_SIZE = 8;
const int LINE_HEIGHT = 12;
const int NUM_CHARS_LINE = 10;

const int MENU_TEXT_X = ((SCREEN_WIDTH / 2) - (CHAR_PIX_SIZE * NUM_CHARS_LINE / 2));
const int MENU_TEXT_Y = (SCREEN_HEIGHT / 2) - 16;
const int MENU_ITEM_1 = MENU_TEXT_Y + (2 * LINE_HEIGHT);
const int MENU_ITEM_2 = MENU_ITEM_1 + LINE_HEIGHT;

const int END_TEXT_X = ((SCREEN_WIDTH / 2) - (CHAR_PIX_SIZE * NUM_CHARS_LINE / 2));
const int END_TEXT_Y = (SCREEN_HEIGHT / 2) - (CHAR_PIX_SIZE / 2);

const int SCORE_Y = 10;
const int PLAYER_SYM_Y = (SCREEN_HEIGHT - LINE_HEIGHT - SCORE_Y);

typedef u16 M3LINE[SCREEN_WIDTH];
#define m3_mem ((M3LINE *)MEM_VRAM)

typedef struct
{
    int x;
    int y;
    int prevX;
    int prevY;
    int width;
    int height;
    int velocityX;
    int velocityY;
} rectangle;

/* Drawing Graphics for Players and Ball */
void drawRectangle(rectangle *rectangle, int color)
{
    for (int i = rectangle->x; i < rectangle->x + rectangle->width; i++)
    {
        for (int j = rectangle->y; j < rectangle->y + rectangle->height; j++)
        {
            m3_mem[j][i] = color;
        }
    }
}

void clearPreviousPosition(rectangle *rectangle)
{
    for (int i = rectangle->prevX; i < rectangle->prevX + rectangle->width; i++)
    {
        for (int j = rectangle->prevY; j < rectangle->prevY + rectangle->height; j++)
        {
            m3_mem[j][i] = CLR_BLACK;
        }
    }
}

/* Clear Generic Rectangular Region */
void clearRegion(int x1, int y1, int x2, int y2)
{
    for (int i = x1; i < x2; i++)
    {
        for (int j = y1; j < y2; j++)
        {
            m3_mem[j][i] = CLR_BLACK;
        }
    }
}

/* Draw Net / Center Line */
void drawCenterLine()
{
    for (int j = 0; j < SCREEN_HEIGHT; j += 8)
    {
        m3_mem[j + 2][SCREEN_WIDTH / 2] = CLR_WHITE;
        m3_mem[j + 2][SCREEN_WIDTH / 2 + 1] = CLR_WHITE;
        m3_mem[j + 3][SCREEN_WIDTH / 2] = CLR_WHITE;
        m3_mem[j + 3][SCREEN_WIDTH / 2 + 1] = CLR_WHITE;
        m3_mem[j + 4][SCREEN_WIDTH / 2] = CLR_WHITE;
        m3_mem[j + 4][SCREEN_WIDTH / 2 + 1] = CLR_WHITE;
        m3_mem[j + 5][SCREEN_WIDTH / 2] = CLR_WHITE;
        m3_mem[j + 5][SCREEN_WIDTH / 2 + 1] = CLR_WHITE;
    }
}

/* Display Player Scores (Bigger Text) */
void printScore(bool scoreArray[64], int x)
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            int color = CLR_BLACK;
            if (scoreArray[i * 8 + j])
                color = CLR_WHITE;

            m3_mem[SCORE_Y + 2 * i][x + 2 * j] = color;
            m3_mem[SCORE_Y + 2 * i][x + 2 * j + 1] = color;
            m3_mem[SCORE_Y + 2 * i + 1][x + 2 * j] = color;
            m3_mem[SCORE_Y + 2 * i + 1][x + 2 * j + 1] = color;
        }
    }
}

void printPlayerScore(bool scoreArray[64])
{
    printScore(scoreArray, SCREEN_WIDTH / 4 - 8);
}

void printCpuScore(bool scoreArray[64])
{
    printScore(scoreArray, 3 * SCREEN_WIDTH / 4 - 8);
}

/* Print Individual Character (Normal Text) */
void printChar(bool characterArray[64], int x, int y)
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            int color = CLR_BLACK;
            if (characterArray[i * 8 + j])
                color = CLR_WHITE;

            m3_mem[y + i][x + j] = color;
        }
    }
}

/*  Display text string (made of characters.h chars). Only capital
    letters, numbers, and some punctuation, not full ascii.
    Limited to NUM_CHARS_LINE characters per line.
*/
void displayText(char textBuffer[], int x, int y)
{
    for (int i = 0; i < NUM_CHARS_LINE; i++)
    {
        // Space
        if (textBuffer[i] == 0x20)
        {
            printChar(selector[0], x + i * 8, y);
            // Exclamation point
        }
        else if (textBuffer[i] == 0x21)
        {
            printChar(punctuation[1], x + i * 8, y);
            // Period
        }
        else if (textBuffer[i] == 0x2E)
        {
            printChar(punctuation[0], x + i * 8, y);
            // Numbers
        }
        else if (textBuffer[i] >= 0x30 && textBuffer[i] <= 0x39)
        {
            printChar(score[textBuffer[i] - 0x30], x + i * 8, y);
            // Letters
        }
        else
        {
            printChar(alphabet[textBuffer[i] - 0x41], x + i * 8, y);
        }
    }
}
