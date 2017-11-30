#include <arpa/inet.h>
#include <errno.h>
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
#include "support.h"
#include "Server.h"


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
void handle_requests(int listenfd, void (*service_function)(int, int), int param, bool multithread)
{
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

		/* serve requests */
		service_function(connfd, param);

		/* clean up, await new connection */
		if(close(connfd) < 0)
		{
			die("Error in close(): ", strerror(errno));
		}
	}
}

/*
 * file_server() - Read a request from a socket, satisfy the request, and
 *                 then close the connection.
 */
void file_server(int connfd, int lru_size)
{
	/* TODO: set up a few static variables here to manage the LRU cache of
	   files */

	/* TODO: replace following sample code with code that satisfies the
	   requirements of the assignment */

	/* sample code: continually read lines from the client, and send them
	   back to the client immediately */
	
	while(1){
		const int MAXLINE = 8192;
		char*      buf = (char*)malloc(MAXLINE);   /* a place to store text from the client */
		bzero(buf, MAXLINE);

		/* read from socket, recognizing that we may get short counts */
		char *bufp = buf;              /* current pointer into buffer */
		ssize_t nremain = MAXLINE;     /* max characters we can still read */
		size_t nsofar;                 /* characters read so far */
		while (1)
		{
			/* read some data; swallow EINTRs */
			if((nsofar = read(connfd, bufp, nremain)) < 0)
			{
				
				if(errno != EINTR)
				{
					die("read error: ", strerror(errno));					
				}
				continue;
			}
			/* end service to this client on EOF */
			if(nsofar == 0)
			{
				fprintf(stderr, "received EOF\n");
				return;
			}
			/* update pointer for next bit of reading */
			bufp += nsofar;
			nremain -= nsofar;
			if(*(bufp-1) == '\n')
			{
				*(bufp - 1) = 0;
				*bufp = 0;				
				break;
			}
		}
		printf("server received %d bytes\n", MAXLINE-nremain);
		printf("%s",buf);
		
		char * firstLine = strtok(buf,"\n");
		char * size = strtok(NULL,"\n");				
		char * thirdLine = strtok(NULL,"\n");			
		char* operation = strtok (firstLine," ");
		printf("operation %s\n",operation);	
		char* filename = strtok (NULL," ");	
		printf("filename %s\n",filename);	
		if(!strcmp(operation,"PUT")){			
			char* file = thirdLine + strlen(thirdLine) + 1;
			char* checkSum = thirdLine + 5;
			unsigned char digest[MD5_DIGEST_LENGTH];
			MD5((unsigned char*)file, strlen(file), (unsigned char*)&digest);  
			char mdString[33]; 
			for(int i = 0; i < 16; i++)
				 sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
			//printf("file %s\n",file);	
			if(!strcmp(checkSum,mdString)){
				printf("checked\n");
			}
			bufp = (char*)malloc(MAXLINE);
			nremain = MAXLINE;			
			strcpy(bufp,"ok\n");
			if((nsofar = write(connfd, bufp, strlen(bufp))) <= 0)
				{
					if(errno != EINTR)
					{
						die("Write error: ", strerror(errno));
					}
					nsofar = 0;
				}
			FILE* newfile = fopen(filename,"wb+");
			fwrite(file, 1, strlen(file), newfile);
			fclose(newfile);
			//printf("finish %i\n",nsofar);
		}
		else if(!strcmp(operation,"GET")){	
			
			FILE* pfile;
			pfile = fopen(filename,"r");			
			std::stringstream ss;
			if(pfile!=NULL){
				//die("read error: ",strerror(errno));
				fseek(pfile, 0L, SEEK_END);
				size_t sz = ftell(pfile);
				rewind(pfile);			
				char* file = (char*)malloc(sz);
				bzero(file,sz);
				fread(file,sz,1,pfile);
				fclose(pfile);	
				unsigned char digest[MD5_DIGEST_LENGTH];
				MD5((unsigned char*)file, strlen(file), (unsigned char*)&digest);  
				char mdString[33]; 
				for(int i = 0; i < 16; i++)
					 sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
				printf("md5 %send\n",mdString);		
				ss<<"OK \n"<<filename<<"\n"<<sz<<"\n"<<"OKC "<<mdString<<"\n"<<file<<"\n";
				
			}else{
				
				ss<<"No such file";				
			}
			std::string sss = ss.str();
			const char* source = sss.c_str();	
			buf = (char*)malloc(strlen(source));
			bzero(buf,strlen(buf));
			strcpy(buf,source);
			//send file	
			size_t nremain = strlen(buf);
			ssize_t nsofar;
			bufp = buf;	
			printf("write %s",bufp);			
			while(nremain > 0)
			{
				if((nsofar = write(connfd, bufp, nremain)) <= 0)
				{
					if(errno != EINTR)
					{
						fprintf(stderr, "Write error: %s\n", strerror(errno));
						exit(0);
					}
					nsofar = 0;
				}
				nremain -= nsofar;
				bufp += nsofar;
				
			}
		}else {
			printf("False\n");
		}		
	}	

	
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
		case 'l': lru_size = atoi(argv[0]); break;
		case 'm': multithread = true;	break;
		case 'p': port = atoi(optarg); break;
		}
	}

	/* open a socket, and start handling requests */
	int fd = open_server_socket(port);
	handle_requests(fd, file_server, lru_size, multithread);

	exit(0);
}
