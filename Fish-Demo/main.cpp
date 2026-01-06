
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
	double size;//大小
	double threatsize;//额外大小
	int image_status;
	int lizixiaoguo;
	int kinds;//0是玩家，1小丑鱼，2是河豚,3剑鱼，4鲨鱼,5变异剑鱼,6变异剑鱼2，7变异鲨鱼，8变异鲨鱼2，9变异河豚
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

typedef struct {
	int active;
	float shift;
	float target;
	float tone;
} DepthTransition;

static DepthTransition depthTrans = { 0, 0, 0, 0 };

// ===== UI STATE =====
typedef enum {
	UI_TITLE,
	UI_PLAYING,
	UI_DEAD,
	UI_STOP,
	UI_SETTING
} UIState;

UIState uiState = UI_TITLE;
int ui_menu_index = 0;
int ui_score = 0;

int menu_end = 1;
MenuItem selected = START;//UI数据

int running = 1;//0是退出，1是正常，2是暂停，3是死亡
int sleeptime = 100, screen_length_x = 2048, screen_length_y = 1152, runingtime = 0;
fish player = { {screen_length_x / 2,screen_length_y / 2},{0,0},5,30.0 * sizetimes,30.0 * sizetimes,0,0,0 };//玩家初始化
int difficult = 1.5, jieduan = 0;  double san = 100;
float bg1_x = 0;
float bg2_x = 0;
float bg3_x = 0;
float bg4_x = 0;//背景偏移量

int slow_timer = 0, super_slow_timer = 0,mutation=0, ate_mutant_fish=0;float fade = 1.0f;

void playermove(fish* player);
void npc_move(fish_NPC* npc);
void init_fish_pool(fishPool* pool);
fish_NPC* get_fish(fishPool* pool);
void release_fish(fishPool* pool, fish_NPC* fishPtr);
void update_all_fish(fishPool* pool);
fish_NPC* create_npcfish(fishPool* pool, float x, float y, int kind);
Vector2 get_legal_point(void);
void collision_npc(fishPool* pool);
void draw_background1(void);

void UI_Update(fishPool* pool);
void UI_Draw(void);
void UI_DrawTitle(void);
void UI_DrawDead(void);
void draw_menu(void);
void draw_settings_menu(void);
void settings_menu_logic(void);
bool isPaused = false;


void change_difficult(fishPool* pool);
void draw_depth_filter(void);
void draw_player_glow(void);
void draw_fish_glow(fishPool* pool);
void DrawFishAutoFlip(
	Texture2D tex_left,
	Vector2 pos,
	float size,
	float baseSize,
	Color tint,
	float vx
);
void playermutation(void);
static float sign_point(Vector2 p1, Vector2 p2, Vector2 p3);
static int point_in_triangle(Vector2 pt, Vector2 v1, Vector2 v2, Vector2 v3);
int check_jitan_sacrifice(Texture jitan);
void draw_menu(void);

