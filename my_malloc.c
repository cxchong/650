#include "my_malloc.h"
#include <assert.h>
#include <stdio.h>

unsigned long heapsize = 0;
blk_t *first = NULL;

__thread blk_t *TLS_first = NULL;

blk_t *ini(blk_t *curr, size_t size) {

  size_t sz = size + sizeof(blk_t);

  blk_t *temp = NULL;
  if ((temp = sbrk(sz)) != (void *)-1) {
    heapsize += sz;
    temp->next = NULL;
    temp->prev = curr;
    temp->blksize = sz;

    if (first == NULL) {
      first = temp;
    }
    return temp;
  } else {
    return NULL;
  }
}

blk_t *ini_nolock(blk_t *curr, size_t size) {

  size_t sz = size + sizeof(blk_t);

  blk_t *temp = NULL;
  pthread_mutex_lock(&sbrk_lock);
  if ((temp = sbrk(sz)) != (void *)-1) {
    pthread_mutex_unlock(&sbrk_lock);
    heapsize += sz;
    temp->next = NULL;
    temp->prev = curr;
    temp->blksize = sz;

    if (TLS_first == NULL) {
      TLS_first = temp;
    }
    return temp;
  } else {
    pthread_mutex_unlock(&sbrk_lock);
    return NULL;
  }
}

void *split_block(blk_t *curr, size_t size) {
  char *temp = (char *)curr + size + sizeof(blk_t);
  blk_t *new = (blk_t *)temp;
  new->next = curr->next;
  new->prev = curr->prev;
  new->blksize = curr->blksize - size - sizeof(blk_t);
  if (new->prev == NULL) {
    first = new;
  }
  if (curr->next) {
    curr->next->prev = new;
  }
  if (curr->prev) {
    curr->prev->next = new;
  }
  curr->blksize = size + sizeof(blk_t);
  return curr + 1;
}

void *split_block_nolock(blk_t *curr, size_t size) {
  char *temp = (char *)curr + size + sizeof(blk_t);
  blk_t *new = (blk_t *)temp;
  new->next = curr->next;
  new->prev = curr->prev;
  new->blksize = curr->blksize - size - sizeof(blk_t);
  if (new->prev == NULL) {
    TLS_first = new;
  }
  if (curr->next) {
    curr->next->prev = new;
  }
  if (curr->prev) {
    curr->prev->next = new;
  }
  curr->blksize = size + sizeof(blk_t);
  return curr + 1;
}

void *ts_malloc_lock(size_t size) {
  pthread_mutex_lock(&lock);
  if (first == NULL) {
    first = ini(first, size);
  }
  blk_t *curr = first;
  while (curr != NULL) {
    if (curr->blksize >= size + sizeof(blk_t)) {
      blk_t *mark = curr;
      while (curr != NULL) {

        // assert(curr!=0x0);
        // printf("%p\n",curr);
        if ((curr->blksize < mark->blksize) &&
            (curr->blksize >= size + sizeof(blk_t))) {
          mark = curr;
        }
        curr = curr->next;
      }
      if (mark->blksize <= size + 2 * sizeof(blk_t)) {
        if (mark == first) {
          first = mark->next;
        }
        if (mark->prev) {
          mark->prev->next = mark->next;
        }
        if (mark->next) {
          mark->next->prev = mark->prev;
        }
        pthread_mutex_unlock(&lock);
        return mark + 1;
      }
      mark = split_block(mark, size);
      pthread_mutex_unlock(&lock);
      return mark;
    } else {
      if (curr->next == NULL) {
        curr->next = ini(curr, size);
        curr = curr->next;
      } else {
        curr = curr->next;
      }
    }
  }
  pthread_mutex_unlock(&lock);
  return NULL;
}

void *ts_malloc_nolock(size_t size) {

  if (TLS_first == NULL) {
    TLS_first = ini_nolock(TLS_first, size);
  }
  blk_t *curr = TLS_first;
  while (curr != NULL) {
    if (curr->blksize >= size + sizeof(blk_t)) {
      blk_t *mark = curr;
      while (curr != NULL) {

        // assert(curr!=0x0);
        // printf("%p\n",curr);
        if ((curr->blksize < mark->blksize) &&
            (curr->blksize >= size + sizeof(blk_t))) {
          mark = curr;
        }
        curr = curr->next;
      }
      if (mark->blksize <= size + 2 * sizeof(blk_t)) {
        if (mark == TLS_first) {
          TLS_first = mark->next;
        }
        if (mark->prev) {
          mark->prev->next = mark->next;
        }
        if (mark->next) {
          mark->next->prev = mark->prev;
        }
        pthread_mutex_unlock(&lock);
        return mark + 1;
      }
      mark = split_block_nolock(mark, size);

      return mark;
    } else {
      if (curr->next == NULL) {
        curr->next = ini_nolock(curr, size);
        curr = curr->next;
      } else {
        curr = curr->next;
      }
    }
  }

  return NULL;
}

blk_t *get_block(void *p) {
  char *temp;
  temp = p;
  if (temp) {
    temp -= sizeof(blk_t);
    // the front of data part
    p = temp;
    return p;
  } else {
    return NULL;
  }
}

