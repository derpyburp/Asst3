#include "libnetfiles.h"

pthread_mutex_t mutex;

//Function that opens a file and sends FD to client 
int nopen(link * head, int socketFD){
	//Variable setup 
	int msgSize, n, err, mode, nFD;
	char * path, message; 
	
	link * tmp=head; //Pulling out args from LL 
	tmp=tmp->next; //Go to next item in the list 
	path=tmp->arg;
	tmp=tmp->next;
	mode=atoi(tmp->arg);
	
	//Opening the file 
	nFD=open(path,mode);
	err=errno;
	errno=0;
	if(nFD!=-1) nFD*=-1;
	
	//Setting up error and getting message ready
	msgSize=intLen(err)+intLen(nFD);
	message=(char*)malloc(sizeof(char)*msgSize+1);
	sprintf(message,"%d,%d,",err,nFD);
	
	//Writing to new socket and error check
	n=write(socketFD,message,strlen(message));
	if(n<0){
		fprintf(stderr,"Couldn't write to socket.\n");
		return -1;
	}
	freeList(head);
	return 0;
}

//Function that closes a file and sends a status packet
int nclose(link * head, int socketFD){
	//Variable setup
	int n,result,err;
	int fd=atoi(head->next->arg);
	char * message;
	if(fd!=-1) fd*=-1;
	
	//Closing and checking errors
	result=close(fd);
	err=errno;
	errno=0;
	
	//Getting proper size of message and assigning status
	message=(char*)malloc(sizeof(char)*(intLen(result)+intLen(err)+1));
	sprintf(message,"%d,$d,",err,result);
	n=write(socketFD,message,strlen(message)+1);
	if(n<0){
		fprintf(stderr,"Couldn't write to socket.\n");
		return -1;
	}
	freeList(head);
	return result;
}

//Function that reads from a file and sends a confirmation
int nread(link * head, int socketFD){
	//Variable setup 
	int n, err, fd=atoi(head->next->arg);
	char * message;
	
	if(fd!=-1) fd*=-1;
		
	size_t intSize=atoi(head->next->next->arg);
	int status;
	
	//Reading the file 
	char * buffer=(char*)malloc(sizeof(char)*intSize+1);
	pthread_mutex_lock(&mutex);
	status=read(fd,buffer,intSize);
	pthread_mutex_unlock(&mutex);
	
	//Pulling out an error info 
	err=errno;
	errno=0;
	if(status<0){
		fprintf(stderr,"Error reading from file\n");
		return -1;
	}

	//Making the return packet
	message=(char*)malloc(sizeof(char)*(strlen(buffer)+intLen(status)+intLen(err)+1);
	sprintf(message,"%d,%d,%s",err,status,buffer);
	n=write(socketFD,message,strlen(message));
	if(n<0){
		fprintf(stderr,"Couldn't write to socket.\n");
		return -1;
	}
	return 0;
}
//Write data to client 
int nwrite(char * buffer, int socketFD){
	//Variable setup 
	link * head=NULL;
	head=writePull(buffer,head);	
	link * tmp=head;
	tmp=tmp->next;
	int n,status,err,fd;
	size_t intSize;
	char * wbuffer,message;
	
	//Getting FD 
	fd=atoi(tmp->arg);
	if(fd!=-1) fd*=-1;
	
	//Getting number of bytes to read
	tmp=tmp->next;
	intSize=atoi(tmp->arg);
	tmp=tmp->next;
	buffer=tmp->arg;
	
	//Assemble buffer that will be used to write to file 
	wbuffer=(char*)malloc(sizeof(char)*intSize+1);
	sprintf(writebuffer,"%s%s",wbuffer,buffer);
	
	//Check the socket for any leftover info 
	while(1){
		if(strlen(wbuffer)<intSize){
			bzero(buffer,256);
			n=read(socketFD,buffer,255);
			sprintf(wbuffer,"%s%s",wbuffer,buffer);
		}else break;
	}
	
	//Reading file 
	pthread_mutex_lock(&mutex);
	status=write(fd,wbuffer,intSize);
	pthread_mutex_unlock(&mutex);

	//Error check using status 
	err=errno;
	errno=0;
	if(status<0){
		fprintf(stderr,"Error writing to file\n");
		return -1;
	}

	//Returning packet
	message=(char*)malloc(sizeof(char)*intLen(status)+intLen(err));
	sprintf(message,"%d,%d,",err,status);
	n=write(socketFD,message,strlen(message));
	if(n<0){
		fprintf(stderr,"Couldn't write to socket.\n");
		return -1;
	}
	freeList(head);
	return 0;	
}


//Takes socketFD and parses the message and will be used in main method 
void * threadRunner(int * args){
	//Setup
	int x,nFD=(int)*args;
	char buffer[256];
	bzero(buffer,256);
	x=read(nFD,buffer,255);
	if(x<0){
		fprintf(stderr,"Couldn't read from socket\n");
		pthread_exit(NULL);
	}
	
	//Parse incoming data
	link * head=NULL;
	head=argPull(buffer,head);
	link * tmp=head;
	char * command=tmp->arg;
	//Do the appropriate command
	if(strncmp("write",command,5)==0){
		freeList(head);
		nwrite(buffer,nFD);
	}else if(strncmp("open",command,4)==0){
		nopen(head,nFD);
	}else if(strncmp("read",command,4)==0){
		nread(head,nFD);
	}else if(strncmp("close",command,5)==0){
		nclose(head,nFD);
	}
	
	x=write(nFD,"Error: Can't parse incoming packet.",36);
	if(x<0){
		frpintf(stderr,"Couldn't write to socket\n");
		pthread_exit(NULL);
	}
	pthread_exit(NULL);

}
