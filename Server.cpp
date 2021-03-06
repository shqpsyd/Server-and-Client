#include <arpa/inet.h>
#include <errno.h>
#include <climits>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include "support.h"
#include "Server.h"
#include <vector>
#include <map>
#include <time.h>
#include <pthread.h>
#include "ThreadPool.h"
#define MaxClient 1024
#define ThreadsNum 4


void help(char *progname)
{
	printf("Usage: %s [OPTIONS]\n", progname);
	printf("Initiate a network file server\n");
	printf("  -m    enable multithreading mode\n");
	printf("  -l    number of entries in the LRU cache\n");
	printf("  -p    port on which to listen for connections\n");
}

void die(const char *msg1, char *msg2)
{
	fprintf(stderr, "%s, %s\n", msg1, msg2);
	exit(0);
}

/*
 * open_server_socket() - Open a listening socket and return its file
 *                        descriptor, or terminate the program
 */

int open_server_socket(int port)
{
	int                listenfd;    /* the server's listening file descriptor */
	struct sockaddr_in addrs;       /* describes which clients we'll accept */
	int                optval = 1;  /* for configuring the socket */

	/* Create a socket descriptor */
	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		die("Error creating socket: ", strerror(errno));
	}

	/* Eliminates "Address already in use" error from bind. */
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0)
	{
		die("Error configuring socket: ", strerror(errno));
	}

	/* Listenfd will be an endpoint for all requests to the port from any IP
	   address */
	bzero((char *) &addrs, sizeof(addrs));
	addrs.sin_family = AF_INET;
	addrs.sin_addr.s_addr = htonl(INADDR_ANY);
	addrs.sin_port = htons((unsigned short)port);
	if(bind(listenfd, (struct sockaddr *)&addrs, sizeof(addrs)) < 0)
	{
		die("Error in bind(): ", strerror(errno));
	}

	/* Make it a listening socket ready to accept connection requests */
	if(listen(listenfd, 1024) < 0)  // backlog of 1024
	{
		die("Error in listen(): ", strerror(errno));
	}

	return listenfd;
}

/*
 * handle_requests() - given a listening file descriptor, continually wait
 *                     for a request to come in, and when it arrives, pass it
 *                     to service_function.  Note that this is not a
 *                     multi-threaded server.
 */
void handle_requests(int listenfd, void (*service_function)(int, int,bool), int param, bool multithread)
{
	ThreadPool pool(ThreadsNum);
	while(1)
	{
		/* block until we get a connection */
		struct sockaddr_in clientaddr;
		memset(&clientaddr, 0, sizeof(sockaddr_in));
		socklen_t clientlen = sizeof(clientaddr);
		int connfd;
		if((connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen)) < 0)
		{
			die("Error in accept(): ", strerror(errno));
		}

		/* print some info about the connection */
		struct hostent *hp;
		hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if(hp == NULL)
		{
			fprintf(stderr, "DNS error in gethostbyaddr() %d\n", h_errno);
			exit(0);
		}
		char *haddrp = inet_ntoa(clientaddr.sin_addr);
		printf("server connected to %s (%s)\n", hp->h_name, haddrp);
		if(!multithread){
			/* serve requests */
			service_function(connfd, param,multithread);
			/* clean up, await new connection */
		}else{
			pool.enqueue(service_function,connfd, param,multithread);
		}
	}
	
}

