#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "txs.h"

// create, modify, list db as set of text lines
//  ./a.out new /tmp/x.db
//  ./a.out add /tmp/x.db < /data/add.txt
//  ./a.out del /tmp/x.db < /data/del.txt
//  ./a.out get /tmp/x.db < /data/get.txt
//  ./a.out lst /tmp/x.db > /data/dat.txt

/* first, paste this
./xsch theDb txs <<eof
CREATE TABLE item (
  item_id int unsigned NOT NULL PRIMARY KEY AUTO_INCREMENT,
  data varchar(255) binary NOT NULL
);

CREATE UNIQUE INDEX item_data ON item (data);
eof
*/
// then, compile using:
// c++ -ggdb -O2 tx.C txs.C tdb.C

theDb *db;
typedef ItemLoc::ikey_Item_data ItemData;
typedef ItemLoc::rkey_Item_data DataItem;
class ItemKey : public ItemObj::t_Data {
public:
  ItemKey(char *key, int len) : ItemObj::t_Data((unsigned char *)key,len) {}
};

char line[4096];

int main(int ac, char **av)
{
  if( !strcmp(av[1],"new") ) {
    remove(av[2]);
    db = new theDb();
    db->create(av[2]);
    delete db;
    return 0;
  }

  db = new theDb();
  if( db->open(av[2]) < 0 ) exit(1);

  int64_t n =  0;

  if( !strcmp(av[1],"add") ) {
    while( fgets(line,sizeof(line),stdin) ) {
      if( !line[0] ) continue;
      int l = strlen(line);
      line[l-1] = 0;
      int ret = ItemData(db->item,ItemKey(line,l)).Find();
      if( !ret ) continue; // duplicate
      db->item.Allocate();
      db->item.Data((unsigned char *)line, l);
      db->item.Construct();
      ++n;
    }
    db->commit();
  }
  else if( !strcmp(av[1],"del") ) {
    while( fgets(line,sizeof(line),stdin) ) {
      if( !line[0] ) continue;
      int l = strlen(line);
      line[l-1] = 0;
      int ret = ItemData(db->item,ItemKey(line,l)).Find();
      if( ret ) continue; // not found
      db->item.Destruct();
      db->item.Deallocate();
      ++n;
    }
    db->commit();
  }
  else if( !strcmp(av[1],"get") ) {
    while( fgets(line,sizeof(line),stdin) ) {
      if( !line[0] ) continue;
      int l = strlen(line);
      line[l-1] = 0;
      int ret = ItemData(db->item,ItemKey(line,l)).Find();
      if( ret ) continue;
      printf("%s\n", (char*)db->item.Data());
      ++n;
    }
  }
  else if( !strcmp(av[1],"lst") ) {
    if( !DataItem(db->item).First() ) do {
      printf("%s\n", (char*)db->item.Data());
      ++n;
    } while( !DataItem(db->item).Next() );
  }
  else {
    fprintf(stderr, "unknown cmd %s\n  must be new,add,del,get,lst\n", av[1]);
    exit(1);
  }

  fprintf(stderr, "%jd items input\n", n);
  fprintf(stderr, "%d items in db\n", db->Item.Count());
  db->close();
  delete db;
  return 0;
}

