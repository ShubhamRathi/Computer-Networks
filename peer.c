#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <dirent.h> // contains constructs that facilitate directory traversing
#include <sys/dir.h>
#include <sys/param.h>
#include <fcntl.h>
#include <time.h>
#include "gettime.c"
#include "GetFile.c"
#include "md5.h"
//#include <openssl/hmac.h>
//#include <openssl/md5.h>

#define LENGTH 1000
#define MD5LENGTH 16
#define FALSE 0
#define TRUE !FALSE

int main(int argc, char *argv[])
{
	if(argc < 4)
	{
		printf("Usage ./peer <IP> <Port of Remote Machine> <Port of Your Machine>\n");
		return -1;
	}
	struct stat st = {0};
	if (stat("./shared", &st) == -1) 
	    mkdir("./shared", 0700);
	
	int peer1 = atoi(argv[2]);
	int peer2 = atoi(argv[3]);
	int fd[2];
	//int id = -1;

	pipe(fd);
	int id = fork();


	if(id < 0)
	{
		printf("Creating child process failed\n");
		exit(0);
	}
	if(id > 0)
	{
		close(fd[0]);
		client(peer1, fd[1], argv[1]);
		kill(id, 9);
	}
	else if(id == 0)
	{
		close(fd[1]);
		server(peer2, fd[0]);
		exit(0);
	}
	return 0;
}
// -------------------------------------------------------------------------------------

/* Structures */
typedef enum
{
	FileDownload,
	FileUpload,
	FileHash,
	IndexGet
} CMD;

struct sFD
{
	CMD command;
	char filename[128];
};

struct sFileUpload
{
	CMD command;
	char filename[128];
};

struct sFileHash
{
	CMD command;
	char type[128];
	char filename[128];
};

struct sFileHash_response
{
	char filename[128];
	MD5_CTX md5Context;
	char time_modified[128];
};

struct sIndexGet
{
	CMD command;
	struct stat vstat;
	char filename[128];
};


typedef struct server_handler
{
	int clientfd;
	int n;
	int listenfd;
}server_handler;

typedef struct client_handler
{
	int serverfd;
	int n;
	int listenfd;
}cl;

typedef struct common_handler
{
	int numrv;
	char line[1000];	
}common_handler;

typedef struct number
{
	int num_responses;	
} number; 

//-------------------------------------------------------------------------------------------------

// globals
struct sFileHash_response sFileHash_response;
struct sFileHash sFileHash;

int pattern(){
int i;
for (i=0;i<100;i++)
{
	usleep( 2 );
	printf("|");
	usleep( 2 );
}
return 0;
}

// get next file
char * GetNxtFile(DIR * fd)
{

	struct dirent *found;

	while((found = readdir(fd)) != NULL)
	{
		if(!strcasecmp(found->d_name, ".") || !strcasecmp(found->d_name, ".."))
			continue;
		printf("%s\n", found->d_name);
		return found->d_name;
	}
	downloadpattern();
	return NULL;
}

// get md5 of buffer
void Getmd5(char *readbuf, int size)
{
	MD5Init(&sFileHash_response.md5Context);
	MD5Update(&sFileHash_response.md5Context,(unsigned char *) readbuf, size);
	MD5Final(&sFileHash_response.md5Context);
}


int file_select(const struct direct *entry)
{
	if((strcasecmp(entry->d_name, ".") == 0)
		||(strcasecmp(entry->d_name, "..") == 0))
		return(FALSE);
	else
		return(TRUE);
}


