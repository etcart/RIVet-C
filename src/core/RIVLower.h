#ifndef RIVLOWER_H_
#define RIVLOWER_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include "RIVaccessories.h"
#include "assert.h"
/* RIVSIZE macro defines the dimensionality off the RIVs we will use
 * 10000 is the standard, but can be redefined specifically
 */
#ifndef RIVSIZE
#define RIVSIZE 10000
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

#if NONZEROS<1
#error "NONZEROS must be greater than 0"
#endif


/* CACHESIZE macro defines the number of RIVs the system will cache.
 * a larger cache means more memory consumption, but will also be significantly
 * faster in aggregation and reading applications. doesn't affect systems
 * that do not use lexPull/Push
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
typedef struct{
	char name[100];
	int *values;
	int *locations;
	size_t count;
	int frequency;
	int contextSize;
	float magnitude;
}sparseRIV;

/* the denseRIV is a RIV form optimized for overwhelmingly non-0 vectors
 * this is rarely the case, but its primary use is for performing vector
 * math, as comparisons and arithmetic between vectors are ideally 
 * performed between sparse and dense (hetero-arithmetic)
 */
typedef struct{
	char name[100];
	void* cached;
	int frequency;
	int contextSize;
	float magnitude;
	int values[RIVSIZE];
}denseRIV;

/* used internally for a few different purposes, when a temporary staging
 * block is needed: consolidateD2S(), textToL2(), and saturationForStaging()
 */
int h_tempBlock[TEMPSIZE];

/* consolidateD2S takes a denseRIV value-set input, and returns a sparse RIV with
 * all 0s removed. it does not automatically carry metadata, which must be assigned
 * to a denseRIV after the fact.  often denseRIVs are only temporary, and don't
 * contain any metadata
 */
sparseRIV consolidateD2S(int *denseInput);  //#TODO fix int*/denseRIV confusion

void addBarcode2Dense(int* denseVector, char* word);

/* highly optimized method for adding vectors.  there is no method 
 * included for adding S2S, as this system is faster-enough
 * to be worth using
 */
void addS2D(int* destination, sparseRIV input);

/* simpler method for adding two dense-vectors 
 */
void addD2D(denseRIV* destination, denseRIV* source);

/*subtracts a words vector from its own context.  regularly used in lex building
 */
void subtractThisWord(denseRIV* vector);


/* begin definitions */

void addS2D(int* destination, sparseRIV input){
	
	int *locations_slider = input.locations;
	int *values_slider = input.values;
	int *locations_stop = locations_slider+input.count;
	
	/* apply values at an index based on locations */
	while(locations_slider<locations_stop){
		
		destination[*locations_slider] += *values_slider;
		locations_slider++;
		values_slider++;
	}
}

void addD2D(denseRIV* destination, denseRIV* source){
	
	int* target = destination->values+RIVSIZE;
	int* value = source->values+RIVSIZE;
	/* scan the pair, summing their values in "destination" */
	while(value>source->values){
		*(target--)+= *(value--);
	}	
}

sparseRIV consolidateD2S(int *denseInput){
	sparseRIV output;
	output.count = 0;
	/* key/value pairs will be loaded to a worst-case sized temporary slot */
	int* locations = h_tempBlock+RIVSIZE;
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
			output.count++;
		}
	}
	/* a slot is opened for the locations/values pair */
	
	output.locations = (int*) malloc(output.count*2*sizeof(int));
	if(!output.locations){
		fprintf(stderr, "memory allocation failed in RIV consolidation");
	}
	/* copy locations values into opened slot */
	memcpy(output.locations, locations, output.count*sizeof(int));
	
	output.values = output.locations + output.count;
	
	/* copy values into opened slot */
	memcpy(output.values, values, output.count*sizeof(int));
	
	return output;
}
void addBarcode2Dense(int* denseVector, char* word){
	/* create a rand() seed from the word to be added */
	srand(wordToSeed(word));
	
	for(int i=0; i<NONZEROS; i++){
		/* generate a +1 or a -1 */
		int value = rand()%2;
		if (!value){
			value = -1;
		}
		/* add that at a random point in the vector */
		denseVector[ rand()%RIVSIZE ] += value;
		
	}
}
void subtractBarcodeFromDense(denseRIV* vector, char* word){
	/* create a rand() seed from the word to be added */
	srand(wordToSeed(word));
		
	for(int i=0; i<NONZEROS; i++){
		/* generate a +1 or a -1 */
		int value = rand()%2;
		if (!value){
			value = -1;
		}
		/* add that at a random point in the vector */
		vector->values[ rand()%RIVSIZE ] -= value;
		
	}
	/* record a context size 1 smaller */
	vector->contextSize-= 1;
	
}

#endif

