#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_W 1280
#define SCREEN_H 720
#define MAX_FISH 30

// ================= UI 状态 =================
typedef enum {
    UI_TITLE,
    UI_PLAYING,
    UI_DEAD
} UIState;

// ================= 鱼结构 =================
typedef struct {
    Vector2 pos;
    Vector2 vel;
    float size;
} Fish;

// ================= 全局变量 =================
UIState uiState = UI_TITLE;

Fish player;
Fish npc[MAX_FISH];

int score = 0;
int menuIndex = 0;

// ================= 初始化 =================
void InitGame(void)
{
    player.pos = (Vector2){ SCREEN_W / 2, SCREEN_H / 2 };
    player.vel = (Vector2){ 0, 0 };
    player.size = 20;

    for (int i = 0; i < MAX_FISH; i++)
    {
        npc[i].pos = (Vector2){
            GetRandomValue(0, SCREEN_W),
            GetRandomValue(0, SCREEN_H)
        };
        npc[i].vel = (Vector2){
            GetRandomValue(-50, 50) / 60.0f,
            GetRandomValue(-50, 50) / 60.0f
        };
        npc[i].size = GetRandomValue(10, 30);
    }

    score = 0;
}

// ================= NPC 更新 =================
void UpdateNPC(void)
{
    for (int i = 0; i < MAX_FISH; i++)
    {
        npc[i].pos.x += npc[i].vel.x;
        npc[i].pos.y += npc[i].vel.y;

        if (npc[i].pos.x < 0 || npc[i].pos.x > SCREEN_W)
            npc[i].vel.x *= -1;
        if (npc[i].pos.y < 0 || npc[i].pos.y > SCREEN_H)
            npc[i].vel.y *= -1;
    }
}

// ================= 玩家 =================
void UpdatePlayer(void)
{
    if (IsKeyDown(KEY_A)) player.pos.x -= 4;
    if (IsKeyDown(KEY_D)) player.pos.x += 4;
    if (IsKeyDown(KEY_W)) player.pos.y -= 4;
    if (IsKeyDown(KEY_S)) player.pos.y += 4;
}

// ================= 碰撞 =================
void CheckCollision(void)
{
    for (int i = 0; i < MAX_FISH; i++)
    {
        if (CheckCollisionCircles(
            player.pos, player.size,
            npc[i].pos, npc[i].size))
        {
            if (player.size >= npc[i].size)
            {
                player.size += 2;
                score += 10;
                npc[i].pos = (Vector2){
                    GetRandomValue(0, SCREEN_W),
                    GetRandomValue(0, SCREEN_H)
                };
            }
            else
            {
                uiState = UI_DEAD;
            }
        }
    }
}

// ================= UI 输入 =================
void UpdateUI(void)
{
    switch (uiState)
    {
    case UI_TITLE:
        if (IsKeyPressed(KEY_DOWN)) menuIndex = (menuIndex + 1) % 1;
        if (IsKeyPressed(KEY_UP))   menuIndex = (menuIndex + 1) % 1;

        if (IsKeyPressed(KEY_ENTER))
        {
            InitGame();
            uiState = UI_PLAYING;
        }
        break;

    case UI_PLAYING:
        UpdatePlayer();
        CheckCollision();
        score++;
        break;

    case UI_DEAD:
        if (IsKeyPressed(KEY_ESCAPE))
            uiState = UI_TITLE;
        break;
    }
}

// ================= 绘制 =================
void DrawNPC(void)
{
    for (int i = 0; i < MAX_FISH; i++)
        DrawCircleV(npc[i].pos, npc[i].size, BLUE);
}

void DrawUI(void)
{
    if (uiState == UI_TITLE)
    {
        DrawRectangle(
            SCREEN_W / 3, SCREEN_H / 4,
            SCREEN_W / 3, SCREEN_H / 2,
            Fade(BLACK, 0.7f));

        DrawText("FISH GAME",
            SCREEN_W / 2 - 120,
            SCREEN_H / 3,
            40, WHITE);

        DrawText("> START",
            SCREEN_W / 2 - 60,
            SCREEN_H / 2,
            24, YELLOW);
    }
    else if (uiState == UI_PLAYING)
    {
        DrawText(TextFormat("Score: %d", score),
            20, 20, 24, WHITE);
    }
    else if (uiState == UI_DEAD)
    {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.7f));

        DrawText("YOU DIED",
            SCREEN_W / 2 - 100,
            SCREEN_H / 2 - 40,
            40, RED);

        DrawText(TextFormat("Score: %d", score),
            SCREEN_W / 2 - 80,
            SCREEN_H / 2 + 10,
            24, WHITE);

        DrawText("Press ESC to return",
            SCREEN_W / 2 - 130,
            SCREEN_H / 2 + 50,
            20, GRAY);
    }
}

// ================= main =================
int main(void)
{
    InitWindow(SCREEN_W, SCREEN_H, "Fish Game UI Demo");
    SetTargetFPS(60);

    srand(time(NULL));

    while (!WindowShouldClose())
    {
        UpdateNPC();   // ⭐ NPC 永远更新
        UpdateUI();    // ⭐ UI 决定玩家/死亡

        BeginDrawing();
        ClearBackground(DARKBLUE);

        DrawNPC();     // 背景鱼
        DrawCircleV(player.pos, player.size, ORANGE);

        DrawUI();      // UI 盖在最上面

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
