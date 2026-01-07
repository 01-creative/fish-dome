#define _CRT_SECURE_NO_WARNINGS
/*
========================================
 主游戏循环（核心状态机）

 本游戏采用单 while + uiState 状态机结构：
 - 所有逻辑都在一个循环中完成
 - uiState 决定当前“世界规则”

 UI_TITLE   : 标题与菜单
 UI_PLAYING: 主游戏循环（生存 / 成长 / 崩坏）
 UI_STOP   : 暂停态（冻结世界）
 UI_BOSS   : 终局挑战（规则被打破）
 UI_END    : 结局展示与存档

 注意：
 - 逻辑更新 与 渲染 强耦合
 - 多数变量同时承担“数值”与“叙事含义”
========================================
*/

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
	SETTINGS_COLLISION_REDUNDANCY, // 碰撞冗余
	SETTINGS_SHOW_HITBOX,//碰撞箱
	SETTINGS_Developer_Mode,
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
	Vector2 xy;//位置
	Vector2 v_xy;//速度
	double a;//加速度
	double size;//渲染大小
	double threatsize;//威胁大小
	int image_status;//图像状态
	int lizixiaoguo;//粒子效果（未完成）
	int kinds;//0是玩家，1小丑鱼，2是河豚,3剑鱼，4鲨鱼,5变异剑鱼,6变异剑鱼2，7变异鲨鱼，8变异鲨鱼2，9变异河豚

	//仅在BOSS阶段调用
	float   timer;   // 行为内部计时
	int     phase;   // 子阶段（仅 kind=102 使用）
	float   angle;   // 当前朝向角度（用于渲染）
	Vector2 last_xy;

}fish;//鱼的基本结构体

typedef struct fish_NPC {
	int active;
	Vector2 aim[7];//鱼的移动目标点
	fish fish;
}fish_NPC;//NPC鱼结构体

typedef struct {
	fish_NPC fishnpc[MAX_fish];
	int used[MAX_fish];
	int active_count;
} fishPool;//NPC鱼对象池结构体

typedef struct {
	int active;
	float shift;
	float target;
	float tone;
} DepthTransition;
static DepthTransition depthTrans = { 0, 0, 0, 0 };//深度过渡效果结构体

// ===== UI STATE =====
/*
UI_TITLE
│
├─ START ─────────► UI_PLAYING
│                     │
│                     ├─ SPACE ─► UI_STOP
│                     │
│                     ├─ 死亡 / 献祭 / 崩溃 ─► UI_END
│                     │
│                     └─ 献祭成功（分支） ─► UI_BOSS
│
├─ SETTINGS ──────► UI_SETTING
│
└─ EXIT ──────────► CloseWindow
*/
typedef enum {
	UI_TITLE,
	UI_PLAYING,
	UI_END,
	UI_STOP,
	UI_BOSS,
	UI_SETTING
} UIState;//UI状态枚举
UIState uiState = UI_TITLE;//当前UI状态-默认标题界面
int ui_menu_index = 0;//标题菜单选项
MenuItem selected = START;//UI数据
Music bgMusic;//背景音乐
int running = 1;//0是退出，1是正常，2是暂停，3是死亡
int  screen_length_x = 1920, screen_length_y = 1080, runingtime = 0;//屏幕分辨率与运行时间
fish player = { {screen_length_x / 2,screen_length_y / 2},{0,0},5,30.0 * sizetimes,30.0 * sizetimes,0,0,0 };//玩家初始化
int difficult = 1, jieduan = 0;  double san = 100;//游戏难度与阶段相关变量
int slow_timer = 0, super_slow_timer = 0,mutation=0, ate_mutant_fish=0,xianji=0,deadtimes=0;float fade = 1.0f;//变异、状态、游戏记录相关变量
bool resetting = 0; // 是否正在重置游戏
int ending_id = 0, compeletedending = 0, deadbymutation = 0, open_I = 0, finish_end0;//结局检查变量
int collision_redundancy = 0,show_hitbox = 0,developer_mode=0;            //碰撞冗余， 显示碰撞箱,开发者模式

void playermove(fish* player);//玩家移动
void init_fish_pool(fishPool* pool);//初始化对象池
fish_NPC* get_fish(fishPool* pool);//从对象池获取鱼，并返回指针
void release_fish(fishPool* pool, fish_NPC* fishPtr);//释放鱼，归还对象池
void update_all_fish(fishPool* pool);//更新所有NPC鱼，包括移动和边界检测
fish_NPC* create_npcfish(fishPool* pool, float x, float y, int kind);//根据指定坐标与种类基于游戏阶段对应查表创建NPC鱼，分为威胁鱼和食物鱼
Vector2 get_legal_point(void);//获取合法的随机坐标点
void collision_npc(fishPool* pool);//玩家与NPC鱼，NPC与NPC的碰撞检测
void draw_background1(void);//绘制背景与背景下沉效果

void UI_Update(fishPool* pool);//UI更新
void UI_Draw(void);//UI绘制
void UI_DrawTitle(void);//标题界面绘制
void UI_DrawEnding(void);//结束界面绘制
void draw_menu(void);//主菜单绘制
void draw_settings_menu(void);//设置菜单绘制
void settings_menu_logic(void);//设置菜单逻辑

void change_difficult(fishPool* pool);//动态难度与鱼的创建调用
void draw_depth_filter(void);//绘制深度滤镜效果
void draw_player_glow(void);//绘制玩家发光效果
void draw_fish_glow(fishPool* pool);//绘制NPC鱼发光效果
void DrawFishAutoFlip(
	Texture2D tex_left,
	Vector2 pos,
	float size,
	float baseSize,
	Color tint,
	float vx
);//自动翻转绘制鱼
void playermutation(void);//玩家变异处理
/*玩家的6段变异：
by1:阶段1进入 + san < 90
	by2 : 阶段2 + san < 80orsan < 70
	by3 : 吃至少一条变异鱼 + （san < 60or阶段2）
	by4 : san < 60 + 阶段3
	by5 : san < 35 + 阶段4
	by6(终极） : （san < 20 + 阶段5）or在“我”阶段死亡*/
int check_jitan_sacrifice(Texture jitan);//祭坛献祭检测
void update_all_bossfish(fishPool* pool, int);//更新所有BOSS鱼，包括移动和边界检测
void create_bossfish(fishPool* pool, int kind, int count, int);//创建BOSS鱼
void DrawFishRotated(Texture2D tex, Vector2 pos, float size, float baseSize, Color tint, float rotation);//旋转绘制鱼
void DrawFishAutoFlipEx(Texture2D tex_left, Vector2 pos, float size, float baseSize, Color tint, float vx, float rotation);//自动翻转并旋转绘制鱼
void collision_player_bossfish(fishPool* pool, int* fishnumber);//玩家与BOSS鱼碰撞检测
void LoadSave(void)
{FILE* f = fopen("save.dat", "r");
	if (!f) return; 
	int sx, sf, sc,sd;
	sx = sf = sc = sd=0;
	if (fscanf(f, "xianji=%d\n", &sx) == 1)
		xianji = sx;
	else
		xianji = 0;  // 读取失败时设置默认值

	if (fscanf(f, "finish_end0=%d\n", &sf) == 1)
		finish_end0 = sf;
	else
		finish_end0 = 0;

	if (fscanf(f, "compeletedending=%d\n", &sc) == 1)
		compeletedending = sc;
	else
		compeletedending = 0;

	if (fscanf(f, "deadtimes=%d\n", &sd) == 1)
		deadtimes = sd;
	else
		deadtimes = 0;

	fclose(f);
}
//游戏存档加载
void SaveGame(void)
{
	FILE* f = fopen("save.dat", "w");
	if (!f) return;
	fprintf(f, "xianji=%d\n", xianji);
	fprintf(f, "finish_end0=%d\n", finish_end0);
	fprintf(f, "compeletedending=%d\n", compeletedending);
	fprintf(f, "deadtimes=%d\n", deadtimes);
	fclose(f);
}
//游戏存档保存

