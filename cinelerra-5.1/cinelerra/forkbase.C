
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "bcsignals.h"
#include "forkbase.h"
#include "mwindow.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

ForkBase::ForkBase()
 : Mutex("ForkBase::lock")
{
	ppid = pid = 0;
	child = 0;

	child_fd = -1;
	child_token = 0;
	child_bytes = 0;
	child_allocated = 0;
	child_data = 0;

	parent_fd = -1;
	parent_token = 0;
	parent_bytes = 0;
	parent_allocated = 0;
	parent_data = 0;
}

ForkBase::~ForkBase()
{
	delete [] child_data;
	delete [] parent_data;
	if( child_fd >= 0 ) close(child_fd);
	if( parent_fd >= 0 ) close(parent_fd);
}

// return 1 parent is running
int ForkChild::is_running()
{
	return !ppid || !kill(ppid, 0) ? 1 : 0;
}

void ForkParent::start_child()
{
	lock("ForkParent::new_child");
	int sockets[2]; // Create the process & socket pair.
	socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	child_fd = sockets[0];  parent_fd = sockets[1];
	ppid = getpid();
	pid = fork();
	if( !pid ) {    // child process
		ForkChild *child = new_fork();
		child->child_fd = child_fd;
		child->parent_fd = parent_fd;
		child->ppid = ppid;
		child->run();
		delete child;
		_exit(0);
	}
	unlock();
}

// Return -1 if the parent is dead
// Return  0 if timeout
// Return  1 if success
int ForkBase::read_timeout(int64_t usec, int fd, void *data, int bytes)
{
	fd_set rfds;
	struct timeval timeout_struct;
	int bytes_read = 0;
	uint8_t *bp = (uint8_t *)data;

	while( bytes_read < bytes ) {
		timeout_struct.tv_sec = usec / 1000000;
		timeout_struct.tv_usec = usec % 1000000;
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		int result = select(fd+1, &rfds, 0, 0, &timeout_struct);
		if( result < 0 ) perror("read_timeout select");
		if( result < 0 || !is_running() ) return -1;
		if( !result && !bytes_read ) return 0;
		int fragment = read(fd, bp + bytes_read, bytes - bytes_read);
		if( fragment < 0 ) perror("read_timeout read");
		if( fragment < 0 || !is_running() ) return -1;
		if( fragment > 0 ) bytes_read += fragment;
	}
	return 1;
}

// return 1 if child, or if parent && child is running
int ForkBase::is_running()
{
	int status = 0;
	if( pid && waitpid(pid, &status, WNOHANG) < 0 ) return 0;
	return !pid || !kill(pid, 0) ? 1 : 0;
}

int ForkBase::read_parent(int64_t usec)
{
	token_bfr_t bfr;
	int ret = read_timeout(usec, parent_fd, &bfr, sizeof(bfr));
	if( ret > 0 ) {
		parent_token = bfr.token;
		parent_bytes = bfr.bytes;
		if( parent_bytes && parent_allocated < parent_bytes ) {
			delete [] parent_data;
			parent_data = new uint8_t[parent_allocated = parent_bytes];
		}
		if( parent_bytes ) {
			ret = read_timeout(1000000, parent_fd, parent_data, parent_bytes);
			if( !ret ) {
				printf("read_parent timeout: %d\n", parent_fd);
				ret = -1;
			}
		}
	}
	return ret;
}

int ForkBase::read_child(int64_t usec)
{
	token_bfr_t bfr;
	int ret = read_timeout(usec, child_fd, &bfr, sizeof(bfr));
	if( ret > 0 ) {
		child_token = bfr.token;
		child_bytes = bfr.bytes;
		if( child_bytes && child_allocated < child_bytes ) {
			delete [] child_data;
			child_data = new uint8_t[child_allocated = child_bytes];
		}
		if( child_bytes ) {
			ret = read_timeout(1000000, child_fd, child_data, child_bytes);
			if( !ret ) {
				printf("read_child timeout: %d\n", child_fd);
				ret = -1;
			}
		}
	}
	return ret;
}

void ForkBase::send_bfr(int fd, const void *bfr, int len)
{
	int ret = 0;
	for( int retries=10; --retries>=0 && (ret=write(fd, bfr, len)) < 0; ) {
		printf("send_bfr socket(%d) write error: %d/%d bytes\n%m\n", fd,ret,len);
		usleep(100000);
	}
	if( ret < len )
		printf("send_bfr socket(%d) write short: %d/%d bytes\n%m\n", fd,ret,len);
}

int ForkBase::send_parent(int64_t token, const void *data, int bytes)
{
	lock("ForkBase::send_parent");
	token_bfr_t bfr;  memset(&bfr, 0, sizeof(bfr));
	bfr.token = token;  bfr.bytes = bytes;
	send_bfr(child_fd, &bfr, sizeof(bfr));
	if( data && bytes ) send_bfr(child_fd, data, bytes);
	unlock();
	return 0;
}

int ForkBase::send_child(int64_t token, const void *data, int bytes)
{
	lock("ForkBase::send_child");
	token_bfr_t bfr;  memset(&bfr, 0, sizeof(bfr));
	bfr.token = token;  bfr.bytes = bytes;
	send_bfr(parent_fd, &bfr, sizeof(bfr));
	if( data && bytes ) send_bfr(parent_fd, data, bytes);
	unlock();
	return 0;
}

ForkChild::ForkChild()
{
	parent_done = 0;
}

ForkChild::~ForkChild()
{
}

ForkParent::ForkParent()
 : Thread(1, 0, 0)
{
	parent_done = -1;
}

ForkParent::~ForkParent()
{
}

// return 1 child is running
int ForkParent::is_running()
{
	int status = 0;
	if( waitpid(pid, &status, WNOHANG) < 0 ) return 0;
	return !kill(pid, 0) ? 1 : 0;
}

// Return -1,0,1 if dead,timeout,success
int ForkParent::parent_iteration()
{
	int ret = read_parent(100);
	if( !ret ) return 0;
	if( ret < 0 ) parent_token = EXIT_CODE;
	return handle_parent();
}

int ForkParent::handle_parent()
{
	printf("ForkParent::handle_parent %d\n", __LINE__);
	return 0;
}

void ForkParent::start()
{
	parent_done = 0;
	Thread::start();
}

void ForkParent::stop()
{
	if( is_running() ) {
		send_child(EXIT_CODE, 0, 0);
		int retry = 10;
		while( --retry>=0 && is_running() ) usleep(100000);
		if( retry < 0 ) kill(pid, SIGKILL);
	}
	join();
}

void ForkParent::run()
{
	while( !parent_done && parent_iteration() >= 0 );
	parent_done = 1;
}