int fileget(char *buf, int Fclientfd)
{
	int count, i;
	struct direct **files;
    //int file_select();
	struct stat vstat;
	struct sIndexGet fstat;
	count = scandir("./shared", &files, file_select, alphasort);

	if(count <= 0)
	{
		strcat(buf, "No files in this directory\n");
		return 0;
	}
	if(send(Fclientfd, &count, sizeof(int), 0) < 0)
	{
		printf("send error\n");
		return 0;
	}
	char dir[200];
	strcpy(dir, "./shared/");
	strcat(buf, "Number of files = ");
	sprintf(buf + strlen(buf), "%d\n", count);
	for(i = 0; i < count; ++i)
	{
		strcpy(dir, "./shared/");
		strcat(dir, files[i]->d_name);
		if(stat(dir, &(vstat)) == -1)
		{
			printf("fstat error\n");
			return 0;
		}
		fstat.vstat = vstat;
		strcpy(fstat.filename, files[i]->d_name);
		downloadpattern();

		if(write(Fclientfd, &fstat, sizeof(fstat)) == -1)
			printf("Failed to send vstat\n");
		else
			printf("Sent vstat\n");
	//sprintf(buf+strlen(buf),"%s    ",files[i-1]->d_name);
	}
//            strcat(buf,"\n"); 
	return 0;
}

void downloadpattern()
{
	int i,j;
	for(i=0;i<1000;i++)
	{
		for(j=1;j<100;j++)
		{}
	}
}

