#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
//RIVSIZE macro must be set to the size of the RIVs in the lexicon
#define RIVSIZE 50000
#define CACHESIZE 0
#define EPSILON 0.95
#define MINPOINTS 20
#define UNCHECKED 0
#define NOISE -1
#define MINSIZE 3000


#include "../RIVtools.h"

struct DBnode{
	sparseRIV RIV;
	int* indexes;
	int indexCount;
	int status;
}*DBset;


void DBdive(int C, int i);
void directoryToL2s(char *rootString, sparseRIV** fileRIVs, int *fileCount);

int main(int argc, char *argv[]){
	if(argc <2){
		printf("give me a directory");
		return 1;
	}
	clock_t begintotal = clock();
	int fileCount = 0;
	
	sparseRIV *fileRIVs = (sparseRIV*) malloc(1*sizeof(sparseRIV));
	char rootString[2000];
	
	lexOpen(argv[1]);
	strcpy(rootString, argv[1]);
	strcat(rootString, "/");

	directoryToL2s(rootString, &fileRIVs, &fileCount);
	printf("fileCount: %d\n", fileCount);

	sparseRIV* fileRIVs_slider = fileRIVs;
	sparseRIV* fileRIVs_stop = fileRIVs+fileCount;
	DBset = malloc(fileCount*sizeof(struct DBnode));
	struct DBnode* DBset_slider = DBset;
	while(fileRIVs_slider <fileRIVs_stop){
		(*fileRIVs_slider).magnitude = getMagnitudeSparse(*fileRIVs_slider);
		(*DBset_slider).RIV = *fileRIVs_slider;
		(*DBset_slider).indexes = malloc(sizeof(int));
		(*DBset_slider).indexCount = 0;
		(*DBset_slider++).status = 0;
		fileRIVs_slider++;

	}
	free(fileRIVs);

	clock_t beginnsquared = clock();
	float cosine;

	denseRIV baseDense;
	baseDense.values = malloc(RIVSIZE*sizeof(int));

	for(int i=0; i<fileCount; i++){
		memset(baseDense.values, 0, RIVSIZE*sizeof(int));
		baseDense.values = addS2D(baseDense.values, DBset[i].RIV);
		baseDense.magnitude = DBset[i].RIV.magnitude;

		for(int j=i+1; j<fileCount; j++){
				cosine = cosCompare(baseDense, DBset[j].RIV);


			if(cosine>EPSILON){

				DBset[i].indexes = realloc(DBset[i].indexes, (DBset[i].indexCount+1)*sizeof(int));
				DBset[i].indexes[DBset[i].indexCount++] = j;
				DBset[j].indexes = realloc(DBset[j].indexes, (DBset[j].indexCount+1)*sizeof(int));
				DBset[j].indexes[DBset[j].indexCount++] = i;
			}
		}
	}
	int C = 0;
	printf("got here\n");
	for(int i=0; i<fileCount; i++){
		if(DBset[i].status) continue;
		if(DBset[i].indexCount <MINPOINTS){
			DBset[i].status = NOISE;
		}
		C++;
		DBset[i].status = C;
		DBdive(C, i);
	}


	clock_t endnsquared = clock();
	double time = (double)(endnsquared - beginnsquared) / CLOCKS_PER_SEC;
	printf("\nnsquared time:%lf\n\n", time);
	clock_t endtotal = clock();
	double time_spent = (double)(endtotal - begintotal) / CLOCKS_PER_SEC;
	printf("total time:%lf\n\n", time_spent);


return 0;
}
void DBdive(int C, int i){
	printf("root: %s\n", DBset[i].RIV.name);
	struct DBnode *DBnet = malloc(sizeof(struct DBnode));
	DBnet[0] = DBset[i];
	int nodeCount = 1;
	for(int j=0; j<nodeCount; j++){
		for(int k=0; k<DBnet[j].indexCount;k++){
			int index = DBnet[j].indexes[k];
			if(DBset[index].status>0) continue;
			printf(">>%s\n", DBset[index].RIV.name);
			DBset[index].status = C;
			if(DBset[index].indexCount> MINPOINTS){
				DBnet = realloc(DBnet, (nodeCount+1)*sizeof(struct DBnode));
				
				DBnet[nodeCount++] = DBset[index];
			}
		}
	}
	free(DBnet);
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
		denseRIV temp = lexPull(files->d_name);
		if(*temp.frequency >MINSIZE){
			(*fileRIVs) = (sparseRIV*)realloc((*fileRIVs), ((*fileCount)+1)*sizeof(sparseRIV));

			(*fileRIVs)[(*fileCount)] = normalize(temp, 500);

			strcpy((*fileRIVs)[(*fileCount)].name, files->d_name);
			(*fileCount)++;
		}
		free(temp.values);
	}
}

