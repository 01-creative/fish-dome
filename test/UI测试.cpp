#include <raylib.h>
#include <time.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define min(a,b)(((a)<(b))?(a):(b))
#define max(a,b)(((a)>(b))?(a):(b))
#define Vector2Add(a,b) (Vector2){(a).x+(b).x,(a).y+(b).y}
#define Vector2Sub(a,b) (Vector2){(a).x-(b).x,(a).y-(b).y}
#define MAX_fish 50
#define sizetimes 1.4
//|************************UI颜色定义***********************************************|
#define BACKGROUND_COLOR CLITERAL(Color){ 30, 30, 40, 255 }     // 深色背景
#define BUTTON_NORMAL CLITERAL(Color){ 80, 80, 100, 255 }      // 按钮正常颜色
#define BUTTON_HOVER CLITERAL(Color){ 100, 100, 120, 255 }     // 按钮悬停颜色
#define BUTTON_SELECTED CLITERAL(Color){ 60, 150, 200, 255 }   // 按钮选中颜色
#define TEXT_COLOR RAYWHITE                                   // 文字颜色
#define HIGHLIGHT_COLOR YELLOW
#define TEXT_COLOR WHITE//UI颜色
//|************************UI菜单及设置参数声明********************************************************|
typedef enum { START, SETTINGS, EXIT, COUNT } MenuItem;
typedef enum {
	SETTINGS_VOLUME,
	SETTINGS_DIFFICULTY,
	SETTINGS_RESOLUTION,
	SETTINGS_BACK,
	SETTINGS_COUNT
} SettingsItem;
// 设置菜单状态
int in_settings_menu = 0;
SettingsItem settings_selected = SETTINGS_VOLUME;
int settings_end = 1;
// 设置项的值
float game_volume = 0.8f;  // 0.0-1.0
int game_difficulty = 1;   // 0=简单, 1=普通, 2=困难
int game_resolution = 0;   // 0=800x600, 1=1024x768, 2=1280x720, 3=1920x1080

//|*****************************************************************************************|
typedef struct fish {
	Vector2 xy;
	Vector2 v_xy;
	double a;//加速度
	double size;//圆的半径
	int image_status;
	int lizixiaoguo;
	int kinds;//0是玩家，1小丑鱼，2是河豚
}fish;

typedef struct fish_NPC {
	int active;
	Vector2 aim[7];//鱼的移动目标点
	fish fish;
}fish_NPC;

typedef struct {
	fish_NPC fishnpc[MAX_fish];
	int used[MAX_fish];
	int active_count;
} fishPool;

// ===== UI STATE =====
typedef enum {
	UI_TITLE,
	UI_PLAYING,
	UI_DEAD
} UIState;

UIState uiState = UI_TITLE;
int ui_menu_index = 0;
int ui_score = 0;

int menu_end = 1;
MenuItem selected = START;//UI数据


int running = 1;//0是退出，1是正常，2是暂停，3是死亡
int sleeptime = 100, screen_length_x = 2048, screen_length_y = 1152, runingtime = 0;
fish player = { {screen_length_x / 2,screen_length_y / 2},{0,0},5,30 * sizetimes,0,0,0 };

float bg1_x = 0;
float bg2_x = 0;
float bg3_x = 0;
float bg4_x = 0;//背景偏移量

void playermove(fish* player);
void npc_move(fish_NPC* npc);
void init_fish_pool(fishPool* pool);
fish_NPC* get_fish(fishPool* pool);
void release_fish(fishPool* pool, fish_NPC* fishPtr);
void update_all_fish(fishPool* pool);
fish_NPC* create_npcfish(fishPool* pool, float x, float y, int kind);
Vector2 get_legal_point(void);
void collision_npc(fishPool* pool);
void draw_background(void);

void UI_Update(fishPool* pool);
void UI_Draw(void);
void UI_DrawTitle(void);
void UI_DrawPlaying(void);
void UI_DrawDead(void);

void menu_logic(void);
void draw_menu(void);
void draw_settings_menu(void);
void settings_menu_logic(void);

Texture bg1, bg2, bg3, bg4;
bool isPaused = false;

