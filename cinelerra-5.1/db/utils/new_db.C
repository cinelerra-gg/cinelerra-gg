#include <stdio.h>
#include <stdint.h>

#include "s.C"

int main(int ac, char **av)
{
	if( ac < 2 ) { printf("usage: %s <new>.db\n",av[0]);  exit(1); }
	theDb *db = new theDb();
	remove(av[1]);
	db->create(av[1]);
	delete db;
	return 0;
}

