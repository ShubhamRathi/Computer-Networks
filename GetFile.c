#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int FileCount()
{
// Get the # of files

	int nof = 0;
	struct dirent *found;
	DIR *fd;	
	fd = opendir("./shared/");// File Pointer to Shared Folder
	if(NULL == fd)
	{
		printf("Cannot open the Folder");
		return 0;
	}
	else{
	while((found = readdir(fd)) != NULL) //readdir(fd) reads one dirent structure from the directory pointed at by fd into the memory area pointed to by dirp
	{
		if(!strcasecmp(found->d_name, ".") || !strcasecmp(found->d_name, ".."))
			continue;
		printf("%s\n", found->d_name);
		nof++;
	}
	closedir(fd);
	}

	return nof;
}