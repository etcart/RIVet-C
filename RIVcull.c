#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>

#define RIVSIZE 25000
#define CACHESIZE 0
#define NONZEROS 2
#define THRESHOLD 0.70
#include "RIVtools.h"


void directoryToL2s(char *rootString, sparseRIV** fileRIVs, int *fileCount);

int main(int argc, char *argv[]){
	clock_t begintotal = clock();
	int fileCount = 0;
	//RIVInit();
	sparseRIV *fileRIVs = (sparseRIV*) malloc(1*sizeof(sparseRIV));
	char rootString[2000];
	if(argc <2){ 
		printf("give me a directory");
		return 1;
	}
	strcpy(rootString, argv[1]);
	strcat(rootString, "/");

	directoryToL2s(rootString, &fileRIVs, &fileCount);
	printf("fileCount: %d\n", fileCount);
	
	sparseRIV* fileRIVs_slider = fileRIVs;
	sparseRIV* fileRIVs_stop = fileRIVs+fileCount;
	while(fileRIVs_slider <fileRIVs_stop){
		(*fileRIVs_slider).magnitude = getMagnitudeSparse(*fileRIVs_slider);
		fileRIVs_slider++;
		
	}
	
	clock_t beginnsquared = clock();
	float cosine;
	float minmag;
	float maxmag;
	denseRIV baseDense;
	baseDense.values = malloc(RIVSIZE*sizeof(int));
	fileRIVs_slider = fileRIVs;
	sparseRIV* comparators_slider;
	int count = 0;
	while(fileRIVs_slider<fileRIVs_stop){
		comparators_slider = fileRIVs;
		memset(baseDense.values, 0, RIVSIZE*sizeof(int));
		baseDense.values = addS2D(baseDense.values, *fileRIVs_slider);
		baseDense.magnitude = (*fileRIVs_slider).magnitude;
		minmag = baseDense.magnitude*.85;
		maxmag  = baseDense.magnitude*1.15;
		while(comparators_slider < fileRIVs_slider){
			if((*comparators_slider).magnitude < maxmag && (*comparators_slider).magnitude > minmag && (*comparators_slider).boolean){
				cosine = cosCompare(baseDense, *comparators_slider);
				
				count++;
				if(cosine>THRESHOLD){
					printf("%s\t%s\n%f\n", (*fileRIVs_slider).name , (*comparators_slider).name, cosine);
				(*comparators_slider).boolean = 0; 
					RIVKey.thing++; 
				}
				
			}
			comparators_slider++;
		}
		
		
		fileRIVs_slider++;
		
	}
	clock_t endnsquared = clock();
	double time = (double)(endnsquared - beginnsquared) / CLOCKS_PER_SEC;
	printf("\nnsquared time:%lf\n\n", time);
	printf("\ncosines: %d \n", count);
	printf("\nsims: %d \n", RIVKey.thing);
	clock_t endtotal = clock();
	double time_spent = (double)(endtotal - begintotal) / CLOCKS_PER_SEC;
	printf("total time:%lf\n\n", time_spent);
	free(fileRIVs);
	
return 0;
}
void directoryToL2s(char *rootString, sparseRIV** fileRIVs, int *fileCount){

	char pathString[2000];
	DIR *directory;
    struct dirent *files = 0;
	
	if(!(directory = opendir(rootString))){
		printf("location not found, %s\n", rootString);
		return;
	}
	
	while((files=readdir(directory))){
		if(*(files->d_name) == '.') continue;
	
		if(files->d_type == DT_DIR){
			strcpy(pathString, rootString);
			
			strcat(pathString, files->d_name);
			strcat(pathString, "/");
			directoryToL2s(pathString, fileRIVs, fileCount);
		}
			

		strcpy(pathString, rootString);
		strcat(pathString, files->d_name);
		FILE *input = fopen(pathString, "r");
		if(!input){
			printf("file %s doesn't seem to exist, breaking out of loop", pathString);
			return;
		}else{
			(*fileRIVs) = (sparseRIV*)realloc((*fileRIVs), ((*fileCount)+1)*sizeof(sparseRIV));
			
			(*fileRIVs)[(*fileCount)] = fileToL2(input);
			strcpy((*fileRIVs)[(*fileCount)].name, pathString);
			
			fclose(input);
			(*fileCount)++;
		}
	}
}
