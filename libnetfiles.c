#include "libnetfiles.h"
//HAVE TO UPDATE PORT NUMBER
static const int portNum = 11111;
const struct hostent *sIP_addr;
struct sockaddr_in sAddr_Info;

int netserverinit(char * hostname){
	//Setting up the connection
	sIP_addr = gethostbyname(hostname);
	
	//If the host doesn't exist, set errno
	if(sIP_addr==NULL){
		h_errno=HOST_NOT_FOUND;
		fprintf(stderr, "Can't find host\n");
		return -1;
	}
	bzero((char*)&sAddr_Info,sizeof(sAddr_Info));
	sAddr_Info.sin_family=AF_INET;
	sAddr_Info.sin_port=htons(portNum);
	bcopy((char*)sIP_addr->h_addr,(char*)&sAddr_Info.sin_addr.s_addr,sIP_addr->h_length);
	//Everything is good 
	return 0;

}

int netopen(char * path, int mode){
	//Set up the message 
	char buffer[256];
	int socketFD = getSocketFD();
	int bytes;
	node * head=NULL;
	int error, fd;
		

	//Error check for modes lel
	if((mode!=O_RDONLY && mode != O_WRONLY) && mode != O_RDWR){
		fprintf(stderr,"Error: Trying to open file with a unsupported RW mode\n");
		bytes=write(socketFD, "Error,BadIO,",16);
		return -1;
	}
	
	//Building the message 
	bzero(buffer,256);
	sprintf(buffer,"open,%s,%d,",path,mode);
	
	//Send message
	bytes=write(socketFD,buffer,strlen(buffer));
	
	
	if(bytes<0){
		//This is just to error check 
		fprintf(stderr,"Couldn't write to socket\n");
		return -1;
	}
	//Read from socket 	
	bzero(buffer,256);
	bytes=read(socketFD,buffer,255);
	
	head=argPull(buffer,head); //Parses the message and stores it into a linked list
	
	if(bytes<0){
		fprintf(stderr,"couldn't read from socket\n");
		return -1;
	}	

	err=atoi(head->arg);
	errno=err;
	errorCheck(err);
	
	if(errorCheck(err)==0) return -1;
	
	fd=atoi(head->next->arg);
	freeList(head);
	return fd;
	
}


int netclose(int fd){
	//Set up message for sending
	char buffer[256];
	int socketFD = getSocketFD();
	int bytes;
	node * head = NULL;
	int result;
	int err;	
	errno=0;
	
	//Construct the message 
	bzero(buffer[256]);
	sprintf(buffer,"close,%d,",fd);
	
	//Send message
	bytes = write(socketFD, buffer, strlen(buffer));
	
	if(bytes<0){ //Error check 
 		fprintf(stderr,"Couldn't write to socket,\n");
		return -1;
	}

	//Read from return socket
	bzero(buffer,256);
	bytes=read(socketFD,buffer,255);
	
	//Error check this stuff 
	if(bytes<0){
		fprintf(stderr,"couldn't read from socket,\n");
		return -1;
	}

	//Parse the message 
	head=argPull(buffer,head);
	err=atoi(head->arg);
	errno=err;
	if(err==-1) fprintf(stderr,"couldn't close file,\n");
	result=atoi(head->next->arg);
	return result;	
}

