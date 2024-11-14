#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include "graphics.h"

#define PADDLE_HEIGHT 24
#define PADDLE_WIDTH 8
#define BALL_SIZE 8

#define NEW_GAME_PAUSE 120 // 2   Seconds
#define ROUND_PAUSE 90     // 1.5 Seconds
#define HALF_PAUSE (ROUND_PAUSE / 2)

#define BALL_START_X (SCREEN_WIDTH / 2) - (BALL_SIZE / 2) + 1
#define PLAYER_START_Y ((SCREEN_HEIGHT / 2) - (PADDLE_HEIGHT / 2))
#define WINNING_SCORE 10

bool paused = true;

/* Show menu cursor on current selection */
void setMenuCursor(int selection)
{
    /* Clear Cursor */
    clearRegion(MENU_TEXT_X - CHAR_PIX_SIZE, MENU_ITEM_1, MENU_TEXT_X, MENU_ITEM_2 + CHAR_PIX_SIZE);

    /* Show Cursor on selection */
    printChar(selector[1], MENU_TEXT_X - CHAR_PIX_SIZE, MENU_ITEM_1 + (selection)*LINE_HEIGHT);
}

/* Bounding Box Collision Detection */
bool hasCollision(
    int x1, int y1, int width1, int height1,
    int x2, int y2, int width2, int height2)
{
    return (x1 + width1 > x2 && x1 < x2 + width2 &&
            y1 + height1 > y2 && y1 < y2 + height2);
}

/* Paddle Bounce Logic - Tweak as needed depending on paddle size */
void bounceOffPaddle(struct rect *playerPaddle, struct rect *ball)
{
    int y_diff = (ball->y + (BALL_SIZE / 2)) - (playerPaddle->y + (PADDLE_HEIGHT / 2));

    /* Set Y Velocity according to distance from center of paddle */
    ball->velocityY = ((y_diff > 0) - (y_diff < 0)) +
                      ((y_diff > 3) - (y_diff < -3)) +
                      ((y_diff > 6) - (y_diff < -6));

    /* Set X Velocity to +/- 3, and add 1 to X speed if hits near center */
    ball->velocityX = ((ball->velocityX < 0) - (ball->velocityX > 0)) * (3 + (1 - ((y_diff > 4) || (y_diff < -4))));
}

/* Scoring Points */
void playerScores(bool isHuman, struct rect *ball, int *humanScore, int *cpuScore, int *pauseLength)
{
    /* Increment Score */
    int *playerScore;
    if (isHuman)
        playerScore = humanScore;
    else
        playerScore = cpuScore;
    *playerScore = *playerScore + 1;
    paused = true;

    /* If Winning Score, Show Winner and Reset */
    if (*playerScore >= WINNING_SCORE)
    {
        clearRegion(SCREEN_WIDTH / 2, MENU_TEXT_Y,
                    SCREEN_WIDTH / 2 + 2, MENU_TEXT_Y + 30);
        if (isHuman)
        {
            displayText(" YOU WIN! ",
                        END_TEXT_X, END_TEXT_Y);
        }
        else
        {
            displayText(" CPU WINS ",
                        END_TEXT_X, END_TEXT_Y);
        }
    }
    else
    {
        ball->velocityX *= -1;
        *pauseLength = ROUND_PAUSE;
    }
}

