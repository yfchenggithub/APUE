#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int uint32_t;

typedef struct _cuckoo_hash_slot
{
	uint32_t m_tag:30;
	uint32_t m_status:2;
}cuckoo_hash_slot;

typedef struct _cuckoo_hash_table
{
	cuckoo_hash_slot** m_pBuckets;
	cuckoo_hash_slot* m_pSlots;
	uint32_t m_slot_num;
	uint32_t m_bucket_num;
}cuckoo_hash_table;

cuckoo_hash_table g_hash_table;

enum SLOT_STATE
{
	STATE_AVAILIBLE = 0,
	STATE_OCCUPIED = 1,
	STATE_DELETED = 2
};

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
	}
}

void cuckoo_hash_dump(cuckoo_hash_table* table)
{
	int i = 0;
	int j = 0;
	for (i=0; i<table->m_bucket_num; ++i)
	{
		cuckoo_hash_slot* slot = table->m_pBuckets[i];
		for (j=0; j<SLOT_NUM_PER_BUCKET; ++j)
		{
			printf("\t%d %s", slot[j].m_tag, cuckoo_stat2str(slot[j].m_status));
		}		
		printf("\n");
	}	
}

/*这里要求slot_num 为 4 的倍数；后面再完善*/
int cuckoo_hash_init(int slot_num)
{
	/*初始化slot*/
	g_hash_table.m_slot_num = slot_num;
	g_hash_table.m_pSlots = calloc(g_hash_table.m_slot_num, sizeof(cuckoo_hash_slot));
	if (NULL == g_hash_table.m_pSlots)
	{
		printf("calloc slots fail\n");
		return -1;
	}		
	
	/*初始化bucket*/
	g_hash_table.m_bucket_num = g_hash_table.m_slot_num/SLOT_NUM_PER_BUCKET;
	g_hash_table.m_pBuckets = calloc(g_hash_table.m_bucket_num, sizeof(cuckoo_hash_slot*));
	if (NULL == g_hash_table.m_pBuckets)
	{
		printf("calloc buckets fail\n");
		free(g_hash_table.m_pSlots);
		return -1;
	}	
	
	/*bucket映射slot*/		
	int i;
	for (i=0; i<g_hash_table.m_bucket_num; ++i)
	{
		g_hash_table.m_pBuckets[i] = &(g_hash_table.m_pSlots[i*4]);
	}
}

/*下面是算法用到的两个hash函数；一般count为桶的个数，&的结果只能比桶的数字小*/
#define cuckoo_hash_lsb(key, count)  (((size_t *)(key))[0] & (count - 1))
#define cuckoo_hash_msb(key, count)  (((size_t *)(key))[1] & (count - 1))

/*在桶tag[0] tag[1]中都没有空位置；*/
int cuckoo_hash_collide(cuckoo_hash_table* table, uint32_t* tag)
{
	/*既然两个桶都满了，都有冲突；选择第一个桶吧*/
	cuckoo_hash_slot* slot = table->m_pBuckets[tag[0]];
	uint32_t kick_tag[2];
	kick_tag[0] = tag[0];
	/*默认把桶的第一个槽给踢出*/
	kick_tag[1] = slot[0].m_tag;
	slot[0].m_tag = tag[1];
	int i = 0 ^ 1;
	int k = 0;
	/*踢人的总的次数到达512 则需要扩大hash表规模*/
	int kick_count = 0;
KICK_OUT:
	slot = table->m_pBuckets[kick_tag[i]];
	int j;
	for (j=0; j<SLOT_NUM_PER_BUCKET; ++j)
	{
		if (STATE_DELETED == slot[j].m_status)
		{
			slot[j].m_status = STATE_OCCUPIED;
			slot[j].m_tag = kick_tag[i^1];
			break;
		}
		
		if (STATE_AVAILIBLE == slot[j].m_status)
		{
			slot[j].m_status = STATE_OCCUPIED;
			slot[j].m_tag = kick_tag[i^1];
			break;
		}
	}						
	
	/*备用位置也被别人占着*/
	if (SLOT_NUM_PER_BUCKET == j)
	{
		if (++kick_count > 512)
		{
			if (k == SLOT_NUM_PER_BUCKET-1)
			{
				return 1;
			}
			++k;
		}
		uint32_t tmp_tag = slot[k].m_tag;
		slot[k].m_tag = kick_tag[i^1];
		kick_tag[i^1] = tmp_tag;
		i ^= 1;
		goto KICK_OUT; 	
	}
	return 0;
}