blk_t *findlast() {
  blk_t *curr = first;
  while (curr != NULL) {
    if (curr->next == NULL) {
      break;
    }
    curr = curr->next;
  }
  return curr;
}

blk_t *findlast_nolock() {
  blk_t *curr = TLS_first;
  //  if (curr->next == NULL) {
  //    return curr;
  //  }
  // blk_t *temp= curr->prev;
  while (curr != NULL) {
    if (curr->next == NULL) {
      break;
    }
    curr = curr->next;
  }
  return curr;
}

void general_free(void *ptr) {
  pthread_mutex_lock(&lock);
  blk_t *b;
  b = get_block(ptr);
  blk_t *curr = first; // wrong!!!!!!!!!!
  int i = 0;
  // char *temp = (char *)curr1 + curr1->blksize;
  // blk_t *t2 = (blk_t *)temp;
  if (curr != NULL) {
    blk_t *last = findlast();
    while (curr != NULL) {
      if (curr > b) {
        b->next = curr;
        b->prev = curr->prev;
        if (curr->prev) {
          curr->prev->next = b;
        }
        curr->prev = b;
        if (b->prev == NULL) {
          first = b;
        }
        i = 1;
        break;
      }
      //      b->prev = curr;
      //      b->next = curr->next;
      //      if (curr->next) {
      //        curr->next->prev = b;
      //      }
      // break;
      //}
      curr = curr->next;
    }
    if (i == 0) {
      b->prev = last;
      b->next = last->next;
      if (last->next) {
        last->next->prev = b;
      }
    }
    blk_t *t4 = NULL;
    blk_t *t6 = NULL;
    if (b->prev) {
      char *t3 = (char *)b->prev + b->prev->blksize;
      t4 = (blk_t *)t3;
    }
    if (b->next) {
      char *t5 = (char *)b->next - b->blksize;
      t6 = (blk_t *)t5;
    }
    if (b == t4) {
      b = fusion1(b);
    }
    if (b == t6) {
      b = fusion2(b);
    }
    if (b->prev == NULL) {
      first = b;
    }
  } else {
    //    size_t sz = b->blksize - sizeof(blk_t);
    //    first = ini(b, sz);//!!!!!!!wrong!!

    first = b;
    b->prev = NULL;
    b->next = NULL;
  }
  pthread_mutex_unlock(&lock);
}

void general_free_nolock(void *ptr) {
  blk_t *b;
  b = get_block(ptr);
  blk_t *curr = TLS_first; // wrong!!!!!!!!!!
  int i = 0;
  // char *temp = (char *)curr1 + curr1->blksize;
  // blk_t *t2 = (blk_t *)temp;
  if (curr != NULL) {
    blk_t *last = findlast_nolock();
    while (curr != NULL) {
      if (curr > b) {
        b->next = curr;
        b->prev = curr->prev;
        if (curr->prev) {
          curr->prev->next = b;
        }
        curr->prev = b;
        if (b->prev == NULL) {
          TLS_first = b;
        }
        i = 1;
        break;
      }
      curr = curr->next;
    }
    if (i == 0) {
      b->prev = last;
      b->next = last->next;
      if (last->next) {
        last->next->prev = b;
      }
    }
    blk_t *t4 = NULL;
    blk_t *t6 = NULL;
    if (b->prev) {
      char *t3 = (char *)b->prev + b->prev->blksize;
      t4 = (blk_t *)t3;
    }
    if (b->next) {
      char *t5 = (char *)b->next - b->blksize;
      t6 = (blk_t *)t5;
    }
    if (b == t4) {
      b = fusion1_nolock(b);
    }
    if (b == t6) {
      b = fusion2_nolock(b);
    }
    if (b->prev == NULL) {
      TLS_first = b;
    }
  } else {
    //    size_t sz = b->blksize - sizeof(blk_t);
    //    first = ini(b, sz);//!!!!!!!wrong!!

    TLS_first = b;
    b->prev = NULL;
    b->next = NULL;
  }
}

blk_t *fusion1(blk_t *b) {
  if (b->next) {
    b->next->prev = b->prev;
  }
  b->prev->next = b->next;
  b->prev->blksize += b->blksize;
  return b->prev;
}

blk_t *fusion2(blk_t *b) {
  b->blksize += b->next->blksize;
  b->next = b->next->next;
  if (b->next) {
    b->next->prev = b;
  }
  return b;
}

blk_t *fusion1_nolock(blk_t *b) {

  if (b->next) {
    b->next->prev = b->prev;
  }
  b->prev->next = b->next;
  b->prev->blksize += b->blksize;
  return b->prev;

  return b;
}

blk_t *fusion2_nolock(blk_t *b) {

  b->blksize += b->next->blksize;
  b->next = b->next->next;
  if (b->next) {
    b->next->prev = b;
  }
  return b;

  return b;
}

void ts_free_lock(void *ptr) { general_free(ptr); }
void ts_free_nolock(void *ptr) { general_free_nolock(ptr); }
