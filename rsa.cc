#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <iostream>
#include <stdio.h>
 
int padding = RSA_PKCS1_PADDING;
 
RSA * createRSA(unsigned char * key,int pub)
{
    RSA *rsa= NULL;
    BIO *keybio ;
    keybio = BIO_new_mem_buf(key, -1);
    if (keybio==NULL)
    {
        printf( "Failed to create key BIO");
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
        printf( "Failed to create RSA");
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
    char * err = (char*)malloc(130);;
    ERR_load_crypto_strings();
    ERR_error_string(ERR_get_error(), err);
    printf("%s ERROR: %s\n",msg, err);
    free(err);
}
int main(){
 
  //char plainText[2048/8] = "Hello this is Ravi"; //key length : 2048

	FILE* pfile = fopen("assignment_06.txt","rb");
	fseek(pfile, 0L, SEEK_END);
	size_t sz = ftell(pfile);
	rewind(pfile);
	char* file = (char*)malloc(sz);
	bzero(file,sz);
	fread(file,sz,1,pfile);
	fclose(pfile);
	
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

	FILE* pfile2 = NULL;
	pfile2 = fopen("private.pem","rb");
	fseek(pfile2, 0L, SEEK_END);
	size_t sz2 = ftell(pfile2);
	rewind(pfile2);
	char* privateKey = (char*)malloc(sz2);
	bzero(privateKey,sz2);
	fread(privateKey,sz2,1,pfile2);
	fclose(pfile2);
	printf("%s",privateKey);
	
	int unit = 128;
	unsigned char* encrypted = (unsigned char*)malloc(unit*2);
	unsigned char* decrypted = (unsigned char*)malloc(unit*2);
	
	unsigned char* outData = (unsigned char*)malloc(((int)(sz/unit)+1)*2*unit);
	//std::cout<<((int)(sz/unit)+1)*256<<"\n";
	bzero(outData, 2*unit);
	int encrypted_length = 0;
	int decrypted_length = 0;
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
	
	//std::cout<<(char*)temp;
	unit = 256;
	//char* inData = outData;
	//bzero(inData,strlen(inData));
	//delete outData;
	//delete text;	
	std::string getFile;
	i = 0;
	for(; i < int(encrypted_length/unit); i++){
		bzero(encrypted,unit);
		memcpy(encrypted,outData + i*unit,unit);
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
 
	

std::cout<<getFile<<"\n";
printf("Decrypted Length =%d\n",decrypted_length);
/*char plainText1[2048/8] = "Hello this is Ravi"; //key length : 2048
char plainText2[2048/8] = "Hello this is Ravi2"; //key length : 2048
unsigned char  encrypted[256]={};
unsigned char decrypted[256]={}; 
	FILE* pfile1 = NULL;
	pfile1 = fopen("public.pem","rb");
	fseek(pfile1, 0L, SEEK_END);
	size_t sz1 = ftell(pfile1);
	rewind(pfile1);
	char* publicKey = (char*)malloc(sz1);
	bzero(publicKey,sz1);
	fread(publicKey,sz1,1,pfile1);
	fclose(pfile1);	
	printf("%s",publicKey);

	FILE* pfile2 = NULL;
	pfile2 = fopen("private.pem","rb");
	fseek(pfile2, 0L, SEEK_END);
	size_t sz2 = ftell(pfile2);
	rewind(pfile2);
	char* privateKey = (char*)malloc(sz2);
	bzero(privateKey,sz2);
	fread(privateKey,sz2,1,pfile2);
	fclose(pfile2);
	printf("%s",privateKey);

int encrypted_length= public_encrypt((unsigned char*)plainText1,strlen(plainText1),(unsigned char*)publicKey,encrypted);
if(encrypted_length == -1)
{
    printLastError("Public Encrypt failed ");
    exit(0);
}
printf("Encrypted length =%d\n",encrypted_length);
 
int decrypted_length = private_decrypt(encrypted,encrypted_length,(unsigned char*)privateKey, decrypted);
if(decrypted_length == -1)
{
    printLastError("Private Decrypt failed ");
    exit(0);
}
encrypted_length= public_encrypt((unsigned char*)plainText2,strlen(plainText2),(unsigned char*)publicKey,encrypted);
if(encrypted_length == -1)
{
    printLastError("Public Encrypt failed ");
    exit(0);
}
printf("Encrypted length =%d\n",encrypted_length);
 
decrypted_length = private_decrypt(encrypted,encrypted_length,(unsigned char*)privateKey, decrypted);
if(decrypted_length == -1)
{
    printLastError("Private Decrypt failed ");
    exit(0);
}
printf("Decrypted Text =%s\n",decrypted);
printf("Decrypted Length =%d\n",decrypted_length);*/
}
