
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

int main() {
	InitWindow(screen_length_x, screen_length_y, "fish game");

	// ================= 玩家贴图 =================
	Texture player_tex[2] = {
		LoadTexture("../img/fish/玩家 左向 模糊.png"),
		LoadTexture("../img/fish/玩家 左向 清晰.png")
	};
	Texture player_gray_left =
		LoadTexture("../img/fish/玩家 灰度 左向.png");
	Texture player_tex_by[6]={
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
	//**************************************************************主循环********************************************************************//
	while (!WindowShouldClose()) {
		/*Vector2 randpoint = get_legal_point();
		if(runingtime%17==0)create_npcfish(&pool, randpoint.x,randpoint.y, rand()%4+1);*/

		change_difficult(&pool);
		playermove(&player);
		playermutation();
		update_all_fish(&pool);
		collision_npc(&pool);


		BeginDrawing();            // 开始绘制
		ClearBackground(BLACK);    // 清除上一帧

		draw_background1();
		//光照模拟
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

		//渲染玩家鱼
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
		if(mutation==0){
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
				player_tex_by[mutation-1],
				player.xy,
				player.size,
				32.0f,
				playerTint,
				player.v_xy.x
			);
		}
		//玩家渲染结束

		//渲染所有NPC鱼
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


			if (IsKeyDown(KEY_Q)) {//调试用代码，显示碰撞箱与大小
				Vector2 cj;
				switch (pool.fishnpc[i].fish.kinds)
				{

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
		//NPC鱼渲染结束

		static int size_threshold[] = {
			55,  // jieduan 0 -> 1
			95,  // 1 -> 2
			140,  // 2 -> 3
			230,  // 3 -> 4
			320   // 4 -> 5
		};
		if (runingtime % 10 == 0)printf("size:%.2f+threat:%.2f 阶段：%d 下阶段门槛：%lf time:%d san:%.3f fade:%.3f\n", player.size, player.threatsize,jieduan, ((size_threshold[jieduan] * sizetimes) * san / 100), runingtime, san,fade);
		//调试用代码，显示玩家大小和当前阶段
		if (jieduan == 5) {
			static float toumingdu = 0;
			if(toumingdu<1.0f)toumingdu = min(toumingdu += 0.001, 1.0f);
			Color a = { 0,0,0,toumingdu * 255 };
			DrawTexture(jitan, screen_length_x - jitan.width / 2, screen_length_y - jitan.height, a);
		}
	


		runingtime++; san += 0.0001; san -= 0.001*jieduan;
		if (running == 3) {
			DrawText("You Died! Press ESC to Exit.", screen_length_x / 2 - 150, screen_length_y / 2, 20, RED);
			if (IsKeyPressed(KEY_SPACE))running = 1;
		}
		WaitTime(0.05);
		EndDrawing();

	/*	if (running == 3) {
			WaitTime(5);
			break;
		}*/
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
) {
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
	if (IsKeyDown(KEY_SPACE)) {
		running = 2;
		return;
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





