#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int uint32_t;

typedef struct _cuckoo_hash_slot
{
	uint32_t m_backup_bucket_position:30;   /*备用桶的位置, 插入元素时发生碰撞时用来查找下一个存放的位置*/
	uint32_t m_status:2; /*槽的状态, AVAILIBLE，OCCUPIED，DELETED*/
}cuckoo_hash_slot_t;

typedef struct _cuckoo_hash_table
{
	cuckoo_hash_slot_t** m_pBuckets; /*桶*/
	cuckoo_hash_slot_t* m_pSlots;    /*桶关联的槽*/
	uint32_t m_slot_num;           /*槽的个数*/
	uint32_t m_bucket_num;         /*桶的个数*/
}cuckoo_hash_table_t;

cuckoo_hash_table_t g_hash_table;

enum SLOT_STATE
{
	STATE_AVAILIBLE = 0,
	STATE_OCCUPIED = 1,
	/*避免元素不断删除的场景可以快速恢复*/
	STATE_DELETED = 2,
};

/*每个桶关联的槽的个数， 关联的个数越大， 则发生碰撞的概率越小，则空间利用率越大*/
#define SLOT_NUM_PER_BUCKET 4 

const char* cuckoo_stat2str(enum SLOT_STATE status)
{
	switch (status)
	{
		case STATE_OCCUPIED:
			return "STATE_OCCUPIED";
		case STATE_AVAILIBLE:
			return "STATE_AVAILIBLE";
		case STATE_DELETED:
			return "STATE_DELETED";
		default:
			return NULL;
	}
}

void cuckoo_hash_dump(cuckoo_hash_table_t* table)
{
	int i = 0;
	int j = 0;

	/*遍历所有的桶*/
	for (i=0; i<table->m_bucket_num; ++i)
	{
		cuckoo_hash_slot_t* slot = table->m_pBuckets[i];

		/*遍历当前的桶关联的所有的槽*/
		for (j=0; j<SLOT_NUM_PER_BUCKET; ++j)
		{
			printf("\t%d %s", slot[j].m_backup_bucket_position, cuckoo_stat2str(slot[j].m_status));
		}		
		printf("\n");
	}	
}

/*判断是否是2的幂指数 2，4，8，16这样的数*/
static inline int is_pow_of_2(uint32_t x)
{
	return !(x & (x-1));
}

/*找到下一个最近的2的幂指数, 保证槽的个数为 SLOT_NUM_PER_BUCKET的整数倍， 要不然导致有个桶下面的槽的个数为1，2，3导致越界读写*/
static inline uint32_t next_pow_of_2(uint32_t x)
{
	if (is_pow_of_2(x))
		return x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}


/*初始化*/
int cuckoo_hash_init(int slot_num)
{
	uint32_t _init_slot_num = next_pow_of_2(slot_num);
	printf("_init_slot_num %d\n", _init_slot_num);

	/*初始化slot*/
	g_hash_table.m_slot_num = _init_slot_num;
	g_hash_table.m_pSlots = calloc(g_hash_table.m_slot_num, sizeof(cuckoo_hash_slot_t));
	if (NULL == g_hash_table.m_pSlots)
	{
		printf("calloc slots fail\n");
		return -1;
	}		
	
	/*初始化bucket*/
	g_hash_table.m_bucket_num = g_hash_table.m_slot_num/SLOT_NUM_PER_BUCKET;
	g_hash_table.m_pBuckets = calloc(g_hash_table.m_bucket_num, sizeof(cuckoo_hash_slot_t*));
	if (NULL == g_hash_table.m_pBuckets)
	{
		printf("calloc buckets fail\n");
		free(g_hash_table.m_pSlots);
		return -1;
	}	
	
	/*bucket关联slot*/		
	int i;
	for (i=0; i<g_hash_table.m_bucket_num; ++i)
	{
		g_hash_table.m_pBuckets[i] = &(g_hash_table.m_pSlots[i * SLOT_NUM_PER_BUCKET]);
	}
	printf("cuckoo_hash_init succ; slot num %d bucket num %d\n", _init_slot_num, g_hash_table.m_bucket_num);
}

/*下面是算法用到的两个hash函数；一般count为桶的个数，&的结果只能比桶的数字小*/
#define cuckoo_hash_lsb(key, count)  (((size_t *)(key))[0] & (count - 1))
#define cuckoo_hash_msb(key, count)  (((size_t *)(key))[1] & (count - 1))

/*这两个函数为了获取元素有可能在的桶的位置, 一个是当前的位置， 一个是备用的位置*/
/*LSB: Least Significant Byte  最低有效字节  msb（most significant bit），即最高有效位*/
#define get_lsb_bucket_position(key, count)  (((size_t *)(key))[0] & (count - 1))
#define get_msb_bucket_position(key, count)  (((size_t *)(key))[1] & (count - 1))
#define CUCKOO_MAX_COLLIDE_TIMES 512

