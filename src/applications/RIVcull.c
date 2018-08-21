#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#define THRESHOLD 0.7
#include "../RIVtools.h"

/* this program identifies all near-duplicates among the documents in the 
 * chosen root directory, using RIV comparison although this is meant for
 * demonstration purposes, it can easily be repurposed to remove 
 * near-duplicates that it finds */

// fills the fileRIVs array with a vector for each file in the root directory
void directoryToL2s(char *rootString, sparseRIV** fileRIVs, int *fileCount);

int main(int argc, char *argv[]){
	
	int fileCount = 0;
	
	//initializes the fileRIVs array to be reallocced by later function
	sparseRIV *fileRIVs = (sparseRIV*) malloc(1*sizeof(sparseRIV));
	char rootString[2000];
	if(argc <2){ 
		printf("give me a directory");
		return 1;
	}
	strcpy(rootString, argv[1]);
	strcat(rootString, "/");

	//gather all vectors ino the fileRIVs array and count them in fileCount
	directoryToL2s(rootString, &fileRIVs, &fileCount);
	printf("fileCount: %d\n", fileCount);
	
	//first calculate all magnitudes for later use
	for(int i = 0; i < fileCount; i++){
		fileRIVs[i].magnitude = getMagnitudeSparse(fileRIVs[i]);
		
	}
	clock_t begintotal = clock();
	double cosine;
	double minmag;
	double maxmag;
	
	//all cosines need a sparse-dense comparison.  so we will create a 
	denseRIV baseDense;
		
	for(int i = 0; i < fileCount; i++){
		
		//0 out the denseVector, and map the next sparseVector to it
		memset(&baseDense, 0, sizeof(denseRIV));
		addS2D(baseDense.values, fileRIVs[i]);
		
		//pass magnitude to the to the dense vector
		baseDense.magnitude = fileRIVs[i].magnitude;
		
		//if these two vectors are too different in size, we can know that they are not duplicates
		minmag = baseDense.magnitude*.85;
		maxmag  = baseDense.magnitude*1.15;
		for(int j = 0; j < i; j++){
			//if this vector is within magnitude threshold
			if(fileRIVs[j].magnitude < maxmag 
			&& fileRIVs[j].magnitude > minmag){
				
				//identify the similarity of these two vectors
				cosine = cosCompare(&baseDense, &fileRIVs[j]);
								
		
				//if the two are similar enough to be flagged
				if(cosine>THRESHOLD){
					printf("%s\t%s\n%f\n", fileRIVs[i].name , fileRIVs[j].name, cosine);
				}	
			}
		}
	}
	printf("fileCount: %d", fileCount);
	free(fileRIVs);
	clock_t endtotal = clock();
	double time_spent = (double)(endtotal - begintotal) / CLOCKS_PER_SEC;
	printf("total time:%lf\n\n", time_spent);
return 0;
}

//mostly a standard recursive Dirent-walk
void directoryToL2s(char *rootString, sparseRIV** fileRIVs, int *fileCount){
/* *** begin Dirent walk *** */
	char pathString[2000];
	DIR *directory;
	struct dirent *files = 0;

	if(!(directory = opendir(rootString))){
		printf("location not found, %s\n", rootString);
		return;
	}

	while((files=readdir(directory))){
		
		if(!files->d_name[0]) break;
		while(*(files->d_name)=='.'){
			files = readdir(directory);
		}
		
		
		
		if(files->d_type == DT_DIR){
			strcpy(pathString, rootString);

			strcat(pathString, files->d_name);
			strcat(pathString, "/");
			directoryToL2s(pathString, fileRIVs, fileCount);
			continue;
		}
		strcpy(pathString, rootString);
		strcat(pathString, files->d_name);

/* *** end dirent walk, begin meat of function  *** */

		FILE *input = fopen(pathString, "r");
		if(input){
			
			*fileRIVs = (sparseRIV*)realloc((*fileRIVs), ((*fileCount)+1)*sizeof(sparseRIV));
			
			(*fileRIVs)[*fileCount] = fileToL2(input);
			strcpy((*fileRIVs)[*fileCount].name, pathString);
			
			fclose(input);
			 *fileCount += 1;
		}
	}
}