int client(int portnum, int fd1, char *IP)
{
	//int n = 0, listenfd = 0, serverfd = 0;
	cl client_handler;

	char *srecvBuff, *crecvBuff;
	char ssendBuff[1025], csendBuff[1025];
	struct sockaddr_in serv_addr, c_addr;	
	struct sFD cFD;
	struct sFD sFD;
	struct sFileUpload CFUp;
	struct sFileUpload sFileUpload;
	number n;

	memset(&c_addr, '0', sizeof(c_addr));
	memset(csendBuff, '0', sizeof(csendBuff));
	memset(ssendBuff, '0', sizeof(ssendBuff));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portnum);
	serv_addr.sin_addr.s_addr = inet_addr(IP);
	downloadpattern();

	if((client_handler.serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\nError creating socket \n");
		return 1;
	}
	else
		printf("created socket\n");
	
	int timeout=0;
	while((connect(client_handler.serverfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) && timeout < 5000)
	{
		timeout++;
		sleep(1);
	}

	printf("Connect to server on port %d successful\n", portnum);

	while(1)
	{
		scanf("%s", csendBuff);	//scanning for input command
		printf("Received command : %s\n", csendBuff);
		int leng;

		
		if(strcasecmp(csendBuff, "FileUploadAllow") == 0)
		{
			printf("File successfully Uploaded\n");
			leng=strlen("FileUploadAllow");
			write(fd1, "FileUploadAllow",(leng + 1));
		}

		if(strcasecmp(csendBuff, "FileUploadDeny") == 0)
		{
			printf("You have denied the Upload\n");
			leng=strlen("FileUploadDeny");
			write(fd1, "FileUploadDeny",(leng + 1));
		}

		if(strcasecmp(csendBuff, "FileDownload") == 0)
		{
			cFD.command = FileDownload;
			scanf("%s", cFD.filename);
			printf("Dowloading file %s: \n", cFD.filename);
			int command = FileDownload;
			if((client_handler.n = write(client_handler.serverfd, &command, sizeof(int))) == -1)
				printf("Could not send command %s\n", csendBuff);
			else
				printf("Last Command: %d %s\n", client_handler.n, csendBuff);

			if(write(client_handler.serverfd, &cFD, sizeof(cFD)) == -1)
				printf("Failed to send file\n");
			else
				printf("Sent:  %s\n", cFD.filename);

			char revbuf[LENGTH];
			printf("Getting file from Server\n");

			char temp[100]="./shared/";			
			strcat(temp, cFD.filename);
			FILE *fr = fopen(temp, "w+");
			downloadpattern();
			if(fr == NULL)
			{
				printf("Could not create file %s\n", temp);
				return 0;
			}
			else
				printf("File did not exist. So we made one! %s\n", temp);
			bzero(revbuf, LENGTH);
			int fr_block_sz = 0;
		    // receive the file size
			int size = 0;
			if((fr_block_sz =
				recv(client_handler.serverfd, &size, sizeof(int), 0)) != sizeof(int))
			{
				printf("Error reading file size %s\n", temp);
				return 0;
			}
			else
				printf("Received size of file %d\n", size);

			int recvdsize = 0;
			crecvBuff =(char *) malloc(size * sizeof(char));
		    // save original pointer to calculate MD5
			while((fr_block_sz = recv(client_handler.serverfd, crecvBuff, LENGTH, 0)) > 0)
			{
				crecvBuff[fr_block_sz] = 0;
				int write_sz =
				fwrite(crecvBuff, sizeof(char), fr_block_sz, fr);
				crecvBuff += fr_block_sz;
				if(write_sz == -1)
				{
					printf("Error writing to file %s\n", temp);
					return 0;
				}

				recvdsize += fr_block_sz;
				printf("%d\n", recvdsize);
				if(recvdsize >= size)
					break;
			}
			printf("done!\n");
			fclose(fr);
			client_handler.n = 0;
		}

		else if(strcasecmp(csendBuff, "FileHash") == 0)
		{

			struct sFileHash_response cFileHash_response;
			struct sFileHash cFileHash;
			struct stat vstat;
			//int num_responses;
			//no n;
			int command = FileHash;
			int i;

		    // set the FileHash command
			cFileHash.command = FileHash;

		    // get the type
			scanf("%s", cFileHash.type);
			printf("FileHash type %s ...\n", cFileHash.type);
			sleep(200);

		    // get the filename if Verify
			if(strcasecmp(cFileHash.type, "Verify") == 0)
			{
				scanf("%s", cFileHash.filename);
				printf("FileHash file %s ...\n", cFileHash.filename);
			}

		    //sending command name
			if((client_handler.n = write(client_handler.serverfd, &command, sizeof(int))) == -1)
				printf("Failed to send command %s\n", csendBuff);
			else
				printf("Command sent: %d %s\n", client_handler.n, csendBuff);

		    // send cFileHash with the the information
			if(write(client_handler.serverfd, &cFileHash, sizeof(cFileHash)) == -1)
				printf("Failed to send    cFileHash\n");
			else
				printf("Sent cFileHash %s %s \n", cFileHash.type,
					cFileHash.filename);

		    // receive number of file hash resposes to expect
			if((client_handler.n =
				recv(client_handler.serverfd, &n.num_responses, sizeof(n.num_responses),
					0) != sizeof(n.num_responses)))
			{
				printf("Error reading number of responses of file\n");
				return 0;
			}
			else
				printf("Expecting %d responses\n", n.num_responses);

		    // print each file hash response
			for(i = 0; i < n.num_responses; i++)
			{

				if((client_handler.n =
					recv(client_handler.serverfd, &cFileHash_response,
						sizeof(cFileHash_response),
						0) != sizeof(cFileHash_response)))
				{
					printf("Error reading cFileHash_response of file %s\n",
						cFileHash.filename);
					return 0;
				}
				else
				{
					printf("Received cFileHash_response of file %s \n",
						cFileHash_response.filename);
					printf("Last Modified @ %s\n",
						cFileHash_response.time_modified);
					char tempmd[1024];
					int s;
					downloadpattern();
					for(s=0;s<=15;s++)
					{
						strcat(tempmd,cFileHash_response.md5Context.digest[s]);
					}
					strcat(tempmd,'\0');
					printf("%s\n",tempmd);
				}
			}
		}

		else if(strcasecmp(csendBuff, "FileUpload") == 0)
		{

			CFUp.command = FileUpload;
			scanf("%s", CFUp.filename);
			printf("Uploading file %s ...\n", CFUp.filename);

			int command = FileUpload;

		    //sending command name
			if((client_handler.n = write(client_handler.serverfd, &command, sizeof(int))) == -1)
				printf("Failed to send command %s\n", csendBuff);
			else
				printf("Command sent: %d %s\n", client_handler.n, csendBuff);

		    //opening the file
			char temp[100];
			strcpy(temp, "./shared/");
			strcat(temp, CFUp.filename);

			char *fname = temp;
			FILE *fs = fopen(fname, "r");
			if(fs == NULL)
			{
				printf("Unable to open file.\n");
				return 0;
			}

			int block;
			char *readbuf;
			struct stat vstat;

		    // get size of file
			if(stat(temp, &vstat) == -1)
			{
				printf("vstat error\n");
				return 0;
			}

			if(write(client_handler.serverfd, &CFUp, sizeof(CFUp)) == -1)
				printf("Failed to send        CFUp\n");
			else
				printf("Sent CFUp %s\n", CFUp.filename);

			int size = vstat.st_size;

			if(send(client_handler.serverfd, &size, sizeof(int), 0) < 0)
			{
				printf("send error\n");
				return 0;
			}
			else
				printf("sending file size %d\n", size);

		    //waiting for accept or deny:
			char found[100];
			if((client_handler.n = read(client_handler.serverfd, &found, sizeof(found))) <= 0)
				printf("Error reading found\n");
			printf("%s", found);
			if(strcasecmp(found, "FileUploadDeny") == 0)
			{
				printf("Upload denied.\n");
				fclose(fs);
				downloadpattern();

				continue;
			}
			else
				printf("Upload accepted\n");
			readbuf =(char *) malloc(size * sizeof(char));
			if(readbuf == NULL)
			{
				printf("Error, No memory\n");
				exit(0);
			}

		    //sending the file
			while((block = fread(readbuf, sizeof(char), size, fs)) > 0)
			{
				if(send(client_handler.serverfd, readbuf, block, 0) < 0)
				{
					printf("send error\n");
					return 0;
				}
			}
			client_handler.n = 0;
		}

		else if(strcasecmp(csendBuff, "IndexGet") == 0)
		{
			char opt[1000];
			char *filelist;
			int command = IndexGet;
			struct stat vstat;
			struct sIndexGet fstat;
			int lenfiles = 0;
			int fr_block_sz;
			char buff[1000];
			char t1[200], t2[200];
			time_t timt1, timt2;
			downloadpattern();
			char regex[100], fname[1000];
			if((client_handler.n = write(client_handler.serverfd, &command, sizeof(int))) == -1)
				printf("Failed to send command %s\n", csendBuff);
			else
				printf("Command sent: %d %s\n", client_handler.n, csendBuff);
			
			scanf("%s", opt);
			puts(opt);
			if(strcasecmp(opt, "ShortList") == 0)
			{
				scanf("%s %s", t1, t2);
				
				timt1 = gettime(t1);
				timt2 = gettime(t2);
			}

			if(strcasecmp(opt, "RegEx") == 0)
			{
				system("touch res");
				scanf("%s", regex);

			}
			FILE *fp = fopen("./res", "a");

			if((fr_block_sz =
				recv(client_handler.serverfd, &lenfiles, sizeof(int), 0)) != sizeof(int))
			{
				printf("Error reading number of file\n");
				return 0;
			}
			else
				printf("Recieved number of files %d\n", lenfiles);
			int i;

			for(i = 0; i < lenfiles; i++)
			{
				if((client_handler.n =
					read(client_handler.serverfd,(void *) &fstat,
						sizeof(fstat))) != sizeof(fstat))
					printf("Error reciving vstat\n");

				if(strcasecmp(opt, "LongList") == 0)
				{
					printf("Filename: ");
					puts(fstat.filename);
					vstat = fstat.vstat;
					printf("Size: %d\t",(int) vstat.st_size);
					ctime_r(&vstat.st_mtime, buff);
					printf("Time Modified: %s\n", buff);
				}

				else if(strcasecmp(opt, "ShortList") == 0)
				{
					vstat = fstat.vstat;
					if(difftime(vstat.st_mtime, timt1) >= 0
						&& difftime(timt2, vstat.st_mtime) >= 0)
					{
						printf("Filename: ");
						puts(fstat.filename);
						vstat = fstat.vstat;
						printf("Size: %d\t",(int) vstat.st_size);
						ctime_r(&vstat.st_mtime, buff);
						printf("Time Modified: %s\n", buff);
					}
				}

				else if(strcasecmp(opt, "RegEx") == 0)
				{

					fwrite(fstat.filename, 1, strlen(fstat.filename), fp);
					fwrite("\n", 1, 1, fp);
				}
			}
			fclose(fp);
			if(strcasecmp(opt, "RegEx") == 0)
			{
				char call[10000];
				strcpy(call, "cat res | grep -E ");
				strcat(call, regex);
				strcat(call, " ; rm -rf res");
				system(call);
			}

		}

		client_handler.n = 0;
		sleep(1);
	}
	return 0;
}

int server(int portnum, int fd0)
{
	number n;
	server_handler server;
	server.clientfd = 0;
	server.n = 0;
	server.listenfd = 0;
	char *srecvBuff, *crecvBuff;
	char ssendBuff[1025], csendBuff[1025];
	struct sockaddr_in s_addr, c_addr;
	struct sFD cFD;
	struct sFD sFD;
	struct sFileUpload CFUp;
	struct sFileUpload sFileUpload;
	memset(&c_addr, '0', sizeof(c_addr));
	memset(csendBuff, '0', sizeof(csendBuff));
	memset(ssendBuff, '0', sizeof(ssendBuff));
	int cmd;
	char *str;
    str = (char *) malloc(1500);

	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	s_addr.sin_port = htons(portnum);

	server.listenfd = socket(AF_INET, SOCK_STREAM, 0);
	printf("created socket\n");
	downloadpattern();

	bind(server.listenfd,(struct sockaddr *) &s_addr, sizeof(s_addr));
	printf("listening on port %d\n", portnum);
	free(str);
	if(listen(server.listenfd, 10) == -1)
	{
		printf("Failed to listen\n");
		return -1;
	}
    server.clientfd = accept(server.listenfd,(struct sockaddr *) NULL, NULL);	// accept awaiting request
    //if(clientfd == -1)
    	//printf
   //("accept faileindent: Standard input:476: Warning:old style assignment ambiguity in \"=-\".        Assuming \"= -\"d\n");
    if(!server.clientfd == -1)
    	printf("Connection Accepted \n");

    while(1)
    {
    	if((server.n = read(server.clientfd, &cmd, sizeof(int))) > 0)
    	{
    		printf("Recieved %d %d\n", server.n, cmd);
    		server.n = 0;
    	}

    	if((CMD) cmd == FileDownload)
    	{
    		printf("Got FileDownload command\n");
    		if((server.n =read(server.clientfd,(void *) &sFD,sizeof(sFD))) != sizeof(sFD))
    			printf("Error reading filename\n");
    		printf("Sending file %s\n", sFD.filename);
    		char temp[100];
    		strcpy(temp, "./shared/");
    		strcat(temp, sFD.filename);
    		char *fname = temp;
    		FILE *fs = fopen(fname, "r");
    		if(fs == NULL)
    		{
    			printf("Unable to open file.\n");
    			return 0;
    		}
    		int block;
    		char *readbuf;
    		struct stat vstat;
	    // get size of file
    		if(stat(temp, &vstat) == -1)
    		{
    			printf("vstat error\n");
    			return 0;
    		}
	    // send file size first
    		int size = vstat.st_size;
    		readbuf =(char *) malloc(size * sizeof(char));
    		if(readbuf == NULL)
    		{
    			printf("No Memory\n");
    			exit(0);
    		}
    		if(send(server.clientfd, &size, sizeof(int), 0) < 0)
    		{
    			printf("send error\n");
    			return 0;
    		}
    		else
    			printf("sending file size %d\n", size);

    		while((block = fread(readbuf, sizeof(char), size, fs)) > 0)
    		{
    			if(send(server.clientfd, readbuf, block, 0) < 0)
    			{
    				printf("send error\n");
    				return 0;
    			}
    			printf("File Sent\n");
    		}
    		fclose(fs);
    	}

    	else if((CMD) cmd == FileUpload)
    	{
    		char temp[100];
			printf("Got FileUpload command\n");

	   		if((server.n = read(server.clientfd,(void *) &sFileUpload,sizeof(sFileUpload))) != sizeof(sFileUpload))
    			printf("Error reading filename\n");
 
     		printf("File name: %s\n", sFileUpload.filename);
    		int size;
    		int fr_block_sz = 0;
    		if((fr_block_sz =
    			recv(server.clientfd, &size, sizeof(int), 0)) != sizeof(int))
    		{
    			printf("Error reading size of file %s\n", temp);
    			return 0;
    		}
    		else
    			printf("File size: %dB\n", size);

    		char found[100];
    		printf("FileUploadDeny or FileUploadAllow?\n");
    		read(fd0, found, sizeof("FileUploadAllow"));

    		if(strcasecmp(found, "FileUploadDeny") == 0)
    		{
    			if((server.n = write(server.clientfd, &found, sizeof(found))) == -1)
    				printf("Failed to send found %s\n", found);
    			else
    				printf("found sent: %d %s\n", server.n, found);
    			continue;
    		}
    		else
    		{
    			printf("Accepted file\n");
    			write(server.clientfd, &found, sizeof(found));
    		}

    		strcpy(temp, "./shared/");
    		strcat(temp, sFileUpload.filename);
    		FILE *fr = fopen(temp, "w+");
    		if(fr == NULL)
    		{
    			printf("Error creating file %s\n", temp);
    			downloadpattern();
    			return 0;
    		}
    		else
    			printf("created file %s\n", temp);

    		srecvBuff =(char *) malloc(size * sizeof(char));
    		int recvdsize = 0;
    		while((fr_block_sz = recv(server.clientfd, srecvBuff, LENGTH, 0)) > 0)
    		{
    			srecvBuff[fr_block_sz] = 0;
    			int write_sz =
    			fwrite(srecvBuff, sizeof(char), fr_block_sz, fr);
    			srecvBuff += fr_block_sz;
    			if(write_sz == -1)
    			{
    				printf("Error writing to file %s\n", temp);
    				return 0;
    			}
    			else
    				printf("wrote to file %s %d bytes\n", temp, write_sz);
    			recvdsize += fr_block_sz;
    			if(recvdsize >= size)
    				break;

    		}
    		printf("Done Upload!\n");
    		fclose(fr);
    	}

    	if((CMD) cmd == FileHash)
    	{
    		MD5_CTX md5Context;
    		char sign[16];
    		//int num_responses;
    		char temp[100];
    		printf("Got FileHash command\n");

    		if((server.n =read(server.clientfd,(void *) &sFileHash,sizeof(sFileHash))) != sizeof(sFileHash))
    			printf("Error reading cFileHash\n");
    		else
    			printf("Type:%s\n", sFileHash.type);

    		if(strcasecmp(sFileHash.type, "Verify") == 0)
    		{

    			printf("Got Verify\n");
    			downloadpattern();

    			n.num_responses = 1;

    			if(send(server.clientfd, &n.num_responses, sizeof(int), 0) < 0)
    			{
    				printf("send error\n");
    				return 0;
    			}
    			else
    				printf("sent number of responses %d\n", n.num_responses);
    			printf("ALL DONE\n");
    			getFileHash();
    			if(send(server.clientfd, &sFileHash_response, sizeof(sFileHash_response),0) < 0)
    			{
    				printf("send error\n");
    				return 0;
    			}
    			else
    				printf("sent sFileHash_response %s\n",
    					sFileHash_response.filename);
    		}
    		else if(strcasecmp(sFileHash.type, "CheckAll") == 0)
    		{
    			DIR *fd;
    			int i;
    			printf("Got CheckAll\n");
    			strcpy(temp, "./shared/");
    			fd = opendir(temp);
    			if(NULL == fd)
    			{
    				printf("Error opening directory %s\n", temp);
    				return 0;
    			}

    			n.num_responses = FileCount();
    			printf("Found %d files in shared folder\n", n.num_responses);
    			if(send(server.clientfd, &n.num_responses, sizeof(n.num_responses), 0) <    0)
    			{
    				printf("send error\n");
    				return 0;
    			}
    			else
    				printf("sent number of responses %d\n", n.num_responses);

    			fd = opendir(temp);
    			for(i = 0; i < n.num_responses; i++)
    			{
    				strcpy(sFileHash.filename, GetNxtFile(fd));
    				getFileHash();

    				if(send(server.clientfd, &sFileHash_response,sizeof(sFileHash_response), 0) < 0)
    				{
    					printf("send error\n");
    					return 0;
    				}
    				else
    					printf("sent sFileHash_response %s\n",sFileHash_response.filename);
    			}
    			closedir(fd);
    		}
    		else
    		{
    			printf("Received invalid FileHash Command %s\n",
    				sFileHash.type);
    			return 0;
    		}

    	}
    	
    	else if((CMD) cmd == IndexGet)
    	{
    		char filelist[1000];
    		strcpy(filelist, "");
    		fileget(filelist, server.clientfd);
    		printf("File List Sent\n");
    	}
    	
    	if(server.n < 0)
    		printf("\n Read Error \n");
    }
    close(server.clientfd);
    return 0;
}

// get file hash information for current sFileHash
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

UDP_Client()
{
    struct sockaddr_in server,client;
    int s,n,ret;size_t fp;
    char filename[20],downloaded[10],filedata[100],c[25];
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    s=socket(AF_INET,SOCK_DGRAM,0);
    server.sin_family=AF_INET;
    server.sin_port=2000;
    server.sin_addr.s_addr=inet_addr("127.0.0.1");
    n=sizeof(server);
    printf("Enter the name of the file: ");
    scanf("%s",filename);
    printf("\nEnter a name to save: ");
    scanf("%s",downloaded);
    printf("\nDownloading...\n");
    sendto(s,filename,sizeof(filename),0,(struct sockaddr *)&server,n);
    fp = open(downloaded, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if(fp==-1)
    {
        printf("\nError...");
        exit(0);
    }
    recvfrom(s,filedata,sizeof(filedata),0,NULL,NULL);
    printf("\nProcessing Contents...\n");
    while(1)
    {
        if(strcmp(filedata,"end")==0)
            break;
    printf("%s",filedata);
        ret=write(fp,filedata,strlen(filedata));
        bzero(filedata,100);
        recvfrom(s,filedata,sizeof(filedata),0,NULL,NULL);
    }
    printf("\nFile downloaded successfully...\n");
}

UDP_Server()
{
    struct sockaddr_in server,client;
    int serversock,n,fp,end;
    char filename[20],buffer[100];
    serversock=socket(AF_INET,SOCK_DGRAM,0);
    server.sin_family=AF_INET;
    server.sin_port=2000;
    server.sin_addr.s_addr=inet_addr("127.0.0.1");
    bind(serversock,(struct sockaddr *)&server,sizeof(server));
    n=sizeof(client);
    recvfrom(serversock,filename,sizeof(filename), 0,(struct sockaddr *)&client,&n);
    fp=open(filename,O_RDONLY);
    while(1)
    {
        end=read(fp,buffer,sizeof(buffer));
        if(end==0)
            break;
        sendto(serversock,buffer,sizeof(buffer),0,(struct sockaddr *)&client,n);
        bzero(buffer,100);
    }
    strcpy(buffer,"end");
    sendto(serversock,buffer,sizeof(buffer),0,(struct sockaddr *)&client,n);
}