int main() {
	screen_length_x = 2048;
	screen_length_y = 1152;

	// 使用默认分辨率
	game_resolution = 3;
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

	// ================= 玩家贴图 =================

	Texture player_tex[2] = {
		LoadTexture("../img/fish/玩家 左向 模糊.png"),
		LoadTexture("../img/fish/玩家 左向 清晰.png")
	};
	Texture player_gray_left =
		LoadTexture("../img/fish/玩家 灰度 左向.png");
	Texture player_tex_by[6] = {
		LoadTexture("../img/fish/玩家 变异1 左向.png"),
		LoadTexture("../img/fish/玩家 变异2 左向.png"),
		LoadTexture("../img/fish/玩家 变异3 左向.png"),
		LoadTexture("../img/fish/玩家 变异4 左向.png"),
		LoadTexture("../img/fish/玩家 变异5 左向.png"),
		LoadTexture("../img/fish/玩家 变异6 左向.png")
	};


	// ================= NPC 鱼贴图 =================

	Texture xiaochouyu_left =
		LoadTexture("../img/fish/小丑鱼 左向.png");

	Texture hetun_left =
		LoadTexture("../img/fish/河豚 左向.png");
	Texture hetun_puffed_left =
		LoadTexture("../img/fish/河豚 鼓起 左向.png");

	Texture jianyu_left =
		LoadTexture("../img/fish/剑鱼 左向.png");
	Texture jianyu_b1_left =
		LoadTexture("../img/fish/剑鱼 变异1 左向.png");
	Texture jianyu_b2_left =
		LoadTexture("../img/fish/剑鱼 变异2 左向.png");

	Texture shayu_left =
		LoadTexture("../img/fish/鲨鱼 左向.png");
	Texture shayu_b1_left =
		LoadTexture("../img/fish/鲨鱼 变异1 左向.png");
	Texture shayu_b2_left =
		LoadTexture("../img/fish/鲨鱼 变异2 左向.png");

	Texture hetun_b1_left =
		LoadTexture("../img/fish/河豚 变异1 左向.png");
	Texture hetun_puffed_b1_left =
		LoadTexture("../img/fish/河豚 鼓起 变异1 左向.png");
	Texture jitan =
		LoadTexture("../img/other/祭坛.png");

	SetWindowState(FLAG_VSYNC_HINT);
	fishPool pool;
	init_fish_pool(&pool);
	srand(time(NULL));

	
	static Rectangle buttons[COUNT]; 
	static float toumingdu = 0;       
	static int size_threshold[] = {   
		55,  // jieduan 0 -> 1
		95,  // 1 -> 2
		140, // 2 -> 3
		230, // 3 -> 4
		320  // 4 -> 5
	};

	//**************************************************************主循环********************************************************************//
	while (!WindowShouldClose()) {
		static Texture stop_sight;
		switch (uiState) {
		case UI_TITLE:
		{
			
			const char* menuTexts[COUNT] = {
				"START",
				"SETTINGS",
				"EXIT"
			};
			float buttonWidth = 300.0f;
			float buttonHeight = 80.0f;
			float buttonSpacing = 30.0f;
			float totalHeight = COUNT * buttonHeight + (COUNT - 1) * buttonSpacing;
			float startY = (screen_length_y - totalHeight) / 2.0f;
			float startX = (screen_length_x - buttonWidth) / 2.0f;
			Rectangle a = { startX, startY, buttonWidth, buttonHeight };
			Rectangle b = { startX, startY + buttonHeight + buttonSpacing, buttonWidth, buttonHeight };
			Rectangle c = { startX, startY + 2 * (buttonHeight + buttonSpacing), buttonWidth, buttonHeight };
			buttons[START] = a;
			buttons[SETTINGS] = b;
			buttons[EXIT] = c;
			Vector2 mouse = GetMousePosition();
			for (int i = 0; i < COUNT; i++) {
				if (CheckCollisionPointRec(mouse, buttons[i]) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
					if (selected == (MenuItem)i) {
						if (selected == START) {
							uiState = UI_PLAYING;
							running = 1;
						}
						else if (selected == SETTINGS) {
							uiState = UI_SETTING;
						}
					}
					selected = (MenuItem)i;
				}
			}
			if (IsKeyPressed(KEY_DOWN)) {
				selected = (MenuItem)(((int)selected + 1) % COUNT);
			}
			if (IsKeyPressed(KEY_UP)) {
				selected = (MenuItem)(((int)selected - 1 + COUNT) % COUNT);
			}
			if (IsKeyPressed(KEY_ENTER)) {
				if (selected == START) {
					uiState = UI_PLAYING;
					running = 1;
				}
				else if (selected == SETTINGS) {
					uiState = UI_SETTING;
					settings_selected = SETTINGS_VOLUME;
				}
				else if (selected == EXIT) {
					CloseWindow();
					return 0;
				}
			}
			
			
			BeginDrawing();
			draw_menu();       
			EndDrawing();

			break;
		}

		case UI_PLAYING:
		{
			uiState = UI_PLAYING;
			change_difficult(&pool);
			playermove(&player);
			playermutation();
			update_all_fish(&pool);
			collision_npc(&pool);
			BeginDrawing();
			ClearBackground(BLACK);

			draw_background1();
			draw_depth_filter();
			draw_player_glow();
			draw_fish_glow(&pool);

			// 注意：toumingdu已经在外部声明，这里直接使用
			if (jieduan == 5) {
				if (toumingdu >= 0.95) {
					if (check_jitan_sacrifice(jitan)) {
						// 祭坛检查逻辑
					}
				}
			}

			float t = depthTrans.tone;
			t = t * t;   // 和光圈一致

			// 亮度系数：0.4 ~ 1.0
			float brightness = 1.0f - t * 0.6f;
			if (brightness < 0.4f) brightness = 0.4f;

			unsigned char v = (unsigned char)(255 * brightness);

			Color fishTint = { v, v, v, 255 };

			// 渲染玩家鱼
			Color stateTint = WHITE;

			// 如果中毒（减速中）
			if (slow_timer > 0) {
				Color green = { 180, 255, 180, 255 };
				stateTint = green;  // 淡绿色
			}

			Color playerTint = {
				(unsigned char)(stateTint.r * brightness),
				(unsigned char)(stateTint.g * brightness),
				(unsigned char)(stateTint.b * brightness),
				(unsigned char)(255 * fade)
			};

			Color grayTint = { v, v, v, 255 };

			// ================= 玩家灰度底 =================
			DrawFishAutoFlip(
				player_gray_left,
				player.xy,
				player.size,
				32.0f,
				grayTint,
				player.v_xy.x
			);

			// ================= 玩家本体动画 =================
			if (mutation == 0) {
				int frame = (runingtime % 10 < 5) ? 0 : 1;
				DrawFishAutoFlip(
					player_tex[frame],
					player.xy,
					player.size,
					32.0f,
					playerTint,
					player.v_xy.x
				);
			}
			else {
				DrawFishAutoFlip(
					player_tex_by[mutation - 1],
					player.xy,
					player.size,
					32.0f,
					playerTint,
					player.v_xy.x
				);
			}
			// 玩家渲染结束

			// 渲染所有NPC鱼
			for (int i = 0; i < MAX_fish; i++) {
				if (!pool.used[i]) continue;

				fish* f = &pool.fishnpc[i].fish;

				switch (f->kinds) {
				case 1:
					DrawFishAutoFlip(xiaochouyu_left, f->xy, f->size, 24.0f, fishTint, f->v_xy.x);
					break;
				case 2:
					DrawFishAutoFlip(
						f->image_status == 2 ? hetun_puffed_left : hetun_left,
						f->xy, f->size, 28.0f, fishTint, f->v_xy.x
					);
					break;
				case 3:
					DrawFishAutoFlip(jianyu_left, f->xy, f->size, 18.0f, fishTint, f->v_xy.x);
					break;
				case 4:
					DrawFishAutoFlip(shayu_left, f->xy, f->size, 20.0f, fishTint, f->v_xy.x);
					break;
				case 5:
					DrawFishAutoFlip(jianyu_b1_left, f->xy, f->size, 18.0f, fishTint, f->v_xy.x);
					break;
				case 6:
					DrawFishAutoFlip(jianyu_b2_left, f->xy, f->size, 18.0f, fishTint, f->v_xy.x);
					break;
				case 7:
					DrawFishAutoFlip(shayu_b1_left, f->xy, f->size, 20.0f, fishTint, f->v_xy.x);
					break;
				case 8:
					DrawFishAutoFlip(shayu_b2_left, f->xy, f->size, 20.0f, fishTint, f->v_xy.x);
					break;
				case 9:
					DrawFishAutoFlip(
						f->image_status == 2 ? hetun_puffed_b1_left : hetun_b1_left,
						f->xy, f->size, 28.0f, fishTint, f->v_xy.x
					);
					break;
				}

				if (IsKeyDown(KEY_Q)) { // 调试用代码，显示碰撞箱与大小
					Vector2 cj;
					switch (pool.fishnpc[i].fish.kinds) {
					case 2:
					case 9:
						cj.x = pool.fishnpc[i].fish.xy.x + pool.fishnpc[i].fish.size * 1.4;
						cj.y = pool.fishnpc[i].fish.xy.y + pool.fishnpc[i].fish.size * 1.3;
						break;
					case 1:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
					case 8:
						cj.x = pool.fishnpc[i].fish.xy.x + pool.fishnpc[i].fish.size * 2;
						cj.y = pool.fishnpc[i].fish.xy.y + pool.fishnpc[i].fish.size * 2;
						break;
					default:
						cj.x = pool.fishnpc[i].fish.xy.x + pool.fishnpc[i].fish.size;
						cj.y = pool.fishnpc[i].fish.xy.y + pool.fishnpc[i].fish.size;
						break;
					}

					DrawCircle(cj.x, cj.y, pool.fishnpc[i].fish.size, RED);
					DrawText(
						TextFormat("Size: %.2f", pool.fishnpc[i].fish.threatsize),
						cj.x,
						cj.y,
						24,
						WHITE
					);
				}
			}
			// NPC鱼渲染结束

			// 注意：size_threshold数组已经在外部声明
			if (runingtime % 10 == 0) {
				printf("size:%.2f+threat:%.2f 阶段：%d 下阶段门槛：%lf time:%d san:%.3f fade:%.3f\n",
					player.size, player.threatsize, jieduan,
					((size_threshold[jieduan] * sizetimes) * san / 100),
					runingtime, san, fade);
			}

			// 调试用代码，显示玩家大小和当前阶段
			if (jieduan == 5) {
				if (toumingdu < 1.0f)
					toumingdu = (toumingdu + 0.005f < 1.0f) ? toumingdu + 0.005f : 1.0f;

				Color a = { 255, 255, 255, (unsigned char)(toumingdu * 255) };
				Rectangle src = { 0, 0, (float)jitan.width, (float)jitan.height };
				Rectangle dst = {
					(screen_length_x - jitan.width * 3.0f) * 0.5f,
					screen_length_y - jitan.height * 3.0f,
					jitan.width * 3.0f,
					jitan.height * 3.0f
				};
				Vector2 origin = { 0, 0 };
				DrawTexturePro(jitan, src, dst, origin, 0.0f, a);
			}

			runingtime++;
			san += 0.0001;
			san -= 0.001 * jieduan;

			if (!isPaused && uiState == UI_PLAYING) {
				DrawText("Press SPACE to pause", 10, 10, 20, WHITE);
			}

			DrawText(
				TextFormat("Score: %d", runingtime),
				20,
				20,
				24,
				WHITE
			);

			
			EndDrawing();

		 runingtime++;

			if (IsKeyPressed(KEY_SPACE)) {
				Image stop_img = LoadImageFromScreen();
				static bool IF_img_used = 0;
				if (IF_img_used) UnloadTexture(stop_sight);
				stop_sight = LoadTextureFromImage(stop_img);
				UnloadImage(stop_img);
				IF_img_used = true;
				uiState = UI_STOP;
				PollInputEvents();
			}

			if (running == 3) {
				uiState = UI_DEAD;
			}
			break;
		}

		case UI_DEAD:
		{
			uiState = UI_DEAD;
			if (IsKeyPressed(KEY_ENTER)) {
				uiState = UI_TITLE;
				PollInputEvents();
				ui_menu_index = 0;
				ui_score = 0;
				running = 1;//0是退出，1是正常，2是暂停，3是死亡
				sleeptime = 100;
				screen_length_x = 2048;
				screen_length_y = 1152;
				runingtime = 0;
				player.xy.x = screen_length_x / 2;
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
			BeginDrawing();
			UI_DrawDead();
			EndDrawing();
		}
			break;

		case UI_STOP:
		{
			if (IsKeyPressed(KEY_SPACE)) {
				uiState = UI_PLAYING;
				PollInputEvents();
			}
			BeginDrawing();
			DrawTexture(stop_sight, 0, -screen_length_y/2, WHITE);
			DrawRectangle(0, 0,
				screen_length_x,
				screen_length_y,
				Fade(BLACK, 0.4f));

			DrawText("Press SPACE to pause",screen_length_x/2-50, screen_length_y/2, 30, RED);
			EndDrawing();
			
		}
			break;

		case UI_SETTING:
		{
			
			settings_menu_logic();
			
			break;
		}
		}

		WaitTime(0.05);
	}
	return 0;
}

	



void DrawFishAutoFlip(
	Texture2D tex_left,
	Vector2 pos,
	float size,
	float baseSize,
	Color tint,
	float vx
)
{
	Rectangle src = {
		0,
		0,
		(float)tex_left.width,
		(float)tex_left.height
	};

	if (vx > 0) { // 向右游 → 翻转
		src.x = tex_left.width;
		src.width = -src.width;
	}

	float scale = size / (baseSize * sizetimes);

	Rectangle dst = {
		pos.x,
		pos.y,
		tex_left.width * scale,
		tex_left.height * scale
	};
	Vector2 origin = {
		0,
		0
	};
	DrawTexturePro(
		tex_left,
		src,
		dst,
		origin,
		0.0f,
		tint
	);
}





void playermove(fish* player) {
	static float init_a = 5.0f;
	static int last_mutation = 0;
	if (IsKeyDown(KEY_SPACE)) {
		running = 2;
		return;
	}
	if (mutation != last_mutation) {
		last_mutation = mutation;
		init_a += 0.5;
	}
	
    player->size = 80*atan((player->threatsize-42)/80)+42;
	if (fade <= 0.6f) { fade -= 0.0001; }
	fade = max(fade, 0);
	if(super_slow_timer>0){
		super_slow_timer--;
		if(super_slow_timer==0){
			init_a *= 0.85;
			player->a = init_a;
		}
	}
	if (slow_timer > 0) {
		slow_timer--;
		if (slow_timer == 0) {
			player->a = init_a;
		}
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
	if (mutation >= 3) {
		static int cooldown = 0;
		static int keyPressCount[4] = { 0 };
		static double lastPressTime[4] = { 0 };
		double currentTime = GetTime();
		int keyIndex = -1;
		if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) keyIndex = 0;
		else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) keyIndex = 1;
		else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) keyIndex = 2;
		else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) keyIndex = 3;

		if (keyIndex != -1 && !cooldown) {
			if (currentTime - lastPressTime[keyIndex] < 0.3) {
				switch (keyIndex) {
				case 0: derta_vx += player->a * 10; break;
				case 1: derta_vx -= player->a * 10; break;
				case 2: derta_vy += player->a * 10; break;
				case 3: derta_vy -= player->a * 10; break;
				}
				cooldown = 100;
				keyPressCount[keyIndex] = 0;
			}
			else {
				keyPressCount[keyIndex] = 1;
			}
			lastPressTime[keyIndex] = currentTime;
		}

		if (cooldown > 0) cooldown--;
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
		pool->fishnpc[i].fish.size = 0;
		pool->fishnpc[i].fish.threatsize = 0;
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
	static int puffer_timer[MAX_fish] = { 0 };

	for (int i = 0; i < MAX_fish; i++) {
		if (pool->used[i]) {
			fish_NPC* fishPtr = &pool->fishnpc[i];
			if (fishPtr->fish.kinds ==3&& fishPtr->fish.kinds == 4)fishPtr->fish.size = fishPtr->fish.threatsize;
				else fishPtr->fish.size = 80 * atan((fishPtr->fish.threatsize - 42) / 80) + 42;

			fishPtr->fish.xy.x += fishPtr->fish.v_xy.x;
			fishPtr->fish.xy.y += fishPtr->fish.v_xy.y;
			if (fishPtr->fish.kinds == 2) {

				if (fishPtr->fish.image_status == 2) {
					// 已鼓起，倒计时
					if (--puffer_timer[i] <= 0) {
						fishPtr->fish.image_status = 1;
					}
				}
				else {
					// 正常状态，检测玩家是否在前方
					Vector2 f = fishPtr->fish.v_xy;
					float fl = sqrt(f.x * f.x + f.y * f.y);
					if (fl > 0.01f) {
						f.x /= fl; f.y /= fl;
						Vector2 toP = {
							player.xy.x - fishPtr->fish.xy.x,
							player.xy.y - fishPtr->fish.xy.y
						};
						float d = sqrt(toP.x * toP.x + toP.y * toP.y);

						if (d < 220.0f * difficult) {
							toP.x /= d; toP.y /= d;
							if (f.x * toP.x + f.y * toP.y > 0.7f && rand() % 100 < (30 * difficult)) {
								fishPtr->fish.image_status = 2; // 鼓起
								puffer_timer[i] = 80;
							}
						}
					}
				}
			}
			if (fishPtr->fish.kinds == 9) {

				if (fishPtr->fish.image_status == 2) {
					// 已鼓起，倒计时
					if (--puffer_timer[i] <= 0) {
						fishPtr->fish.image_status = 1;
					}
				}
				else {
					// 正常状态，检测玩家是否在前方
					Vector2 f = fishPtr->fish.v_xy;
					float fl = sqrt(f.x * f.x + f.y * f.y);
					if (fl > 0.01f) {
						f.x /= fl; f.y /= fl;
						Vector2 toP = {
							player.xy.x - fishPtr->fish.xy.x,
							player.xy.y - fishPtr->fish.xy.y
						};
						float d = sqrt(toP.x * toP.x + toP.y * toP.y);

						if (d < 320.0f * difficult) {
							toP.x /= d; toP.y /= d;
							if (f.x * toP.x + f.y * toP.y > 0.7f && rand() % 100 < (40 * difficult)) {
								fishPtr->fish.image_status = 2; // 鼓起
								puffer_timer[i] = 110;
							}
						}
					}
				}
			}



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

	static int kind_min_size[] = {
	0,    // kind 0（不用）
	20,   // kind 1
	15,   // kind 2
	45,   // kind 3
	60,   // kind 4

	65,  // kind 5
	120,  // kind 6
	100,  // kind 7
	145,  // kind 8
	45	// kind 9
	};

	static int kind_max_size[] = {
		0,
		60,
		50,
		90,
		130,
		135,//剑鱼变异1
		180,//剑鱼变异2
		160,//鲨鱼变异1
		220,//鲨鱼变异2
		70
	};

	fish_NPC* newFish = get_fish(pool);
	int numberofaim = 5;
	if (newFish == NULL) {
		return 0; // 池已满，无法创建新鱼
	}
	/*double change = difficult * sqrt(player.size * (jieduan + 1)) * 0.15-0.05;*/
		// 危险鱼尺寸解锁
	static int danger_min_size[] = {
		53, 67, 89, 135, 149, 180
	};

	static int danger_max_size[] = {
		70, 110,150, 220, 220, 230
	};


	static int danger_rate[] = {
		20, 30, 25, 35, 40, 50
	};
	int is_danger = (rand() % 100) < danger_rate[jieduan], threatsize0 = 1;
	if (!is_danger)
	{
		int min_size = (int)(player.threatsize * 0.4f);
		int max_size = (int)(player.threatsize * 0.8f);
		if (min_size < 10) min_size = 10;
		if (max_size <= min_size) max_size = min_size + 1;
		threatsize0 = GetRandomValue(min_size, max_size);
	}
	else
	{
		int min_size = danger_min_size[jieduan] * difficult;
		int max_size = danger_max_size[jieduan] * difficult;
		threatsize0 = GetRandomValue(min_size, min(max_size, player.threatsize * 1.2));
		int must_be_bigger = (int)(player.threatsize * 1.2f);
		if (threatsize0 < must_be_bigger)
			threatsize0 = must_be_bigger;
	}while (kind > 0)
	{
		if (threatsize0 < kind_min_size[kind] * difficult)
		{
			kind--;          // 尺寸太小，降级
			continue;
		}

		if (threatsize0 > kind_max_size[kind] * difficult)
		{
			release_fish(pool, newFish); // 尺寸过大，世界拒绝
			return 0;
		}

		break; // 合法
	}


	newFish->fish.xy.x = x;
	newFish->fish.xy.y = y;
	newFish->fish.v_xy.x = (rand() % 7) - 2;
	newFish->fish.v_xy.y = (rand() % 7) - 2;
	newFish->fish.threatsize = threatsize0 * sizetimes;
	newFish->fish.kinds = kind;
	switch (kind)
	{
	case 1: {
		newFish->fish.a = 1 * difficult;
		newFish->fish.image_status = kind;
		for (int i = 0; i < numberofaim; i++) {
			newFish->aim[i].x = rand() % screen_length_x * 1.2;
			newFish->aim[i].y = rand() % screen_length_y * 1.2;
		}

	}break;
	case 2: {
		newFish->fish.a = 0.6 * difficult;
		newFish->fish.image_status = 1;
		for (int i = 0; i < numberofaim; i++) {
			newFish->aim[i].x = rand() % screen_length_x * 1.2;
			newFish->aim[i].y = rand() % screen_length_y * 1.2;
		}
	} break;
	case 3: {
		newFish->fish.a = 3 * difficult;
		newFish->fish.image_status = kind;
		for (int i = 0; i < numberofaim; i++) {
			newFish->aim[i].x = rand() % screen_length_x * 1.2;
			newFish->aim[i].y = rand() % screen_length_y * 0.05 + newFish->fish.xy.y;
		}
	}break;
	case 4: {
		newFish->fish.a = 2 * difficult;
		newFish->fish.image_status = kind;
		for (int i = 0; i < numberofaim; i++) {
			newFish->aim[i].x = rand() % screen_length_x * 1.2;
			newFish->aim[i].y = rand() % screen_length_y * 0.5 + newFish->fish.xy.y;
		}
	} break;
	case 5: {
		newFish->fish.a = 4* difficult;
		newFish->fish.image_status = kind;
		for (int i = 0; i < numberofaim; i++) {
			newFish->aim[i].x = rand() % screen_length_x * 1.1;
			newFish->aim[i].y = rand() % screen_length_y * 0.1+ newFish->fish.xy.y;
		}
	} break;
	case 6: {
		newFish->fish.a = 5.5f * difficult;
		newFish->fish.image_status = kind;
		for (int i = 0; i < numberofaim; i++) {
			newFish->aim[i].x = rand() % screen_length_x * 1.2;
			newFish->aim[i].y = rand() % screen_length_y * 0.01 + newFish->fish.xy.y;
		}
	}break;
	case 7: {
		newFish->fish.a = 3 * difficult;
		newFish->fish.image_status = kind;
		for (int i = 0; i < numberofaim; i++) {
			newFish->aim[i].x = rand() % screen_length_x * 1.1;
			newFish->aim[i].y = rand() % screen_length_y * 0.3 + newFish->fish.xy.y;
		}
	} break;
	case 8: {
		newFish->fish.a = 3.5 * difficult;
		newFish->fish.image_status = kind;
		for (int i = 0; i < numberofaim; i++) {
			newFish->aim[i].x = rand() % screen_length_x * 1.1;
			newFish->aim[i].y = rand() % screen_length_y * 0.3 + newFish->fish.xy.y;
		}
	} break;
	case 9: {
		newFish->fish.a = 1.2 * difficult;
		newFish->fish.image_status = 1;
		for (int i = 0; i < numberofaim; i++) {
			newFish->aim[i].x = rand() % screen_length_x * 1.2;
			newFish->aim[i].y = rand() % screen_length_y * 1.2;
		}
	} break;
	default:
		release_fish(pool, newFish);
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
		point.x = rand() % 100 - 200;
	else
		point.x = rand() % 100 + screen_length_x + 100;
	point.y = rand() % screen_length_y;

	return point;
}

void collision_npc(fishPool* pool) {
	for (int i = 0; i < MAX_fish; i++) {
		if (pool->used[i]) {
			Vector2 ci;
			switch (pool->fishnpc[i].fish.kinds)
			{

			case 2:
			case 9:
				ci.x = pool->fishnpc[i].fish.xy.x + pool->fishnpc[i].fish.size * 1.4;
				ci.y = pool->fishnpc[i].fish.xy.y + pool->fishnpc[i].fish.size * 1.3;

				break;
			case 1:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
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

					case 2:
					case 9:
						cj.x = pool->fishnpc[j].fish.xy.x + pool->fishnpc[j].fish.size * 1.4;
						cj.y = pool->fishnpc[j].fish.xy.y + pool->fishnpc[j].fish.size * 1.3;
						break;
					case 1:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
					case 8:
						cj.x = pool->fishnpc[j].fish.xy.x + pool->fishnpc[j].fish.size * 2;
						cj.y = pool->fishnpc[j].fish.xy.y + pool->fishnpc[j].fish.size * 2;
						break;
					default:
						break;
					}
					if (fabs(ci.x + ci.y - cj.x - cj.y) <
						pool->fishnpc[i].fish.size + pool->fishnpc[j].fish.size) {

						if (CheckCollisionCircles(ci, pool->fishnpc[i].fish.size, cj, pool->fishnpc[j].fish.size)) {
							if (pool->fishnpc[i].fish.threatsize > pool->fishnpc[j].fish.threatsize) {
								release_fish(pool, &pool->fishnpc[j]);
								pool->fishnpc[i].fish.threatsize += sqrt(pool->fishnpc[j].fish.threatsize) * 0.3;
								Vector2 target{ pool->fishnpc[j].fish.xy.x - pool->fishnpc[i].fish.xy.x,pool->fishnpc[j].fish.xy.y - pool->fishnpc[i].fish.xy.y };
								pool->fishnpc[i].fish.v_xy.x += 5 * pool->fishnpc[i].fish.a * target.x / sqrt((target.x) * (target.x) + (target.y) * (target.y));
								pool->fishnpc[i].fish.v_xy.y += 5 * pool->fishnpc[i].fish.a * target.y / sqrt((target.x) * (target.x) + (target.y) * (target.y));
							}
							else {
								release_fish(pool, &pool->fishnpc[i]);
								pool->fishnpc[j].fish.threatsize += sqrt(pool->fishnpc[i].fish.threatsize) * 0.3;
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

					if (pool->fishnpc[i].fish.threatsize > player.threatsize) {
						running = 3;
					}
					else {//玩家吃掉NPC鱼
						release_fish(pool, &pool->fishnpc[i]);
						player.threatsize += sqrt(pool->fishnpc[i].fish.threatsize) * 0.2 - 0.5;
						san -= 0.3;
						Vector2 target{ pool->fishnpc[i].fish.xy.x - player.xy.x,pool->fishnpc[i].fish.xy.y - player.xy.y };
						player.v_xy.x += 3 * player.a * target.x / sqrt((target.x) * (target.x) + (target.y) * (target.y));
						player.v_xy.y += 3 * player.a * target.y / sqrt((target.x) * (target.x) + (target.y) * (target.y));
						if (pool->fishnpc[i].fish.kinds == 2) {
							if (pool->fishnpc[i].fish.image_status == 2) {
								player.threatsize -= 1.0 * difficult;
								// 刷新减速
								san -= 0.5;
								player.a *= 0.6f;
								slow_timer = 100 * difficult;
							}
							else { san+=0.5*jieduan+0.5; }
						}
						if (pool->fishnpc[i].fish.kinds == 9)
						{
							static int eat_number = 0;
							if (pool->fishnpc[i].fish.image_status == 2) {
								player.threatsize -= 3.5 * difficult;
								// 刷新减速
								san -= 2.5;
								player.a *= 0.3f;
								super_slow_timer = 120 * difficult;
								fade -= 0.15;
								eat_number++;
							}
							else { san += 3; }
							if (eat_number >= 3) {
								fade = 0.6;//续命
							}
						}
						if (pool->fishnpc[i].fish.kinds >= 5)ate_mutant_fish++;
					}
				}
			}
		}
	}
}
void draw_background1(void)
{
	static int start = 0;
	static Texture bg1, bg2, bg3, bg4, bg5;//背景贴图
	if (!start) {
		bg1 = LoadTexture("../img/bg/Background1.png"); bg2 = LoadTexture("../img/bg/Background2.png"); bg3 = LoadTexture("../img/bg/Background3.png"); bg4 = LoadTexture("../img/bg/Background4.png"); bg5 = LoadTexture("../img/bg/Background6.png");
		start = 1;
	}

	if (depthTrans.active) {
		if (player.v_xy.y >= -0.01) {
			float delta = player.v_xy.y * 0.35f;
			if (player.xy.y - screen_length_y + player.size < 0.01 && (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))) {
				delta = 10.0f;
			}
			depthTrans.shift += delta;

			if (depthTrans.shift >= depthTrans.target) {
				depthTrans.shift = depthTrans.target;
				depthTrans.active = 0; // 切换完成
			}

			if (depthTrans.target > 0)
				depthTrans.tone = depthTrans.shift / depthTrans.target;
		}
	}

	float yOffset = -depthTrans.shift;


	float scale1 = (float)screen_length_y / bg1.height;
	float scale2 = (float)screen_length_y / bg2.height;
	float scale3 = (float)screen_length_y / bg3.height;
	float scale4 = (float)screen_length_y / bg4.height;

	float w1 = bg1.width * scale1;
	float w2 = bg2.width * scale2;
	float w3 = bg3.width * scale3;
	float w4 = bg4.width * scale4;

	if (!depthTrans.active) {
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

	Rectangle src1, src2, src3, src4;
	src1.x = 0; src1.y = 0; src1.width = bg1.width; src1.height = bg1.height;
	src2.x = 0; src2.y = 0; src2.width = bg2.width; src2.height = bg2.height;
	src3.x = 0; src3.y = 0; src3.width = bg3.width; src3.height = bg3.height;
	src4.x = 0; src4.y = 0; src4.width = bg4.width; src4.height = bg4.height;

	Vector2 origin;
	origin.x = 0;
	origin.y = 0;

	Rectangle bg1x, bg1y, bg2x, bg2y, bg3x, bg3y, bg4x, bg4y;

	bg1x.x = bg1_x + w1 / 2; bg1x.y = yOffset; bg1x.width = w1; bg1x.height = screen_length_y;
	bg1y.x = bg1_x - w1 / 2; bg1y.y = yOffset; bg1y.width = w1; bg1y.height = screen_length_y;

	bg2x.x = bg2_x + w2 / 2; bg2x.y = yOffset; bg2x.width = w2; bg2x.height = screen_length_y;
	bg2y.x = bg2_x - w2 / 2; bg2y.y = yOffset; bg2y.width = w2; bg2y.height = screen_length_y;

	bg3x.x = bg3_x + w3 / 2; bg3x.y = yOffset; bg3x.width = w3; bg3x.height = screen_length_y;
	bg3y.x = bg3_x - w3 / 2; bg3y.y = yOffset; bg3y.width = w3; bg3y.height = screen_length_y;

	bg4x.x = bg4_x + w4 / 2; bg4x.y = yOffset; bg4x.width = w4; bg4x.height = screen_length_y;
	bg4y.x = bg4_x - w4 / 2; bg4y.y = yOffset; bg4y.width = w4; bg4y.height = screen_length_y;

	DrawTexturePro(bg1, src1, bg1x, origin, 0, WHITE);
	DrawTexturePro(bg1, src1, bg1y, origin, 0, WHITE);

	DrawTexturePro(bg2, src2, bg2x, origin, 0, WHITE);
	DrawTexturePro(bg2, src2, bg2y, origin, 0, WHITE);

	DrawTexturePro(bg3, src3, bg3x, origin, 0, WHITE);
	DrawTexturePro(bg3, src3, bg3y, origin, 0, WHITE);

	DrawTexturePro(bg4, src4, bg4x, origin, 0, WHITE);
	DrawTexturePro(bg4, src4, bg4y, origin, 0, WHITE);

	{
		float scale5 = (float)screen_length_y / bg5.height;
		float w5 = bg5.width * scale5;

		Rectangle src5;
		src5.x = 0;
		src5.y = 0;
		src5.width = bg5.width;
		src5.height = bg5.height;

		Rectangle dst5;
		dst5.x = (screen_length_x - w5) / 2.0f;

		dst5.y = yOffset + screen_length_y;
		dst5.width = w5;
		dst5.height = screen_length_y;

		Vector2 origin5;
		origin5.x = 0;
		origin5.y = 0;
		DrawTexturePro(bg5, src5, dst5, origin5, 0, WHITE);
	}
}



void change_difficult(fishPool* pool) {
	int possibility[5] = { 23,22,25,28,30 };
	static int size_threshold[] = {
		55,  // jieduan 0 -> 1
		95,  // 1 -> 2
		140,  // 2 -> 3
		230,  // 3 -> 4
		320   // 4 -> 5
	};
	if (jieduan < 5 && player.threatsize >((size_threshold[jieduan] * sizetimes) * san / 100))

	{
		jieduan++;
	}
	static int list[5][10] = {
		{1,1,1,1,1,1,2,2,2,3},
		{1,1,1,2,2,2,3,3,3,4},
		{1,1,2,2,3,3,3,5,4,4},
		{2,3,3,5,5,6,4,4,7,8},
		{9,9,5,5,6,4,7,7,8,8}
	};
	Vector2 pos = get_legal_point(); int kinds = list[jieduan][rand() % 10];
	if (runingtime <= 5) {
		Vector2 pos = get_legal_point();
		create_npcfish(pool, pos.x, pos.y, list[0][rand() % 10]);
	}//开局刷新补偿
	if (pool->active_count <3)create_npcfish(pool, pos.x, pos.y, kinds);//最低刷新补偿
	if (jieduan >= 2 && kinds == 2&&rand()%5<=jieduan)kinds = 9;//变异河豚概率刷新
	if (runingtime % possibility[jieduan] == 0)create_npcfish(pool, pos.x, pos.y, kinds);//动态难度刷新
	if (jieduan == 1 && depthTrans.active == 0 && depthTrans.target == 0) {
		depthTrans.active = 1;
		depthTrans.shift = 0;
		depthTrans.target = screen_length_y * 1.0f;
		depthTrans.tone = 0.0f;
	}
}
void draw_depth_filter(void)
{
	if (jieduan <= 0 && depthTrans.tone <= 0.001f)
		return;

	const int MAX_STAGE = 5;
	float stageFactor = (float)jieduan / MAX_STAGE;
	if (stageFactor > 1.0f) stageFactor = 1.0f;
	stageFactor = stageFactor * stageFactor;
	float baseAlpha = depthTrans.tone * 160.0f;
	float stageAlpha = stageFactor * 140.0f;
	float finalAlpha = baseAlpha + stageAlpha;
	if (finalAlpha > 255.0f) finalAlpha = 255.0f;
	unsigned char r = (unsigned char)(10 * (1.0f - stageFactor));
	unsigned char g = (unsigned char)(30 * (1.0f - stageFactor));
	unsigned char b = (unsigned char)(60 * (1.0f - stageFactor));

	Color filter = {
		r,
		g,
		b,
		(unsigned char)finalAlpha
	};

	DrawRectangle(0, 0, screen_length_x, screen_length_y, filter);
	if (jieduan >= 4) {
		unsigned char extra = (unsigned char)((jieduan - 3) * 80);
		if (extra > 160) extra = 160;
		Color black = { 0,0,0,extra };
		DrawRectangle(0, 0, screen_length_x, screen_length_y,black);
	}

}
void draw_player_glow()
{
	Vector2 ci;
	ci.x = player.xy.x + player.size * 1.4f;
	ci.y = player.xy.y + player.size * 1.3f;

	float size = player.size;

	float t = depthTrans.tone;
	t = t * t;

	float baseAlpha = t * 60.0f;

	/* ===== 呼吸控制：默认（mutation < 3） ===== */
	float pulseFreq = 3.0f;
	float pulseAmp = 0.3f;
	float pulseBase = 0.8f;

	/* ===== 阶段 3：异常开始，呼吸变弱 ===== */
	if (mutation >= 3 && mutation < 5) {
		pulseFreq = 3.5f;
		pulseAmp = 0.4f;
		pulseBase = 0.87f;
	}

	/* ===== 阶段 5：非生物感，几乎无呼吸 ===== */
	if (mutation >= 5) {
		pulseFreq =4.0f;
		pulseAmp = 0.5f;
		pulseBase = 0.95f;
	}

	float pulse = sinf(GetTime() * pulseFreq) * pulseAmp + pulseBase;
	baseAlpha *= pulse;

	Color c1, c2, c3;

	/* ===== 颜色方案 ===== */
	if (mutation < 3) {
		/* 原始冷蓝 */
		c1.r = 180; c1.g = 220; c1.b = 255;
		c1.a = (unsigned char)(baseAlpha * 0.6f);

		c2.r = 120; c2.g = 170; c2.b = 255;
		c2.a = (unsigned char)(baseAlpha * 0.35f);

		c3.r = 80;  c3.g = 130; c3.b = 220;
		c3.a = (unsigned char)(baseAlpha * 0.2f);
	}
	else if (mutation < 5) {
		/* 阶段 3：冷白偏青 */
		c1.r = 200; c1.g = 230; c1.b = 255;
		c1.a = (unsigned char)(baseAlpha * 0.65f);

		c2.r = 150; c2.g = 200; c2.b = 255;
		c2.a = (unsigned char)(baseAlpha * 0.4f);

		c3.r = 110; c3.g = 160; c3.b = 230;
		c3.a = (unsigned char)(baseAlpha * 0.25f);
	}
	else {
		/* 阶段 5：偏紫 */
		c1.r = 210; c1.g = 170; c1.b = 255;
		c1.a = (unsigned char)(baseAlpha * 0.7f);

		c2.r = 170; c2.g = 130; c2.b = 230;
		c2.a = (unsigned char)(baseAlpha * 0.45f);

		c3.r = 120; c3.g = 90;  c3.b = 180;
		c3.a = (unsigned char)(baseAlpha * 0.3f);
	}

	/* ===== 半径：非常克制的变化 ===== */
	float r1 = size * 1.0f;
	float r2 = size * 1.6f;
	float r3 = size * 2.4f;

	if (mutation >= 3 && mutation < 5) {
		r2 *= 1.05f;
		r3 *= 1.1f;
	}

	if (mutation >= 5) {
		r2 *= 1.1f;
		r3 *= 1.15f;
	}

	DrawCircle(ci.x, ci.y, r1, c1);
	DrawCircle(ci.x, ci.y, r2, c2);
	DrawCircle(ci.x, ci.y, r3, c3);
}


void draw_fish_glow(fishPool* pool)
{
	int i;
	for (i = 0; i < MAX_fish; i++) {

		if (pool->used[i] == 0)
			continue;

		Vector2 ci;
		int kind = pool->fishnpc[i].fish.kinds;
		float size = pool->fishnpc[i].fish.size;

		switch (kind)
		{
		case 2:
		case 9:
			ci.x = pool->fishnpc[i].fish.xy.x + size * 1.4f;
			ci.y = pool->fishnpc[i].fish.xy.y + size * 1.3f;
			break;
		case 1:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			ci.x = pool->fishnpc[i].fish.xy.x + size * 2.0f;
			ci.y = pool->fishnpc[i].fish.xy.y + size * 2.0f;
			break;
		default:
			ci.x = pool->fishnpc[i].fish.xy.x + size;
			ci.y = pool->fishnpc[i].fish.xy.y + size;
			break;
		}

		/* 深度影响 */
		float t = depthTrans.tone;
		t = t * t;

		/* 默认蓝光参数 */
		float baseAlpha = t * 40.0f;
		float pulse = sinf(GetTime() * 2.0f + i) * 0.2f + 0.8f;

		float r1 = 0.9f, r2 = 1.4f, r3 = 2.0f;

		Color c1, c2, c3;

		/* 默认蓝色 */
		c1.r = 120; c1.g = 170; c1.b = 255; c1.a = (unsigned char)(baseAlpha * 0.55f);
		c2.r = 80;  c2.g = 130; c2.b = 220; c2.a = (unsigned char)(baseAlpha * 0.35f);
		c3.r = 60;  c3.g = 100; c3.b = 200; c3.a = (unsigned char)(baseAlpha * 0.20f);
		if (kind == 5 || kind == 7) {

			baseAlpha = t * 70.0f;
			pulse = sinf(GetTime() * 3.0f + i) * 0.3f + 0.85f;

			c1.r = 120; c1.g = 255; c1.b = 160; c1.a = (unsigned char)(baseAlpha * 0.7f);
			c2.r = 80;  c2.g = 220; c2.b = 130; c2.a = (unsigned char)(baseAlpha * 0.45f);
			c3.r = 60;  c3.g = 180; c3.b = 110; c3.a = (unsigned char)(baseAlpha * 0.25f);

			r1 = 1.0f;
			r2 = 1.6f;
			r3 = 2.3f;
		}
		if (kind == 6 || kind == 8 || kind == 9) {

			baseAlpha = t * 95.0f;
			pulse = sinf(GetTime() * 4.0f + i * 1.3f) * 0.4f + 0.9f;
			c1.r = 180; c1.g = 255; c1.b = 110; c1.a = (unsigned char)(baseAlpha * 0.85f);
			c2.r = 130; c2.g = 220; c2.b = 70;  c2.a = (unsigned char)(baseAlpha * 0.55f);
			c3.r = 90;  c3.g = 180; c3.b = 40;  c3.a = (unsigned char)(baseAlpha * 0.35f);

			r1 = 1.25f;
			r2 = 2.0f;
			r3 = 3.0f;
			Color radiation;
			radiation.r = 160;
			radiation.g = 220;
			radiation.b = 60;
			radiation.a = (unsigned char)(baseAlpha * 0.22f);

			DrawCircle(ci.x, ci.y, size * 3.8f * pulse, radiation);
		}

		DrawCircle(ci.x, ci.y, size * r1 * pulse, c1);
		DrawCircle(ci.x, ci.y, size * r2 * pulse, c2);
		DrawCircle(ci.x, ci.y, size * r3 * pulse, c3);
	}
}
void playermutation(void) {
	if (mutation ==0) {
		if (jieduan >= 1 && san < 90)
			mutation = 1;
	}

	if (mutation ==1) {
		if ((jieduan >= 2 && san < 80)||san<70)
			mutation = 2;
	}

	if (mutation ==2) {
		if ((jieduan >= 3 || san < 60)&& ate_mutant_fish)
			mutation = 3;
	}

	if (mutation ==3) {
		if (jieduan >= 3 && san < 50)
			mutation = 4;
	}

	if (mutation ==4) {
		if (jieduan >= 4 && san < 35)
			mutation = 5;
	}

	if (mutation ==5) {
		if (jieduan >= 5 && san < 20)
			mutation = 6;
	}
}
static float sign_point(Vector2 p1, Vector2 p2, Vector2 p3)
{
	return (p1.x - p3.x) * (p2.y - p3.y) -
		(p2.x - p3.x) * (p1.y - p3.y);
}

static int point_in_triangle(Vector2 pt, Vector2 v1, Vector2 v2, Vector2 v3)
{
	float d1 = sign_point(pt, v1, v2);
	float d2 = sign_point(pt, v2, v3);
	float d3 = sign_point(pt, v3, v1);

	int has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
	int has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

	return !(has_neg && has_pos);
}
int check_jitan_guidance_and_sacrifice(Texture jitan)
{
	if (jieduan != 5) return 0;
	/* ===== 透明度（同步视觉） ===== */
	static float toumingdu = 0.0f;
	if (toumingdu < 1.0f)
		toumingdu = fminf(toumingdu + 0.001f, 1.0f);

	float jitan_alpha = toumingdu;

	/* ===== 祭坛矩形 ===== */
	float jitanW = jitan.width * 3.0f;
	float jitanH = jitan.height * 3.0f;
	float jitanX = (screen_length_x - jitanW) * 0.5f;
	float jitanY = screen_length_y - jitanH;

	/* ===== 玩家中心 ===== */
	Vector2 p;
	p.x = player.xy.x + 1.3f * player.size;
	p.y = player.xy.y + 1.4f * player.size;

	/* ===== 本帧位移向量 ===== */
	static Vector2 last_p = { 0, 0 };
	Vector2 delta;
	delta.x = p.x - last_p.x;
	delta.y = p.y - last_p.y;
	last_p = p;

	/* ===== 最大合法位移（防止高速横穿） ===== */
	float maxStep = 2.5f + 2.0f * (1.0f - jitan_alpha);
	float len2 = delta.x * delta.x + delta.y * delta.y;

	if (len2 > maxStep * maxStep) {
		float len = sqrtf(len2);
		delta.x = delta.x / len * maxStep;
		delta.y = delta.y / len * maxStep;

		/* 回写裁剪后的位移 */
		player.xy.x = last_p.x - 1.3f * player.size + delta.x;
		player.xy.y = last_p.y - 1.4f * player.size + delta.y;
		p.x = last_p.x + delta.x;
		p.y = last_p.y + delta.y;
	}

	/* ===== 引导区域 ===== */
	float guideTop = jitanY;
	float guideBottom = jitanY + jitanH * 0.45f;
	float guideLeft = jitanX + jitanW * 0.25f;
	float guideRight = jitanX + jitanW * 0.75f;

	if (p.x > guideLeft && p.x < guideRight &&
		p.y > guideTop && p.y < guideBottom) {

		/* 非法方向判定：
		   - 向上
		   - 横向分量明显大于纵向
		*/
		int illegal =
			(delta.y < -0.2f) ||
			(fabsf(delta.x) > fabsf(delta.y));

		if (illegal) {
			/* 沿非法方向反推 */
			float kick = 2.0f + 4.0f * jitan_alpha;

			player.xy.x -= delta.x * kick;
			player.xy.y -= delta.y * kick;

			return 0;
		}
	}

	/* ===== 献祭判定（中心） ===== */
	float cx = jitanX + jitanW * 0.5f;
	float cy = jitanY + jitanH * 0.5f;

	float dx = p.x - cx;
	float dy = p.y - cy;

	if (dx * dx + dy * dy < player.size * player.size * 0.8f)
		return 1;

	return 0;
}


int check_jitan_sacrifice(Texture jitan)
{
	/* ===== 祭坛绘制参数 ===== */
	float jitanW = jitan.width * 3.0f;
	float jitanH = jitan.height * 3.0f;
	float jitanX = (screen_length_x - jitanW) * 0.5f;
	float jitanY = screen_length_y - jitanH;

	/* ===== 定义三角形有效区域 ===== */
	Vector2 tri_top;
	tri_top.x = jitanX + jitanW * 0.5f;
	tri_top.y = jitanY + jitanH * 0.15f;

	Vector2 tri_left;
	tri_left.x = jitanX + jitanW * 0.2f;
	tri_left.y = jitanY + jitanH * 0.9f;

	Vector2 tri_right;
	tri_right.x = jitanX + jitanW * 0.8f;
	tri_right.y = jitanY + jitanH * 0.9f;

	/* ===== 玩家中心 ===== */
	Vector2 p;
	p.x = player.xy.x + 1.3*player.size;
	p.y = player.xy.y + 1.4*player.size;
	//DrawTriangleLines(tri_top, tri_left, tri_right, RED);

	/* ===== 第一层：必须在三角形内部 ===== */
	if (!point_in_triangle(p, tri_top, tri_left, tri_right))
		return 0;

	float dx = p.x - tri_top.x;
	float dy = p.y - tri_top.y;
	float dist = sqrtf(dx * dx + dy * dy);

	/* 阈值 */
	//if (dist > player.size * 0.2f)
	//	return 0;

	DrawText(
		"Press the 'K' key to complete the sacrifice",
		screen_length_x / 2 - 90,
		screen_length_y - 40,
		30,
		RED
	);

	/* ===== 连续按 K ===== */
	if(check_jitan_guidance_and_sacrifice(jitan))
	{
		static int k_count = 0;
		static float last_k_time = 0.0f;

		float now = GetTime();

		if (IsKeyPressed(KEY_K)) {
			if (now - last_k_time > 0.6f)
				k_count = 0;

			k_count++;
			last_k_time = now;
		}

		if (k_count >= 6) {
			k_count = 0;
			printf("success\n");
			return 1;
		}
	}

	return 0;
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
	/*while (settings_end && !WindowShouldClose())*/ {
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
					uiState = UI_TITLE;
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
				uiState = UI_TITLE;

			}
		}

		// 绘制
		BeginDrawing();
		draw_settings_menu();  
		EndDrawing();
	}
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
	buttons[SETTINGS] = { startX, startY + buttonHeight + buttonSpacing, buttonWidth, buttonHeight };
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


