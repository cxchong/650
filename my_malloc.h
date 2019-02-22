#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sbrk_lock= PTHREAD_MUTEX_INITIALIZER;

typedef struct blk blk_t;
struct blk {
  blk_t *next;
  blk_t *prev;
  
  size_t blksize;

  //void * proof;
};

blk_t* ini(blk_t *curr,size_t size);
void *ts_malloc_lock(size_t size);
void *ts_malloc_nolock(size_t size);
void ts_free_lock(void *ptr);
void ts_free_nolock(void *ptr);
void general_free(void *ptr);
void *split_block(blk_t *curr, size_t size);
blk_t *fusion1(blk_t *b);
blk_t *fusion2(blk_t *b);
blk_t *fusion1_nolock(blk_t *b);
blk_t *fusion2_nolock(blk_t *b);
blk_t *get_block(void *p);
unsigned long get_data_segment_size();
unsigned long get_data_segment_free_space_size();