ssize_t netread(int fd, void * buffer, size_t bytes){
	int nbytes;
	int socketFD = getSocketFD();
	if(fd==-1){
		fprintf(stderr,"Error:invalid FD\n");
		nbytes=write(socketFD,"Error,bad input",5);
		return -1;
	}
	
	//Sending message and checking the errors and stuff
	char buf[1024];
	int err;
	char * read_buff;
	node * head = NULL;
	size_t readbytes;
	
	sprintf(buf,"read.%d.%d.",fd,bytes);
	nbytes=write(socketFD,buf,strlen(buf));
	if(nbytes<0){
		fprintf(stderr,"Couldn't write to socket,\n");
		return -1;
	}
	
	//Read from socket
	bzero(buf,1000);
	nbytes=read(socketFD, buf,1000);

	//Parse return and do some error checking and stuff 
	head = readPull(buf,head);
	err=atoi(head->arg);
	if(errorCheck(err)==0) return -1;

	//Make efficient buffer and get everything out of the stream
	readbytes=atoi(head->next->arg);
	int x=readbytes+1;
	read_buff=malloc(sizeof(char)*x);
	bzero(read_buff,x);
	sprintf(read_buff,"%s",head->next->next->arg);
	while(1){
		if(stlen(read_buff)<readbytes){
			bzero(buf,1000);
			nbytes=read(socketFD,buf,1000);
			sprintf(read_buff,"%s%s",read_buff,buf);
		}else break;
	}
	sprintf(buf,"%s%s",(char*)buf,read_buff);
	
	//Do some error checking here and there

	if(nbytes<0){
		fprintf(stderr,"Couldn't read from socket\n");
		free(read_buff);
		return -1;
	}
	free(read_buff);
	return readbytes;
}

ssize_t netwrite(int fd, const void *buffer, size_t bytes){
	if(fd==-1){ //IF FD is bad 
		fprintf(stderr,"Error: bad fd\n");
		return -1;
	}
	//Set up stuff 
	size_t size=intLen(fd)+strlen((char*)buffer)+intLen(bytes)+6;
	char sBuff[size];
	int socketFD = getSocketFD();
	int x,err;
	errno=0;
	size_t writtenbytes;
	char * readBuffer;
	node * head = NULL;
	
	//Send a message and check teh errors for the send 
	sprintf(sBuff,"write,%d,%d,%s",fd,bytes,(char*)buffer);
	x=write(socketFD,sBuff,strlen(sBuff));
	if(x<0){
		fprintf(stderr,"Couldn't write to socket\n");
		return -1;	
	}
	
	//Read from socket
	bzero(sBuff,size);
	x=read(socketFD,sBuff,size);

	//Parse, set error, and then handle appropriately 
	head=argPull(sBuff,head);
	err=atoi(head->arg);
	errno=err;
	if(errorCheck(err)==0) return -1;
	
	//Get data and return 
	writtenbytes=atoi(head->next->arg);
	return writtenbytes;
}

//Gets socket FD 
int getSocketFD(){
	//Creating a socket, using AF_INET and SOCK_STREAM for streaming connection 
	//and use IP protocol
	int socketFD = socket(AF_INET,SOCK_STREAM,0);
	//Rest is just error checking
	if(socketFD<0){
		fprintf(stderr,"Couldn't make a socket\n");
		return -1;
	}
	if(connect(socketFD,(struct sockaddr *)&sAddr_Info,sizeof(sAddr_Info))<0){
		fprintf(stderr,"Couldn't connect to socket\n");
		return -1;
	}
	return socketFD;
}

//Helper for parser methods 
char * pullString(int start, int end, int size, char * og){
	int x,y;
	char * t=(char*)calloc(size+1,sizeof(char));
	for(x=0,y=start;y<end;x++,y++){
		t[x]=og[y];
	}
	return t;

}
//More helper method
int intLen(int x){
	int r=0;
	wihle(x>0){
		r++;
		x/=10;
	}
	return r;
}

//Creates a linklist 
node * create(char * arg){
	node * tmp=(node*)malloc(sizeof(node));
	tmp->arg=strdup(arg);
	tmp->next=NULL;
	return tmp;	
}

//Recursively adds new node to end of list 
node * add(node * head, node * new){
	if(head==NULL){
		head=new;
		return head;
	}else{
		head->next=add(head->next,new);
		return head;
	}
}

//Free data from the list 
void freeList(node * head){
	if(head==NULL) return;
	else{
		freeList(head->next);
		free(head->arg);
		free(head);
	}
}

