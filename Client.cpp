#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <string>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "support.h"
#include "Client.h"
#include<iostream>

void help(char *progname)
{
	printf("Usage: %s [OPTIONS]\n", progname);
	printf("Perform a PUT or a GET from a network file server\n");
	printf("  -P    PUT file indicated by parameter\n");
	printf("  -G    GET file indicated by parameter\n");
	printf("  -s    server info (IP or hostname)\n");
	printf("  -p    port on which to contact server\n");
	printf("  -S    for GETs, name to use when saving file locally\n");
}

void die(const char *msg1, const char *msg2)
{
	fprintf(stderr, "%s, %s\n", msg1, msg2);
	exit(0);
}

/*
 * connect_to_server() - open a connection to the server specified by the
 *                       parameters
 */
int connect_to_server(char *server, int port)
{
	int clientfd;
	struct hostent *hp;
	struct sockaddr_in serveraddr;
	char errbuf[256];                                   /* for errors */

	/* create a socket */
	if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		die("Error creating socket: ", strerror(errno));
	}

	/* Fill in the server's IP address and port */
	if((hp = gethostbyname(server)) == NULL)
	{
		sprintf(errbuf, "%d", h_errno);
		die("DNS error: DNS error ", errbuf);
	}
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)hp->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
	serveraddr.sin_port = htons(port);

	/* connect */
	if(connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
	{
		die("Error connecting: ", strerror(errno));
	}
	return clientfd;
}


/*
 * put_file() - send a file to the server accessible via the given socket fd
 */
void put_file(int fd, char *put_name)
{
	/* TODO: implement a proper solution, instead of calling the echo() client */
	const int MAXLINE = 8192;
	
		FILE* pfile = fopen(put_name,"r");
		if(pfile==NULL){
			die("read error: ",strerror(errno));
		}
		fseek(pfile, 0L, SEEK_END);
		size_t sz = ftell(pfile);
		rewind(pfile);			
		char* file = (char*)malloc(sz);
		bzero(file,sz);
		fread(file,sz,1,pfile);
		fclose(pfile);
		std::stringstream ss;
		//printf("file %s",file);		
		//md5
		unsigned char digest[MD5_DIGEST_LENGTH];	
		MD5((unsigned char*)file, strlen(file), (unsigned char*)&digest);  
		char mdString[33]; 
		for(int i = 0; i < 16; i++)
			 sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
		ss<<"PUT "<<put_name<<"\n"<<sz<<"\n"<<"PUTC "<<mdString<<"\n"<<file<<"\n";
		std::string sss = ss.str();
		const char* source = sss.c_str();	
		char* buf = (char*)malloc(strlen(source));
		strcpy(buf,source);
		//printf("%s",buf);
		//send file	
		size_t nremain = strlen(buf);
		ssize_t nsofar;
		char *bufp = buf;	
		while(nremain > 0)
		{
			if((nsofar = write(fd, bufp, nremain)) <= 0)
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
		buf = (char*)malloc(MAXLINE);
		bufp = buf;
		bzero(bufp,strlen(bufp));
		nremain = MAXLINE;
		while(1){
			if((nsofar = read(fd, bufp, nremain)) < 0)
			{
				if(errno != EINTR)
				{
					die("read error: ", strerror(errno));
				}
				continue;
			}	
			nremain -= nsofar;
			bufp += nsofar;
			if(*(bufp-1) == '\n')
			{	
				*bufp = 0;					
				break;
			}		
		}
		printf("%s", buf);
			
			
		
	
	//read file
}

/*
 * get_file() - get a file from the server accessible via the given socket
 *              fd, and save it according to the save_name
 */
void get_file(int fd, char *get_name, char *save_name)
{
	/* TODO: implement a proper solution, instead of calling the echo() client */
	//echo_client(fd);
	const int MAXLINE = 8192;
	std::stringstream ss;
	ss<<"GET "<<get_name<<"\n";
	std::string sss = ss.str();
	const char* source = sss.c_str();
	char* buf = (char*)malloc(strlen(source));
	strcpy(buf,source);
	
	//send file	
	size_t nremain = strlen(buf);
	ssize_t nsofar;
	char *bufp = buf;	
	while(nremain > 0)
	{
		if((nsofar = write(fd, bufp, nremain)) <= 0)
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
	//printf("%i",nremain);
	//fflush;
	buf = (char*)malloc(MAXLINE);
	bufp = buf;
	bzero(bufp,strlen(bufp));
	nremain = MAXLINE;
	while(1){
		if((nsofar = read(fd, bufp, nremain)) < 0)
		{
			if(errno != EINTR)
			{
				die("read error: ", strerror(errno));
			}
			continue;
		}
		nremain -= nsofar;
		bufp += nsofar;
		if(*(bufp-1) == '\n')
		{	
			*(bufp-1) = 0;
			*bufp = 0;					
			break;
		}			
	}
	printf("%s",buf);
	char * firstLine = strtok(buf,"\n");	
	char * size = strtok(NULL,"\n");					
	char * thirdLine = strtok(NULL,"\n");	
	char * status = strtok(firstLine," ");	
	if(!strcmp(status,"OK")){
		char* filename = firstLine + 3;
		char* checkSum = thirdLine + 4;
		char* file = thirdLine + strlen(thirdLine) + 1;
		//std::cout<<"filename "<<filename<<"\ncheckSum "<<checkSum<<"\nfile "<<file;
		unsigned char digest[MD5_DIGEST_LENGTH];
		MD5((unsigned char*)file, strlen(file), (unsigned char*)&digest);  
		char mdString[33]; 
		for(int i = 0; i < 16; i++)
			 sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
		//printf("file %s\n",file);	
		if(!strcmp(checkSum,mdString)){
			printf("checked\n");
		}

	}
	
	
}

/*
 * main() - parse command line, open a socket, transfer a file
 */
int main(int argc, char **argv)
{
	/* for getopt */
	long  opt;
	char *server = NULL;
	char *put_name = NULL;
	char *get_name = NULL;
	int   port;
	char *save_name = NULL;

	check_team(argv[0]);

	/* parse the command-line options. */
	while((opt = getopt(argc, argv, "hs:P:G:S:p:")) != -1)
	{
		switch(opt)
		{
		case 'h': help(argv[0]); break;
		case 's': server = optarg; break;
		case 'P': put_name = optarg; break;
		case 'G': get_name = optarg; break;
		case 'S': save_name = optarg; break;
		case 'p': port = atoi(optarg); break;
		}
	}

	/* open a connection to the server */
	int fd = connect_to_server(server, port);

	/* put or get, as appropriate */
	if(put_name)
	{
		put_file(fd, put_name);
	}
	else
	{
		get_file(fd, get_name, save_name);
	}

	/* close the socket */
	int rc;
	if((rc = close(fd)) < 0)
	{
		die("Close error: ", strerror(errno));
	}
	exit(0);
}