int main() {
	screen_length_x = 2048;
	screen_length_y = 1152;

	// 使用默认分辨率
	game_resolution = 0;  // 默认800x600

	// 根据默认分辨率设置窗口大小
	switch (game_resolution) {
	case 0: screen_length_x = 800; screen_length_y = 600; break;
	case 1: screen_length_x = 1024; screen_length_y = 768; break;
	case 2: screen_length_x = 1280; screen_length_y = 720; break;
	case 3: screen_length_x = 1920; screen_length_y = 1080; break;
	}
	




	InitWindow(screen_length_x, screen_length_y, "fish game");
	int monitor = GetCurrentMonitor();
	int monitorWidth = GetMonitorWidth(monitor);
	int monitorHeight = GetMonitorHeight(monitor);
	SetWindowPosition(
		(monitorWidth - screen_length_x) / 2,
		(monitorHeight - screen_length_y) / 2
	);
	Texture xiaocouyu_texture[2] = { LoadTexture("../img/fish/小丑鱼 左向 模糊.png"),LoadTexture("../img/fish/小丑鱼 左向 清晰.png") }, xiaocouyu_texture2[2] = { LoadTexture("../img/fish/小丑鱼 右向 模糊.png"),LoadTexture("../img/fish/小丑鱼 右向 清晰.png") };
	Texture hetun_texture[2] = { LoadTexture("../img/fish/河豚 左向.png"),LoadTexture("../img/fish/河豚 右向.png") };
	Texture jianyu_texture[2] = { LoadTexture("../img/fish/剑鱼 左向.png"),LoadTexture("../img/fish/剑鱼 右向.png") };
	Texture shayu_texture[2] = { LoadTexture("../img/fish/鲨鱼 左向.png"),LoadTexture("../img/fish/鲨鱼 右向.png") };
	bg1 = LoadTexture("../img/bg/Background1.png"); bg2 = LoadTexture("../img/bg/Background2.png"); bg3 = LoadTexture("../img/bg/Background3.png"); bg4 = LoadTexture("../img/bg/Background4.png");


	while (0) {
	start:
		
		 ui_menu_index = 0;
		 ui_score = 0;
         running = 1;//0是退出，1是正常，2是暂停，3是死亡
		 sleeptime = 100;
		 screen_length_x = 2048;
		 screen_length_y = 1152;
		 runingtime = 0;
		player.xy.x =screen_length_x / 2;
		player.xy.y = screen_length_y / 2;
		player.v_xy.x = 0;
		player.v_xy.y = 0;
		player.a = 5;
		player.size = 30 * sizetimes;
		player.image_status = 0;
		player.lizixiaoguo = 0;
		player.kinds = 0;
		 bg1_x = 0;
		 bg2_x = 0;
		 bg3_x = 0;
		bg4_x = 0;//背景偏移量
	}

	SetWindowState(FLAG_VSYNC_HINT);
	fishPool pool;
	init_fish_pool(&pool);

	//**************************************************************主循环********************************************************************//
	while (!WindowShouldClose()) {
		if (IsKeyPressed(KEY_SPACE)) {
			isPaused = !isPaused;  // 切换暂停状态
		}

		if (!isPaused) {
			Vector2 randpoint = get_legal_point();
			if (runingtime % 39 == 0)create_npcfish(&pool, randpoint.x, randpoint.y, rand() % 4 + 1);
			srand(time(NULL));

			update_all_fish(&pool);
			UI_Update(&pool);
			if (uiState == UI_DEAD && IsKeyPressed(KEY_ENTER)) {
				uiState = UI_TITLE;
			    running = 1;
				goto start;
			}
		}
		
		BeginDrawing();            // 开始绘制
		ClearBackground(BLACK);    // 清除上一帧
		
		if (uiState == 1) {//游戏进行中
			
			draw_background();
			//渲染玩家鱼
			if (player.v_xy.x > 0)
			{
				if (runingtime % 10 < 5)
					DrawTextureEx(xiaocouyu_texture2[0], player.xy, 0, player.size / (32.0 * sizetimes), WHITE);
				else DrawTextureEx(xiaocouyu_texture2[1], player.xy, 0, player.size / (32.0 * sizetimes), WHITE);
			}
			else {
				if (runingtime % 10 < 5)
					DrawTextureEx(xiaocouyu_texture[0], player.xy, 0, player.size / (32.0 * sizetimes), WHITE);
				else DrawTextureEx(xiaocouyu_texture[1], player.xy, 0, player.size / (32.0 * sizetimes), WHITE);
			}


			//渲染所有NPC鱼
			for (int i = 0; i < MAX_fish; i++) {
				if (pool.used[i]) {
					switch (xiaocouyu_texture2, pool.fishnpc[i].fish.kinds) {
					case 1:if (pool.fishnpc[i].fish.v_xy.x > 0) {
						if (runingtime % 10 < 5)DrawTextureEx(xiaocouyu_texture2[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (32.0 * sizetimes), WHITE);
						else DrawTextureEx(xiaocouyu_texture2[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (32.0 * sizetimes), WHITE);
					}
						  else {
						if (runingtime % 10 < 5)
							DrawTextureEx(xiaocouyu_texture[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (32.0 * sizetimes), WHITE);
						else DrawTextureEx(xiaocouyu_texture[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (32.0 * sizetimes), WHITE);
					}
						  break;
					case 2:if (pool.fishnpc[i].fish.v_xy.x > 0) {
						DrawTextureEx(hetun_texture[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (28.0 * sizetimes), WHITE);
					}
						  else { DrawTextureEx(hetun_texture[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (28.0 * sizetimes), WHITE); }

						  break;
					case 3:if (pool.fishnpc[i].fish.v_xy.x > 0) {
						DrawTextureEx(jianyu_texture[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (24.0 * sizetimes), WHITE);
					}
						  else { DrawTextureEx(jianyu_texture[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (24.0 * sizetimes), WHITE); }

						  break;
					case 4:if (pool.fishnpc[i].fish.v_xy.x > 0) {
						DrawTextureEx(shayu_texture[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (24.0 * sizetimes), WHITE);
					}
						  else { DrawTextureEx(shayu_texture[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (24.0 * sizetimes), WHITE); }

						  break;

					}
				}
				if (IsKeyDown(KEY_Q)) {
					Vector2 cj;
					switch (pool.fishnpc[i].fish.kinds)
					{
					case 1:
					case 2:

						cj.x = pool.fishnpc[i].fish.xy.x + pool.fishnpc[i].fish.size * 1.4;
						cj.y = pool.fishnpc[i].fish.xy.y + pool.fishnpc[i].fish.size * 1.3;

						break;
					case 3:

						cj.x = pool.fishnpc[i].fish.xy.x + pool.fishnpc[i].fish.size * 2;
						cj.y = pool.fishnpc[i].fish.xy.y + pool.fishnpc[i].fish.size * 2;

						break;
					case 4:

						cj.x = pool.fishnpc[i].fish.xy.x + pool.fishnpc[i].fish.size * 2;
						cj.y = pool.fishnpc[i].fish.xy.y + pool.fishnpc[i].fish.size * 2;

						break;
					default:
						cj.x = pool.fishnpc[i].fish.xy.x + pool.fishnpc[i].fish.size;
						cj.y = pool.fishnpc[i].fish.xy.y + pool.fishnpc[i].fish.size;

						break;
					}
					DrawCircle(cj.x, cj.y, pool.fishnpc[i].fish.size, RED);

				}

			}

			BeginDrawing();
			if (isPaused) {
				// 半透明覆盖层
				DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));

				DrawText("Pause",
					GetScreenWidth() / 2 - 100,
					GetScreenHeight() / 2 - 60,
					40,
					RED);
				DrawText("PRESS SPACE TO RETURN",
					GetScreenWidth() / 2 - 160,
					GetScreenHeight() / 2 + 50,
					20,
					LIGHTGRAY);
			}
		}
		
	//游戏进行中

	UI_Draw();   // 永远最后画，盖在最上面



	if (!isPaused && uiState == 1) {
		DrawText("Press SPACE to pause", 10, 10, 20, WHITE);
	}







	/*if (running == 3) {
		DrawText("You Died! Press ESC to Exit.", screen_length_x / 2 - 150, screen_length_y / 2, 20, RED);
	}*/
	
	EndDrawing();

	if (!isPaused)runingtime++;
	WaitTime(0.05);
}
		/*if (running == 3) {
			WaitTime(5);
			break;
		}*/
	




	return 0;
}




void playermove(fish* player) {
	if (IsKeyDown(KEY_ESCAPE)) {
		running = 2;
		return;
	}
	player->v_xy.x *= 0.9;
	player->v_xy.y *= 0.8;
	float derta_vx = 0, derta_vy = 0;
	if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) derta_vx += player->a;
	if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) derta_vx -= player->a;
	if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) derta_vy += player->a;
	if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) derta_vy -= player->a;
	if (derta_vx != 0 && derta_vy != 0) {
		derta_vx *= 0.7071f;  // 1/√2
		derta_vy *= 0.7071f;
	}
	player->v_xy.x += derta_vx;
	player->v_xy.y += derta_vy;
	player->xy.x += player->v_xy.x;
	player->xy.y += player->v_xy.y;
	if (player->xy.x < 0)
	{
		player->xy.x = 0; player->v_xy.x = 0;
	}
	if (player->xy.x > (screen_length_x - player->size))
	{
		player->xy.x = (screen_length_x - player->size); player->v_xy.x = 0.1;
	}
	if (player->xy.y < 0)
	{
		player->xy.y = 0; player->v_xy.y = 0;
	}
	if (player->xy.y > (screen_length_y - player->size))
	{
		player->xy.y = (screen_length_y - player->size); player->v_xy.y = 0.1;
	}


}



void init_fish_pool(fishPool* pool) {
	for (int i = 0; i < MAX_fish; i++) {
		pool->used[i] = 0;
		pool->fishnpc[i].active = 0;
		pool->fishnpc[i].fish.xy.x = 0;
		pool->fishnpc[i].fish.xy.y = 0;
		pool->fishnpc[i].active = 0;
	}
	pool->active_count = 0;
}


fish_NPC* get_fish(fishPool* pool) {
	for (int i = 0; i < MAX_fish; i++) {
		if (pool->used[i] == 0) {
			pool->used[i] = 1;
			pool->active_count++;
			pool->fishnpc[i].active = 1;
			return &pool->fishnpc[i];
		}
	}
	return 0;
}


void release_fish(fishPool* pool, fish_NPC* fishPtr) {
	int index = fishPtr - pool->fishnpc;
	if (index >= 0 && index < MAX_fish) {
		pool->used[index] = 0;
		pool->active_count--;
		fishPtr->active = 0;
	}
}
void update_all_fish(fishPool* pool) {
	for (int i = 0; i < MAX_fish; i++) {
		if (pool->used[i]) {
			fish_NPC* fishPtr = &pool->fishnpc[i];

			fishPtr->fish.xy.x += fishPtr->fish.v_xy.x;
			fishPtr->fish.xy.y += fishPtr->fish.v_xy.y;

			if (fishPtr->fish.xy.x - fishPtr->aim[0].x <= 1.0 && fishPtr->fish.xy.y - fishPtr->aim[0].y <= 1.0) {
				for (int p = 0; p < 7; p++) {
					fishPtr->aim[p] = fishPtr->aim[p + 1];
				}
			}
			fishPtr->aim[6] = { 3000,2000 };
			if (rand() % 10 > 1) {
				float k = rand() % 40 / 100.0 + 0.8;
				Vector2 direction = { fishPtr->aim[0].x - fishPtr->fish.xy.x,fishPtr->aim[0].y - fishPtr->fish.xy.y };
				fishPtr->fish.v_xy.x += k * (direction.x) * fishPtr->fish.a / sqrt((direction.x) * (direction.x) + (direction.y) * (direction.y));
				fishPtr->fish.v_xy.y += k * (direction.y) * fishPtr->fish.a / sqrt((direction.x) * (direction.x) + (direction.y) * (direction.y));
			}
			fishPtr->fish.v_xy.x *= 0.95;
			fishPtr->fish.v_xy.y *= 0.9;
			// 边界检查
			if (fishPtr->fish.xy.x < -300 || fishPtr->fish.xy.x > screen_length_x + 300 ||
				fishPtr->fish.xy.y < -150 || fishPtr->fish.xy.y > screen_length_y + 150) {
				release_fish(pool, fishPtr);
			}
		}
	}
}

fish_NPC* create_npcfish(fishPool* pool, float x, float y, int kind) {
	fish_NPC* newFish = get_fish(pool);
	int numberofaim = 5;
	if (newFish == NULL) {
		return 0; // 池已满，无法创建新鱼
	}
	newFish->fish.xy.x = x;
	newFish->fish.xy.y = y;
	switch (kind)
	{
	case 1: {
		newFish->fish.v_xy.x = (rand() % 7) - 2;
		newFish->fish.v_xy.y = (rand() % 7) - 2;
		newFish->fish.size = (20 + rand() % 30) * sizetimes;
		newFish->fish.a = 1;
		newFish->fish.kinds = kind;
		newFish->fish.image_status = kind;
		for (int i = 0; i < numberofaim; i++) {
			newFish->aim[i].x = rand() % screen_length_x * 1.2;
			newFish->aim[i].y = rand() % screen_length_y * 1.2;
		}

	}
		  break;
	case 2: {
		newFish->fish.v_xy.x = (rand() % 7) - 2;
		newFish->fish.v_xy.y = (rand() % 7) - 2;
		newFish->fish.size = (15 + rand() % 25) * sizetimes;
		newFish->fish.a = 0.6;
		newFish->fish.kinds = kind;
		newFish->fish.image_status = kind;
		for (int i = 0; i < numberofaim; i++) {
			newFish->aim[i].x = rand() % screen_length_x * 1.2;
			newFish->aim[i].y = rand() % screen_length_y * 1.2;
		}

	}
		  break;
	case 3: {
		newFish->fish.v_xy.x = (rand() % 10) - 4;
		newFish->fish.v_xy.y = (rand() % 10) - 4;
		newFish->fish.size = (35 + rand() % 40) * sizetimes;
		newFish->fish.a = 3;
		newFish->fish.kinds = kind;
		newFish->fish.image_status = kind;
		for (int i = 0; i < numberofaim; i++) {
			newFish->aim[i].x = rand() % screen_length_x * 1.2;
			newFish->aim[i].y = rand() % screen_length_y * 0.1 + newFish->fish.xy.y;

		}

	}
		  break;
	case 4: {
		newFish->fish.v_xy.x = (rand() % 7) - 2;
		newFish->fish.v_xy.y = (rand() % 7) - 2;
		newFish->fish.size = (50 + rand() % 60) * sizetimes;
		newFish->fish.a = 2;
		newFish->fish.kinds = kind;
		newFish->fish.image_status = kind;
		for (int i = 0; i < numberofaim; i++) {
			newFish->aim[i].x = rand() % screen_length_x * 1.2;
			newFish->aim[i].y = rand() % screen_length_y * 0.5 + newFish->fish.xy.y;

		}
	}
		  break;
	default:
		return 0;
		break;
	}
	newFish->aim[numberofaim].x = max(100 * rand() % screen_length_x, 3000);
	newFish->aim[numberofaim].y = max(100 * rand() % screen_length_y, 2000);
	return newFish;
}

Vector2 get_legal_point(void) {

	Vector2 point;
	if (rand() % 2 == 0)
		point.x = rand() % 100 - 100;
	else
		point.x = rand() % 100 + screen_length_x;
	point.y = rand() % screen_length_y;

	return point;
}

void collision_npc(fishPool* pool) {
	for (int i = 0; i < MAX_fish; i++) {
		if (pool->used[i]) {
			Vector2 ci;
			switch (pool->fishnpc[i].fish.kinds)
			{
			case 1:
			case 2:

				ci.x = pool->fishnpc[i].fish.xy.x + pool->fishnpc[i].fish.size * 1.4;
				ci.y = pool->fishnpc[i].fish.xy.y + pool->fishnpc[i].fish.size * 1.3;

				break;
			case 3:

				ci.x = pool->fishnpc[i].fish.xy.x + pool->fishnpc[i].fish.size * 2;
				ci.y = pool->fishnpc[i].fish.xy.y + pool->fishnpc[i].fish.size * 2;

				break;
			case 4:

				ci.x = pool->fishnpc[i].fish.xy.x + pool->fishnpc[i].fish.size * 2;
				ci.y = pool->fishnpc[i].fish.xy.y + pool->fishnpc[i].fish.size * 2;

				break;
			default:
				ci.x = pool->fishnpc[i].fish.xy.x + pool->fishnpc[i].fish.size;
				ci.y = pool->fishnpc[i].fish.xy.y + pool->fishnpc[i].fish.size;

				break;
			}

			/*Vector2 ci = { pool->fishnpc[i].fish.xy.x + pool->fishnpc[i].fish.size,
				pool->fishnpc[i].fish.xy.y + pool->fishnpc[i].fish.size };*/

			for (int j = i + 1; j < MAX_fish; j++) {
				if (pool->used[j]) {

					Vector2 cj;
					switch (pool->fishnpc[j].fish.kinds)
					{
					case 1:
					case 2:
						cj.x = pool->fishnpc[j].fish.xy.x + pool->fishnpc[j].fish.size * 1.4;
						cj.y = pool->fishnpc[j].fish.xy.y + pool->fishnpc[j].fish.size * 1.3;
						break;
					case 3:

						cj.x = pool->fishnpc[j].fish.xy.x + pool->fishnpc[j].fish.size * 2;
						cj.y = pool->fishnpc[j].fish.xy.y + pool->fishnpc[j].fish.size * 2;
						break;
					case 4:

						cj.x = pool->fishnpc[j].fish.xy.x + pool->fishnpc[j].fish.size * 2;
						cj.y = pool->fishnpc[j].fish.xy.y + pool->fishnpc[j].fish.size * 2;
						break;
					default:
						break;
					}

					if (fabs(ci.x + ci.y - cj.x - cj.y) <
						pool->fishnpc[i].fish.size + pool->fishnpc[j].fish.size) {

						if (CheckCollisionCircles(ci, pool->fishnpc[i].fish.size, cj, pool->fishnpc[j].fish.size)) {
							if (pool->fishnpc[i].fish.size > pool->fishnpc[j].fish.size) {
								release_fish(pool, &pool->fishnpc[j]);
								pool->fishnpc[i].fish.size += sqrt(pool->fishnpc[j].fish.size) * 0.2;
								Vector2 target{ pool->fishnpc[j].fish.xy.x - pool->fishnpc[i].fish.xy.x,pool->fishnpc[j].fish.xy.y - pool->fishnpc[i].fish.xy.y };
								pool->fishnpc[i].fish.v_xy.x += 5 * pool->fishnpc[i].fish.a * target.x / sqrt((target.x) * (target.x) + (target.y) * (target.y));
								pool->fishnpc[i].fish.v_xy.y += 5 * pool->fishnpc[i].fish.a * target.y / sqrt((target.x) * (target.x) + (target.y) * (target.y));
							}
							else {
								release_fish(pool, &pool->fishnpc[i]);
								pool->fishnpc[j].fish.size += sqrt(pool->fishnpc[i].fish.size) * 0.2;
								Vector2 target{ pool->fishnpc[i].fish.xy.x - pool->fishnpc[j].fish.xy.x,pool->fishnpc[i].fish.xy.y - pool->fishnpc[j].fish.xy.y };
								pool->fishnpc[j].fish.v_xy.x += 5 * pool->fishnpc[j].fish.a * target.x / sqrt((target.x) * (target.x) + (target.y) * (target.y));
								pool->fishnpc[j].fish.v_xy.y += 5 * pool->fishnpc[j].fish.a * target.y / sqrt((target.x) * (target.x) + (target.y) * (target.y));
							}
						}
					}
				}
			}

			Vector2 cplayer = {
				player.xy.x + player.size * 1.4,
				player.xy.y + player.size * 1.3
			};

			if (fabs(ci.x + ci.y - cplayer.x - cplayer.y) <
				pool->fishnpc[i].fish.size + player.size) {

				if (CheckCollisionCircles(
					ci,
					pool->fishnpc[i].fish.size,
					cplayer,
					player.size)) {

					if (pool->fishnpc[i].fish.size > player.size) {
						running = 3;
					}
					else {
						release_fish(pool, &pool->fishnpc[i]);
						player.size += sqrt(pool->fishnpc[i].fish.size) * 0.2;
						Vector2 target{ pool->fishnpc[i].fish.xy.x - player.xy.x,pool->fishnpc[i].fish.xy.y - player.xy.y };
						player.v_xy.x += 3 * player.a * target.x / sqrt((target.x) * (target.x) + (target.y) * (target.y));
						player.v_xy.y += 3 * player.a * target.y / sqrt((target.x) * (target.x) + (target.y) * (target.y));
					}
				}
			}
		}
	}
}
void draw_background(void)
{
	float scale1 = (float)screen_length_y / bg1.height;
	float scale2 = (float)screen_length_y / bg2.height;
	float scale3 = (float)screen_length_y / bg3.height;
	float scale4 = (float)screen_length_y / bg4.height;

	float w1 = bg1.width * scale1;
	float w2 = bg2.width * scale2;
	float w3 = bg3.width * scale3;
	float w4 = bg4.width * scale4;

	if (!isPaused) {
		bg1_x -= player.v_xy.x * 0.05f;
		bg2_x -= player.v_xy.x * 0.10f;
		bg3_x -= player.v_xy.x * 0.20f;
		bg4_x -= player.v_xy.x * 0.35f;
	}

	if (bg1_x <= -w1) bg1_x += w1;
	if (bg1_x >= w1) bg1_x -= w1;

	if (bg2_x <= -w2) bg2_x += w2;
	if (bg2_x >= w2) bg2_x -= w2;

	if (bg3_x <= -w3) bg3_x += w3;
	if (bg3_x >= w3) bg3_x -= w3;

	if (bg4_x <= -w4) bg4_x += w4;
	if (bg4_x >= w4) bg4_x -= w4;

	Rectangle src1 = { 0, 0, bg1.width, bg1.height };
	Rectangle src2 = { 0, 0, bg2.width, bg2.height };
	Rectangle src3 = { 0, 0, bg3.width, bg3.height };
	Rectangle src4 = { 0, 0, bg4.width, bg4.height };
	Vector2 origin = { 0, 0 };
	Rectangle bg1x = { bg1_x + w1 / 2, 0, w1, screen_length_y }, bg1y = { bg1_x - w1 / 2, 0, w1, screen_length_y };
	Rectangle bg2x = { bg2_x + w2 / 2, 0, w2, screen_length_y }, bg2y = { bg2_x - w2 / 2, 0, w2, screen_length_y };
	Rectangle bg3x = { bg3_x + w3 / 2, 0, w3, screen_length_y }, bg3y = { bg3_x - w3 / 2, 0, w3, screen_length_y };
	Rectangle bg4x = { bg4_x + w3 / 2, 0, w4, screen_length_y }, bg4y = { bg4_x - w4 / 2, 0, w4, screen_length_y };
	DrawTexturePro(bg1, src1, bg1x, origin, 0, WHITE);
	DrawTexturePro(bg1, src1, bg1y, origin, 0, WHITE);

	DrawTexturePro(bg2, src2, bg2x, origin, 0, WHITE);
	DrawTexturePro(bg2, src2, bg2y, origin, 0, WHITE);

	DrawTexturePro(bg3, src3, bg3x, origin, 0, WHITE);
	DrawTexturePro(bg3, src3, bg3y, origin, 0, WHITE);

	DrawTexturePro(bg4, src4, bg4x, origin, 0, WHITE);
	DrawTexturePro(bg4, src4, bg4y, origin, 0, WHITE);
}

void UI_Update(fishPool* pool)
{
	switch (uiState)
	{
	case UI_TITLE:
		PollInputEvents();
		menu_logic();
		break;

	case UI_PLAYING:
		playermove(&player);
		collision_npc(pool);
		ui_score++;
		if (running == 3)
		{
			uiState = UI_DEAD;
		}
		break;

	case UI_DEAD:
		//if (IsKeyPressed(KEY_ENTER))
		//{
			//uiState = UI_TITLE;
			//running = 1;

		//}
		break;
	}
}
void UI_Draw(void)
{
	if (uiState == UI_TITLE) UI_DrawTitle();
	else if (uiState == UI_PLAYING) UI_DrawPlaying();
	else if (uiState == UI_DEAD) UI_DrawDead();
}
void UI_DrawTitle(void)
{
	DrawRectangle(
		screen_length_x / 3,
		screen_length_y / 4,
		screen_length_x / 3,
		screen_length_y / 2,
		Fade(BLACK, 0.7f)
	);
	draw_menu();
}
void UI_DrawPlaying(void)
{
	DrawText(
		TextFormat("Score: %d", ui_score),
		20,
		20,
		24,
		WHITE
	);
	
}
void UI_DrawDead(void)
{
	DrawRectangle(
		0,
		0,
		screen_length_x,
		screen_length_y,
		Fade(BLACK, 0.7f)
	);

	DrawText(
		"YOU DIED",
		screen_length_x / 2 - 100,
		screen_length_y / 2 - 60,
		40,
		RED
	);

	DrawText(
		TextFormat("Score: %d", ui_score),
		screen_length_x / 2 - 80,
		screen_length_y / 2,
		24,
		WHITE
	);

	DrawText(
		"PRESS ENTER TO RETURN",
		screen_length_x / 2 - 160,
		screen_length_y / 2 + 50,
		20,
		GRAY
	);
}
void draw_menu(void) {
	static Rectangle buttons[COUNT];  
	

	const char* menuTexts[COUNT] = {
		"START",
		"SETTINGS",
		"EXIT"
	};

		// 计算居中位置
		float buttonWidth = 300.0f;  // 适当加大宽度
		float buttonHeight = 80.0f;  // 适当加大高度
		float buttonSpacing = 30.0f;  // 按钮间距

		// 计算总高度
		float totalHeight = COUNT * buttonHeight + (COUNT - 1) * buttonSpacing;

		// 垂直居中计算
		float startY = (screen_length_y - totalHeight) / 2.0f;

		// 水平居中计算
		float startX = (screen_length_x - buttonWidth) / 2.0f;

		// 初始化按钮位置
		buttons[START] = { startX, startY, buttonWidth, buttonHeight };
		buttons[SETTINGS] ={ startX, startY + buttonHeight + buttonSpacing, buttonWidth, buttonHeight };
		buttons[EXIT] = { startX, startY + 2 * (buttonHeight + buttonSpacing), buttonWidth, buttonHeight };


	

	// 绘制背景
	ClearBackground(BACKGROUND_COLOR);

	// 绘制标题
	int titleWidth = MeasureText("FISH GAME", 60);
	int titleX = (screen_length_x - titleWidth) / 2;
	DrawText("FISH GAME", titleX, 150, 60, SKYBLUE);

	// 绘制所有菜单项
	for (int i = 0; i < COUNT; i++) {
		Color buttonColor = BUTTON_NORMAL;
		Color textColor = TEXT_COLOR;
		Color borderColor = DARKGRAY;

		// 获取鼠标位置
		Vector2 mouse = GetMousePosition();

		// 检查鼠标悬停
		bool isHovered = CheckCollisionPointRec(mouse, buttons[i]);

		// 设置颜色
		if (i == selected) {
			buttonColor = BUTTON_SELECTED;  // 当前选中的项
			textColor = HIGHLIGHT_COLOR;
			borderColor = HIGHLIGHT_COLOR;
		}
		else if (isHovered) {
			buttonColor = BUTTON_HOVER;     // 鼠标悬停
			textColor = WHITE;
		}

		// 绘制按钮背景（圆角矩形）
		DrawRectangleRounded(buttons[i], 0.3f, 10, buttonColor);
		DrawRectangleLinesEx(buttons[i], 2, borderColor);


		// 计算文字居中位置
		int textWidth = MeasureText(menuTexts[i], 30);
		int textX = (int)(buttons[i].x + (buttons[i].width - textWidth) / 2.0f);
		int textY = (int)(buttons[i].y + (buttons[i].height - 30) / 2.0f);

		// 绘制文字
		DrawText(menuTexts[i], textX, textY, 30, textColor);

		// 如果是选中项，绘制指示器
		if (i == selected) {
			float centerY = buttons[i].y + buttons[i].height / 2.0f;

			// 左侧箭头指示器
			DrawTriangle(
			{
				buttons[i].x - 15.0f, centerY
			},
			 {
				buttons[i].x - 5.0f, centerY - 10.0f
			},
			{
				buttons[i].x - 5.0f, centerY + 10.0f
			},
				HIGHLIGHT_COLOR
			);

			// 右侧箭头指示器
			DrawTriangle(
			{
				buttons[i].x + buttons[i].width + 15.0f, centerY
			},
			{
				buttons[i].x + buttons[i].width + 5.0f, centerY - 10.0f
			},
		    {
				buttons[i].x + buttons[i].width + 5.0f, centerY + 10.0f
			},
				HIGHLIGHT_COLOR
			);

			// 在箭头后面加个小矩形
			DrawRectangle((int)(buttons[i].x - 30), (int)(centerY - 3), 20, 6, HIGHLIGHT_COLOR);
			DrawRectangle((int)(buttons[i].x + buttons[i].width + 10), (int)(centerY - 3), 20, 6, HIGHLIGHT_COLOR);
		}
	}

	// 绘制操作提示
	int hintWidth = MeasureText("USE ARROW KEYS TO SELECT, ENTER TO CONFIRM, OR CLICK", 25);
	int hintX = (screen_length_x - hintWidth) / 2;
	DrawText("USE ARROW KEYS TO SELECT, ENTER TO CONFIRM, OR CLICK", hintX, screen_length_y - 100, 25, LIGHTGRAY);

	//int escWidth = MeasureText("ESC TO RETURN", 25);
	//int escX = (screen_length_x - escWidth) / 2;
	//DrawText("ESC TO RETURN", escX, screen_length_y - 70, 25, LIGHTGRAY);
}

void menu_logic(void) {
	selected = START;
	menu_end = 1;
	// 菜单项位置



		
	
	
	
	while (menu_end&& !WindowShouldClose()) {
		Rectangle buttons[COUNT];
		// 计算居中位置
		float buttonWidth = 300.0f;
		float buttonHeight = 80.0f;
		float buttonSpacing = 30.0f;
		float totalHeight = COUNT * buttonHeight + (COUNT - 1) * buttonSpacing;
		float startY = (screen_length_y - totalHeight) / 2.0f;
		float startX = (screen_length_x - buttonWidth) / 2.0f;

		// 初始化按钮位置
		buttons[START] = { startX, startY, buttonWidth, buttonHeight };
		buttons[SETTINGS] = { startX, startY + buttonHeight + buttonSpacing, buttonWidth, buttonHeight };
		buttons[EXIT] = { startX, startY + 2 * (buttonHeight + buttonSpacing), buttonWidth, buttonHeight };

		Vector2 mouse = GetMousePosition();
		// 鼠标控制
		for (int i = START; i < COUNT; i++) {
			if (CheckCollisionPointRec(mouse, buttons[i]) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
				if (selected == (MenuItem)i) menu_end = 0;  // 选择相同项则退出菜单
					selected = (MenuItem)i;
				
			}
		}
		// 键盘控制
		if (IsKeyPressed(KEY_DOWN)) selected = (MenuItem)(((int)selected + 1) % COUNT);
		if (IsKeyPressed(KEY_UP)) selected = (MenuItem)(((int)selected - 1 + COUNT) % COUNT);
        if (IsKeyPressed(KEY_ENTER))menu_end = 0;
		BeginDrawing();
		draw_menu();
		EndDrawing();
	}
	if (selected == START) {
		uiState = UI_PLAYING;
		running = 1;
		ui_score = 0;
	}
	else if (selected == SETTINGS) {
		// 设置菜单逻辑
		in_settings_menu = 1;
		settings_menu_logic();
		in_settings_menu = 0;
		// 设置菜单返回后，重新显示主菜单
		menu_logic();  // 递归调用，重新显示主菜单
		return;  // 返回，避免执行下面的代码
		
	}
	else if (selected == EXIT) {
		CloseWindow();
		exit(0);
	}

}
void draw_settings_menu(void) {
	// 设置项文本
	const char* settingsTexts[SETTINGS_COUNT] = {
		"VOLUME",
		"DIFFICULTY",
		"RESOLUTION",
		"BACK"
	};

	// 值文本
	const char* difficultyTexts[] = { "EASY", "NORMAL", "HARD" };
	const char* resolutionTexts[] = { "800x600", "1024x768", "1280x720", "1920x1080" };

	// 计算设置项位置
	int itemWidth = 400;
	int itemHeight = 60;
	int itemSpacing = 20;
	int totalHeight = SETTINGS_COUNT * itemHeight + (SETTINGS_COUNT - 1) * itemSpacing;
	int startY = (screen_length_y - totalHeight) / 3;  // 从屏幕1/3处开始
	int startX = (screen_length_x - itemWidth) / 2;

	// 绘制半透明背景
	DrawRectangle(0, 0, screen_length_x, screen_length_y, Fade(BLACK, 0.7f));

	// 绘制设置菜单背景
	DrawRectangle(
		startX - 20,
		startY - 20,
		itemWidth + 40,
		totalHeight + 100,
    {
		40, 40, 50, 240
	}
	);
	DrawRectangleLines(
		startX - 20,
		startY - 20,
		itemWidth + 40,
		totalHeight + 100,
		SKYBLUE
	);

	// 绘制标题
	int titleWidth = MeasureText("SETTINGS", 50);
	DrawText("SETTINGS", (screen_length_x - titleWidth) / 2, startY - 80, 50, SKYBLUE);

	// 绘制所有设置项
	for (int i = 0; i < SETTINGS_COUNT; i++) {
		Color itemColor = BUTTON_NORMAL;
		Color textColor = TEXT_COLOR;
		Color valueColor = LIGHTGRAY;

		// 检查鼠标悬停
		Vector2 mouse = GetMousePosition();
		Rectangle itemRect = { startX, startY + i * (itemHeight + itemSpacing), itemWidth, itemHeight };
		bool isHovered = CheckCollisionPointRec(mouse, itemRect);

		// 设置颜色
		if (i == settings_selected) {
			itemColor = BUTTON_SELECTED;
			textColor = HIGHLIGHT_COLOR;
			valueColor = HIGHLIGHT_COLOR;
		}
		else if (isHovered) {
			itemColor = BUTTON_HOVER;
		}

		// 绘制设置项背景
		DrawRectangleRounded(itemRect, 0.2f, 8, itemColor);
		DrawRectangleLinesEx(itemRect, 2, (i == settings_selected) ? HIGHLIGHT_COLOR : DARKGRAY);

		// 绘制设置项名称
		DrawText(settingsTexts[i], startX + 20,
			startY + i * (itemHeight + itemSpacing) + (itemHeight - 30) / 2, 30, textColor);

		// 绘制设置项值
		const char* valueText = "";
		switch (i) {
		case SETTINGS_VOLUME:
			valueText = TextFormat("%.0f%%", game_volume * 100);
			// 绘制音量条
			DrawRectangle(startX + 200, startY + i * (itemHeight + itemSpacing) + 20, 150, 20, DARKGRAY);
			DrawRectangle(startX + 200, startY + i * (itemHeight + itemSpacing) + 20,
				(int)(150 * game_volume), 20, SKYBLUE);
			DrawRectangleLines(startX + 200, startY + i * (itemHeight + itemSpacing) + 20, 150, 20, WHITE);
			break;

		case SETTINGS_DIFFICULTY:
			valueText = difficultyTexts[game_difficulty];
			break;

		case SETTINGS_RESOLUTION:
			valueText = resolutionTexts[game_resolution];
			break;

		case SETTINGS_BACK:
			valueText = "";
			break;
		}

		int valueWidth = MeasureText(valueText, 25);
		DrawText(valueText, startX + itemWidth - valueWidth - 20,
			startY + i * (itemHeight + itemSpacing) + (itemHeight - 25) / 2, 25, valueColor);

		// 如果是当前选中项，绘制指示器
		if (i == settings_selected) {
			float centerY = startY + i * (itemHeight + itemSpacing) + itemHeight / 2.0f;

			// 左侧箭头
			DrawTriangle(
			{
				startX - 15.0f, centerY
			},
			{
				startX - 5.0f, centerY - 10.0f
			},
			{
				startX - 5.0f, centerY + 10.0f
			},
				HIGHLIGHT_COLOR
			);

			// 右侧箭头
			DrawTriangle(
		   {
				startX + itemWidth + 15.0f, centerY
		   },
		    {
				startX + itemWidth + 5.0f, centerY - 10.0f
			},
			{
				startX + itemWidth + 5.0f, centerY + 10.0f
			},
				HIGHLIGHT_COLOR
			);
		}
	}

	// 绘制操作提示
	DrawText("USE ARROW KEYS TO NAVIGATE, LEFT/RIGHT TO ADJUST, ENTER TO CONFIRM",
		(screen_length_x - MeasureText("USE ARROW KEYS TO NAVIGATE, LEFT/RIGHT TO ADJUST, ENTER TO CONFIRM", 20)) / 2,
		screen_length_y - 60, 20, LIGHTGRAY);
	DrawText("ESC TO RETURN",
		(screen_length_x - MeasureText("ESC TO RETURN", 20)) / 2,
		screen_length_y - 30, 20, LIGHTGRAY);
}
void settings_menu_logic(void) {
	settings_selected = SETTINGS_VOLUME;
	settings_end = 1;

	while (settings_end && !WindowShouldClose()) {
		Vector2 mouse = GetMousePosition();

		// 计算设置项位置
		int itemWidth = 400;
		int itemHeight = 60;
		int itemSpacing = 20;
		int totalHeight = SETTINGS_COUNT * itemHeight + (SETTINGS_COUNT - 1) * itemSpacing;
		int startY = (screen_length_y - totalHeight) / 3;
		int startX = (screen_length_x - itemWidth) / 2;

		// 鼠标控制
		for (int i = 0; i < SETTINGS_COUNT; i++) {
			Rectangle itemRect = { startX, startY + i * (itemHeight + itemSpacing), itemWidth, itemHeight };
			if (CheckCollisionPointRec(mouse, itemRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
				settings_selected = (SettingsItem)i;
				if (i == SETTINGS_BACK) {
					settings_end = 0;
				}
			}
		}

		// 键盘控制
		if (IsKeyPressed(KEY_DOWN)) {
			settings_selected = (SettingsItem)((settings_selected + 1) % SETTINGS_COUNT);
		}
		if (IsKeyPressed(KEY_UP)) {
			settings_selected = (SettingsItem)((settings_selected - 1 + SETTINGS_COUNT) % SETTINGS_COUNT);
		}

		// 调整设置值
		if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_LEFT)) {
			int direction = IsKeyPressed(KEY_RIGHT) ? 1 : -1;

			switch (settings_selected) {
			case SETTINGS_VOLUME:
				game_volume += direction * 0.1f;
				if (game_volume < 0.0f) game_volume = 0.0f;
				if (game_volume > 1.0f) game_volume = 1.0f;
				// 这里可以添加设置音量的代码
				// SetMasterVolume(game_volume);
				break;

			case SETTINGS_DIFFICULTY:
				game_difficulty += direction;
				if (game_difficulty < 0) game_difficulty = 0;
				if (game_difficulty > 2) game_difficulty = 2;
				break;

			case SETTINGS_RESOLUTION:
				int oldResolution = game_resolution;
				game_resolution += direction;
				if (game_resolution < 0) game_resolution = 0;
				if (game_resolution > 3) game_resolution = 3;
				// 如果分辨率改变了，立即应用
				if (game_resolution != oldResolution) {
					int newWidth, newHeight;
					switch (game_resolution) {
					case 0: newWidth = 800; newHeight = 600; break;
					case 1: newWidth = 1024; newHeight = 768; break;
					case 2: newWidth = 1280; newHeight = 720; break;
					case 3: newWidth = 1920; newHeight = 1080; break;
					default: newWidth = 2048; newHeight = 1152; break;
					}

					// 更新窗口大小
					screen_length_x = newWidth;
					screen_length_y = newHeight;
					SetWindowSize(screen_length_x, screen_length_y);

					// 居中窗口
					int monitor = GetCurrentMonitor();
					int monitorWidth = GetMonitorWidth(monitor);
					int monitorHeight = GetMonitorHeight(monitor);
					SetWindowPosition(
						(monitorWidth - screen_length_x) / 2,
						(monitorHeight - screen_length_y) / 2
					);

					// 短暂显示提示
					BeginDrawing();
					ClearBackground(BLACK);
					DrawText("Resolution changed",
						screen_length_x / 2 - 100,
						screen_length_y / 2 - 20,
						30, SKYBLUE);
					EndDrawing();
					WaitTime(0.3);
				}
				break;
			}
		}

		// 确认/返回
		if (IsKeyPressed(KEY_ENTER)) {
			if (settings_selected == SETTINGS_BACK) {
				settings_end = 0;
			}
			else {
				settings_end = 0;
			}
		}

		if (IsKeyPressed(KEY_ESCAPE)) {
			settings_end = 0;
		}

		// 绘制
		BeginDrawing();
		draw_menu();  // 先绘制主菜单背景
		draw_settings_menu();  // 再绘制设置菜单
		EndDrawing();
	}
}

















