#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

typedef struct link{
	char * arg;
	struct link *next;
}link;

int netopen(char * path, int mode);
ssize_t netread(int fd,const void *buffer, size_t bytes);
ssize_t netwrite(int fd, const void *buffer,size_t bytes);
int netclose(int fd);
int intLen(int x);
int getSocketFD();
char * pullString(int start,int end,int size, char * og);
link * create(char * arg);
link * add(link * head, link * new);
void freeList(link * head);
link argPull(char * buffer, link * head);
int netserverinit(char * hostname);
int openbytes(link *head, int fd);
int closebytes(link *head, int fd);
int readbytes(link *head, int fd);
int writebytes(char * buffer, int fd);
link * readPull(char * buffer, link *head);
link * writePull(char * buffer, link *head);
void *threadControl(int * args);
int p_thread_mutex_lock(pthread_mutex_t *mutex);
int p_thread_mutex_unlock(pthread_mutex_t *mutex);
int errorCheck(int e);
int pullSize(char * buffer);