/* Game Logic */
void matchMode(struct rect *player, struct rect *cpuPlayer,
               struct rect *ball, int *playerScore, int *cpuScore,
               int *pauseLength, int *pauseCounter)
{
    /* If players are rallying */
    if (!paused)
    {
        /* If ball has hit opponents wall, player scores */
        if (ball->x <= 3 && ball->velocityX < 0)
        {
            ball->x = player->x;
            playerScores(false, ball, playerScore, cpuScore, pauseLength);
        }

        else if (ball->x >= SCREEN_WIDTH - ball->width - 3 && ball->velocityX > 0)
        {
            ball->x = cpuPlayer->x + PADDLE_WIDTH - BALL_SIZE;
            playerScores(true, ball, playerScore, cpuScore, pauseLength);
        }
        /* If ball hits ceiling or floor, bounce off */
        if (ball->y <= 0 && ball->velocityY < 0)
        {
            ball->velocityY *= -1;
        }

        else if (ball->y >= SCREEN_HEIGHT - ball->height && ball->velocityY > 0)
        {
            ball->velocityY *= -1;
        }

        /* Move human player based on input */
        int keys_pressed = keysDown();
        int keys_released = keysUp();

        if ((keys_released & KEY_UP) || (keys_released & KEY_DOWN))
        {
            player->velocityY = 0;
        }
        if ((keys_pressed & KEY_UP) && (player->y >= 0))
        {
            player->velocityY = -2;
        }
        if ((keys_pressed & KEY_DOWN) &&
            (player->y <= SCREEN_HEIGHT - player->height))
        {
            player->velocityY = 2;
        }
        if ((player->y <= 0 && player->velocityY < 0) ||
            ((player->y >= SCREEN_HEIGHT - player->height) &&
             player->velocityY > 0))
        {
            player->velocityY = 0;
        }

        /* Computer Player Movement Logic */
        if (ball->y + (BALL_SIZE - 2) > cpuPlayer->y + (PADDLE_HEIGHT / 2) &&
            cpuPlayer->y + PADDLE_HEIGHT <= SCREEN_HEIGHT &&
            ball->x > SCREEN_WIDTH / 4 &&
            !(ball->velocityY < -2) &&
            (ball->velocityX > 0 || ball->x > SCREEN_WIDTH / 2))
        {
            cpuPlayer->velocityY = 2;
        }
        else if (ball->y + (BALL_SIZE - 2) < cpuPlayer->y + (PADDLE_HEIGHT / 2) &&
                 cpuPlayer->y >= 0 &&
                 ball->x > SCREEN_WIDTH / 4 &&
                 !(ball->velocityY > 2) &&
                 (ball->velocityX > 0 || ball->x > SCREEN_WIDTH / 2))
        {
            cpuPlayer->velocityY = -2;
        }
        else
        {
            cpuPlayer->velocityY = 0;
        }

        /* Update Positions */
        if (!paused)
        {
            ball->x += ball->velocityX;
            ball->y += ball->velocityY;
            player->y += player->velocityY;
            cpuPlayer->y += cpuPlayer->velocityY;
        }

        /* Wait a moment after score before new rally */
    }
    else
    {
        *pauseCounter = *pauseCounter + 1;
        if (*pauseCounter == (int)HALF_PAUSE)
        {
            ball->x = BALL_START_X;
            player->y = PLAYER_START_Y;
            cpuPlayer->y = PLAYER_START_Y;
        }
        if (*pauseCounter > *pauseLength)
        {
            *pauseCounter = 0;
            paused = false;
        }
    }

    /* Clear Previous Ball, Player Graphics */
    clearPrevious(ball);
    clearPrevious(player);
    clearPrevious(cpuPlayer);

    drawCenterLine();

    printHumanScore(score[*playerScore]);
    printComputerScore(score[*cpuScore]);
    printPlayerSymbols();

    /* Draw Ball, Players at current positions */
    drawRect(ball, CLR_LIME);
    drawRect(player, CLR_BLUE);
    drawRect(cpuPlayer, CLR_BLUE);

    /* Update previous positions for clearing pixels */
    ball->prevX = ball->x;
    ball->prevY = ball->y;
    player->prevX = player->x;
    player->prevY = player->y;
    cpuPlayer->prevX = cpuPlayer->x;
    cpuPlayer->prevY = cpuPlayer->y;

    /* Check for paddle / ball collisions, if so ball bounces */
    if (hasCollision(
            player->x, player->y,
            player->width, player->height,
            ball->x, ball->y,
            ball->width, ball->height) &&
        ball->velocityX < 0)
    {
        bounceOffPaddle(player, ball);
    }
    if (hasCollision(
            cpuPlayer->x, cpuPlayer->y,
            cpuPlayer->width, cpuPlayer->height,
            ball->x, ball->y,
            ball->width, ball->height) &&
        ball->velocityX > 0)
    {
        bounceOffPaddle(cpuPlayer, ball);
    }

    return;
}

int main(void)
{
    // Interrupt handlers
    irqInit();

    // Enable Vblank Interrupt, Allow VblankIntrWait
    irqEnable(IRQ_VBLANK);

    /* Set screen to mode 3 */
    SetMode(MODE_3 | BG2_ON);

    /* Match Variables */
    struct rect humanPlayer, computerPlayer, ball;
    int humanScore, computerScore;
    int pauseLength, pauseCounter;

    humanPlayer.x = 1;
    humanPlayer.y = PLAYER_START_Y;
    humanPlayer.prevX = humanPlayer.x;
    humanPlayer.prevY = humanPlayer.y;
    humanPlayer.width = PADDLE_WIDTH;
    humanPlayer.height = PADDLE_HEIGHT;
    humanPlayer.velocityX = 0;
    humanPlayer.velocityY = 0;
    humanScore = 0;

    computerPlayer.x = SCREEN_WIDTH - PADDLE_WIDTH - 1;
    computerPlayer.y = PLAYER_START_Y;
    computerPlayer.prevX = computerPlayer.x;
    computerPlayer.prevY = computerPlayer.y;
    computerPlayer.width = PADDLE_WIDTH;
    computerPlayer.height = PADDLE_HEIGHT;
    computerPlayer.velocityX = 0;
    computerPlayer.velocityY = 0;
    computerScore = 0;

    ball.x = BALL_START_X;
    ball.y = (SCREEN_HEIGHT / 2) - (BALL_SIZE / 2);
    ball.prevX = ball.x;
    ball.prevY = ball.y;
    ball.width = BALL_SIZE;
    ball.height = BALL_SIZE;
    ball.velocityX = 2;
    ball.velocityY = 2;

    pauseLength = NEW_GAME_PAUSE;
    pauseCounter = 0;

    /* Main Game Loop */
    while (1)
    {
        VBlankIntrWait();
        scanKeys();

        matchMode(&humanPlayer,
                  &computerPlayer,
                  &ball,
                  &humanScore,
                  &computerScore,
                  &pauseLength,
                  &pauseCounter);

        /* Reset after completed game */
    }
}