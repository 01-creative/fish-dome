
#include <raylib.h>
#include <time.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define min(a,b)(((a)<(b))?(a):(b))
#define max(a,b)(((a)>(b))?(a):(b))

typedef struct fish {
	double x;
	double y;
	double v_x;
	double v_y;
	double a;//加速度
	double size;
	int aim[1][1];
	int image_status;
	int lizixiaoguo;
	int kinds;//0是玩家，
}fish;


fish player = { 0,0,0,0,5,40,NULL,0,0,0 };
int running = 1;//0是退出，1是正常，2是暂停，3是死亡
int sleeptime = 100, screen_length_x = 2048, screen_length_y = 1152;

void playermove(fish* player);
int main() {





	InitWindow(screen_length_x, screen_length_y, "fish game");
	Texture2D fish_texture = LoadTexture("D:/任务/游戏项目/img/鱼test.png"), fish_texture2 = LoadTexture("D:/任务/游戏项目/img/鱼 翻转 test.png");
	while (running) {


		playermove(&player);

		BeginDrawing();            // 开始绘制
		ClearBackground(BLACK);    // 清除上一帧
		Vector2 test = { player.x,player.y };
		if (player.v_x > 0)
		{
			DrawTextureEx(fish_texture2, test, 0, 0.08, WHITE);
		}
		else {
			DrawTextureEx(fish_texture, test, 0, 0.08, WHITE);
		}

		//DrawCircle(player.x, player.y, player.size, RED);
		WaitTime(0.05);
		EndDrawing();


	}




	return 0;
}




void playermove(fish* player) {
	if (IsKeyDown(KEY_ESCAPE)) {
		running = 2;
		return;
	}
	player->v_x *= 0.9;
	player->v_y *= 0.8;
	float derta_vx = 0, derta_vy = 0;
	if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) derta_vx += player->a;
	if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) derta_vx -= player->a;
	if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) derta_vy += player->a;
	if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) derta_vy -= player->a;
	if (derta_vx != 0 && derta_vy != 0) {
		derta_vx *= 0.7071f;  // 1/√2
		derta_vy *= 0.7071f;
	}
	player->v_x += derta_vx;
	player->v_y += derta_vy;
	player->x += player->v_x;
	player->y += player->v_y;
	if (player->x < 0)
	{
		player->x = 0; player->v_x = 0;
	}
	if (player->x > (screen_length_x - player->size))
	{
		player->x = (screen_length_x - player->size); player->v_x = 0.1;
	}
	if (player->y < 0)
	{
		player->y = 0; player->v_y = 0;
	}
	if (player->y > (screen_length_y - player->size))
	{
		player->y = (screen_length_y - player->size); player->v_y = 0.1;
	}
}