node * argPull(char * buffer, node * head){
	//Setup
	char * tString;
	node * tLink;
	size_t startPos=-1, endPos=0,size=0,len=0,i=0;
	len=strlen(buffer);
	for(i=0;i<=len;i++){
		//Check if current character is not a comma or null and compensate based on that 
		if(buffer[i]==','||buffer[i]=='\0'){
			//do nothing if string is empty
			if(size==0) continue;
			else{
				endPos=i;
				tString=pullString(startPos,endPos,size,buffer);
				tLink=create(tString);
				head=add(head,tLink);
				free(tString);
				startPos=-1;
				size=0;
			}
		}else{
			if(startPos==-1){
				startPos=i;
				size++;
			}else{
				size++;
			}
		}
	}
	
	return head;

}
//Pulling data in the same way as readPull 
link * readPull(char * buffer, node * head){
	//Modified argPull, it's for specific data 
	char * tString; 
	node * tLink;
	size_t startPos=0,endPos=0,size=0,len=0,i=0;
	len=strlen(buffer);
	
	//Getting command 
	for(i=0;i<len;i++){
		if(buffer[i]==','){
			endPos=i;
			tString=pullString(startPos,endPos,size,buffer);
			tLink = create(tString);
			head=add(head,tLink);
			free(tString);
			startPos=0;
			break;
		}else{
			size++;
		}
	}

	//Getting the File Descriptor 
	for(i;i<strlen(buffer);i++){
		if(buffer[i]==','){
			if(size==0){
				continue;
			}else{
				endPos=i;
				tLink=create(tString);	
				tString=pullString(startPos,endPos,size,buffer);
				head=add(head,tLink);
				free(tString);
				startPos=endPos+1;
				size=0;
				break;
			}
		}else{
			size++;	
		
		}
	}

	//Getting the rest of the info
	endPos=strlen(buffer);
	tString=pullString(startPos,endPos,strlen(buffer)-startPos,buffer);
	tLink=create(tString);
	head=add(head,tLink);
	free(tString);
	i+=2;
	startPos=i;
	size=0;
	return head;				

}

node * writePull(char * buffer, node * head){
	//More bad functions 
	char * tString;
	node * tLink;
	size_t startPos=0,endPos=0,size=0,i=0;
	
	//Getting command out 
	for(i=0;i<strlen(buffer);i++){
		if(buffer[i]==','){
			if(size==0){
				continue;
			}else{
				endPos=i;
				tString=pullString(startPos,endPos,size,buffer);
				tLink=create(tString);
				head=add(head,tLink);
				free(tString);
				startPos=endPos+1;
				size=0;
				break;
			}
		}else{
			size++;
		}
	}

	//Getting number of bytes to read out 
	for(i;i<strlen(buffer);i++){
		if(buffer[i]==','){
			if(size==0){
				continue;
			}else{
				endPos=i;
				tString=pullString(startPos,endPos,size,buffer);
				tLink=create(tString);
				head=add(head,tLink);
				free(tString);
				startPos=endPos+1;
				size=0;
				break;
			}
		}else size++;
	}
	
	//Doing the rest 
	endPos=strlen(buffer);
	tString=pullString(startPos,endPos,strlen(buffer)-startPos,buffer);
	tLink=create(tString);
	head=add(head,tLink);
	free(tString);
	i+=2;
	startPos=i;
	size=0;
	return head;
}

int errorCheck(int err){
	int x=0;
	switch(err){
		case 0:
			x=1;
			break;
		case 1: 
			fprintf(stderr,"Error EPERM: Operation not permitted.\n");
			break;
		case 2:
			fprintf(stderr,"Error ENOENT: No such file.\n");
			break;
		case 4:
			fprintf(stderr,"Error EINTR: Interrupted system call.\n");
			break;
		case 9: 
			fprintf(stderr,"Error EBADF: Bad file descriptor.\n");
			break;
		case 13: 
			fprintf(stderr,"Error EACCES: You don't have permission to access that file.\n");
			break;
		case 21: 
			fprintf(stderr,"Error EISDIR: Given path is a directory, not a file.\n");
			break;
		case 23:
			fprintf(stderr,"Error ENFILE: File table overflow.\n");
			break;
		case 30: 	
			fprintf(stderr,"Error EROFS: Read-only file system.\n");
			break;
		case 104: 
			fprintf(stderr,"Error ECONNRESET: Connection reset by peer.\n");
			break;
		case 110: 
			fprintf(stderr,"Error ETIMEDOUT: Connection times out.\n");
			break;
		default: 
			x=0;
			break;
	}
	return x;

}





































