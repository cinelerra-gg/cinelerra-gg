#include "../libzmpeg3.h"

zcache_t::
~cache_t()
{
  clear();
}

void zcache_t::
clear()
{
  if( frames ) {
    for( int i=0; i<allocation; ++i ) {
      cacheframe_t *frame = &frames[i];
      if( frame->y ) delete [] frame->y;
      if( frame->u ) delete [] frame->u;
      if( frame->v ) delete [] frame->v;
    }
    delete [] frames;
    total = 0;
    allocation = 0;
    frames = 0;
    seq = 0;
  }
}

int zcache_t::
extend_cache()
{
  if( total >= MAX_CACHE_FRAMES ) {
    int i = 0, n = 0;
    uint32_t vtim = frames[0].age;
    while( ++i < total ) {
      if( frames[i].age < vtim ) {
        n = i;  vtim = frames[i].age;
      }
    }
    frames[n].age = seq++;
    return n;
  }
  if( total >= allocation ) {
    int new_allocation = ZMAX(allocation*2,8);
    cacheframe_t *new_frames = new cacheframe_t[new_allocation];
    for( int i=0; i<total; ++i )
      new_frames[i] = frames[i];
    delete [] frames;
    frames = new_frames;
    allocation = new_allocation;
//zmsgs("%d %d %d\n", new_allocation, allocation, total);
  }
  frames[total].age = seq++;
  return total++;
}

void zcache_t::
put_frame(int64_t zframe_number,
  uint8_t *zy, uint8_t *zu, uint8_t *zv,
  int zy_size, int zu_size, int zv_size)
{
  cacheframe_t *frame = 0;
  int i;
//zmsgs("save %d\n",zframe_number);

  for( i=0; i<total; ++i ) { /* Get existing frame */
    if( frames[i].frame_number == zframe_number ) {
      frame = &frames[i];
      break;
    }
  }

  if( !frame ) {
    int i = extend_cache();
    frame = &frames[i];
//zmsgs("30 %d %p %p %p\n", ptr->total, frame->y, frame->u, frame->v);
    if( zy ) {
      if( zy_size > frame->y_alloc ) {
        delete [] frame->y;
        frame->y = new uint8_t[frame->y_alloc=zy_size];
      }
      memcpy(frame->y, zy, frame->y_size=zy_size);
    }
    if( zu ) {
      if( zu_size > frame->u_alloc ) {
        delete [] frame->u;
        frame->u = new uint8_t[frame->u_alloc=zu_size];
      }
      memcpy(frame->u, zu, frame->u_size=zu_size);
    }
    if( zv ) {
      if( zv_size > frame->v_alloc ) {
        delete [] frame->v;
        frame->v = new uint8_t[frame->v_alloc=zv_size];
      }
      memcpy(frame->v, zv, frame->v_size=zv_size);
    }
    frame->frame_number = zframe_number;
  }
}

int zcache_t::
get_frame(int64_t zframe_number,
  uint8_t **zy, uint8_t **zu, uint8_t **zv)
{
  for( int i=0; i<total; ++i ) {
    cacheframe_t *frame = &frames[i];
    if( frame->frame_number == zframe_number ) {
      frame->age = seq++;
      *zy = frame->y;
      *zu = frame->u;
      *zv = frame->v;
//zmsgs("hit %d\n",zframe_number);
      return 1;
    }
  }
  clear();
//zmsgs("missed %d\n",zframe_number);
  return 0;
}

int zcache_t::
has_frame(int64_t zframe_number)
{
  for( int i=0; i<total; ++i )
    if( frames[i].frame_number == zframe_number ) return 1;

  return 0;
}

int64_t zcache_t::
memory_usage()
{
  int64_t result = 0;
  for( int i=0; i<allocation; ++i ) {
    cacheframe_t *frame = &frames[i];
    result += frame->y_alloc + frame->u_alloc + frame->v_alloc;
  }
  return result;
}