/*在桶position[0] position[1]中都没有空位置；处理碰撞*/
/*冲突的条件： 桶的位置相同 但备用的位置不一样 学习布谷鸟 把元素踢走*/
int cuckoo_hash_collide(cuckoo_hash_table_t* table, uint32_t* position)
{
	/*既然两个桶都满了，都有冲突；选择第一个桶*/
	cuckoo_hash_slot_t* slot = table->m_pBuckets[position[0]];
	uint32_t kick_position[2];
	kick_position[0] = position[0];

	/*默认把桶的第一个槽给踢出*/
	kick_position[1] = slot[0].m_backup_bucket_position;
	slot[0].m_backup_bucket_position = position[1];

	int i = 0 ^ 1;
	int k = 0;
	/*踢人的总的次数到达512 则需要扩大hash表规模*/
	int kick_count = 0;

KICK_OUT:
	slot = table->m_pBuckets[kick_position[i]];
	int j;
	for (j=0; j<SLOT_NUM_PER_BUCKET; ++j)
	{
		if (STATE_DELETED == slot[j].m_status)
		{
			slot[j].m_status = STATE_OCCUPIED;
			slot[j].m_backup_bucket_position = kick_position[i^1];
			break;
		}
		
		if (STATE_AVAILIBLE == slot[j].m_status)
		{
			slot[j].m_status = STATE_OCCUPIED;
			slot[j].m_backup_bucket_position = kick_position[i^1];
			break;
		}
	}						
	
	/*备用位置也被别人占着*/
	if (SLOT_NUM_PER_BUCKET == j)
	{
		if (++kick_count > CUCKOO_MAX_COLLIDE_TIMES)
		{
			if (k == (SLOT_NUM_PER_BUCKET - 1))
			{
				return 1;
			}
			printf("go to next slot\n");
			++k;
		}

		uint32_t tmp_position = slot[k].m_backup_bucket_position;
		slot[k].m_backup_bucket_position = kick_position[i^1];
		kick_position[i^1] = tmp_position;
		i ^= 1;
		goto KICK_OUT; 	
	}
	return 0;
}

/*把数据插入到hash_table中*/
/*key: 通过url生成的md5sum值, 再根据md5值查找元素有可能在的桶的位置*/
int cuckoo_hash_insert(cuckoo_hash_table_t* table, void* key)
{
	uint32_t position[2];
	position[0] = get_lsb_bucket_position(key, table->m_bucket_num);
	position[1] = get_msb_bucket_position(key, table->m_bucket_num);

	/*position只能判断在哪个桶里面， 具体到哪个槽里面只能通过遍历来确认*/
	/*先看看是否可以放在position[0]所指定的桶里面， 看这个桶里面是否有空位置*/
	cuckoo_hash_slot_t* slot = table->m_pBuckets[position[0]];
	int i = 0;
	for (i=0; i<SLOT_NUM_PER_BUCKET; ++i)
	{
		if (STATE_DELETED == slot[i].m_status)
		{
			printf("position %d STATE_DELETED ==> STATE_OCCUPIED\n", position[0]);
			slot[i].m_status = STATE_OCCUPIED;
			slot[i].m_backup_bucket_position = position[1];
			break;
		}	
		
		if (STATE_AVAILIBLE == slot[i].m_status)
		{
			printf("position %d STATE_AVAILIBLE ==> STATE_OCCUPIED\n", position[0]);
			slot[i].m_status = STATE_OCCUPIED;
			slot[i].m_backup_bucket_position = position[1];	
			break;
		}
	}	

	/*在position[0]位置没有找到空位置；准备在position[1]桶里面找*/
	int j = 0;
	if (SLOT_NUM_PER_BUCKET == i)
	{
		slot = table->m_pBuckets[position[1]];
		for (j=0; j<SLOT_NUM_PER_BUCKET; ++j)
		{
			if (STATE_DELETED == slot[i].m_status)
			{
				printf("position %d STATE_DELETED ==> STATE_OCCUPIED\n", position[1]);
				slot[i].m_status = STATE_OCCUPIED;
				slot[i].m_backup_bucket_position = position[0];
				break;
			}

			if (STATE_AVAILIBLE == slot[i].m_status)
			{
				printf("position %d STATE_AVAILIBLE ==> STATE_OCCUPIED\n", position[1]);
				slot[i].m_status = STATE_OCCUPIED;
				slot[i].m_backup_bucket_position = position[0];
				break;
			}
		}	
	}
	
	/*运气不好，没有空位置, 进行碰撞处理*/
	if (SLOT_NUM_PER_BUCKET == j)	
	{
		uint32_t collide_position[2];
		collide_position[0] = position[0];
		collide_position[1] = position[1];
		printf(" collide_position %d %d\n", collide_position[0], collide_position[1]);
		if (cuckoo_hash_collide(table, collide_position))
		{
			printf("Hash table collision!\n");
			return -1;
		}
	}
}

