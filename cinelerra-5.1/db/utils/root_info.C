#include<stdio.h>
#include<stdint.h>
#include <sys/ipc.h>
#include <sys/shm.h>

struct info {
  int magic;
  int info_key, info_id;
  int root_info_id;
  int index_info_id;
  int page_info_id;
  int64_t oldness, newness;
};

int main(int ac, char **av)
{
  void *vp;
  struct info *ip;
  int id = shmget(34543, 30792, 0666);
  if( id < 0 ) return 1;
  vp = (void *) shmat(id, NULL, 0);
  if( vp == (void*)-1 ) return 2;
  ip = (struct info *)vp;
  printf(" magic = %08x\n",ip->magic);
  printf(" info_key = %d\n",ip->info_key);
  printf(" info_id = %d\n",ip->info_id);
  printf(" root_info_id = %d\n",ip->root_info_id);
  printf(" index_info_id = %d\n",ip->index_info_id);
  printf(" page_info_id = %d\n",ip->page_info_id);
  printf(" oldness = %jd\n",ip->oldness);
  printf(" newness = %jd\n",ip->newness);
  return 0;
}

