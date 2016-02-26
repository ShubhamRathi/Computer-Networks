
#include <stdio.h>
#include <time.h>


time_t gettime(char *intime)
{
	char *ch = strtok(intime, "_");
	int time1[6];
	int i = 0;
	struct tm tm1={0};
	while(ch != NULL)
	{
		time1[i++] = atoi(ch);
		ch = strtok(NULL, "_");
	}
	while(i < 6)
		time1[i++] = 0;
	if(time1[2] == 0)
		time1[2] = 1;

	tm1.tm_year = time1[0] - 1900;
	tm1.tm_mon = time1[1] - 1;
	tm1.tm_mday = time1[2];
	tm1.tm_hour = time1[3];
	tm1.tm_min = time1[4];
	tm1.tm_sec = time1[5];

	return mktime(&tm1);
}