
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

fish player = { {0,0},{0,0},5,30,0,0,0 };

int running = 1;//0是退出，1是正常，2是暂停，3是死亡
int sleeptime = 100, screen_length_x = 2048, screen_length_y = 1152,runingtime=0;

void playermove(fish* player);
void npc_move(fish_NPC* npc);
void init_fish_pool(fishPool* pool);
fish_NPC* get_fish(fishPool* pool);
void release_fish(fishPool* pool, fish_NPC* fishPtr);
void update_all_fish(fishPool* pool);
fish_NPC* create_npcfish(fishPool* pool, float x, float y, int kind);
Vector2 get_legal_point(void);
void collision_npc(fishPool* pool);

int main() {





	InitWindow(screen_length_x, screen_length_y, "fish game");
	Texture xiaocouyu_texture[2] = { LoadTexture("../img/小丑鱼 左向 模糊.png"),LoadTexture("../img/小丑鱼 左向 清晰.png") }, xiaocouyu_texture2[2] = {LoadTexture("../img/小丑鱼 右向 模糊.png"),LoadTexture("../img/小丑鱼 右向 清晰.png")};
	Texture hetun_texture[2] = { LoadTexture("../img/河豚 左向.png"),LoadTexture("../img/河豚 右向.png") };
	Texture jianyu_texture[2] = { LoadTexture("../img/剑鱼 左向.png"),LoadTexture("../img/剑鱼 右向.png") };
	SetWindowState(FLAG_VSYNC_HINT);
	fishPool pool;
	init_fish_pool(&pool);

	//**************************************************************主循环********************************************************************//
	while (!WindowShouldClose()) {
		Vector2 randpoint = get_legal_point();
		if(runingtime%39==0)create_npcfish(&pool, randpoint.x,randpoint.y, 3);
		srand(time(NULL)); 
		playermove(&player);
		update_all_fish(&pool);
		collision_npc(&pool);


		BeginDrawing();            // 开始绘制
		ClearBackground(BLACK);    // 清除上一帧
		


		if (player.v_xy.x > 0)
		{
			if (runingtime % 10<5)
				DrawTextureEx(xiaocouyu_texture2[0], player.xy, 0, player.size / 32.0, WHITE);
			else DrawTextureEx(xiaocouyu_texture2[1], player.xy, 0, player.size / 32.0, WHITE);
		}
		else {
			if (runingtime % 10<5)
			DrawTextureEx(xiaocouyu_texture[0], player.xy, 0, player.size / 32.0, WHITE);
			else DrawTextureEx(xiaocouyu_texture[1], player.xy, 0, player.size / 32.0, WHITE);
		}
		
		//DrawCircle(player.xy.x+player.size/0.7, player.xy.y+ player.size / 1, player.size, RED);
		//渲染所有NPC鱼
		for (int i = 0; i < MAX_fish; i++) {
			if (pool.used[i]) {
				switch (xiaocouyu_texture2, pool.fishnpc[i].fish.kinds) {
				case 1:if (pool.fishnpc[i].fish.v_xy.x > 0){
					if (runingtime % 10<5)DrawTextureEx(xiaocouyu_texture2[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 32.0, WHITE);
					  else DrawTextureEx(xiaocouyu_texture2[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 32.0, WHITE);
				}else {
					if (runingtime % 10<5)
						DrawTextureEx(xiaocouyu_texture[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 32.0, WHITE);
					else DrawTextureEx(xiaocouyu_texture[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 32.0, WHITE);
				}
						break;
				case 2:if (pool.fishnpc[i].fish.v_xy.x > 0) {
					DrawTextureEx(hetun_texture[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 28.0, WHITE);
				}else{ DrawTextureEx(hetun_texture[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 28.0, WHITE); }
					/*if (runingtime % 10 < 5)DrawTextureEx(xiaocouyu_texture2[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 32.0, WHITE);
					else DrawTextureEx(xiaocouyu_texture2[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 32.0, WHITE);
				}
					  else {
					if (runingtime % 10 < 5)
						DrawTextureEx(xiaocouyu_texture[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 32.0, WHITE);
					else DrawTextureEx(xiaocouyu_texture[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 32.0, WHITE);
				}*/
					  break;
				case 3:if (pool.fishnpc[i].fish.v_xy.x > 0) {
					DrawTextureEx(jianyu_texture[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 28.0, WHITE);
				}
					  else { DrawTextureEx(jianyu_texture[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 28.0, WHITE); }
					  /*if (runingtime % 10 < 5)DrawTextureEx(xiaocouyu_texture2[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 32.0, WHITE);
					  else DrawTextureEx(xiaocouyu_texture2[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 32.0, WHITE);
				  }
						else {
					  if (runingtime % 10 < 5)
						  DrawTextureEx(xiaocouyu_texture[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 32.0, WHITE);
					  else DrawTextureEx(xiaocouyu_texture[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / 32.0, WHITE);
				  }*/
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
			fishPtr->aim[6] = { 3000,2000};
			if(rand()%10>1){
				float k = rand() % 40 / 100.0 + 0.8;
				Vector2 direction = { fishPtr->aim[0].x - fishPtr->fish.xy.x,fishPtr->aim[0].y - fishPtr->fish.xy.y };
				fishPtr->fish.v_xy.x += k * (direction.x) * fishPtr->fish.a / sqrt((direction.x) * (direction.x) + (direction.y) * (direction.y));
				fishPtr->fish.v_xy.y += k * (direction.y) * fishPtr->fish.a / sqrt((direction.x) * (direction.x) + (direction.y) * (direction.y));
			}
			fishPtr->fish.v_xy.x *= 0.95;
			fishPtr->fish.v_xy.y *= 0.9;
			// 边界检查
			if (fishPtr->fish.xy.x < -150 || fishPtr->fish.xy.x > screen_length_x+150 ||
				fishPtr->fish.xy.y < -100 || fishPtr->fish.xy.y > screen_length_y+100) {
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
			newFish->fish.a = 1;
			newFish->fish.kinds = kind;
			newFish->fish.image_status = kind;
			for(int i=0;i<numberofaim;i++){
				newFish->aim[i].x = rand() % screen_length_x*1.2;
				newFish->aim[i].y = rand() % screen_length_y*1.2;
			}
			newFish->aim[numberofaim].x = max(100*rand() % screen_length_x,3000);
			newFish->aim[numberofaim].y = max(100*rand() % screen_length_y,2000);
		}
			  break;
		case 2: {
			newFish->fish.v_xy.x = (rand() % 7) - 2;
			newFish->fish.v_xy.y = (rand() % 7) - 2;
			newFish->fish.size = 15 + rand() % 25;
			newFish->fish.a = 0.6;
			newFish->fish.kinds = kind;
			newFish->fish.image_status = kind;
			for (int i = 0; i < numberofaim; i++) {
				newFish->aim[i].x = rand() % screen_length_x * 1.2;
				newFish->aim[i].y = rand() % screen_length_y * 1.2;
			}
			newFish->aim[numberofaim].x = max(100 * rand() % screen_length_x, 3000);
			newFish->aim[numberofaim].y = max(100 * rand() % screen_length_y, 2000);
		}
			  break;
		case 3: {
			newFish->fish.v_xy.x = (rand() % 10) - 4;
			newFish->fish.v_xy.y = (rand() % 10) - 4;
			newFish->fish.size = 35 + rand() % 50;
			newFish->fish.a = 3;
			newFish->fish.kinds = kind;
			newFish->fish.image_status = kind;
			for (int i = 0; i < numberofaim; i++) {
				newFish->aim[i].x = rand() % screen_length_x * 1.2;
				newFish->aim[i].y = rand() % screen_length_y * 0.1+newFish->fish.xy.y;
				
			}
			newFish->aim[numberofaim].x = max(100 * rand() % screen_length_x, 3000);
			newFish->aim[numberofaim].y = max(100 * rand() % screen_length_y, 2000);
		}
			  break;
		default:
			return 0;
			break;
		}

	return newFish;
}

Vector2 get_legal_point(void) {
	
	Vector2 point;
	if(rand()%2==0)
		point.x = rand() %100 -100;
	else
	point.x = rand() %100+ screen_length_x;
	point.y = rand() % screen_length_y;

	return point;
}
void collision_npc(fishPool* pool) {
	for (int i = 0; i < MAX_fish; i++) {
		if (pool->used[i]) {
			for (int j = i + 1; j < MAX_fish; j++) {
				if (pool->used[j]) {
					if (fabs(pool->fishnpc[i].fish.xy.x + pool->fishnpc[i].fish.xy.y - pool->fishnpc[j].fish.xy.x - pool->fishnpc[j].fish.xy.y) < pool->fishnpc[i].fish.size + pool->fishnpc[j].fish.size) {
						if (CheckCollisionCircles(pool->fishnpc[i].fish.xy, pool->fishnpc[i].fish.size, pool->fishnpc[j].fish.xy, pool->fishnpc[j].fish.size)) {
							if (pool->fishnpc[i].fish.size > pool->fishnpc[j].fish.size) {
								release_fish(pool, &pool->fishnpc[j]);
								pool->fishnpc[i].fish.size += sqrt(pool->fishnpc[j].fish.size)*0.2;//大鱼吃掉小鱼size增加1
								//速度没写
							}
							else {
								release_fish(pool, &pool->fishnpc[i]);
								pool->fishnpc[j].fish.size += sqrt(pool->fishnpc[i].fish.size) * 0.2;//大鱼吃掉小鱼size增加1
								//速度没写
							}
						}
					}

				}
			}
			if (fabs(pool->fishnpc[i].fish.xy.x + pool->fishnpc[i].fish.xy.y - player.xy.x - player.xy.y) < pool->fishnpc[i].fish.size + player.size) {
				if (CheckCollisionCircles(pool->fishnpc[i].fish.xy, pool->fishnpc[i].fish.size, player.xy, player.size)) {
					if (pool->fishnpc[i].fish.size > player.size) {
						running = 3; //玩家死亡
					}
					else {
						release_fish(pool, &pool->fishnpc[i]);
						player.size += sqrt(pool->fishnpc[i].fish.size) * 0.2;//player吃掉小鱼size增加1
						//速度没写
					}
				}
			}

		}
	}

}