int main() {
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
	Texture player_gray_left =LoadTexture("../img/fish/玩家 灰度 左向.png");
	Texture player_tex_by[6] = {
		LoadTexture("../img/fish/玩家 变异1 左向.png"),
		LoadTexture("../img/fish/玩家 变异2 左向.png"),
		LoadTexture("../img/fish/玩家 变异3 左向.png"),
		LoadTexture("../img/fish/玩家 变异4 左向.png"),
		LoadTexture("../img/fish/玩家 变异5 左向.png"),
		LoadTexture("../img/fish/玩家 变异6 左向.png")
	};


	// ================= NPC 鱼贴图 =================

	Texture xiaochouyu_left =LoadTexture("../img/fish/小丑鱼 左向.png");

	Texture hetun_left =LoadTexture("../img/fish/河豚 左向.png");
	Texture hetun_puffed_left =LoadTexture("../img/fish/河豚 鼓起 左向.png");
	Texture hetun_puffed_b1_left = LoadTexture("../img/fish/河豚 鼓起 变异1 左向.png");
	Texture hetun_b1_left = LoadTexture("../img/fish/河豚 变异1 左向.png");

	Texture jianyu_left =LoadTexture("../img/fish/剑鱼 左向.png");
	Texture jianyu_b1_left =LoadTexture("../img/fish/剑鱼 变异1 左向.png");
	Texture jianyu_b2_left =LoadTexture("../img/fish/剑鱼 变异2 左向.png");

	Texture shayu_left =LoadTexture("../img/fish/鲨鱼 左向.png");
	Texture shayu_b1_left =LoadTexture("../img/fish/鲨鱼 变异1 左向.png");
	Texture shayu_b2_left =LoadTexture("../img/fish/鲨鱼 变异2 左向.png");

	Texture jitan =LoadTexture("../img/other/祭坛.png");
	InitAudioDevice();  // 初始化音频设备
	bgMusic = LoadMusicStream("../music/background.mp3");
	SetMusicVolume(bgMusic, 0.3f);  
	PlayMusicStream(bgMusic);
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
	LoadSave();
	//**************************************************************主循环********************************************************************//
	while (!WindowShouldClose()) {
		UpdateMusicStream(bgMusic);
		static Texture stop_sight;
		switch (uiState) {
		case UI_TITLE:
		{
			if (compeletedending && finish_end0) {
				BeginDrawing();
				DrawText("The story of the sea had overed.", screen_length_x / 2 - 200, screen_length_y / 2, 50, RED);
				EndDrawing();
				WaitTime(5);
				CloseWindow();
				return 0;
			}
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
// ================= 世界状态更新顺序 =================
// 1. 动态难度与生成节奏调整
// 2. 玩家输入与物理更新
// 3. 变异判定（可能改变能力或结局路径）
// 4. 所有NPC更新（移动 / 边界）
// 5. 碰撞检测（吃 / 被吃 / 状态改变）

			if (IsKeyPressed(KEY_SPACE)) {
				Image stop_img = LoadImageFromScreen();
				static bool IF_img_used = 0;
				if (IF_img_used) UnloadTexture(stop_sight);
				stop_sight = LoadTextureFromImage(stop_img);
				UnloadImage(stop_img);
				IF_img_used = true;
				uiState = UI_STOP;
				PollInputEvents();
				continue;
			}
			change_difficult(&pool);   // 难度与生成节奏
			playermove(&player);       // 玩家输入 → 物理
			playermutation();          // 变异判定
			update_all_fish(&pool);    // NPC 逻辑更新
			collision_npc(&pool);      // 碰撞与吃/被吃
			BeginDrawing();
			ClearBackground(BLACK);

			draw_background1();
			draw_depth_filter();
			draw_player_glow();
			draw_fish_glow(&pool);

			

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
			// 遍历鱼对象池（对象池模式，避免频繁分配内存）
			// used[i] == false 的槽位被视为“空”

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

				if(show_hitbox){
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
			}
			// NPC鱼渲染结束
			if (show_hitbox) {
				if (IsKeyDown(KEY_Q)) {
					Vector2 cj;
					cj.x = player.xy.x + 1.3 * player.size; cj.y = player.xy.y + 1.4 * player.size;
					DrawCircle(cj.x, cj.y, player.size, RED);
					DrawText(
						TextFormat("Size: %.2f", player.threatsize),
						cj.x,
						cj.y,
						24,
						WHITE
					);
				}
			}
			if (mutation >= 3) {
				static int get_ablility = 0;
				if (get_ablility < 100) {
					get_ablility++;
					DrawText("You feel stronger and better.(Double Tap Ability Unlocked)", screen_length_x / 2 - 300, screen_length_y - 80, 20, YELLOW);
				}
			}
			// 注意：size_threshold数组已经在外部声明
			if(developer_mode){
				if (runingtime % 10 == 0) {
					printf("size:%.2f+threat:%.2f 阶段：%d 下阶段门槛：%lf time:%d san:%.3f fade:%.3f\n",
						player.size, player.threatsize, jieduan,
						((size_threshold[jieduan] * sizetimes) * san / 100),
						runingtime, san, fade);
				}
			}
			if (jieduan == 5) {
				if (toumingdu < 1.0f)
					toumingdu = (toumingdu + 0.005f < 1.0f) ? toumingdu + 0.005f : 1.0f;
				if(xianji){
					float h = screen_length_y;   // 光柱高度
					float base_y = screen_length_y - h;
					float alpha_light = sinf(runingtime) * 0.1 + 1.0;
					Color c1 = { 120, 20, 20, (unsigned char)40 * alpha_light*toumingdu };
					Color c2 = { 160, 30, 30, (unsigned char)70 * alpha_light*toumingdu };
					Color c3 = { 200, 60, 60,(unsigned char)110 * alpha_light*toumingdu };

					// 最外层柔光（很淡）
					DrawRectangle(
						screen_length_x / 2 - 50-10, base_y,
						100, h,
						c1
					);

					// 中层
					DrawRectangle(
						screen_length_x / 2 - 40-10, base_y,
						80, h,
						c2
					);

					// 核心亮部
					DrawRectangle(
						screen_length_x / 2 - 25-10, base_y,
						50, h,
						c3
					);
				}
				else {
					float h = screen_length_y;   // 光柱高度
					float base_y = screen_length_y - h;
					float alpha_light = sinf(runingtime) * 0.1 + 1.0;
					Color c1 = { 120, 20, 20, (unsigned char)40 * alpha_light * toumingdu };
					Color c2 = { 160, 30, 30, (unsigned char)70 * alpha_light * toumingdu };
					Color c3 = { 200, 60, 60,(unsigned char)110 * alpha_light * toumingdu };

					// 最外层柔光（很淡）
					DrawRectangle(
						screen_length_x / 2 - 12 - 10, base_y,
						24, h,
						c1
					);

					// 中层
					DrawRectangle(
						screen_length_x / 2 - 7 - 10, base_y,
						14, h,
						c2
					);

					// 核心亮部
					DrawRectangle(
						screen_length_x / 2 - 3 - 10, base_y,
						6, h,
						c3
					);
				}
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
			san += 0.00005;
			san -= 0.001 * jieduan*jieduan;
			//渲染Tips
			{
				const char* tip = NULL;

				if (jieduan < 2) {
					static const char* tips_stage0_3[] = {
	"Swordfish are more dangerous than they look.",
	"Have you noticed the sharks?",
	"Swordfish are the fastest swimmers in the sea.",
	"They do not favor vertical movement.",
	"Remember: there is only one of you.",
	"Beware of inertia.",
	"Eating grants a slight acceleration.",
	"Have you ever eaten a pufferfish?",
	"Pufferfish are cute when inflated... and toxic.",
	"Uninflated pufferfish may sharpen your mind."
					};


					int count = sizeof(tips_stage0_3) / sizeof(tips_stage0_3[0]);
					tip = tips_stage0_3[(runingtime / 180) % count];
				}
				else {
					static const char* tips_stage2_plus[] = {
	"The contamination is spreading.",
	"Do not underestimate Its limbs.",
	"The toxin of pale pufferfish is almost irreversible.",
	"Fighting poison with poison may grant a moment of clarity.",
	"I have become Death, the destroyer of worlds."
					};


					int count = sizeof(tips_stage2_plus) / sizeof(tips_stage2_plus[0]);
					tip = tips_stage2_plus[(runingtime / 240) % count];
				}

				if (tip) {
					int fontSize = 25;
					int textWidth = MeasureText(tip, fontSize);

					int x = screen_length_x / 2 - textWidth / 2;
					int y = screen_length_y - 40;

					DrawText(tip, x, y, fontSize, Fade(WHITE, 0.75f));
				}
			}
			// 注意：toumingdu已经在外部声明，这里直接使用
			if (jieduan == 5) {
				if (toumingdu >= 0.95) {
					if (check_jitan_sacrifice(jitan)) {
						// 祭坛检查逻辑
						if (xianji == 0) {
							xianji = 1;
							running = 3;
							SaveGame();
						}
						else {
							xianji = 0;
							player.threatsize = 60;
							uiState = UI_BOSS;
							SaveGame();
						}
					}
				}
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
		    if (resetting)resetting = 0;
			if (running == 3) {
				uiState = UI_END;
			}
			break;
		}

		case UI_END:
		{
			SaveGame(); static int endend = 0;
			if (IsKeyPressed(KEY_ENTER)) {
				uiState = UI_TITLE;
				PollInputEvents();
				//初始化
				ui_menu_index = 0;
				endend = 0;
				running = 1;//0是退出，1是正常，2是暂停，3是死亡
				runingtime = 0;
				player.xy.x = screen_length_x / 2;
				player.xy.y = screen_length_y / 2;
				player.v_xy.x = 0;
				player.v_xy.y = 0;
				player.a = 5;
				player.threatsize = 30 * sizetimes;
				player.image_status = 0;
				player.lizixiaoguo = 0;
				player.kinds = 0;
				selected = START;//UI数据
				difficult = 1.5; jieduan = 0; san = 100.0f; deadbymutation = 0; open_I = 0;
				slow_timer = 0; super_slow_timer = 0;if(!xianji) mutation = 0; if(!xianji)ate_mutant_fish = 0; fade = 1.0f; toumingdu = 0;
				depthTrans.active = 0; depthTrans.shift = 0; depthTrans.target = 0; depthTrans.tone = 0;
				//全局变量初始化
				init_fish_pool(&pool);
				srand(time(NULL));//函数初始化
				resetting = 1;//静态本地变量初始化
			}
			if(!endend)
			{/*
================= 结局判定 =================
以下 if-else 为严格优先级结构：
- 同时满足多个条件时，只会触发最靠前的结局
- 顺序本身即为叙事权重
- 修改顺序将直接改变游戏表达
*/

				if (player.size < 35)ending_id = 12;
				else if (compeletedending && finish_end0 && mutation <= 4)ending_id = 11;
				else if (compeletedending && fade <= 0.6f)ending_id = 9;
				else if (compeletedending)ending_id = 10;
				else if (!compeletedending && open_I)ending_id = 8;
				else if (xianji == 1 && jieduan == 5)ending_id = 7;
				else if (fade <= 0.0f)ending_id = 13;
				else if (san <= 0.0f)ending_id = 5;
				else if (ate_mutant_fish && jieduan >= 3)ending_id = 6;
				else if (deadbymutation && jieduan >= 2)ending_id = 4;
				else if (!deadbymutation && jieduan >= 2)ending_id = 3;
				else if (jieduan == 1)ending_id = 2;
				else if (jieduan == 0 && runingtime >= 10000 && san >= 100)ending_id = 0;
				else if (jieduan == 0)ending_id = 1;
				if (ending_id == 0)finish_end0 = 1;
				deadtimes++;
				SaveGame();
				endend = 1;
				//复杂的多结局判断
				/*自然语言表示如下：
存活超过10000帧（5min)，且san>=100,jieduan==0，被大鱼吃掉-->“最初的逝去 “
jieduan==0,死亡"风和日丽“
jieduan==1,死亡”帷幕一角“
jieduan>=2,死于普通鱼，”肉体凡胎”
jieduan>=2,死于变异鱼，“深渊恐惧”
jieduan>=3,死于低san,"世界向我走来“
jieduan>=3,死于fade,"褪色者“
jieduan>=3,吃掉至少一条变异鱼，”祂！“
jieduan==5,为开始”我“阶段死亡，”必要的代价“
jieduan==5,在”我“阶段死亡，”我？“
jieduan==5,完成”我“阶段,死亡"结束了？”
jieduan==5,完成“我”阶段，"祂在我体内苏醒了"
jieduan==5,完成“我”阶段，达成过第一个结局，全程san>55,“我胜利了，再无可吞之物。...这一切值得吗？”
彩蛋结局（被河豚毒死）”河豚味美，不可多食“。size<30
				
				*/
			}
			BeginDrawing();
			UI_DrawEnding();
			EndDrawing();
		}
			break;

		case UI_STOP:
		{
			if (IsKeyPressed(KEY_SPACE)) {
				uiState = UI_PLAYING;
				PollInputEvents();
			}
			if (IsKeyPressed(KEY_E)) {
				uiState = UI_TITLE;
			}if (IsKeyPressed(KEY_R)) {
				uiState = UI_END;
			}
			BeginDrawing();
			DrawTexture(stop_sight, 0, -screen_length_y/2, WHITE);
			DrawRectangle(0, 0,
				screen_length_x,
				screen_length_y,
				Fade(BLACK, 0.4f));

			DrawText("Press SPACE to pause",screen_length_x/2-180, screen_length_y/2, 40, RED);
			DrawText("Press E to TITLE", screen_length_x / 2 - 80, screen_length_y / 2+40, 30,GRAY);
			DrawText("Press R to OVERGAME", screen_length_x / 2 - 100, screen_length_y / 2 + 80, 30, GRAY);
			EndDrawing();
			
		}
			break;

		case UI_SETTING:
		{
			settings_menu_logic();
		}
		break;
		case UI_BOSS://************************************************BOSS**************************************************************//
		{
			open_I = 1;
			static Texture bossfish[6] = {player_tex[1],player_tex_by[2],player_gray_left,player_tex_by[3],player_tex_by[4],player_tex_by[5] };
			if (developer_mode) {
				if (runingtime % 10 == 0) {
					printf("size:%.2f+threat:%.2f 阶段：%d 下阶段门槛：%lf time:%d san:%.3f fade:%.3f\n",
						player.size, player.threatsize, jieduan,
						((size_threshold[jieduan] * sizetimes) * san / 100),
						runingtime, san, fade);
				}
				
			}
			playermove(&player);
			static float boss_wave_timer = 0.0f;
			static int   boss_wave_phase = 0;
			static int   boss_wave_loop = 0;
			// ================= BOSS Wave 调度 =================
			/*
================= BOSS 波次系统 =================
通过 phase + timer 构成的小型状态机：
- 非随机刷怪
- 强调节奏、呼吸感与压迫感
- loop 次数限制最大强度
*/

			boss_wave_timer += 0.02f;
			static int fishnumber = (int)min((deadtimes) * 100.0f / (san * 3) * difficult + 1,25);
			switch (boss_wave_phase)
			{
			case 0:
				// 第一拍
				create_bossfish(&pool, 101, (4 + boss_wave_loop / 2)*difficult, fishnumber);
				boss_wave_phase = 1;
				boss_wave_timer = 0;
				break;

			case 1:
				// 停顿（呼吸）
				if (boss_wave_timer > 0.6f) {
					create_bossfish(&pool, 103, (2 + boss_wave_loop / 2)*difficult, fishnumber);
					boss_wave_phase = 2;
					boss_wave_timer = 0;
				}
				break;

			case 2:
				// 环绕登场
				if (boss_wave_timer > 1.8f) {
					create_bossfish(&pool, 102, min(3 + boss_wave_loop/1.5,6)*difficult, fishnumber);
					boss_wave_phase = 3;
					boss_wave_timer = 0;
				}
				break;

			case 3:
				// 混合压迫
				if (boss_wave_timer > 1.8f) {
					create_bossfish(&pool, 101, 4,fishnumber);
					create_bossfish(&pool, 103, 3,fishnumber);
					boss_wave_phase = 4;
					boss_wave_timer = 0;
				}
				break;

			case 4:
				// 收束 / 空拍
				if (boss_wave_timer > 2.2f) {
					boss_wave_phase = 0;
					boss_wave_loop++;
					boss_wave_timer = 0;
				}
				break;
			}
			if (boss_wave_loop > 5)
				boss_wave_loop = 5;
			update_all_bossfish(&pool,fishnumber);
			collision_player_bossfish(&pool, &fishnumber);

			BeginDrawing();
			ClearBackground(BLACK);
			draw_background1();
			draw_depth_filter();
			draw_player_glow();
			draw_fish_glow(&pool);
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
			DrawFishAutoFlip(
				player_gray_left,
				player.xy,
				player.size,
				32.0f,
				grayTint,
				player.v_xy.x
			);
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
			// ================= BOSS鱼渲染 =================
			for (int i = 0; i <fishnumber; i++) {
				if (!pool.used[i]) continue;

				fish* f = &pool.fishnpc[i].fish;

				// 只渲染BOSS鱼种类（101, 102, 103）
				if (f->kinds < 100) continue;

				// 根据BOSS鱼种类选择不同的贴图和颜色
				Texture2D* tex_to_use = NULL;
				Color bossTint = fishTint;
				if(f->kinds==103){
					tex_to_use = &bossfish[f->image_status];
					Color black = { 0,0,0,140 };
					bossTint = black;
					bossTint.r = (unsigned char)(bossTint.r * brightness);
					bossTint.g = (unsigned char)(bossTint.g * brightness);
					bossTint.b = (unsigned char)(bossTint.b * brightness);
				}
				else if (f->kinds == 101) {
					tex_to_use = &bossfish[f->image_status];
					Color white = { 230,230,230,200 };
					bossTint = white;
					bossTint.r = (unsigned char)(bossTint.r * brightness);
					bossTint.g = (unsigned char)(bossTint.g * brightness);
					bossTint.b = (unsigned char)(bossTint.b * brightness);
				}
				else {
					tex_to_use = &bossfish[f->image_status];
					/*Color red = { 255,255,255,255 };*/
					bossTint = WHITE;
					bossTint.r = (unsigned char)(bossTint.r * brightness);
					bossTint.g = (unsigned char)(bossTint.g * brightness);
					bossTint.b = (unsigned char)(bossTint.b * brightness);
				}
				if (tex_to_use) {
					DrawFishRotated(
						*tex_to_use,
						f->xy,
						f->size,
						32.0f,
						bossTint,
						f->angle
					);
				}
				// 调试：显示碰撞箱
				if (show_hitbox && IsKeyDown(KEY_Q)&&developer_mode) {
					Vector2 center = {
						f->xy.x + f->size * 1.4f,
						f->xy.y + f->size * 1.3f
					};
					DrawCircleLines(center.x, center.y, f->size, RED);
					DrawText(
						TextFormat("Kind:%d Ang:%.1f", f->kinds, f->angle),
						center.x,
						center.y,
						20,
						WHITE
					);
				}
			}
			// BOSS鱼渲染结束

				if (IsKeyDown(KEY_SPACE)) {
					DrawText("The Power Doesn't Delong To You", screen_length_x / 2 - 300, screen_length_y / 2, 30, RED);
					
				}else if (show_hitbox&&!developer_mode) {
					if (IsKeyDown(KEY_Q))DrawText("The Knowledge Doesn't Delong To You", screen_length_x / 2 - 300, screen_length_y / 2, 30, RED);
					
				}
				if (fishnumber == 0) {
					compeletedending = 1; SaveGame();
					static int endtime = 0;
					endtime++;
					DrawRectangle(0, 0, screen_length_x, screen_length_y, Fade(BLACK, 0.01f * endtime));
					if (endtime > 100)uiState = UI_END;
				}
				EndDrawing();
				runingtime++;
				if (running == 3) {
					uiState = UI_END;
				}
				break;
		}
		break;
		}
		
		WaitTime(0.05);
		
	}
	StopMusicStream(bgMusic);
	UnloadMusicStream(bgMusic);
	CloseAudioDevice();
	return 0;
}

void DrawFishAutoFlip(Texture2D tex_left,Vector2 pos,float size,float baseSize,Color tint,float vx)
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
/**
 * 玩家移动控制与物理更新
 * 功能：处理玩家输入、移动物理、边界检测、状态管理和特殊能力
 * 核心机制：
 * 1. 双键位支持（方向键+WASD）
 * 2. 双击冲刺（变异3+解锁）
 * 3. 状态效果（减速、变异加成）
 * 4. 边界反弹/限制
 * 5. 游戏失败条件检查
 */
void playermove(fish* player) {
	// 静态变量：基础加速度，只在重置时恢复
	static float init_a = 5.0f;
	if (resetting) init_a = 5.0f;

	// 静态变量：记录上次变异等级，用于检测变异升级
	static int last_mutation = 0;
	if (resetting) last_mutation = 0;

	// 空格键暂停游戏
	if (IsKeyPressed(KEY_SPACE)) {
		running = 2;
		return;
	}

	// 变异升级时增加基础加速度
	if (mutation != last_mutation) {
		last_mutation = mutation;
		init_a += 0.5;  // 每级变异增加0.5加速度
	}

	// 调试作弊键：Z键增大体型
	//if (IsKeyPressed(KEY_Z)) {
	//	player->threatsize += 20;  // 增加威胁值
	//	san -= 5;  // 消耗理智
	//}

	// ================= 玩家大小计算 =================
	// 使用反正切函数实现非线性成长曲线
	// 当threatsize=42时，size=42（最小值）
	// threatsize越大，size增长越平缓
	player->size = 80 * atan((player->threatsize - 42) / 80) + 42;

	// ================= 屏幕淡出处理 =================
	if (fade <= 0.6f) {
		fade -= 0.0001;  // 缓慢淡出
	}
	fade = max(fade, 0);  // 确保不超界

	// ================= 减速效果处理 =================
	// 超级减速效果（来自变异河豚）
	if (super_slow_timer > 0) {
		super_slow_timer--;
		if (super_slow_timer == 0) {
			init_a *= 0.85;  // 减速结束后永久降低15%加速度
			player->a = init_a;  // 更新玩家加速度
		}
	}

	// 普通减速效果（来自普通河豚）
	if (slow_timer > 0) {
		slow_timer--;
		if (slow_timer == 0) {
			player->a = init_a;  // 恢复基础加速度
		}
	}

	// ================= 速度衰减（模拟水阻） =================
	// 水平方向衰减10%，垂直方向衰减20%（模拟重力效果）
	player->v_xy.x *= 0.9;
	player->v_xy.y *= 0.8;

	// ================= 输入处理 =================
	float derta_vx = 0, derta_vy = 0;  // 速度增量

	// 双键位支持：方向键和WASD
	if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) derta_vx += player->a;
	if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) derta_vx -= player->a;
	if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) derta_vy += player->a;
	if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) derta_vy -= player->a;

	// 对角线移动规范化（防止斜向移动更快）
	if (derta_vx != 0 && derta_vy != 0) {
		derta_vx *= 0.7071f;  // 1/√2 ≈ 0.7071
		derta_vy *= 0.7071f;
	}

	// ================= 双击冲刺系统 =================
	// 变异等级≥3时解锁的特殊能力
	static int cooldown = 0;  // 冷却时间
	static int keyPressCount[4] = { 0 };  // 四个方向的按键计数
	static double lastPressTime[4] = { 0 };  // 上次按键时间
	if (resetting) {  // 重置时清空所有状态
		cooldown = 0;
		for (int i = 0; i < 4; i++) {
			keyPressCount[i] = 0;
			lastPressTime[i] = 0.0;
		}
	}

	static bool getability = 0;  // 是否获得能力标志
	if (resetting) getability = 0;
	if (mutation >= 3) {  // 达到变异3级获得能力
		getability = 1;
	}

	if (getability) {
		double currentTime = GetTime();  // 获取当前时间
		int keyIndex = -1;  // 按下的键索引

		// 检测哪个方向键被按下
		if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) keyIndex = 0;      // 右
		else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) keyIndex = 1;  // 左
		else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) keyIndex = 2;  // 下
		else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) keyIndex = 3;    // 上

		// 检测双击（0.3秒内按两次同一方向键）
		if (keyIndex != -1 && !cooldown) {
			if (currentTime - lastPressTime[keyIndex] < 0.3) {  // 双击成功
				switch (keyIndex) {
				case 0: derta_vx += player->a * 10; break;  // 右冲刺
				case 1: derta_vx -= player->a * 10; break;  // 左冲刺
				case 2: derta_vy += player->a * 12; break;  // 下冲刺（更强）
				case 3: derta_vy -= player->a * 12; break;  // 上冲刺（更强）
				}
				cooldown = 60;  // 设置60帧冷却
				keyPressCount[keyIndex] = 0;  // 重置计数
			}
			else {  // 第一次按下
				keyPressCount[keyIndex] = 1;
			}
			lastPressTime[keyIndex] = currentTime;  // 更新最后一次按键时间
		}

		if (cooldown > 0) cooldown--;  // 冷却计数
	}

	// ================= 应用速度 =================
	player->v_xy.x += derta_vx;
	player->v_xy.y += derta_vy;

	// ================= 更新位置 =================
	player->xy.x += player->v_xy.x;
	player->xy.y += player->v_xy.y;

	// ================= 屏幕边界检测 =================
	// 左边界
	if (player->xy.x < 0) {
		player->xy.x = 0;
		player->v_xy.x = 0;  // 完全停止
	}

	// 右边界
	if (player->xy.x > (screen_length_x - player->size)) {
		player->xy.x = (screen_length_x - player->size);
		player->v_xy.x = 0.1;  // 轻微反弹
	}

	// 上边界
	if (player->xy.y < 0) {
		player->xy.y = 0;
		player->v_xy.y = 0;
	}

	// 下边界
	if (player->xy.y > (screen_length_y - player->size)) {
		player->xy.y = (screen_length_y - player->size);
		player->v_xy.y = 0.1;  // 轻微反弹
	}

	// ================= 失败条件检查 =================
	if (fade <= 0.0001f) running = 3;      // 完全淡出死亡
	if (san <= 0.0001f) running = 3;       // 理智归零死亡
	if (player->size <= 35.0f) running = 3; // 体型过小死亡
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

