#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>

#define LINE_SIZE 32

unsigned char *gen_md5(char *filename)
{
    FILE *pFile;
    pFile = fopen(filename, "r");
    int nread;
    unsigned char *hash = (unsigned char *)calloc(1, MD5_DIGEST_LENGTH);
    MD5_CTX ctx;
    MD5_Init(&ctx);
    char buf[LINE_SIZE];
    memset(buf, 0, LINE_SIZE);
    if(pFile)
    {
        while ((nread = fread(buf, 1, sizeof buf, pFile)) > 0)
        {    
            //fwrite(buf, nread, 1, stdout); 
            memset(buf, 0, LINE_SIZE);
            MD5_Update(&ctx, (const void *)buf, nread);
        }
            
        if (ferror(pFile)) {
            /* deal with error */
        }
        fclose(pFile);
    }
    MD5_Final(hash, &ctx);
    return hash;
}



int main()
{
/*
    unsigned char *data = "Hello, world!\n";
    char result[MD5_DIGEST_LENGTH*2];
    memset(result,0, MD5_DIGEST_LENGTH*2);
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, (const void *)"hello, ", 7);
    MD5_Update(&ctx, (const void *)"world ", 6);
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5_Final(hash, &ctx);
    int i;
    char tmp[4];
    for(i=0; i<MD5_DIGEST_LENGTH; i++) {
        sprintf(tmp, "%02x", hash[i]);
        strcat(result, tmp);
        printf("%02x", hash[i]);
    }
    printf("\n%s\n", result);
    */

    unsigned char *hash = gen_md5("client.c");
    unsigned char *hash1 = gen_md5("client.c");
    if(strcmp(hash,hash1) == 0)
        printf("yes\n");
    /*
    char result[MD5_DIGEST_LENGTH*2];
    memset(result,0, MD5_DIGEST_LENGTH*2);
    int i;
    char tmp[4];
    for(i=0; i<MD5_DIGEST_LENGTH; i++) {
        printf("%02x", hash[i]);
        sprintf(tmp, "%02x", hash[i]);
        strcat(result, tmp);
    }
    printf("\n%s\n", result);
    */
    return 0;
}
