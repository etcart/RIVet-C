/* this DB scan algorithm is not meant to be an example of an easily written 
 * program. rather it is a useful tool that can be used to validate the contents
 * of a lexicon.  it will identify, using a density based algorithm
 * clusters of vectors.  if the lexicon is well formed, these clusters should
 * be numerous, as well as containing well related words */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
//RIVSIZE macro must be set to the size of the RIVs in the lexicon
#define RIVSIZE 25000
#define CACHESIZE 0
#define EPSILON 0.98
#define MINPOINTS 1
#define UNCHECKED 0
#define NOISE -1
#define MINSIZE -1


#include "../RIVtools.h"

/* the node holds a vector, and metadata:
 * -indexes will hold the array indexes of its neighbors
 * -indexCount will hold the number of neighbors
 * -status will hold its cluster, either a cluster number or "unchecked"
 */
struct DBnode{
	sparseRIV RIV;
	struct DBnode** neighbors;
	int neighborCount;
	int status;
};

void intercompare(struct DBnode* DBset, int nodeCount);
void DBdive(struct DBnode* root, struct DBnode *DBset, int C);
void directoryToL2s(char *rootString, sparseRIV** fileRIVs, int *fileCount);

int main(int argc, char *argv[]){
	if(argc <2){
		printf("give me a directory");
		return 1;
	}
	int fileCount = 0;
	
	sparseRIV *fileRIVs = (sparseRIV*) malloc(1*sizeof(sparseRIV));
	char rootString[1000];
	
	lexOpen(argv[1]);
	strcpy(rootString, argv[1]);
	strcat(rootString, "/");

	directoryToL2s(rootString, &fileRIVs, &fileCount);
	printf("fileCount: %d\n", fileCount);
	/* an array of nodes, one for each vector */
	struct DBnode DBset[fileCount];
	
	/* fill the node array with vectors and initialize metadata */
	for(int i = 0; i < fileCount; i++){
		fileRIVs[i].magnitude = getMagnitudeSparse(fileRIVs[i]);
		DBset[i].RIV = fileRIVs[i];
		/* a single malloc for later realloc'ing */
		DBset[i].neighbors = malloc(sizeof(struct DBnode*));
		DBset[i].neighborCount = 0;
		DBset[i].status = UNCHECKED;
		
	}
	/* fileRIVs was only temporary */
	free(fileRIVs);

	intercompare(DBset, fileCount);

	
	int C = 0;
	
	for(int i=0; i<fileCount; i++){
		if(DBset[i].status) continue;
		if(DBset[i].neighborCount <MINPOINTS){
			DBset[i].status = NOISE;
			continue;
		}
		C++;
		printf("\ncluster %d\n", C);
		DBset[i].status = C;
		printf("root: %s, %d, %lf\n", DBset[i].RIV.name, DBset[i].RIV.frequency, DBset[i].RIV.magnitude);
		DBdive(&DBset[i], DBset, C);
	}


return 0;
}
void DBdive(struct DBnode* root, struct DBnode *DBset, int C){

	for(int i = 0; i < root->neighborCount; i++){
		/* if this node is not already claimed by a cluster */
		if(root->neighbors[i]->status > 0){
			continue;
		}
		/* for easier coding, put it in a local variable */
		struct DBnode *branch = root->neighbors[i];
		
		printf(">>%s, %d, %lf\n", branch->RIV.name, branch->RIV.frequency, branch->RIV.magnitude);
		
		/* include this in the cluster C */
		branch->status = C;
		/* if this branch has enough neighbors to spread */
		if(branch->neighborCount > MINPOINTS){
			/* recursive dive into next branch */
			DBdive(branch, DBset, C);
		
		}
		
	}
}
/* fileRIVs and fileCount are accessed as pointers, so that we can find them changed outside this function
 */
void directoryToL2s(char *rootString, sparseRIV** fileRIVs, int *fileCount){
	
	DIR *directory;
    struct dirent *files = 0;

	if(!(directory = opendir(rootString))){
		printf("location not found, %s\n", rootString);
		return;
	}

	while((files=readdir(directory))){
		if(*(files->d_name) == '.') continue;

		if(files->d_type == DT_DIR){
			/* the lexicon should not have valid sub-directories */
			continue;
		}
		
		denseRIV* temp = lexPull(files->d_name);
		/* if the vector has been encountered more than MINSIZE times
		 * then it should be statistically significant, and useful */
		if(temp->contextSize >MINSIZE){
			(*fileRIVs) = (sparseRIV*)realloc((*fileRIVs), ((*fileCount)+1)*sizeof(sparseRIV));
			(*fileRIVs)[(*fileCount)] = normalize(*temp, 500);
			strcpy((*fileRIVs)[(*fileCount)].name, files->d_name);
			(*fileCount)++;
		}
		free(temp);
	}
}

void intercompare(struct DBnode* DBset, int nodeCount){
	double cosine;
	denseRIV baseDense;
	for(int i=0; i<nodeCount; i++){
		/* map the RIV in question to a dense for comparison */
		memset(baseDense.values, 0, RIVSIZE*sizeof(int));
		addS2D(baseDense.values, DBset[i].RIV);
		baseDense.magnitude = DBset[i].RIV.magnitude;
		/* for each previous vector */
		for(int j=i+1; j<nodeCount; j++){
				/* get cosine distance to that vector */
				cosine = cosCompare(baseDense, DBset[j].RIV);

			/* if this pair is close enough */
			if(cosine>EPSILON){
				
				/* add the pairing to each node's list of neighbors */
				DBset[i].neighbors = realloc(DBset[i].neighbors, (DBset[i].neighborCount+1)*sizeof(struct DBnode*));
				DBset[j].neighbors = realloc(DBset[j].neighbors, (DBset[j].neighborCount+1)*sizeof(struct DBnode*));
				
				DBset[i].neighbors[DBset[i].neighborCount++] = &DBset[j];
				DBset[j].neighbors[DBset[j].neighborCount++] = &DBset[i];
			}
		}
	}
}
