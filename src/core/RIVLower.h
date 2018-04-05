#ifndef RIVLOWER_H_
#define RIVLOWER_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include "RIVaccessories.h"
/* RIVSIZE macro defines the dimensionality off the RIVs we will use
 * 25000 is the standard, but can be redefined specifically
 */
#ifndef RIVSIZE
#define RIVSIZE 25000
#endif

#if RIVSIZE<0
#error "RIVSIZE must be a positive number (preferably a large positive)"
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
#define CACHESIZE 20
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
	double magnitude;
	int contextSize;
	int frequency;
}sparseRIV;
/* the denseRIV is a RIV form optimized for overwhelmingly non-0 vectors
 * this is rarely the case, but its primary use is for performing vector
 * math, as comparisons and arithmetic between vectors are ideally 
 * performed between sparse and dense (hetero-arithmetic)
 */
typedef struct{
	int cached;
	char name[100];
	int frequency;
	double magnitude;
	int contextSize;
	int values[RIVSIZE];
}denseRIV;

/*RIVKey, holds global variables used under the hood, primarily for the lexicon
 * it also holds a "temp block" that will be used by the dense to sparse 
 * conversion and implicit RIV aggregation 
*/
struct RIVData{
	int h_tempBlock[TEMPSIZE];
	int tempSize;
	char lexName[255];
	denseRIV* RIVCache[CACHESIZE];
}static RIVKey;

/*consolidateD2S takes a denseRIV value-set input, and returns a sparse RIV with
 * all 0s removed. it does not automatically carry metadata, which must be assigned
 * to a denseRIV after the fact.  often denseRIVs are only temporary, and don't
 * contain any metadata
 */
sparseRIV consolidateD2S(int *denseInput);  //#TODO fix int*/denseRIV confusion

/* makeSparseLocations must be called repeatedly in the processing of a 
 * file to produce a series of locations from the words of the file
 * this produces an "implicit" RIV which can be used with the mapI2D function
 * to create a denseRIV.
 */
void makeSparseLocations(char* word,  int *seeds, size_t seedCount);

/* mapI2D maps an "implicit RIV" that is, an array of index values, 
 * arranged by chronological order of generation (as per makesparseLocations)
 * it assigns, in the process of mapping, values according to ordering
 */
int* mapI2D(int *locations, size_t seedCount);

/* highly optimized method for adding vectors.  there is no method 
 * included for adding D2D or S2S, as this system is faster-enough
 * to be more than worth using
 */
int* addS2D(int* destination, sparseRIV input);

/* caheDump flushes the RIV cache out to relevant files, backing up all 
 * data.  this is called by the lexClose and signalSecure functions
 */
int cacheDump();

/* adds all elements of an implicit RIV (a sparseRIV represented without values)
 * to a denseRIV.  used by the file2L2 functions in aggregating a document vector
 */
int* addI2D(int* destination, int* locations, size_t seedCount);

/* begin definitions */

int* addS2D(int* destination, sparseRIV input){// #TODO fix destination parameter vs calloc of destination
	
	int *locations_slider = input.locations;
	int *values_slider = input.values;
	int *locations_stop = locations_slider+input.count;
	
	/* apply values at an index based on locations */
	while(locations_slider<locations_stop){
		destination[*locations_slider] += *values_slider;
		locations_slider++;
		values_slider++;
	}
	
	return destination;
}

int* mapI2D(int *locations, size_t valueCount){// #TODO fix destination parameter vs calloc of destination
	int *destination = (int*)calloc(RIVSIZE,sizeof(int));
	int *locations_slider = locations;
	int *locations_stop = locations_slider+valueCount;

	/*apply values +1 or -1 at an index based on locations */
	while(locations_slider<locations_stop){
	
		destination[*locations_slider] +=1;
		locations_slider++;
		destination[*locations_slider] -= 1;
		locations_slider++;
	}

	return destination;
}
int* addI2D(int* destination, int *locations, size_t valueCount){// #TODO fix destination parameter vs calloc of destination
	int *locations_slider = locations;
	int *locations_stop = locations_slider+valueCount;

	/*apply values +1 or -1 at an index based on locations */
	while(locations_slider<locations_stop){
	
		destination[*locations_slider] +=1;
		locations_slider++;
		destination[*locations_slider] -= 1;
		locations_slider++;
	}
	
	
	return destination;
}



sparseRIV consolidateD2S(int *denseInput){
	sparseRIV output;
	output.count = 0;
	/* key/value pairs will be loaded to a worst-case sized temporary slot */
	int* locations = RIVKey.h_tempBlock+RIVSIZE;
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
		printf("memory allocation failed"); //*TODO enable fail point knowledge and security
	}
	/* copy locations values into opened slot */
	memcpy(output.locations, locations, output.count*sizeof(int));
	
	output.values = output.locations + output.count;
	
	/* copy values into opened slot */
	memcpy(output.values, values, output.count*sizeof(int));
	
	return output;
}



void makeSparseLocations(char* word,  int *locations, size_t count){
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
	

#endif

