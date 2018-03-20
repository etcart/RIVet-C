#ifndef RIVLOWER_H_
#define RIVLOWER_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
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
	double magnitude;
	int boolean;
	int contextSize;
}sparseRIV;
/* the denseRIV is a RIV form optimized for overwhelmingly non-0 vectors
 * this is rarely the case, but its primary use is for performing vector
 * math, as comparisons and arithmetic between vectors are ideally 
 * performed between sparse and dense (hetero-arithmetic)
 */
typedef struct{
	char name[100];
	int* values;
	int* frequency;
	double magnitude;
	int cached;
	int *contextSize;
}denseRIV;

/*RIVKey, holds globally important data that should not be changed partway through
* first function call in the program should always be: 
* RIVinit();
* this will set these variables, check for incompatible choices, and open up 
* memory blocks which the system will use in the background
*/
struct RIVData{
	int h_tempBlock[TEMPSIZE];
	int tempSize;
	char lexName[255];
	denseRIV RIVCache[CACHESIZE];
}static RIVKey;

/* RIVOpen should be the first function called in any usage of this library
 * it sets global variables that practically all functions will reference,
 * it checks that your base parameters are valid, and allocates memory for
 * the functions to use, so that we can move fast with rare allocations.
 */
void RIVOpen();

/* RIVCleanup should always be called to close a RIV program.  it frees 
 * blocks allocated by RIVinit, and dumps the cached data to appropriate lexicon files
 */
void RIVClose();

/*consolidateD2S takes a denseRIV value-set input, and returns a sparse RIV with
 * all 0s removed. it does not automatically carry metadata, which must be assigned
 * to a denseRIV after the fact.  often denseRIVs are only temporary, and don't
 * need to carry metadata
 */
sparseRIV consolidateD2S(int *denseInput);  //#TODO fix int*/denseRIV confusion

/* mapS2D expands a sparseRIV out to denseRIV values, filling array locations
 * based on location-value pairs 
 */


/* makeSparseLocations must be called repeatedly in the processing of a 
 * file to produce a series of locations from the words of the file
 * this produces an "implicit" RIV which can be used with the mapI2D function
 * to create a denseRIV.
 */
void makeSparseLocations(unsigned char* word,  int *seeds, size_t seedCount);

/* fLexPush pushes the data contained in a denseRIV out to a lexicon file,
 * saving it for long-term aggregation.  function is called by "lexpush",
 * which is what users should actually use.  lexPush, unlike fLexPush,
 * has cache logic under the hood for speed and harddrive optimization
 */
int fLexPush(denseRIV RIVout);

denseRIV fLexPull(FILE* lexWord);

/* creates a standard seed from the characters in a word, hopefully unique */
int wordtoSeed(unsigned char* word);

/* mapI2D maps an "implicit RIV" that is, an array of index values, 
 * arranged by chronological order of generation (as per makesparseLocations)
 * it assigns, in the process of mapping, values according to ordering
 */
int* mapI2D(int *locations, size_t seedCount);

int* addS2D(int* destination, sparseRIV input);

sparseRIV consolidateI2SIndirect(int *implicit, size_t valueCount);
sparseRIV consolidateI2SDirect(int *implicit, size_t valueCount);
int cacheDump();
int* addI2D(int* destination, int* locations, size_t seedCount);
denseRIV denseAllocate();
void signalSecure(int signum, siginfo_t *si, void* arg);
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

sparseRIV consolidateI2SIndirect(int *implicit, size_t valueCount){
	int *denseTemp = mapI2D(implicit, valueCount);
	
	sparseRIV sparseOut = consolidateD2S(denseTemp);
	
	free(denseTemp);
	
	
	return sparseOut;
	
	
}
sparseRIV consolidateI2SDirect(int *implicit, size_t valueCount){
	sparseRIV sparseOut;
	int *locationsTemp = RIVKey.h_tempBlock+RIVSIZE;
	int *valuesTemp = RIVKey.h_tempBlock+2*RIVSIZE;
	sparseOut.count = 0;
	int add = 1;
	int found;
	for(int i=0; i<valueCount; i++){
		found = 0;
		for(int j=0; j<sparseOut.count; j++){
			if(implicit[i] == locationsTemp[j]){
				valuesTemp[i] += add;
				add *= -1;
				found = 1;
			}
		}
		if(!found){
			locationsTemp[sparseOut.count] = implicit[i];
			
			valuesTemp[sparseOut.count] = add;
			sparseOut.count++;
			add*= -1;
		}
	}
	sparseOut.locations = (int*)malloc(2*sparseOut.count*sizeof(int));
	sparseOut.values = sparseOut.locations+sparseOut.count;
	memcpy(sparseOut.locations, locationsTemp, sparseOut.count*sizeof(int));
	memcpy(sparseOut.values, valuesTemp, sparseOut.count*sizeof(int));
	return sparseOut;
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
		printf("memory allocation failed"); //*TODO enable fail point knowledge
	}
	/* copy locations values into opened slot */
	memcpy(output.locations, locations, output.count*sizeof(int));
	
	output.values = output.locations + output.count;
	
	/* copy values into opened slot */
	memcpy(output.values, values, output.count*sizeof(int));
	
	return output;
}


