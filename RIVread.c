#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define CACHESIZE 15000
#include <setjmp.h>
#include <signal.h>
#include "RIVtools.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <error.h>

void fileGrind(FILE* textFile);
void addS2Ds(denseRIV *denseSet, sparseRIV additive, int RIVCount);
int checkDupe(denseRIV* RIVSet, char* word, int wordCount);
void directoryGrind(char *rootString);
void readdirContingency(int sigNumber);

jmp_buf readdirRecov;
int main(int argc, char *argv[]){
	clock_t begintotal = clock();
	lexOpen("/home/drbob/Documents/lexicon");
	char pathString[1000];
	strcpy(pathString, argv[1]);
	strcat(pathString, "/");
	struct stat st = {0};
	if(stat(pathString, &st) == -1) {
		return 1;
	}
	
	directoryGrind(pathString);

	clock_t endtotal = clock();
	double time_spent = (double)(endtotal - begintotal) / CLOCKS_PER_SEC;
	printf("total time:%lf\n\n", time_spent);
	lexClose();
	return 0;
}

void addS2Ds(denseRIV *denseSet, sparseRIV additive, int RIVCount){
	denseRIV *denseSet_slider = denseSet;
	denseRIV *dense_stop = denseSet+RIVCount;



	while(denseSet_slider<dense_stop){
		addS2D((*denseSet_slider).values, additive);
		*(denseSet_slider->contextSize) += additive.frequency;
		denseSet_slider++;

	}
	
}
int checkDupe(denseRIV* RIVSet, char* word, int wordCount){
	denseRIV* RIVStop = RIVSet+wordCount;
	while(RIVSet<RIVStop){
		if(!strcmp(word, RIVSet->name)){
			return 1;
		}
		RIVSet++;
	}
	return 0;
}
void directoryGrind(char *rootString){

	char pathString[2000];
	DIR *directory;
	struct dirent *files = 0;

	if(!(directory = opendir(rootString))){
		printf("location not found, %s\n", rootString);
		return;
	}

	while((files=readdir(directory))){
		if(setjmp(readdirRecov)){
			continue;
		}

		//printf("reclen: %d, d_name pointer: %p, firstDigit, %d", files->d_reclen,files->d_name,*(files->d_name));
		while(*(files->d_name)=='.'){
			files = readdir(directory);
		}
		//signal(SIGSEGV, signalSecure);

		if(files->d_type == DT_DIR){
			strcpy(pathString, rootString);

			strcat(pathString, files->d_name);
			strcat(pathString, "/");
			directoryGrind(pathString);
		}
		strcpy(pathString, rootString);
		strcat(pathString, files->d_name);
		printf("%s\n", pathString);
		FILE *input = fopen(pathString, "r+");
		if(input){
			fileGrind(input);
			fclose(input);
		}
	}
}

void fileGrind(FILE* textFile){
	sparseRIV aggregateRIV = fileToL2Clean(textFile);
	fseek(textFile, 0, SEEK_SET);

	int wordCount = 0;
	denseRIV *RIVArray = (denseRIV*)malloc(aggregateRIV.frequency*sizeof(denseRIV));
	char word[200];

	while(fscanf(textFile, "%99s", word)){

		if(feof(textFile)) break;
		if(!(*word))continue;

		if(!isWordClean((char*)word)){
			continue;
		}
		if(checkDupe(RIVArray, word, wordCount)){
			continue;
		}
		RIVArray[wordCount] = lexPull(word);

		if(!*((RIVArray[wordCount].name))) break;

		*(RIVArray[wordCount].frequency)+= 1;;
		//printf("%s, %d, %d\n", RIVArray[wordCount].name, *(RIVArray[wordCount].frequency), *thing);

		wordCount++;

	}
	//printf("%d\n", wordCount);

	addS2Ds(RIVArray, aggregateRIV, wordCount);
	denseRIV* RIVArray_slider = RIVArray;
	denseRIV* RIVArray_stop = RIVArray+wordCount;
	while(RIVArray_slider<RIVArray_stop){

		lexPush(*RIVArray_slider);
		RIVArray_slider++;
	}
	free(RIVArray);
	free(aggregateRIV.locations);

}
void readdirContingency(int sigNumber){
	("readdir segfaulted, trying to recover");
	longjmp(readdirRecov, 1);

}