/*把数据插入到hash_table中*/
int cuckoo_hash_insert(cuckoo_hash_table* table, void* key)
{
	uint32_t tag[2];
	tag[0] = cuckoo_hash_lsb(key, table->m_bucket_num);
	tag[1] = cuckoo_hash_msb(key, table->m_bucket_num);
	/*tag只能判断在哪个桶里面， 具体到哪个槽里面只能通过遍历来确认*/
	/*先看看是否可以放在tag[0]所指定的桶里面， 看这个桶里面是否有空位置*/
	cuckoo_hash_slot* slot = table->m_pBuckets[tag[0]];
	int i = 0;
	for (i=0; i<SLOT_NUM_PER_BUCKET; ++i)
	{
		if (STATE_DELETED == slot[i].m_status)
		{
			printf("tag %d STATE_DELETED ==> STATE_OCCUPIED\n", tag[0]);
			slot[i].m_status = STATE_OCCUPIED;
			slot[i].m_tag = tag[1];
			break;
		}	
		
		if (STATE_AVAILIBLE == slot[i].m_status)
		{
			printf("tag %d STATE_AVAILIBLE ==> STATE_OCCUPIED\n", tag[0]);
			slot[i].m_status = STATE_OCCUPIED;
			slot[i].m_tag = tag[1];	
			break;
		}
	}	

	/*在tag[0]位置没有找到空位置；准备在tag[1]桶里面找*/
	int j = 0;
	if (SLOT_NUM_PER_BUCKET == i)
	{
		slot = table->m_pBuckets[tag[1]];
		for (j=0; j<SLOT_NUM_PER_BUCKET; ++j)
		{
			if (STATE_DELETED == slot[i].m_status)
			{
				printf("tag %d STATE_DELETED ==> STATE_OCCUPIED\n", tag[1]);
				slot[i].m_status = STATE_OCCUPIED;
				slot[i].m_tag = tag[0];
				break;
			}

			if (STATE_AVAILIBLE == slot[i].m_status)
			{
				printf("tag %d STATE_AVAILIBLE ==> STATE_OCCUPIED\n", tag[1]);
				slot[i].m_status = STATE_OCCUPIED;
				slot[i].m_tag = tag[0];
				break;
			}
		}	
	}
	
	/*运气不好，没有空位置，处理好碰撞*/
	if (SLOT_NUM_PER_BUCKET == j)	
	{
		printf("no lucky; kick one\n");
		if (cuckoo_hash_collide(table, tag))
		{
			printf("Hash table collision!\n");
			return -1;
		}
	}
}

int cuckoo_hash_get(cuckoo_hash_table* table, void* key)
{
	uint32_t tag[2];
	tag[0] = cuckoo_hash_lsb(key, table->m_bucket_num);
	tag[1] = cuckoo_hash_msb(key, table->m_bucket_num);
	printf("tag[0] %d tag[1] %d\n", tag[0], tag[1]);
	cuckoo_hash_slot* slot = table->m_pBuckets[tag[0]];
	int i;
	for (i=0; i<SLOT_NUM_PER_BUCKET; ++i)
	{
		if (slot[i].m_tag == tag[1])
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
		slot = table->m_pBuckets[tag[1]];
		for (j=0; j<SLOT_NUM_PER_BUCKET; ++j)
		{
			if (slot[j].m_tag == tag[0])
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
		printf("key not exist\n");
		return STATE_AVAILIBLE;		
	}

	return STATE_OCCUPIED;	
}

int main(int argc, char** argv)
{
	return 0;
}
