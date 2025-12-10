#define MAX_FISH 100
#define MAX_BUBBLES 50
#define MAX_PARTICLES 200


typedef struct fish {
    double x;
    double y;
    double v_x;
    double v_y;
    double a;//加速度
    double size;
	bool active; // 是否活跃
    int aim[1][1];
    int image_status;
    int lizixiaoguo;
    int kinds;//0是玩家，
}Fish;
// 鱼的对象池
typedef struct {
    Fish data[MAX_FISH];    // 鱼数组
    int used[MAX_FISH];     // 0=空闲，1=使用中
    int active_count;       // 当前活跃数量
} FishPool;

// 初始化对象池
void init_fish_pool(FishPool* pool) {
    for (int i = 0; i < MAX_FISH; i++) {
        pool->used[i] = 0;  // 全部标记为空闲
        // 可以在这里初始化一些默认值
        pool->data[i].x = 0;
        pool->data[i].y = 0;
        pool->data[i].active = 0;
    }
    pool->active_count = 0;
}

// 从池中获取一条鱼
Fish* get_fish(FishPool* pool) {
    // 遍历寻找空闲的鱼
    for (int i = 0; i < MAX_FISH; i++) {
        if (pool->used[i] == 0) {
            pool->used[i] = 1;  // 标记为使用中
            pool->active_count++;
            pool->data[i].active = 1;  // 激活鱼
            return &pool->data[i];     // 返回鱼的指针
        }
    }
    return 0;  // 池子已满
}

// 回收鱼到池中
void release_fish(FishPool* pool, Fish* fish) {
    int index = fish - pool->data;  // 计算索引
    if (index >= 0 && index < MAX_FISH) {
        pool->used[index] = 0;  // 标记为空闲
        pool->active_count--;
        fish->active = 0;  // 禁用鱼
    }
}

// 批量创建鱼
void create_fish_group(FishPool* pool, int count) {
    for (int i = 0; i < count; i++) {
        Fish* fish = get_fish(pool);
        if (fish) {
            // 初始化鱼的位置、大小等
            fish->x = rand() % SCREEN_WIDTH;
            fish->y = rand() % SCREEN_HEIGHT;
            fish->size = 20 + rand() % 30;
            fish->speed = 1.0 + (rand() % 10) / 10.0;
        }
    }
}

// 更新所有活跃的鱼
void update_all_fish(FishPool* pool) {
    for (int i = 0; i < MAX_FISH; i++) {
        if (pool->used[i]) {  // 只更新活跃的鱼
            Fish* fish = &pool->data[i];

            // 更新位置
            fish->x += fish->speed * cos(fish->angle);
            fish->y += fish->speed * sin(fish->angle);

            // 边界检查
            if (fish->x < 0 || fish->x > SCREEN_WIDTH ||
                fish->y < 0 || fish->y > SCREEN_HEIGHT) {
                release_fish(pool, fish);  // 出界就回收
            }
        }
    }
}