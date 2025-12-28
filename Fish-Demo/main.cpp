
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



int running = 1;//0是退出，1是正常，2是暂停，3是死亡
int sleeptime = 100, screen_length_x = 2048, screen_length_y = 1152,runingtime=0;
fish player = { {screen_length_x/2,screen_length_y/2},{0,0},5,30 * sizetimes,0,0,0 };//玩家初始化
int deepth[5] = {50,200,1000,5000,10000}, difficult = 1,jieduan=0;//深度：50-200-1000-5000-10000
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
void draw_background1(void);
void change_difficult(fishPool* pool);


Texture bg1, bg2, bg3 , bg4,bg5;
static int slow_timer = 0;
int main() {

	



	InitWindow(screen_length_x, screen_length_y, "fish game");
	Texture xiaocouyu_texture[2] = { LoadTexture("../img/fish/小丑鱼 左向 模糊.png"),LoadTexture("../img/fish/小丑鱼 左向 清晰.png") }, xiaocouyu_texture2[2] = { LoadTexture("../img/fish/小丑鱼 右向 模糊.png"),LoadTexture("../img/fish/小丑鱼 右向 清晰.png") };
	Texture hetun_texture[2] = { LoadTexture("../img/fish/河豚 左向.png"),LoadTexture("../img/fish/河豚 右向.png") };
	Texture hetun_texture2[2] = { LoadTexture("../img/fish/河豚 鼓起 左向.png"),LoadTexture("../img/fish/河豚 鼓起 右向.png") };
	Texture jianyu_texture[2] = { LoadTexture("../img/fish/剑鱼 左向.png"),LoadTexture("../img/fish/剑鱼 右向.png") };
	Texture shayu_texture[2] = { LoadTexture("../img/fish/鲨鱼 左向.png"),LoadTexture("../img/fish/鲨鱼 右向.png") };
	bg1 = LoadTexture("../img/bg/Background1.png"); bg2 = LoadTexture("../img/bg/Background2.png"); bg3 = LoadTexture("../img/bg/Background3.png"); bg4 = LoadTexture("../img/bg/Background4.png"); bg5 = LoadTexture("../img/bg/Background6.png");

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
		update_all_fish(&pool);
		collision_npc(&pool);


		BeginDrawing();            // 开始绘制
		ClearBackground(BLACK);    // 清除上一帧
		
		draw_background1();
		//渲染玩家鱼
		Color playerTint = WHITE;

		// 如果中毒（减速中）
		if (slow_timer > 0 ) {
			Color green= { 180, 255, 180, 255 };
			playerTint = green;  // 淡绿色
		}
		if (player.v_xy.x > 0)
		{
			if (runingtime % 10<5)
				DrawTextureEx(xiaocouyu_texture2[0], player.xy, 0, player.size / (32.0 * sizetimes), playerTint);
			else DrawTextureEx(xiaocouyu_texture2[1], player.xy, 0, player.size / (32.0 * sizetimes), playerTint);
		}
		else {
			if (runingtime % 10<5)
				DrawTextureEx(xiaocouyu_texture[0], player.xy, 0, player.size / (32.0 * sizetimes), playerTint);
			else DrawTextureEx(xiaocouyu_texture[1], player.xy, 0, player.size / (32.0 * sizetimes), playerTint);
		}
		
		
		//渲染所有NPC鱼
		for (int i = 0; i < MAX_fish; i++) {
			if (pool.used[i]) {
				switch (xiaocouyu_texture2, pool.fishnpc[i].fish.kinds) {
				case 1:if (pool.fishnpc[i].fish.v_xy.x > 0){
					if (runingtime % 10 < 5)DrawTextureEx(xiaocouyu_texture2[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (32.0 * sizetimes), WHITE);
					else DrawTextureEx(xiaocouyu_texture2[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (32.0 * sizetimes), WHITE);
				}else {
					if (runingtime % 10<5)
						DrawTextureEx(xiaocouyu_texture[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (32.0 * sizetimes), WHITE);
					else DrawTextureEx(xiaocouyu_texture[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (32.0 * sizetimes), WHITE);
				}
						break;
				case 2:if (pool.fishnpc[i].fish.v_xy.x > 0) {
					if (pool.fishnpc[i].fish.image_status == 2)DrawTextureEx(hetun_texture2[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (28.0 * sizetimes), WHITE);
					  else DrawTextureEx(hetun_texture[1], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (28.0 * sizetimes), WHITE);
				}
					  else {
					if (pool.fishnpc[i].fish.image_status == 2)DrawTextureEx(hetun_texture2[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (28.0 * sizetimes), WHITE);
					else DrawTextureEx(hetun_texture[0], pool.fishnpc[i].fish.xy, 0, pool.fishnpc[i].fish.size / (28.0 * sizetimes), WHITE);
				}
					
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
			if(IsKeyDown(KEY_Q)){
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
				DrawText(
					TextFormat("Size: %.2f", pool.fishnpc[i].fish.size),
					cj.x,
					cj.y,
					24,
					WHITE
				);
			}
			
			
		}

		if (runingtime % 10 == 0)printf("size:%f 阶段：%d change:%f 进度：%lf time:%d\n", player.size, jieduan, difficult * sqrt(player.size * (jieduan + 1)) * 0.1, (sqrt(player.size * (jieduan + 1)) + pow((float)runingtime, 0.2)), runingtime);








		runingtime++;
		if (running == 3) {
			DrawText("You Died! Press ESC to Exit.", screen_length_x / 2 - 150, screen_length_y / 2, 20, RED);
		}
		WaitTime(0.05);
		EndDrawing();
	
		if (running == 3) {
			WaitTime(5);
			break;
		}
	}




	return 0;
}




void playermove(fish* player) {
	if (IsKeyDown(KEY_SPACE)) {
		running = 2;
		return;
	}

	if (slow_timer > 0) {
		slow_timer--;
		if (slow_timer == 0) {
			player->a = 5;
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
					// 正常状态，检测玩家是否在正前方
					Vector2 f = fishPtr->fish.v_xy;
					float fl = sqrt(f.x * f.x + f.y * f.y);
					if (fl > 0.01f) {
						f.x /= fl; f.y /= fl;
						Vector2 toP = {
							player.xy.x - fishPtr->fish.xy.x,
							player.xy.y - fishPtr->fish.xy.y
						};
						float d = sqrt(toP.x * toP.x + toP.y * toP.y);

						if (d < 220.0f) {
							toP.x /= d; toP.y /= d;
							if (f.x * toP.x + f.y * toP.y > 0.7f && rand() % 100 < 30) {
								fishPtr->fish.image_status = 2; // 鼓起
								puffer_timer[i] = 100;
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
			if (fishPtr->fish.xy.x < -300 || fishPtr->fish.xy.x > screen_length_x+300 ||
				fishPtr->fish.xy.y < -150 || fishPtr->fish.xy.y > screen_length_y+150) {
				release_fish(pool, fishPtr); 
			}
		}
	}
}

fish_NPC* create_npcfish(fishPool* pool,float x,float y ,int kind) {

	static int kind_min_size[] = {
	0,    // kind 0（不用）
	20,   // kind 1
	15,   // kind 2
	45,   // kind 3
	60   // kind 4
	};

	static int kind_max_size[] = {
		0,
		60,
		50,
		115,
		150
	};

			fish_NPC* newFish = get_fish(pool);
			int numberofaim = 5;
			if (newFish == NULL) {
				return 0; // 池已满，无法创建新鱼
			}
			/*double change = difficult * sqrt(player.size * (jieduan + 1)) * 0.15-0.05;*/
				// 危险鱼尺寸解锁
			static int danger_min_size[] = {
				43, 57, 79, 115, 129, 150
			};

			static int danger_max_size[] = {
				64, 93,114, 150, 165, 190
			};


			static int danger_rate[] = {
				20, 25, 32, 45, 55, 60
			};
			int is_danger = (rand() % 100) < danger_rate[jieduan], size0 = 1;
			if (!is_danger)
			{
				int min_size = (int)(player.size * 0.4f);
				int max_size = (int)(player.size * 0.8f);
				if (min_size < 10) min_size = 10;
				if (max_size <= min_size) max_size = min_size + 1;
				size0 = GetRandomValue(min_size, max_size);
			}
			else
			{
				int min_size = danger_min_size[jieduan];
				int max_size = danger_max_size[jieduan];
				size0 = GetRandomValue(min_size, min( max_size,player.size*1.5));
				int must_be_bigger = (int)(player.size * 1.08f);
				if (size0< must_be_bigger)
					size0 = must_be_bigger;
			}while (kind > 0)
			{
				if (size0 < kind_min_size[kind])
				{
					kind--;          // 尺寸太小，降级
					continue;
				}

				if (size0 > kind_max_size[kind])
				{
					release_fish(pool, newFish); // 尺寸过大，世界拒绝
					return 0;
				}

				break; // 合法
			}
			

		newFish->fish.xy.x = x;
		newFish->fish.xy.y = y;
		switch (kind)
		{
		case 1: {
			newFish->fish.v_xy.x = (rand() % 7) - 2;
			newFish->fish.v_xy.y = (rand() % 7) - 2;
			newFish->fish.size = /*(20 + rand() % 30)*/size0 * sizetimes;
			newFish->fish.a = 1*difficult;
			newFish->fish.kinds = kind;
			newFish->fish.image_status = kind;
			for(int i=0;i<numberofaim;i++){
				newFish->aim[i].x = rand() % screen_length_x*1.2;
				newFish->aim[i].y = rand() % screen_length_y*1.2;
			}
			
		}
			  break;
		case 2: {
			newFish->fish.v_xy.x = (rand() % 7) - 2;
			newFish->fish.v_xy.y = (rand() % 7) - 2;
			newFish->fish.size =/* (15 + rand() % 25)*/ size0* sizetimes;
			newFish->fish.a = 0.6*difficult;
			newFish->fish.kinds = kind;
			newFish->fish.image_status = 1;
			for (int i = 0; i < numberofaim; i++) {
				newFish->aim[i].x = rand() % screen_length_x * 1.2;
				newFish->aim[i].y = rand() % screen_length_y * 1.2;
			}
			
		}
			  break;
		case 3: {
			newFish->fish.v_xy.x = (rand() % 10) - 4;
			newFish->fish.v_xy.y = (rand() % 10) - 4;
			newFish->fish.size = /*(45 + rand() % 20)*/ size0* sizetimes;
			newFish->fish.a = 3*difficult;
			newFish->fish.kinds = kind;
			newFish->fish.image_status = kind;
			for (int i = 0; i < numberofaim; i++) {
				newFish->aim[i].x = rand() % screen_length_x * 1.2;
				newFish->aim[i].y = rand() % screen_length_y * 0.05+newFish->fish.xy.y;
				
			}
			
		}
			  break;
		case 4: {
			newFish->fish.v_xy.x = (rand() % 7) - 2;
			newFish->fish.v_xy.y = (rand() % 7) - 2;
			newFish->fish.size = /*(60 + rand() % 50)*/size0 * sizetimes;
			newFish->fish.a = 2*difficult;
			newFish->fish.kinds = kind;
			newFish->fish.image_status = kind;
			for (int i = 0; i < numberofaim; i++) {
				newFish->aim[i].x = rand() % screen_length_x * 1.2;
				newFish->aim[i].y = rand() % screen_length_y * 0.5 + newFish->fish.xy.y;

			}
		}
			  break;
		default:
			release_fish(pool,newFish);
			return 0;
			break;
		}
		newFish->aim[numberofaim].x = max(100 * rand() % screen_length_x, 3000);
		newFish->aim[numberofaim].y = max(100 * rand() % screen_length_y, 2000);
	return newFish;
}

Vector2 get_legal_point(void) {
	
	Vector2 point;
	if(rand()%2==0)
		point.x = rand() %100 -200;
	else
	point.x = rand() %100+ screen_length_x+100;
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
				
				ci.x=pool->fishnpc[i].fish.xy.x + pool->fishnpc[i].fish.size*1.4;
				ci.y=pool->fishnpc[i].fish.xy.y + pool->fishnpc[i].fish.size*1.3;
				
				break;
			case 3:
				
			   ci.x=pool->fishnpc[i].fish.xy.x + pool->fishnpc[i].fish.size * 2;
				ci.y=pool->fishnpc[i].fish.xy.y + pool->fishnpc[i].fish.size * 2;
				
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
						cj.x = pool->fishnpc[j].fish.xy.x + pool->fishnpc[j].fish.size*1.4;
							cj.y = pool->fishnpc[j].fish.xy.y + pool->fishnpc[j].fish.size*1.3;
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

						if (CheckCollisionCircles(ci,pool->fishnpc[i].fish.size,cj,pool->fishnpc[j].fish.size)) {
							if (pool->fishnpc[i].fish.size > pool->fishnpc[j].fish.size) {
								release_fish(pool, &pool->fishnpc[j]);
								pool->fishnpc[i].fish.size +=sqrt(pool->fishnpc[j].fish.size) * 0.3;
								Vector2 target{ pool->fishnpc[j].fish.xy.x - pool->fishnpc[i].fish.xy.x,pool->fishnpc[j].fish.xy.y - pool->fishnpc[i].fish.xy.y };
								pool->fishnpc[i].fish.v_xy.x += 5 * pool->fishnpc[i].fish.a * target.x / sqrt((target.x) * (target.x) + (target.y) * (target.y));
								pool->fishnpc[i].fish.v_xy.y += 5* pool->fishnpc[i].fish.a * target.y / sqrt((target.x) * (target.x) + (target.y) * (target.y));
							}
							else {
								release_fish(pool, &pool->fishnpc[i]);
								pool->fishnpc[j].fish.size +=sqrt(pool->fishnpc[i].fish.size) * 0.3;
								Vector2 target{ pool->fishnpc[i].fish.xy.x - pool->fishnpc[j].fish.xy.x,pool->fishnpc[i].fish.xy.y - pool->fishnpc[j].fish.xy.y };
								pool->fishnpc[j].fish.v_xy.x += 5 * pool->fishnpc[j].fish.a * target.x / sqrt((target.x) * (target.x) + (target.y) * (target.y));
								pool->fishnpc[j].fish.v_xy.y += 5 * pool->fishnpc[j].fish.a * target.y / sqrt((target.x) * (target.x) + (target.y) * (target.y));
							}
						}
					}
				}
			}

			Vector2 cplayer = {
				player.xy.x + player.size*1.4,
				player.xy.y + player.size*1.3
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
						player.size +=sqrt(pool->fishnpc[i].fish.size) * 0.2-0.5;
						Vector2 target{ pool->fishnpc[i].fish.xy.x - player.xy.x,pool->fishnpc[i].fish.xy.y - player.xy.y };
						player.v_xy.x += 3 * player.a * target.x / sqrt((target.x) * (target.x) + (target.y) * (target.y));
						player.v_xy.y += 3 * player.a * target.y / sqrt((target.x) * (target.x) + (target.y) * (target.y));
						if (pool->fishnpc[i].fish.kinds == 2 &&
							pool->fishnpc[i].fish.image_status == 2) {
							player.size -= 0.5;
							// 刷新 5 秒减速
							player.a *= 0.5f;
							slow_timer = 200; 
						}
					}
				}
			}
		}
	}
}
void draw_background1(void)
{
	float scale1 = (float)screen_length_y / bg1.height;
	float scale2 = (float)screen_length_y / bg2.height;
	float scale3 = (float)screen_length_y / bg3.height;
	float scale4 = (float)screen_length_y / bg4.height;
	float scale5 = (float)screen_length_y / bg5.height;
	float w1 = bg1.width * scale1;
	float w2 = bg2.width * scale2;
	float w3 = bg3.width * scale3;
	float w4 = bg4.width * scale4;
	float w5 = bg5.width * scale4;
	
	bg1_x -= player.v_xy.x * 0.05f;
	bg2_x -= player.v_xy.x * 0.10f;
	bg3_x -= player.v_xy.x * 0.20f;
	bg4_x -= player.v_xy.x * 0.35f;

	
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
	Vector2 origin = { 0, 0 }, origin2 = { 0,screen_length_y };
	Rectangle bg1x = { bg1_x+w1/2, 0, w1, screen_length_y }, bg1y = { bg1_x - w1/2, 0, w1, screen_length_y };
	Rectangle bg2x = { bg2_x+w2/2, 0, w2, screen_length_y }, bg2y = { bg2_x - w2/2, 0, w2, screen_length_y };
	Rectangle bg3x = { bg3_x+w3/2, 0, w3, screen_length_y }, bg3y = { bg3_x - w3/2, 0, w3, screen_length_y };
	Rectangle bg4x = { bg4_x+w4/2, 0, w4, screen_length_y }, bg4y = { bg4_x - w4/2, 0, w4, screen_length_y };
	
	DrawTexturePro(bg1, src1, bg1x, origin, 0, WHITE);
	DrawTexturePro(bg1, src1, bg1y, origin, 0, WHITE);

	DrawTexturePro(bg2, src2, bg2x , origin, 0, WHITE);
	DrawTexturePro(bg2, src2, bg2y , origin, 0, WHITE);

	DrawTexturePro(bg3, src3, bg3x , origin, 0, WHITE);
	DrawTexturePro(bg3, src3, bg3y , origin, 0, WHITE);

	DrawTexturePro(bg4, src4, bg4x, origin, 0, WHITE);
	DrawTexturePro(bg4, src4, bg4y , origin, 0, WHITE);

	}
void change_difficult(fishPool* pool){
	int possibility[5] = { 20,31,25,17,13 };
	static int size_threshold[] = {
		45,  // jieduan 0 -> 1
		70,  // 1 -> 2
		95,  // 2 -> 3
		120,  // 3 -> 4
		150   // 4 -> 5
	};
	if (jieduan < 5 && player.size >((size_threshold[jieduan] * sizetimes) + runingtime / 50))
	{
		jieduan++;
	}
	static int list[5][10] = {
		{1,1,1,1,1,1,2,2,2,3},
		{1,1,1,2,2,2,2,3,3,3},
		{1,1,2,2,3,3,3,3,4,4},
		{2,3,3,3,3,3,4,4,4,4}
	};
	Vector2 pos = get_legal_point();
	if (runingtime % possibility[jieduan] == 0)create_npcfish(pool, pos.x, pos.y, list[jieduan][rand() % 10]);


}