/**
 * 更新所有NPC鱼的状态
 * 功能：处理NPC鱼的移动、AI行为、特殊状态、边界检查
 * 包含的AI类型：
 * 1. 基础寻路AI（跟随目标点）
 * 2. 河豚特殊行为（检测玩家并鼓起）
 * 3. 变异河豚行为（增强版河豚）
 * 4. 速度衰减和边界回收
 */
void update_all_fish(fishPool* pool) {
	// 河豚鼓起的计时器数组（每个鱼一个计时器）
	static int puffer_timer[MAX_fish] = { 0 };
	if (resetting) {
		for (int i = 0; i < MAX_fish; i++) {
			puffer_timer[i] = 0;  // 重置时清空所有计时器
		}
	}

	// 遍历所有NPC鱼
	for (int i = 0; i < MAX_fish; i++) {
		if (pool->used[i]) {  // 只处理活动的鱼
			fish_NPC* fishPtr = &pool->fishnpc[i];

			// ================= 鱼的大小计算 =================
			// 剑鱼和鲨鱼：大小等于威胁值（线性成长）
			if (fishPtr->fish.kinds == 3 || fishPtr->fish.kinds == 4) {
				fishPtr->fish.size = fishPtr->fish.threatsize;
			}
			// 其他鱼：非线性成长（与玩家相同）
			else {
				fishPtr->fish.size = 80 * atan((fishPtr->fish.threatsize - 42) / 80) + 42;
			}

			// ================= 基本移动 =================
			fishPtr->fish.xy.x += fishPtr->fish.v_xy.x;
			fishPtr->fish.xy.y += fishPtr->fish.v_xy.y;

			// ================= 河豚特殊行为 =================
			if (fishPtr->fish.kinds == 2) {  // 普通河豚
				if (fishPtr->fish.image_status == 2) {  // 鼓起状态
					// 鼓起倒计时
					if (--puffer_timer[i] <= 0) {
						fishPtr->fish.image_status = 1;  // 恢复普通状态
					}
				}
				else {  // 普通状态
					// 检测玩家是否在前方
					Vector2 f = fishPtr->fish.v_xy;  // 河豚前进方向
					float fl = sqrt(f.x * f.x + f.y * f.y);  // 方向长度

					if (fl > 0.01f) {  // 有移动方向
						f.x /= fl;  // 归一化x
						f.y /= fl;  // 归一化y

						// 计算指向玩家的向量
						Vector2 toP = {
							player.xy.x - fishPtr->fish.xy.x,
							player.xy.y - fishPtr->fish.xy.y
						};
						float d = sqrt(toP.x * toP.x + toP.y * toP.y);  // 到玩家的距离

						// 检测条件：距离<220且方向与玩家夹角<45度
						if (d < 220.0f * difficult) {  // 距离检测
							toP.x /= d;  // 归一化
							toP.y /= d;

							// 点积>0.7表示夹角<45度，30%概率触发
							if (f.x * toP.x + f.y * toP.y > 0.7f && rand() % 100 < (30 * difficult)) {
								fishPtr->fish.image_status = 2;  // 切换到鼓起状态
								puffer_timer[i] = 80;  // 设置鼓起时间
							}
						}
					}
				}
			}

			// ================= 变异河豚特殊行为 =================
			if (fishPtr->fish.kinds == 9) {  // 变异河豚
				if (fishPtr->fish.image_status == 2) {  // 鼓起状态
					// 鼓起倒计时
					if (--puffer_timer[i] <= 0) {
						fishPtr->fish.image_status = 1;  // 恢复普通状态
					}
				}
				else {  // 普通状态
					// 检测玩家是否在前方（增强版）
					Vector2 f = fishPtr->fish.v_xy;
					float fl = sqrt(f.x * f.x + f.y * f.y);

					if (fl > 0.01f) {
						f.x /= fl;
						f.y /= fl;

						Vector2 toP = {
							player.xy.x - fishPtr->fish.xy.x,
							player.xy.y - fishPtr->fish.xy.y
						};
						float d = sqrt(toP.x * toP.x + toP.y * toP.y);

						// 更远距离检测（320），更高触发概率（40%）
						if (d < 320.0f * difficult) {
							toP.x /= d;
							toP.y /= d;

							if (f.x * toP.x + f.y * toP.y > 0.7f && rand() % 100 < (40 * difficult)) {
								fishPtr->fish.image_status = 2;  // 切换到鼓起状态
								puffer_timer[i] = 110;  // 更长的鼓起时间
							}
						}
					}
				}
			}

			// ================= 目标点更新 =================
			// 到达当前目标点后，更新到下一个目标点
			if (fishPtr->fish.xy.x - fishPtr->aim[0].x <= 1.0 &&
				fishPtr->fish.xy.y - fishPtr->aim[0].y <= 1.0) {
				// 向前移动目标点队列
				for (int p = 0; p < 7; p++) {
					fishPtr->aim[p] = fishPtr->aim[p + 1];
				}
			}
			fishPtr->aim[6] = { 3000, 2000 };  // 设置最终目标点（屏幕外）

			// ================= AI移动决策 =================
			// 80%的概率向目标点移动
			if (rand() % 10 > 1) {
				// 随机系数0.8-1.2，增加移动的随机性
				float k = rand() % 40 / 100.0 + 0.8;

				// 计算到目标点的方向向量
				Vector2 direction = {
					fishPtr->aim[0].x - fishPtr->fish.xy.x,
					fishPtr->aim[0].y - fishPtr->fish.xy.y
				};

				// 归一化方向并应用加速度
				float distance = sqrt((direction.x) * (direction.x) + (direction.y) * (direction.y));
				if (distance > 0.01f) {  // 防止除零
					fishPtr->fish.v_xy.x += k * direction.x * fishPtr->fish.a / distance;
					fishPtr->fish.v_xy.y += k * direction.y * fishPtr->fish.a / distance;
				}
			}

			// ================= 速度衰减 =================
			fishPtr->fish.v_xy.x *= 0.95;  // 水平衰减5%
			fishPtr->fish.v_xy.y *= 0.9;   // 垂直衰减10%

			// ================= 边界检查与回收 =================
			// 移出屏幕外一定距离后回收鱼对象
			if (fishPtr->fish.xy.x < -300 || fishPtr->fish.xy.x > screen_length_x + 300 ||
				fishPtr->fish.xy.y < -150 || fishPtr->fish.xy.y > screen_length_y + 150) {
				release_fish(pool, fishPtr);
			}
		}
	}
}
fish_NPC* create_npcfish(fishPool* pool, float x, float y, int kind) {
	/**
 * 创建NPC鱼
 * 参数：对象池，坐标x,y，鱼种类
 * 返回：创建的鱼指针，失败返回0
 * 流程：
 * 1. 获取空闲鱼对象
 * 2. 计算威胁值（危险鱼/普通鱼）
 * 3. 调整种类以匹配威胁值
 * 4. 初始化鱼属性
 */
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
		20, 30, 25, 30, 35, 50
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
	/**
 * NPC鱼碰撞检测与处理
 * 处理：1. NPC鱼之间的碰撞 2. 玩家与NPC鱼的碰撞
 * 复杂度：O(n²) - 双重循环检测所有碰撞
 * 碰撞检测流程：
 * 外层循环(i)：遍历所有NPC鱼
 * └→ 内层循环(j)：检测与后续NPC鱼的碰撞
 * └→ 玩家碰撞检测
 */
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
			static int eat_number = 0;
			if (resetting)eat_number = 0;
			if (fabs(ci.x + ci.y - cplayer.x - cplayer.y) <
				pool->fishnpc[i].fish.size + player.size) {

				if (CheckCollisionCircles(
					ci,
					pool->fishnpc[i].fish.size,
					cplayer,
					player.size)) {

					if (pool->fishnpc[i].fish.threatsize > player.threatsize) {
						if (pool->fishnpc[i].fish.kinds >= 5)deadbymutation = 1;
						if(!collision_redundancy)running = 3;
						else if(pool->fishnpc[i].fish.threatsize > player.threatsize*1.05f)running = 3;
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
	static float bg1_x = 0, bg2_x = 0,bg3_x = 0,bg4_x = 0;//背景偏移量
	
	if (resetting) {
		bg1_x = 0; bg2_x = 0; bg3_x = 0; bg4_x = 0; 
	}
	
	static Texture bg1, bg2, bg3, bg4, bg5;
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
	/**
 * 动态难度调整与NPC鱼生成
 * 根据游戏阶段、玩家状态、时间等因素动态调整游戏难度
 * 流程图：
 * 检查阶段阈值 → 更新阶段 → 选择鱼种类 → 生成NPC鱼
 * 特殊处理：阶段2+时河豚可能变异
 */
	int possibility[5] = { 23,22,25,28,30 };
	static int size_threshold[] = {
		55,  // jieduan 0 -> 1
		90,  // 1 -> 2
		135,  // 2 -> 3
		230,  // 3 -> 4
		350   // 4 -> 5
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
	float pulseFreq = 3.0f;
	float pulseAmp = 0.3f;
	float pulseBase = 0.8f;

	if (mutation >= 3 && mutation < 5) {
		pulseFreq = 3.5f;
		pulseAmp = 0.4f;
		pulseBase = 0.87f;
	}

	if (mutation >= 5) {
		pulseFreq =4.0f;
		pulseAmp = 0.5f;
		pulseBase = 0.95f;
	}

	float pulse = sinf(GetTime() * pulseFreq) * pulseAmp + pulseBase;
	baseAlpha *= pulse;

	Color c1, c2, c3;

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
		c1.r = 200; c1.g = 260; c1.b = 255;
		c1.a = (unsigned char)(baseAlpha * 0.65f);

		c2.r = 150; c2.g = 220; c2.b = 255;
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
int check_jitan_sacrifice(Texture jitan)
{
	float jitanW = jitan.width * 3.0f;
	float jitanH = jitan.height * 3.0f;
	float jitanX = (screen_length_x - jitanW) * 0.5f;
	float jitanY = screen_length_y - jitanH;
	Vector2 p;
	p.x = player.xy.x + 1.3f * player.size;
	p.y = player.xy.y + 1.4f * player.size;
	float sac_x = jitanX + jitanW * 0.47f;
	float sac_y = jitanY + jitanH * 0.14f;
	//DrawCircle(sac_x, sac_y, 50, RED);
	float dx = p.x - sac_x;
	float dy = p.y - sac_y;
	if (dx * dx + dy * dy < 15000.0f)
		player.v_xy.x *= 0.8;
	player.v_xy.y *= 0.8;
	if (dx * dx + dy * dy > 2500.0f)
		return 0;
	player.xy.x += (rand() % 20) / 4.0f - 2.5f;
	player.v_xy.x *= 0.5;
	player.v_xy.y *= 0.5;
	DrawText(
		"Press the 'K' key to complete the sacrifice",
		screen_length_x / 2 - 150,
		screen_length_y - 60,
		30,
		RED
	);
	static int k_count = 0;
	static float last_k_time = 0.0f;
	//if (resetting) {
	//	k_count = 0; last_k_time = 0.0f;
 //   }
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
void UI_DrawEnding(void)
{
	typedef struct {
		const char* title;   // 主标题
		Color       color;   // 字体颜色
		int         fontSize;// 字体大小
	} EndingUI;
	static const Color COLOR_ABYSS = { 160, 60, 60, 255 };
	static const Color COLOR_GRAY = { 170, 170, 170, 255 };
	static const Color COLOR_WHITE = { 220, 220, 220, 255 };
	static const Color COLOR_MINT = { 180, 220, 180, 255 };
	EndingUI ending_table[14] = {
		// 0 最初的逝去
		{ "The First Departure", COLOR_WHITE, 36 },
		// 1 风和日丽
		{ "Calm Waters", COLOR_GRAY, 34 },
		// 2 帷幕一角
		{ "A Corner of the Veil", COLOR_GRAY, 36 },
		// 3 肉体凡胎
		{ "Mortal Flesh", COLOR_GRAY, 36 },
		// 4 深渊恐惧
		{ "Fear of the Abyss", COLOR_GRAY, 40 },
		// 5 世界向我走来（低 san）
		{ "The World Draws Near", COLOR_ABYSS, 36 },
		// 6 祂！
		{ "The One.", COLOR_ABYSS, 44 },
		// 7 必要的代价
		{ "The Necessary Price", COLOR_GRAY, 36 },
		// 8 我？
		{ "Me?", COLOR_ABYSS, 40 },
		// 9 结束了？
		{ "Is It Over?", COLOR_WHITE, 34 },
		// 10 祂在我体内苏醒了
		{ "It Awakens Within Me", COLOR_ABYSS, 40 },
		// 11 胜利？……值得吗？
		{ "Triumph, Yet Nothing Remains… Was It Worth It?", COLOR_WHITE, 34 },
		// 12 彩蛋：河豚
		{ "Delicious, Yet Deadly", COLOR_MINT, 36 },
		// 13 褪色者
		{ "Tarnished", COLOR_WHITE, 36 }
	};

	if (ending_id < 0 || ending_id >= 13)
		ending_id = 0;

	EndingUI* e = &ending_table[ending_id];
	int size = e->fontSize + (rand() % 3 - 1);
	DrawRectangle(
		0,
		0,
		screen_length_x,
		screen_length_y,
		Fade(BLACK, 0.7f)
	);
	if (ending_id == 10 || ending_id == 7 || ending_id == 5) {
		DrawText(
			e->title,
			screen_length_x / 2 - MeasureText(e->title, e->fontSize) / 2,
			screen_length_y / 2 - e->fontSize / 2,
			size,
			e->color
		);
	}
	else {
		DrawText(
			e->title,
			screen_length_x / 2 - MeasureText(e->title, e->fontSize) / 2,
			screen_length_y / 2 - e->fontSize / 2,
			e->fontSize,
			e->color
		);
	}
	DrawText(TextFormat("Score: %d", runingtime), screen_length_x / 2 - 80, screen_length_y / 2+e->fontSize, 24, WHITE);
	DrawText(
		"PRESS ENTER TO RETURN",
		screen_length_x / 2 - 160,
		screen_length_y / 2 + e->fontSize+40,
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
	"REDUNDANCY",  
	"SHOW HITBOX",  
	"DEVELOPER MODE",
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
		case SETTINGS_COLLISION_REDUNDANCY:
			// ← 这里只负责“显示状态”，不改逻辑
			valueText = collision_redundancy ? "ON" : "OFF";
			break;

		case SETTINGS_SHOW_HITBOX:
			// ← 显示当前是否开启碰撞箱
			valueText = show_hitbox ? "ON" : "OFF";
			break;
		case SETTINGS_Developer_Mode:
			// ← 显示当前是否开启碰撞箱
			valueText = developer_mode ? "ON" : "OFF";
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
				SetMusicVolume(bgMusic, game_volume);  
				break;

			case SETTINGS_DIFFICULTY:
				game_difficulty += direction;
				if (game_difficulty < 0) game_difficulty = 0;
				if (game_difficulty > 2) game_difficulty = 2;
				difficult = 0.7 + 0.5* game_difficulty;
				break;

			case SETTINGS_RESOLUTION:
			{
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
			}
				break;
			case SETTINGS_COLLISION_REDUNDANCY:
			{
				// ← 在这里修改“碰撞冗余”的全局变量
				collision_redundancy = !collision_redundancy;
			}
				break;

			case SETTINGS_SHOW_HITBOX:
			{
				// ← 在这里修改“显示碰撞箱”的全局变量
				show_hitbox = !show_hitbox;
				break;
			}
			case SETTINGS_Developer_Mode:
			{
				// ← 在这里修改“开发者模式”的全局变量
				developer_mode = !developer_mode;
				break;
			}

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

void update_all_bossfish(fishPool* pool,int fishnumber)
{/**
 * 更新所有BOSS鱼状态
 * 参数：对象池，BOSS鱼数量
 * BOSS种类：
 * 101 - 放射型：直线移动，逐渐减速
 * 102 - 环绕型：先环绕玩家，后发射
 * 103 - 追踪型：向玩家加速，一段时间后惯性移动
 */
	Vector2 playeri = { player.xy.x + 1.3 * player.size,player.xy.y + 1.4 * player.size };

	for (int i = 0; i < fishnumber; i++) {
		if (!pool->used[i]) continue;

		fish* f = &pool->fishnpc[i].fish;
		f->size = f->threatsize; f->last_xy = f->xy;

		// ================= kind 101：放射型 =================
		if (f->kinds == 101) {
			f->xy.x += f->v_xy.x;
			f->xy.y += f->v_xy.y;
			f->v_xy.x *= 0.998f;
			f->v_xy.y *= 0.998f;
		}

		// ================= kind 102：环绕 → 射出 =================
		else if (f->kinds == 102)
		{
			

			// ================= phase 0：入场 =================
			if (f->phase == 0) {
				f->xy.x += f->v_xy.x;
				f->xy.y += f->v_xy.y;

				// 接近玩家后，切换到环绕
				float dx = f->xy.x - playeri.x;
				float dy = f->xy.y - playeri.y;
				float dist = sqrtf(dx * dx + dy * dy);

				if (dist < 300.0f+ 1.5 * f->size) {
					f->phase = 1;
					 f->timer = atan2f(dy, dx);
				}
			}
			
			// ================= phase 1：环绕 =================
			else if (f->phase == 1) {
				float r = 280.0f+1.5*f->size;
				float rot_speed = 0.1f;

				f->timer += rot_speed;

				f->xy.x = playeri.x + cosf(f->timer) * r;
				f->xy.y = playeri.y + sinf(f->timer) * r;

				// 环绕一圈后射出
				if (f->timer > 2 * PI + 0.5f) {
					Vector2 d = {
						cosf(f->timer),
						sinf(f->timer)
					};

					f->v_xy.x = -d.x * 30.0f;
					f->v_xy.y = -d.y * 30.0f;
					f->phase = 2;
				}


				// ================= phase 2：射出 =================
				else {
					f->xy.x += f->v_xy.x;
					f->xy.y += f->v_xy.y;
					f->v_xy.x *= 0.995f;
					f->v_xy.y *= 0.995f;
				}
			}
			else {
				f->xy.x += f->v_xy.x;
				f->xy.y += f->v_xy.y;

				// 射出后减速
				f->v_xy.x *= 0.995f;
				f->v_xy.y *= 0.995f;
			}
		}

		// ================= kind 103：残影型 =================
		else if (f->kinds == 103) {
			f->xy.x += f->v_xy.x;
			f->xy.y += f->v_xy.y;
			if (f->timer < 50.0f) {

				Vector2 d = {
					player.xy.x - f->xy.x,
					player.xy.y - f->xy.y
				};
				float len = sqrtf(d.x * d.x + d.y * d.y);
				if (len > 0.001f) {
					d.x /= len;
					d.y /= len;
				}
				float accel = 1.5f;
				f->v_xy.x += d.x * accel;
				f->v_xy.y += d.y * accel;
				float speed = sqrtf(f->v_xy.x * f->v_xy.x + f->v_xy.y * f->v_xy.y);
				float max_speed = 12.0f;
				if (speed > max_speed) {
					f->v_xy.x = f->v_xy.x / speed * max_speed;
					f->v_xy.y = f->v_xy.y / speed * max_speed;
				}
			}
			f->timer += 1.0f;
		}
		if (f->xy.x < -300 ||f->xy.x > screen_length_x + 300 ||
			f->xy.y < -250 || f->xy.y > screen_length_y + 250) {
			pool->used[i] = 0;
			pool->active_count--;
		}
		Vector2 move = {
	f->xy.x - f->last_xy.x,
	f->xy.y - f->last_xy.y
		};

		float len2 = move.x * move.x + move.y * move.y;
		if (len2 > 0.0001f) {
			f->angle = atan2f(move.y, move.x) * RAD2DEG + 180.0f;
		}


	}
}
void create_bossfish(fishPool* pool, int kind, int count,int fishnumber)
{
	int created = 0;
	Vector2 playeri = { player.xy.x + 1.3 * player.size,player.xy.y + 1.4 * player.size };
	for (int i = 0; i <fishnumber&& created < count; i++) {
		if (pool->used[i]) continue;

		fish* f = &pool->fishnpc[i].fish;
		Vector2 spawn = get_legal_point();
		f->xy = spawn;
		pool->used[i] = 1;
		f->kinds = kind;
		f->timer = 0.0f;
		f->phase = 0;
		f->threatsize = player.threatsize * (0.6f + rand() % 80 / 100.0f);
		int r = rand() % 100;  // 0 ~ 99

		if (r < 20) {
			f->image_status = 0;          // 20%
		}
		else if (r < 40) {
			f->image_status = 2;          // 20%
		}
		else if (r < 60) {
			f->image_status = 3;          // 20%
		}
		else if (r < 80) {
			f->image_status = 4;          // 20%
		}
		else {
			f->image_status = 5;          // 20%
		}
		Vector2 d = {
			playeri.x - f->xy.x,
			playeri.y - f->xy.y
		};
		float len = sqrtf(d.x * d.x + d.y * d.y);
		if (len > 0.001f) {
			d.x /= len;
			d.y /= len;
		}
		// 101：放射型（略带散射）
		if (kind == 101) {
			float spread = ((rand() % 100) / 100.0f - 0.5f) * 0.5f;
			float ca = cosf(spread);
			float sa = sinf(spread);

			float vx = d.x * ca - d.y * sa;
			float vy = d.x * sa + d.y * ca;

			f->v_xy.x = vx * 40.0f;
			f->v_xy.y = vy * 40.0f;
		}

		// 102：环绕型（先慢速入场，update 中接管）
		else if (kind == 102) {
			f->v_xy.x = d.x * 20.0f;
			f->v_xy.y = d.y * 20.0f;

			// timer 用作初始环绕相位
			f->timer = (float)(rand() % 360) * DEG2RAD;
		}

		// 103：残影型（缓慢、稳定）
		else if (kind == 103) {
			f->v_xy.x = d.x * 12.0f;
			f->v_xy.y = d.y * 12.0f;
		}

		// ================= 初始朝向 =================
		f->angle = atan2f(f->v_xy.y, f->v_xy.x) * RAD2DEG + 180.0f;

		created++;
	}
}
void DrawFishRotated(Texture2D tex, Vector2 pos, float size, float baseSize, Color tint, float rotation)
{
	Rectangle src = {
		0,
		0,
		(float)tex.width,
		(float)tex.height
	};

	float scale = size / (baseSize * sizetimes);

	Rectangle dst = {
		pos.x + size * 1.4f,  // 调整中心点
		pos.y + size * 1.3f,
		tex.width * scale,
		tex.height * scale
	};

	Vector2 origin = {
		dst.width * 0.5f,
		dst.height * 0.5f
	};

	// 使用Raylib的DrawTexturePro进行旋转绘制
	DrawTexturePro(
		tex,
		src,
		dst,
		origin,
		rotation,  // 旋转角度
		tint
	);
}
void DrawFishAutoFlipEx(Texture2D tex_left, Vector2 pos, float size, float baseSize, Color tint, float vx, float rotation)
{
	Rectangle src = {
		0,
		0,
		(float)tex_left.width,
		(float)tex_left.height
	};

	// 根据水平速度决定是否翻转
	if (vx > 0) { // 向右游 → 翻转
		src.x = tex_left.width;
		src.width = -src.width;
	}

	float scale = size / (baseSize * sizetimes);

	Rectangle dst = {
		pos.x + size * 1.4f,  // 调整中心点
		pos.y + size * 1.3f,
		tex_left.width * scale,
		tex_left.height * scale
	};

	Vector2 origin = {
		dst.width * 0.5f,
		dst.height * 0.5f
	};

	DrawTexturePro(
		tex_left,
		src,
		dst,
		origin,
		rotation,  // 旋转角度
		tint
	);
}
void collision_player_bossfish(fishPool* pool,int *fishnumber)
{
	Vector2 p_center = {
		player.xy.x + player.size * 1.4f,
		player.xy.y + player.size * 1.3f
	};

	float p_radius = player.size;

	for (int i = 0; i < *fishnumber; i++) {
		if (!pool->used[i]) continue;
		fish* f = &pool->fishnpc[i].fish;
		if (f->kinds < 100) continue;
		Vector2 f_center = {
			f->xy.x + f->size * 1.4f,
			f->xy.y + f->size * 1.3f
		};
		float dx = p_center.x - f_center.x;
		float dy = p_center.y - f_center.y;
		float dist_sq = dx * dx + dy * dy;
		float hit_r = p_radius + f->size;
		if (dist_sq <= hit_r * hit_r) {
			if (f->image_status == 0) {
				player.a += 0.1f;
				san += 5;
				if (*fishnumber >= 1)
					(*fishnumber)--;
				pool->used[i] = 0;
			}
			else {
				for (int j = 0; j < *fishnumber; j++) {
					if (!pool->used[j]) continue;
					if (pool->fishnpc[j].fish.kinds >= 100&&pool->fishnpc[j].fish.image_status!=0) {
						pool->used[j] = 0;
					}
				}
				san -= 10.0f;
				if (san < 0) san = 0;
				return;
			}
		}
	}
}
