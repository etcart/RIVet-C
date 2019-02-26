#ifndef RIVLOWER_H_
#define RIVLOWER_H_

#include "RIVaccessories.h"

/* RIVSIZE macro defines the dimensionality off the RIVs we will use
 * 25000 is the standard, but can be redefined specifically
 */
#ifndef RIVSIZE
#define RIVSIZE 25000
#endif

#if RIVSIZE<4
#error "RIVSIZE must be a positive number, greater than 4 (preferably a large positive)"
#endif

/* NONZeros macro defines the number of non-zero values that will be generated
 * for any level one (barcode) RIV.  2 is simple and lightweight to begin
 */
#ifndef NONZEROS
#define NONZEROS 2
#endif

#if NONZEROS%2 || NONZEROS<1
#error "NONZEROS must be an even, greater than 0 number"
#endif


/* CACHESIZE macro defines the number of RIVs the system will cache.
 * a larger cache means more memory consumption, but will also be significantly
 * faster in aggregation and reading applications. doesn't affect systems
 * that do not use lexpull/push
 */
#ifndef CACHESIZE
#define CACHESIZE 10000
#endif

#if CACHESIZE<0
#error "CACHESIZE cannot be a negative number"
#endif

/* the size of the tempBlock used in consolidation and implicit RIVs */
#define TEMPSIZE 3*RIVSIZE


/* the sparseRIV is a RIV form optimized for RIVs that will be mostly 0s
 * as this is often an ideal case, it is adviseable as the default 
 * unless we are doing long term RIV aggregation.
 * specifically, a sparseRIV contains a pair of arrays, 
 * containing locations and values, where pairs are found in like array 
 * indices.
 */
 
typedef struct sparseRIV{
	char name[100];
	int *values;
	
	size_t count;
	int frequency;
	int contextSize;
	float magnitude;
	int locations[];
}sparseRIV;
/* the denseRIV is a RIV form optimized for overwhelmingly non-0 vectors
 * this is rarely the case, but its primary use is for performing vector
 * math, as comparisons and arithmetic between vectors are ideally 
 * performed between sparse and dense (hetero-arithmetic)
 */
typedef struct denseRIV{
	char name[100];
	void* cached;
	int frequency;
	int contextSize;
	float magnitude;
	int values[RIVSIZE];
}denseRIV;

int tempBlock[TEMPSIZE];

/*consolidateD2S takes a denseRIV value-set input, and returns a sparse RIV with
 * all 0s removed. it does not automatically carry metadata, which must be assigned
 * to a denseRIV after the fact.  often denseRIVs are only temporary, and don't
 * contain any metadata
 */
sparseRIV* consolidateD2S(int *denseInput);  //#TODO fix int*/denseRIV confusion

/* makeSparseLocations must be called repeatedly in the processing of a 
 * file to produce a series of locations from the words of the file
 * this produces an "implicit" RIV which can be used with the mapI2D function
 * to create a denseRIV.
 */
void makeSparseLocations(char* word,  int *seeds, int seedCount);

/* adds the barcode (L1) vector of a word to a denseVector */
void addBarcodeToDense(int* base, char* word);

/*subtracts a words vector from its own context.  regularly used in lex building
 */
void subtractThisWord(denseRIV* vector);

/* begin definitions */


sparseRIV* sparseAllocate(int valueCount){
	sparseRIV* output = malloc(sizeof(sparseRIV)+(valueCount*2*sizeof(int)));
	if(!output) return NULL;
	output->values = output->locations+valueCount;
	output->count = valueCount;
	return output;
	
}

sparseRIV* consolidateD2S(int *denseInput){
	sparseRIV* output;
	int count = 0;
	/* key/value pairs will be loaded to a worst-case sized temporary slot */
	int* locations = tempBlock+RIVSIZE;
	int* values = locations+RIVSIZE;
	int* locations_slider = locations;
	int* values_slider = values;
	for(int i=0; i<RIVSIZE; i++){
		
		/* act only on non-zeros */
		if(denseInput[i]){
			
			/* assign index to locations */
			*(locations_slider++) = i;
			
			/* assign value to values */
			*(values_slider++) = denseInput[i];
			
			/* track size of forming sparseRIV */
			count++;
		}
	}
	/* a slot is opened for the locations/values pair */
	
	output = sparseAllocate(count);
	if(!output){
		printf("memory allocation failed"); //*TODO enable fail point knowledge and security
	}
	/* copy locations values into opened slot */
	memcpy(output->locations, locations, output->count*sizeof(int));
	
	/* copy values into opened slot */
	memcpy(output->values, values, output->count*sizeof(int));
	
	return output;
}



void makeSparseLocations(char* word,  int *locations, int count){
	locations+=count;
	srand(wordtoSeed(word));
	int *locations_stop = locations+NONZEROS;
	while(locations<locations_stop){
		/* unrolled for speed, guaranteed to be an even number of steps */
		*locations = rand()%RIVSIZE;
		locations++;
		*locations = rand()%RIVSIZE;
		locations++;
	}
	return;
}

sparseRIV* sparseAllocateFormatted(){
	sparseRIV* output = (sparseRIV*)calloc(1, sizeof(sparseRIV));
	
	
	
	
	return output;
}
void subtractThisWord(denseRIV* vector){
	//set the rand() seed to the word
	srand(wordtoSeed(vector->name));
	/* the base word vector is composed of NONZERO (always an even number)
	 * +1s and -1s at "random" points (defined by the above seed.
	 * if we invert it to -1s and +1s, we have subtraction */
	
	for(int i = 0; i < NONZEROS; i+= 2){
		vector->values[rand()%RIVSIZE] -= 1;
		vector->values[rand()%RIVSIZE] += 1;	
	}
	/* record a context size 1 smaller */
	vector->contextSize-= 1;
	
}
void subtractThisWordPacked(denseRIV* vector){
	//set the rand() seed to the word
	srand(wordtoSeed(vector->name));
	/* the base word vector is composed of NONZERO (always an even number)
	 * +1s and -1s at "random" points (defined by the above seed.
	 * if we invert it to -1s and +1s, we have subtraction */
	int value;
	for(int i=0; i<NONZEROS; i++){
		value = rand()%2;
		if(!value) value = -1;
		vector->values[rand()%RIVSIZE] -= value;
	}
	/* record a context size 1 smaller */
	vector->contextSize-= 1;
	
}

void addBarcodeToDense(int* base, char* word){
	srand(wordtoSeed(word));
	int value;
	for(int i=0; i<NONZEROS; i++){
		value = rand()%2;
		if(!value) value = -1;
		base[rand()%RIVSIZE] += value;
	}
}

int BOWcursor = 0;

int getBOWIndex(char* word, RIVtree* root){
	
	int* index = treeSearch(root, word);
	if(!index){
	
		index = malloc(sizeof(int));
		*index = BOWcursor++;
		treeInsert(root, word, index);
	}
	
	return *index;
	
	
}

#endif

