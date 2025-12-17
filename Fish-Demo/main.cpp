
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
#define MAX_fish 20


typedef struct fish {
	Vector2 xy;
	Vector2 v_xy;
	double a;//加速度
	double size;//圆的半径
	int image_status;
	int lizixiaoguo;
	int kinds;//0是玩家，
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

fish player = { {0,0},{0,0},5,60,0,0,0 };

int running = 1;//0是退出，1是正常，2是暂停，3是死亡
int sleeptime = 100, screen_length_x = 2048, screen_length_y = 1152,runingtime=0;

void playermove(fish* player);
void npc_move(fish_NPC* npc);
void init_fish_pool(fishPool* pool);
fish_NPC* get_fish(fishPool* pool);
void release_fish(fishPool* pool, fish_NPC* fishPtr);
void update_all_fish(fishPool* pool);
fish_NPC* create_npcfish(fishPool* pool, float x, float y, int kind);


int main() {





	InitWindow(screen_length_x, screen_length_y, "fish game");
	Texture2D fish_texture = LoadTexture("../img/鱼test.png"), fish_texture2 = LoadTexture("../img/鱼 翻转 test.png");
	SetWindowState(FLAG_VSYNC_HINT);
	fishPool pool;
	init_fish_pool(&pool);

	create_npcfish(&pool, 300, 300, 1);
	while (!WindowShouldClose()) {
		if(runingtime%71==0)create_npcfish(&pool, rand()%screen_length_x, rand()%screen_length_y, 1);
		srand(time(NULL)); 
		playermove(&player);
		update_all_fish(&pool);
		BeginDrawing();            // 开始绘制
		ClearBackground(BLACK);    // 清除上一帧
		
		if (player.v_xy.x > 0)
		{
			DrawTextureEx(fish_texture2, player.xy, 0, 0.08, WHITE);
		}
		else {
			DrawTextureEx(fish_texture, player.xy, 0, 0.08, WHITE);
		}
		
		DrawCircle(player.xy.x+player.size/0.7, player.xy.y+ player.size / 1, player.size, RED);
		//渲染所有NPC鱼
		for (int i = 0; i < MAX_fish; i++) {
			if (pool.used[i]) {
					switch (fish_texture2, pool.fishnpc[i].fish.kinds){
					case 1:DrawTextureEx(fish_texture2, pool.fishnpc[i].fish.xy, 0, 0.08, WHITE);
						break;

				}
			}
		}












		WaitTime(0.05);
		EndDrawing();
		runingtime++;

	}




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
			fishPtr->aim[6] = { -100,-100};
			float k = rand() % 40 / 100.0+0.8;
			Vector2 direction = { fishPtr->aim[0].x- fishPtr->fish.xy.x,fishPtr->aim[0].y - fishPtr->fish.xy.y };
			fishPtr->fish.v_xy.x += k*(direction.x)* fishPtr->fish.a/pow(((direction.x)* (direction.x)+ (direction.y)* (direction.y)),0.5);
			fishPtr->fish.v_xy.y += k*(direction.y) * fishPtr->fish.a / pow(((direction.x) * (direction.x) + (direction.y) * (direction.y)), 0.5);
			fishPtr->fish.v_xy.x *= 0.9;
			fishPtr->fish.v_xy.y *= 0.8;
			// 边界检查
			if (fishPtr->fish.xy.x < 0 || fishPtr->fish.xy.x > screen_length_x ||
				fishPtr->fish.xy.y < 0 || fishPtr->fish.xy.y > screen_length_y) {
				release_fish(pool, fishPtr); 
			}
		}
	}
}

fish_NPC* create_npcfish(fishPool* pool,float x,float y ,int kind) {
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
			newFish->fish.size = 20 + rand() % 30;
			newFish->fish.a = 2;
			newFish->fish.kinds = kind;
			newFish->fish.image_status = kind;
			for(int i=0;i<numberofaim;i++){
				newFish->aim[i].x = rand() % screen_length_x*1.5;
				newFish->aim[i].y = rand() % screen_length_y*1.5;
			}
			newFish->aim[numberofaim].x = max(100*rand() % screen_length_x,3000);
			newFish->aim[numberofaim].y = max(100*rand() % screen_length_y,2000);
		}
			  break;
		default:
			return 0;
			break;
		}

	return newFish;
}