/*
 * file_server() - Read a request from a socket, satisfy the request, and
 *                 then close the connection.
 */

 struct Node
 {
	unsigned char* file;
	time_t count;
	size_t size;
	Node(unsigned char* f, time_t t,size_t s):count(t),file(f),size(s)
	{
		
	}
 };
 std::string findLRU(std::map<std::string, Node*> * LRU){
	std::map<std::string, Node*>::iterator it = LRU->begin();
	time_t early = it->second->count;
	std::string filename = it->first;
	while(it!=LRU->end()){
		if(difftime(it->second->count,early)<0){
			early = it->second->count;
			filename = it->first;
		}		
		it++;
	}
	return filename;
}
void file_server(int connfd, int lru_size,bool multithread)
{

	/* TODO: set up a few static variables here to manage the LRU cache of
	   files */
	//std::cout<<"lru_size "<<lru_size<<"\n";
	static pthread_rwlock_t fileMap_lock = PTHREAD_RWLOCK_INITIALIZER;
	static std::map<std::string,pthread_rwlock_t> fileMap;
	static pthread_rwlock_t lru_lock = PTHREAD_RWLOCK_INITIALIZER;
	static std::map<std::string, Node*> LRU;
			

	/* TODO: replace following sample code with code that satisfies the
	   requirements of the assignment */

	/* sample code: continually read lines from the client, and send them
	   back to the client immediately */
	
	   //usleep(30000000);
	
		const int MAXLINE = 100;
		void* buf = malloc(0) ;   /* a place to store text from the client */
		


		/* read from socket, recognizing that we may get short counts */
		void *bufp = malloc(MAXLINE);              /* current pointer into buffer */
		size_t nsofar;				 /* characters read so far */
		size_t recvSize = 0;
		for (int i = 0;1;i++)
		{
			/* read some data; swallow EINTRs */
			if((nsofar = read(connfd, bufp, MAXLINE)) > 0)
			{
				//std::cout<<(char*)(bufp+nsofar-3)<<"\n";
				recvSize+=nsofar;					
					buf = realloc(buf,MAXLINE*i + nsofar);					
					//if(*(char*)(bufp+nsofar-1)=='\0'&&*(char*)(bufp+nsofar-2)=='\n'&&*(char*)(bufp+nsofar-3)=='\0'){
					if(!strcmp((char*)(bufp+nsofar-3),"end")){
						//*(char*)(bufp+nsofar-1) = 0;
						bzero(bufp+nsofar-3,3);
						memcpy(buf+i*MAXLINE,bufp,nsofar);

						break;
					}
					memcpy(buf+i*MAXLINE,bufp,nsofar);
					bzero(bufp,MAXLINE);												
			}
			
			/* end service to this client on EOF */
			else if(nsofar == 0)
			{
				fprintf(stderr, "received EOF\n");
				break;
			}else {
				if(errno != EINTR)
				{
					die("read error: ", strerror(errno));					
				}
				continue;
			}
			
			/* update pointer for next bit of reading */
		}
		printf("server received %d bytes\n", recvSize);
		//printf("%s",buf);		
		char * firstLine = strtok((char*)buf,"\n");
		char* sizetemp = strtok(NULL,"\n");				
		char * thirdLine = strtok(NULL,"\n");			
		char* operation = strtok (firstLine," ");
		printf("operation %s\n",operation);	
		char* filename = strtok (NULL," ");	
		printf("filename %s\n",filename);
		if(recvSize != 0){
		if(!strcmp(operation,"PUT")){	
			int size = atoi(sizetemp);		
			unsigned char* file = (unsigned char*)(thirdLine + strlen(thirdLine) + 1);
			char* checkSum = thirdLine + 5;
			unsigned char digest[MD5_DIGEST_LENGTH];
			MD5((unsigned char*)file, size, (unsigned char*)&digest);  
			char mdString[33]; 
			for(int i = 0; i < 16; i++)
				 sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
			//printf("file %s\n",file);	
			if(!strcmp(checkSum,mdString)){
				printf("md5 checked\n");
			}
			char* buf1 = (char*)malloc(MAXLINE);
						
			strcpy(buf1,"ok\n");
			if((nsofar = write(connfd, buf1, strlen(buf1))) <= 0)
				{
					if(errno != EINTR)
					{
						die("Write error: ", strerror(errno));
					}
				}
			std::string fn(filename);
			if(multithread){
				pthread_rwlock_wrlock(&fileMap_lock);
				if(fileMap.find(fn)==fileMap.end()){
					pthread_rwlock_t flock = PTHREAD_RWLOCK_INITIALIZER;
					fileMap[fn] = flock;								
				}
				pthread_rwlock_wrlock(&(fileMap[fn]));
				pthread_rwlock_unlock(&fileMap_lock);
				FILE* newfile = fopen(filename,"wb+");
				fwrite(file, 1,size , newfile);
				fclose(newfile);
				delete buf1;
				pthread_rwlock_unlock(&(fileMap[fn]));
			}else{
				FILE* newfile = fopen(filename,"wb+");
				fwrite(file, 1,size , newfile);
				fclose(newfile);
				delete buf1;
			}
			
			//printf("finish %i\n",nsofar);
			unsigned char* f = (unsigned char*)malloc(size);
			memcpy(f,file,size);
			
			
			//LRU
			if(lru_size != 0){
			if(multithread){
				pthread_rwlock_wrlock(&lru_lock);
			}
			
				if(LRU.size()<lru_size){			
					Node* node = new Node(f,time(NULL),size);
					LRU[fn] = node;
					std::cout<<"LRU size "<<LRU.size()<<"\n";
					std::cout<<"put "<<fn<<" into LRU\n";
					
				}else{			
					if(LRU.find(fn) == LRU.end()){
						std::string LRUfile = findLRU(&LRU);
						delete LRU[LRUfile]->file;
						LRU.erase(LRUfile);					
						Node* node = new Node(f,time(NULL),size);
						LRU[fn] = node;
						std::cout<<"remove "<<LRUfile<<" from LRU\n";
						std::cout<<"put "<<fn<<" into LRU\n";
					}else{
						Node* node = LRU[fn];
						node->count = time(NULL);
						LRU[fn] = node;
						std::cout<<"put "<<fn<<" into LRU\n";
					}
				}
			if(multithread){
				pthread_rwlock_unlock(&lru_lock);
			}				
		}				
		}
		else if(!strcmp(operation,"GET")){
			
			unsigned char* file = NULL;
			std::string fn(filename);
			size_t sz;	
			if(multithread){
				pthread_rwlock_rdlock(&lru_lock);
			}
			if(lru_size != 0 and LRU.find(fn) != LRU.end()){
				file = LRU[fn]->file;
				sz = LRU[fn]->size;
				if(multithread){
					pthread_rwlock_unlock(&lru_lock);
				}
				std::cout<<"get "<<fn<<" from LRU\n";
			}else{
				std::cout<<"get "<<fn<<" from file system\n";
				FILE* pfile = NULL;
				if(multithread){
					pthread_rwlock_rdlock(&fileMap[fn]);
				}
					pfile = fopen(filename,"rb");
					if(pfile!=NULL){
						fseek(pfile, 0L, SEEK_END);
						sz = ftell(pfile);
						rewind(pfile);			
						file = (unsigned char*)malloc(sz);
						bzero(file,sz);
						fread(file,sz,1,pfile);
						fclose(pfile);
					}
				if(multithread){
					pthread_rwlock_unlock(&fileMap[fn]);
				}	
							
				

				
			}
			
			std::stringstream ss;
			if(file!=NULL){
				unsigned char digest[MD5_DIGEST_LENGTH];
				MD5(file, sz, (unsigned char*)&digest);  
				char mdString[33]; 
				for(int i = 0; i < 16; i++)
					 sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
				//printf("md5 %send\n",mdString);		
				ss<<"OK "<<filename<<"\n"<<sz<<"\n"<<"OKC "<<mdString<<"\n";
			}		
			else{				
				ss<<"No such file\n";				
			}
			//delete file;
			std::string sss = ss.str();
			const char* head = sss.c_str();	
			//delete buf;
			void* buf1 = NULL;
			int totalSize = strlen(head);
			if(file!=NULL){				
				totalSize += sz;
				buf1 = malloc(totalSize);			
				bzero(buf1,totalSize);
				memcpy(buf1,head,strlen(head));
				memcpy(buf1+strlen(head),file,sz);
			}else{
				buf1 = malloc(totalSize);			
				bzero(buf1,strlen(head));
				memcpy(buf1,head,strlen(head));
			}
			
			//send file	
				
			//printf("write to client");			
			
			if((nsofar = write(connfd, buf1, totalSize)) <= 0)
			{
				if(errno != EINTR)
				{
						fprintf(stderr, "Write error: %s\n", strerror(errno));
						exit(0);
				}
			}
			delete buf1;
			if((nsofar = write(connfd, "end", 3)) <= 0)
			{
				if(errno != EINTR)
				{
						fprintf(stderr, "Write error: %s\n", strerror(errno));
						exit(0);
				}
			}
							
		}else {
				printf("ERROR: False operation\n");
			}
		
		}
		else{
			printf("ERROR: client closed without sending messages\n");
		}
		int closeFlag = close(connfd);
		if(closeFlag < 0)
		{
			//die("Error in close(): ", strerror(errno));
			printf("Error in close(): %s", strerror(errno));			
		}		
		//return closeFlag+2;//if successful it will return 2 if fail return 1

	
}

/*
 * main() - parse command line, create a socket, handle requests
 */
int main(int argc, char **argv)
{
	/* for getopt */
	long opt;
	int  lru_size = 10;
	int  port     = 9000;
	bool multithread = false;

	check_team(argv[0]);

	/* parse the command-line options.  They are 'p' for port number,  */
	/* and 'l' for lru cache size, 'm' for multi-threaded.  'h' is also supported. */
	while((opt = getopt(argc, argv, "hml:p:")) != -1)
	{
		switch(opt)
		{
		case 'h': help(argv[0]); break;
		case 'l': lru_size = atoi(optarg); break;
		case 'm': multithread = true;	break;
		case 'p': port = atoi(optarg); break;
		}
	}

	/* open a socket, and start handling requests */
	if(multithread==true){
		printf("enable multithread\n");		
	}else{
		printf("disable multithread\n");		
	}
	printf("lru_size = %i\n",lru_size);
	int fd = open_server_socket(port);	
	handle_requests(fd, file_server, lru_size, multithread);

	exit(0);
}
