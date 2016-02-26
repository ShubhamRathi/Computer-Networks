#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "md5.h"

void getFileHash()
{

	char temp[100];
	struct stat vstat;
	char *fname = temp;
	int block;
	char *readbuf;

    // copy filename into the response
	strcpy(sFileHash_response.filename, sFileHash.filename);

	strcpy(temp, "./shared/");
	strcat(temp, sFileHash.filename);

    // open file
	FILE *fs = fopen(fname, "r");
	if(fs == NULL)
	{
		printf("Cannot open File.\n");
		return;
	}

    // get time modified of file
	if(stat(temp, &vstat) == -1)
	{
		printf("fstat error\n");
		return;
	}

    // copy the tiem modified into the response
	ctime_r(&vstat.st_mtime, sFileHash_response.time_modified);


    // allocate space for readng the file
	readbuf =(char *) malloc(vstat.st_size * sizeof(char));
	if(readbuf == NULL)
	{
		printf("error, No memory\n");
		exit(0);
	}

    // read the file
	printf("Reading file...\n");
	block = fread(readbuf, sizeof(char), vstat.st_size, fs);
	if(block != vstat.st_size)
	{
		printf("Unable to read file.\n");
		return;
	}

    // get the md5 of the file into the response
	Getmd5(readbuf, vstat.st_size);

    // print the final response
	printf("sFileHash_response of file %s \n", sFileHash_response.filename);
	printf("    - Last Modified @ %s", sFileHash_response.time_modified);
	char tempmd1[1024];
	int s1;
	for(s1=0;s1<16;s1++)
	{
		strcat(tempmd1, sFileHash_response.md5Context.digest[s1]);
	}
	strcat(tempmd1,"\0");
	printf("%s\n", tempmd1);

	return;
}

void Getmd5(char *readbuf, int size)
{
	MD5Init(&sFileHash_response.md5Context);
	MD5Update(&sFileHash_response.md5Context,(unsigned char *) readbuf, size);
	MD5Final(&sFileHash_response.md5Context);
}