void lexOpen(char* lexName){
	/* RIVKey.I2SThreshold = sqrt(RIVSIZE);*/ //deprecate?
	struct stat st = {0};
	if (stat(lexName, &st) == -1) {
		mkdir(lexName, 0777);
	}	
	strcpy(RIVKey.lexName, lexName);
	/* open a slot at least large enough for worst case handling of
	 * sparse to dense conversion.  may be enlarged by filetoL2 functions */
	struct sigaction action;
	action.sa_sigaction = signalSecure;
	action.sa_flags = SA_SIGINFO;
	//for(int i=1; i<27; i++){
		sigaction(11,&action,NULL);
	//}
	 

	/* open a slot for a cache of dense RIVs, optimized for frequent accesses */
	memset(RIVKey.RIVCache, 0, sizeof(denseRIV)*CACHESIZE);
}
void lexClose(){
	
	 
	if(cacheDump()){
		puts("cache dump failed, some lexicon data was lost");
	}
}
int wordtoSeed(unsigned char* word){
	int i=0;
	int seed = 0;
	while(*word){
		/* left-shift 5 each time *should* make seeds unique to words
		 * this means letters are taken as characters couned in base 32, which
		 * should be large enough to hold all english characters plus a few outliers
		 * */
		seed += (*(word))<<(i*5);
		word++;
		i++;
	}
	return seed;
}

void makeSparseLocations(unsigned char* word,  int *locations, size_t count){
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

int fLexPush(denseRIV RIVout){	
	char pathString[200] = {0};

	/* word data will be placed in a (new?) file under the lexicon directory
	 * in a file named after the word itself */
	sprintf(pathString, "%s/%s", RIVKey.lexName, RIVout.name);
	FILE *lexWord = fopen(pathString, "wb");

	if(!lexWord){
		printf("lexicon push has failed for word: %s\nconsider cleaning inputs", pathString);
		return 1;
	}

	sparseRIV temp = consolidateD2S(RIVout.values);
	if(temp.count<(RIVSIZE/2)){
		/* smaller stored as sparse vector */

		fwrite(&temp.count, 1, sizeof(size_t), lexWord);
		fwrite(RIVout.frequency, 1, sizeof(int), lexWord);
		fwrite(RIVout.contextSize, 1, sizeof(int), lexWord);
		fwrite(&RIVout.magnitude, 1, sizeof(float), lexWord);
		fwrite(temp.locations, temp.count, sizeof(int), lexWord);
		fwrite(temp.values, temp.count, sizeof(int), lexWord);
	//	printf("%s, writing as sparse, frequency: %d", RIVout.name, *RIVout.frequency);
	}else{
		/* saturation is too high, better to store dense */
		/* there's gotta be a better way to do this */
		temp.count = 0;
		fwrite(&temp.count, 1, sizeof(size_t), lexWord);
		fwrite(RIVout.frequency, 1, sizeof(int), lexWord);
		fwrite(RIVout.contextSize, 1, sizeof(int), lexWord);
		fwrite(&RIVout.magnitude, 1, sizeof(float), lexWord);
		fwrite(RIVout.values, RIVSIZE, sizeof(int), lexWord);
	//	printf("%s, writing as dense, frequency: %d", RIVout.name, *RIVout.frequency);
	}

	fclose(lexWord);
	free(RIVout.values);
	free(temp.locations);

	return 0;
}

denseRIV fLexPull(FILE* lexWord){
	denseRIV output = denseAllocate();
	size_t typeCheck;
	/* get metadata for vector */
	fread(&typeCheck, 1, sizeof(size_t), lexWord);
	fread(output.frequency, 1, sizeof(int), lexWord);
	fread(output.contextSize, 1, sizeof(int), lexWord);
	fread(&(output.magnitude), 1, sizeof(float), lexWord);

	/* first value stored is the value count if sparse, and 0 if dense */
	if (typeCheck){
		/* pull as sparseVector */
		sparseRIV temp;
		/* value was not 0, so it's the value count */
		temp.count = typeCheck;

		temp.locations = (int*)malloc(temp.count*2*sizeof(int));
		temp.values = temp.locations+temp.count;
		fread(temp.locations, temp.count, sizeof(int), lexWord);
		fread(temp.values, temp.count, sizeof(int), lexWord);

		addS2D(output.values, temp);
		free(temp.locations);
	}else{
		/* typecheck is thrown away, just a flag in this case */
		fread(output.values, RIVSIZE, sizeof(int), lexWord);
	}


	output.cached = 0;

	return output;

}

void signalSecure(int signum, siginfo_t *si, void* arg){
  if(cacheDump()){
	  puts("cache dump failed, some lexicon data lost");
  }else{
	puts("cache dumped successfully");
  }
  signal(signum, SIG_DFL);
  kill(getpid(), signum);
}

int cacheDump(){
	int i=0;
	int j=0;
	int flag = 0;
	denseRIV* cache_slider = RIVKey.RIVCache;
	denseRIV* cache_stop = RIVKey.RIVCache+CACHESIZE;
	while(cache_slider<cache_stop){
		if((*cache_slider).cached){

			j++;
			flag += fLexPush(*cache_slider);
		}
		else{
			i++;
		}
		cache_slider++;
	}
	printf("%d cacheslots unused\n%d, cacheslots used", i, j);
	return flag;
}

denseRIV denseAllocate(){
	/* allocates a 0 vector */
	denseRIV output;
	output.values = (int*)calloc(RIVSIZE+2, sizeof(int));
	/* for compact memory use, frequency is placed immediately after values */
	output.frequency = output.values+RIVSIZE;
	output.contextSize = output.frequency+1;
	output.magnitude = 0;
	output.cached = 0;
	return output;
}

/*TODO add a simplified free function*/

#endif

