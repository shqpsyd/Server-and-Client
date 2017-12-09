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
#include <fstream>
int padding = RSA_PKCS1_PADDING;

RSA * createRSA(unsigned char * key,int pub)
{
   RSA *rsa= NULL;
   BIO *keybio ;
   keybio = BIO_new_mem_buf(key, -1);
   if (keybio==NULL)
   {
	   printf( "Failed to create key BIO\n");
	   return 0;
   }
   if(pub)
   {
	   rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa,NULL, NULL);
   }
   else
   {
	   rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa,NULL, NULL);
   }
   if(rsa == NULL)
   {
	   printf( "Failed to create RSA\n");
   }

   return rsa;
}
int public_encrypt(unsigned char * data,int data_len,unsigned char * key, unsigned char *encrypted)
{
   RSA * rsa = createRSA(key,1);
   int result = RSA_public_encrypt(data_len,data,encrypted,rsa,padding);
   return result;
}
int private_decrypt(unsigned char * enc_data,int data_len,unsigned char * key, unsigned char *decrypted)
{
   RSA * rsa = createRSA(key,0);
   int  result = RSA_private_decrypt(data_len,enc_data,decrypted,rsa,padding);
   return result;
}

void printLastError(char *msg)
{
   char * err = (char*)malloc(130);
   ERR_load_crypto_strings();
   ERR_error_string(ERR_get_error(), err);
   printf("%s ERROR: %s\n",msg, err);
   free(err);
}
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

		//rsa encrypted
		FILE* pfile1 = NULL;
		pfile1 = fopen("public.pem","rb");
		fseek(pfile1, 0L, SEEK_END);
		size_t sz1 = ftell(pfile1);
		rewind(pfile1);
		char* publicKey = (char*)malloc(sz1);
		bzero(publicKey,sz1);
		fread(publicKey,sz1,1,pfile1);
		fclose(pfile1);	
		//printf("%s",publicKey);

		int unit = 128;
		unsigned char* encrypted = (unsigned char*)malloc(unit*2);
		unsigned char* outData = (unsigned char*)malloc(((int)(sz/unit)+1)*2*unit);
		//std::cout<<((int)(sz/unit)+1)*256<<"\n";
		bzero(outData, 2*unit);
		int encrypted_length = 0;
		char* text = (char*)malloc(unit);
		int i = 0;	
		//std::cout<<file;
		for(; i < (int)(sz/unit); i++){
			bzero(text,strlen(text));
			strncpy(text,file + i*unit,unit);
			//printf("%s",text);
			bzero(encrypted,2*unit);
			encrypted_length += public_encrypt((unsigned char*)text,strlen(text),(unsigned char*)publicKey,encrypted);		
			if(encrypted_length == -1)
			{
					printLastError("Public Encrypt failed ");
					exit(0);
			}
			memcpy(outData+2*unit*i,encrypted,2*unit);		
		}
		if(sz - i*unit > 0){
			bzero(text,strlen(text));
			
			strncpy(text,file + i*unit,sz - i*unit);
			//std::cout<<text;
			bzero(encrypted,2*unit);
			encrypted_length += public_encrypt((unsigned char*)text,strlen(text),(unsigned char*)publicKey,encrypted);
			if(encrypted_length == -1)
			{
					printLastError("Public Encrypt failed ");
					exit(0);
			}
			memcpy(outData+i*2*unit,encrypted,2*unit);
		}
		printf("Encrypted length =%d\n",encrypted_length);		
		//md5
		unsigned char digest[MD5_DIGEST_LENGTH];	
		MD5(outData, encrypted_length, (unsigned char*)&digest);  
		char mdString[33]; 
		for(int i = 0; i < 16; i++)
			 sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
		ss<<"PUT "<<put_name<<"\n"<<encrypted_length<<"\n"<<"PUTC "<<mdString<<"\n";
		std::string sss = ss.str();
		const char* head = sss.c_str();			
		size_t totalSize = strlen(head)+encrypted_length;
		void* buf = (void*)malloc(totalSize);		
		bzero(buf,totalSize);
		memcpy(buf,head,strlen(head));
		memcpy(buf+strlen(head),outData,encrypted_length);		
		//printf("%s",(char*)buf);
		//send file	
			
		int flag;
			if((flag = write(fd, buf, totalSize)) <= 0)
			{
				if(errno != EINTR)
				{
					fprintf(stderr, "Write error: %s\n", strerror(errno));
					exit(0);
				}
			}
			//char eof[1] = {std::char_traits<char>::eof()}; 
			if((flag = write(fd,"end" , 3)) <= 0)
			{
				if(errno != EINTR)
				{
					fprintf(stderr, "Write error: %s\n", strerror(errno));
					exit(0);
				}
			}	
		bzero(buf,totalSize);		
		while(1){
			if((flag= read(fd, buf, MAXLINE)) < 0)
			{
				if(errno != EINTR)
				{
					die("read error: ", strerror(errno));
				}
				continue;
			}
			printf("%s", (char*)buf);
			delete buf;
			return;
				
		}
		
			
			
		
	
}