/*返回key在哈希表中的状态，STATE_DELETED STATE_OCCUPIED STATE_AVAILIBLE*/
int cuckoo_hash_status_get(cuckoo_hash_table_t* table, void* key)
{
	uint32_t position[2];
	position[0] = get_lsb_bucket_position(key, table->m_bucket_num);
	position[1] = get_msb_bucket_position(key, table->m_bucket_num);
	printf("position[0] %d position[1] %d\n", position[0], position[1]);

	cuckoo_hash_slot_t* slot = table->m_pBuckets[position[0]];
	int i;
	for (i=0; i<SLOT_NUM_PER_BUCKET; ++i)
	{
		if (slot[i].m_backup_bucket_position == position[1])
		{
			if (STATE_DELETED == slot[i].m_status)
			{
				return STATE_DELETED;
			}

			if (STATE_OCCUPIED == slot[i].m_status)
			{
				return STATE_OCCUPIED;	
			}
		}	
	}		

	int j;
	if (SLOT_NUM_PER_BUCKET == i)
	{
		slot = table->m_pBuckets[position[1]];
		for (j=0; j<SLOT_NUM_PER_BUCKET; ++j)
		{
			if (slot[j].m_backup_bucket_position == position[0])
			{
				if (STATE_DELETED == slot[j].m_status)
				{
					return STATE_DELETED;
				}	
				
				if (STATE_OCCUPIED == slot[i].m_status)
				{
					return STATE_OCCUPIED;
				}
			}
		}	
	}
	
	if (SLOT_NUM_PER_BUCKET == j)
	{
		printf("key not exist in hash table\n");
		return STATE_AVAILIBLE;		
	}

	return STATE_OCCUPIED;	
}


static void cuckoo_hash_status_set(cuckoo_hash_table_t *table, uint32_t *key, int status)
{
	uint32_t position[2];
	position[0] = get_lsb_bucket_position(key, table->m_bucket_num);
	position[1] = get_msb_bucket_position(key, table->m_bucket_num);
	
	cuckoo_hash_slot_t* slot = table->m_pBuckets[position[0]];
	int i, j;
	for (i=0; i<SLOT_NUM_PER_BUCKET; ++i)
	{
		if (slot[i].m_backup_bucket_position == position[1])
		{
			slot[i].m_status = status;
		}
	}

	if (SLOT_NUM_PER_BUCKET == i)
	{
		slot = table->m_pBuckets[position[1]];
		for (j=0; j<SLOT_NUM_PER_BUCKET; ++j)
		{
			if (slot[j].m_backup_bucket_position == position[0])
			{
				slot[j].m_status = status;
			}
		}
	}

	if (SLOT_NUM_PER_BUCKET == j)
	{
		printf("key not exist\n");
		return;
	}

	printf("cuckoo_hash_status_set succ\n");
}


/*删除和恢复都只是设置状态码而已*/
static void cuckoo_hash_delete(cuckoo_hash_table_t* table, uint32_t *key)
{
        cuckoo_hash_status_set(table, key, STATE_DELETED);
}

static void cuckoo_hash_recover(cuckoo_hash_table_t* table, uint32_t *key)
{
        cuckoo_hash_status_set(table, key, STATE_OCCUPIED);
}

static void cuckoo_rehash(cuckoo_hash_table_t* table)
{
	cuckoo_hash_table_t old_table;
	old_table.m_pSlots = table->m_pSlots;
	old_table.m_slot_num = table->m_slot_num;
	
	table->m_slot_num *= 2;
	table->m_pSlots = calloc(table->m_slot_num, sizeof(cuckoo_hash_slot_t));
	if (NULL == table->m_pSlots)
	{
		printf("rehash slot fail\n");
		return;
	}	
	
	old_table.m_pBuckets = table->m_pBuckets;
	old_table.m_bucket_num = table->m_bucket_num;
	
	table->m_bucket_num *= 2;
	table->m_pBuckets = calloc(table->m_bucket_num, sizeof(cuckoo_hash_slot_t*));
	if (NULL == table->m_pBuckets)
	{
		printf("rehash bucket fail\n");
		free(table->m_pSlots);
		table->m_pSlots = old_table.m_pSlots;
		table->m_slot_num = old_table.m_slot_num;
		return;
	}	

	int i;
	for (i=0; i<table->m_bucket_num; ++i)
	{
		table->m_pBuckets[i] = &(table->m_pSlots[i * SLOT_NUM_PER_BUCKET]);
	}	
	
	/*rehash all hash slots*/
		
	/*free old memory*/
	free(old_table.m_pSlots);
	free(old_table.m_pBuckets);	
}

int main(int argc, char** argv)
{
	printf("hello main\n");
	return 0;
}