/*
 * get_file() - get a file from the server accessible via the given socket
 *              fd, and save it according to the save_name
 */
void get_file(int fd, char *get_name, char *save_name)
{
	/* TODO: implement a proper solution, instead of calling the echo() client */
	
	const int MAXLINE = 100;
	std::stringstream ss;
	ss<<"GET "<<get_name<<"\n";
	std::string sss = ss.str();
	const char* source = sss.c_str();
	char* buf1 = (char*)malloc(strlen(source));
	strcpy(buf1,source);
	int flag;
	//send file	
	
		if((flag = write(fd, buf1, MAXLINE)) <= 0)
		{
			if(errno != EINTR)
			{
				fprintf(stderr, "Write error: %s\n", strerror(errno));
				exit(0);
			}
		}
		if((flag = write(fd,"end" , 3)) <= 0)
		{
			if(errno != EINTR)
			{
				fprintf(stderr, "Write error: %s\n", strerror(errno));
				exit(0);
			}
		}
	//printf("%i",nremain);
	//fflush;
	void* buf = malloc(0) ;   /* a place to store text from the client */
	


	/* read from socket, recognizing that we may get short counts */
	void *bufp = malloc(MAXLINE);              /* current pointer into buffer */
	size_t nsofar;				 /* characters read so far */
	size_t recvSize = 0;
	for (int i = 0;1;i++)
	{
		/* read some data; swallow EINTRs */
		if((nsofar = read(fd, bufp, MAXLINE)) > 0)
		{
			recvSize+=nsofar;					
				buf = realloc(buf,MAXLINE*i + nsofar);
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
	printf("client received %d bytes\n", recvSize);
	//printf("%s",(char*)buf);
	char * firstLine = strtok((char*)buf,"\n");	
	char * size1 = strtok(NULL,"\n");
					
	char * thirdLine = strtok(NULL,"\n");	
	char * status = strtok(firstLine," ");	
	if(!strcmp(status,"OK")){
		size_t size = atoi(size1);	
		char* filename = firstLine + 3;
		char* checkSum = thirdLine + 4;
		unsigned char* file = (unsigned char*)(thirdLine + strlen(thirdLine) + 1);
		//std::cout<<"filename "<<filename<<"\ncheckSum "<<checkSum<<"\nfile "<<file;
		unsigned char digest[MD5_DIGEST_LENGTH];
		MD5(file, size, (unsigned char*)&digest);  
		char mdString[33]; 
		for(int i = 0; i < 16; i++)
			 sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
		//printf("file %s\n",file);	
		if(!strcmp(checkSum,mdString)){
			printf("md5 checked\n");
		}
		FILE* pfile2 = NULL;
		pfile2 = fopen("private.pem","rb");
		fseek(pfile2, 0L, SEEK_END);
		size_t sz2 = ftell(pfile2);
		rewind(pfile2);
		char* privateKey = (char*)malloc(sz2);
		bzero(privateKey,sz2);
		fread(privateKey,sz2,1,pfile2);
		fclose(pfile2);
		//printf("%s",privateKey);		
		int unit = 256;
		unsigned char* decrypted = (unsigned char*)malloc(unit);
		unsigned char* encrypted = (unsigned char*)malloc(unit);
		std::string getFile;
		int i = 0;
		int decrypted_length = 0;
		for(; i < int(size/unit); i++){
			bzero(encrypted,unit);
			memcpy(encrypted,file + i*unit,unit);
			bzero(decrypted,unit);
			int num = private_decrypt(encrypted,256,(unsigned char*)privateKey, decrypted);
			decrypted_length+=num;
			if(num == -1)
			{
					printLastError("Private Decrypt failed ");
					exit(0);
			}
			getFile+=(char*)decrypted;
		}
		//std::cout<<getFile;
		std::fstream save_file;
		save_file.open(save_name, std::fstream::out | std::fstream::binary);
		save_file<<getFile;
		save_file.close();
	}else{
		std::cout<<(char*)buf<<" "<<(char*)(buf+3);
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
	//int fd = connect_to_server(server, port);
	int fd = connect_to_server(server, port);
	/* put or get, as appropriate */
	if(put_name)
	{
		put_file(fd, put_name);
	}
	else if(get_name and save_name)
	{
		get_file(fd, get_name, save_name);
	}
	else{
		printf("ERROR: illegal command\n");
	}

	/* close the socket */
	int rc;
	if((rc = close(fd)) < 0)
	{
		die("Close error: ", strerror(errno));
	}
	exit(0);
}
