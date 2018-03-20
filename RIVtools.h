#ifndef RIVTOOLS_H_
#define RIVTOOLS_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "RIVLower.h"
#include "RIVaccessories.h"



/* lexPush writes a denseRIV to a file for permanent storage */
int lexPush(denseRIV RIVout);
/* lexPull reads an existing lexicon entry (under directory "lexicon")
 * and creates a denseRIV with those attributes.
 * if the file does not exist, it creates a 0 vector with the name of word
 */
denseRIV lexPull(char* word);

/* fileToL2 takes an input file, reads words (delimiting on " " and "\n") 
 * and returns a sparse RIV which is the vector sum of the base RIVs of each 
 * word contained
 */
sparseRIV fileToL2(FILE *input);

/* fileToL2Clean operates the same as fileToL2 butkeeps only words 
 * containing lowercase letters and the '_' symbol
 * this is important if you will be lexPush-ing those words later
 */
sparseRIV fileToL2Clean(FILE *data);

/*filetoL2direct is an experiment in simplifying the process.  it's slow */
sparseRIV fileToL2direct(FILE *data);

/*cosine determines the "similarity" between two RIVs. */
double cosCompare(denseRIV baseRIV, sparseRIV comparator);

/*currently unused */
sparseRIV wordtoL2(char* word);

/* converts an implicit RIV (a set of unvalued locations) into a formal 
 * sparse RIV.  this chooses the best method to perform the consolidation
 * and launches that function */
sparseRIV consolidateI2S(int *implicit, size_t valueCount);

sparseRIV normalizeFloored(denseRIV input, int factor);

sparseRIV normalize(denseRIV input, int factor);


int roundMultiply(int base, float divisor);
/* like fileToL2 but takes a block of text */
sparseRIV text2L2(char *text);
double getMagnitudeSparse(sparseRIV input);

sparseRIV text2L2(char *text){
	int wordCount = 0;
	unsigned char word[100] = {0};

	int denseTemp[RIVSIZE] = {0};
	/* locations (implicit RIV) are temp stored in temp block, and moved 
	 * to permanent home in consolidation */
	int *locations = RIVKey.h_tempBlock;
	int locationCount = 0;
	int displacement;

	while(sscanf(text, "%99s%n", word, &displacement)){
		text += displacement+1;
		if(!displacement){
			break;
		}

		if(!(*word)){
			break;
		}

		/* if this word would overflow the locations block, map it to the denseVector */
		if((locationCount+NONZEROS)>TEMPSIZE){
			addI2D(denseTemp, locations, locationCount);
			locationCount = 0;
		}
		/* add word's L1 RIV to the accumulating implicit RIV */
		makeSparseLocations(word, locations, locationCount);
		locationCount+= NONZEROS;
		wordCount++;
	}
	/* map remaining locations to the denseTemp */
	addI2D(denseTemp, locations, locationCount);
	sparseRIV output = consolidateD2S(denseTemp);

	/* frequency records the number of words in this file, untill frequency
	 * is needed to hold some more useful data point */
	output.frequency = wordCount;
	output.boolean = 1;
	return output;
}

sparseRIV fileToL2(FILE *data){
	unsigned char word[100] = {0};

	/* locations (implicit RIV) are temp stored in temp block, and moved
	 * to permanent home in consolidation */
	int *locations = RIVKey.h_tempBlock;
	int locationCount = 0;
	int denseTemp[RIVSIZE] = {0};
	int wordCount = 0;
	while(fscanf(data, "%99s", word)){

		if(feof(data)){
			break;
		}
		if(!(*word)){
			break;
		}

		/* if this word would overflow the locations block, map it to the denseVector */
		if((locationCount+NONZEROS)>TEMPSIZE){
			addI2D(denseTemp, locations, locationCount);
			locationCount = 0;
		}
		/* add word's L1 RIV to the accumulating implicit RIV */
		makeSparseLocations(word, locations, locationCount);
		locationCount+= NONZEROS;
		wordCount++;
	}
	/* map remaining locations to the denseTemp */
	addI2D(denseTemp, locations, locationCount);
	sparseRIV output = consolidateD2S(denseTemp);

	/* frequency records the number of words in this file */
	output.frequency = wordCount;
	output.boolean = 1;

	return output;
}

sparseRIV fileToL2Clean(FILE *data){

	int denseTemp[RIVSIZE] = {0};
	unsigned char word[100] = {0};
	int *locations = RIVKey.h_tempBlock;
	unsigned int wordCount = 0;

	int locationCount = 0;
	while(fscanf(data, "%99s", word)){

		if(feof(data)){
			break;
		}

		if(!(*word)){
			break;
		}
		/* if the word is not clean, skip it */
		if(!isWordClean((char*)word)){
			continue;
		}
		/* if this word would overflow the locations block, map it to the denseVector */
		if((locationCount+NONZEROS)>TEMPSIZE){
			addI2D(denseTemp, locations, locationCount);
			locationCount = 0;
		}
		/* add word's L1 RIV to the accumulating implicit RIV */
		makeSparseLocations(word, locations, locationCount);
		locationCount+= NONZEROS;
		wordCount++;
	}
	/* map remaining locations to the denseTemp */
	addI2D(denseTemp, locations, locationCount);
	sparseRIV output = consolidateD2S(denseTemp);

	/* frequency records the number of words in this file */
	output.frequency = locationCount/NONZEROS;
	output.boolean = 1;
	return output;
}

/*sparseRIV consolidateI2S(int *implicit, size_t valueCount){
	if(valueCount<RIVKey.I2SThreshold){
		 //direct method is faster on small datasets, but has geometric scaling on large datasets 
		return consolidateI2SDirect(implicit, valueCount);
	}else{
		// optimized for large datasets 
		return consolidateI2SIndirect(implicit, valueCount);
	}

}*/
void aggregateWord2D(denseRIV destination, char* word){


	srand(wordtoSeed((unsigned char*)word));
	for(int i=0; i<NONZEROS; i++){

		destination.values[(rand()%RIVSIZE)] +=1;
		destination.values[(rand()%RIVSIZE)] -= 1;
	}
}

double cosCompare(denseRIV baseRIV, sparseRIV comparator){

	int dot = 0;
	int n = comparator.count;
	while(n){
		n--;
		/* we calculate the dot-product to derive the cosine 
		 * comparing sparse to dense by index*/
		//dot += values[i]*baseRIV.values[locations[i]];
		dot += comparator.values[n] * baseRIV.values[comparator.locations[n]];

		//printf("%d, %d, %d\n",baseRIV.values[comparator.locations[n]],comparator.values[n] , n);

	}
	/*dot divided by product of magnitudes */

	return dot/(baseRIV.magnitude*comparator.magnitude);
}

double getMagnitudeSparse(sparseRIV input){
	size_t temp = 0;
	int *values = input.values;
	int *values_stop = values+input.count;
	while(values<values_stop){
		temp += (*values)*(*values);
		if(temp> 0x0AFFFFFFFFFFFFFF) printf("%s, fuuuuuuuuuuuuck*****************************************",input.name );
		values++;
	}
	return sqrt(temp);
}

denseRIV lexPull(char* word){
	#if CACHESIZE > 0

	/* if there is a cache, first check if the word is cached */
	srand(wordtoSeed((unsigned char*)word));
	int hash = rand()%CACHESIZE;
	if(!strcmp(word, RIVKey.RIVCache[hash].name)){

		/* if word is cached, pull from cache and exit */
		return RIVKey.RIVCache[hash];
	}
	#endif /* CACHESIZE > 0 */

	/* if not, attempt to pull the word data from lexicon file */
	denseRIV output;

	char pathString[200];

	sprintf(pathString, "%s/%s", RIVKey.lexName, word);
	FILE *lexWord = fopen(pathString, "rb");

	/* if this lexicon file already exists */
	if(lexWord){
		/* pull data from file */
		output = fLexPull(lexWord);
		fclose(lexWord);
	}else{
		/*if file does not exist, return a 0 vector (word is new to the lexicon */ //#TODO enable NO-NEW features to protect mature lexicons? 
		output = denseAllocate();
	}

	strcpy(output.name, word);
	return output;
}
int lexPush(denseRIV RIVout){
	#if CACHESIZE == 0
	/* if there is no cache, simply push to file */
	fLexPush(RIVout);
	return 0;
	#else /* CACHESIZE != 0 */

	/* if our RIV was cached, there are two options (hopefully)
	 * either the RIV is still cached, and the data has been updated 
	 * to the cache or the RIV was pushed out from under it, 
	 * in which case it has already been pushed! move on*/

	if(RIVout.cached){
		return 0;
	}

	srand(wordtoSeed((unsigned char*)RIVout.name));
	int hash = rand()%CACHESIZE;

	if(!RIVKey.RIVCache[hash].cached){
		/* if there is no word in this cache slot, push to cache instead of file */
		RIVKey.RIVCache[hash] = RIVout;
		RIVKey.RIVCache[hash].cached = 1;
		return 0;
	/*if the current RIV is more frequent than the RIV holding its slot */
	}else if(*(RIVout.frequency) > *(RIVKey.RIVCache[hash].frequency) ){
		/* push the current cache entry to a file */
		int diag = fLexPush(RIVKey.RIVCache[hash]);
		/* push the current RIV to cache */

		RIVKey.RIVCache[hash] = RIVout;
		RIVKey.RIVCache[hash].cached = 1;
		return diag;
	}else{
		/* push current RIV to file */
		fLexPush(RIVout);
	}
	return 0;
	#endif /* CACHESIZE == 0 */

}
sparseRIV fileToL2direct(FILE *data){;
	unsigned char word[100] = {0};
	denseRIV denseTemp;
	// a temporary dense RIV is stored in the tempBlock 
	denseTemp.values = RIVKey.h_tempBlock;
	memset(RIVKey.h_tempBlock, 0, RIVSIZE*sizeof(int));
	int count = 0;
	while(fscanf(data, "%99s", word)){
		count++;
		if(feof(data)){
			break;
		}
		if(!(*word)){
			break;
		}
		
		
		// add word's L1 RIV to the accumulating implicit RIV 
		aggregateWord2D(denseTemp, (char*)word);
		
	}
	sparseRIV output = consolidateD2S(denseTemp.values);
	
	// frequency records the number of words in this file 
	output.frequency = count;
	output.boolean = 1;
	return output;
}

sparseRIV normalizeFloored(denseRIV input, int factor){
	float divisor = (float)factor/(*input.contextSize);
//	printf("in norm: %d, %d, %f\n", *input.contextSize, factor, divisor);
	int* locations = RIVKey.h_tempBlock;
	int* values = locations+RIVSIZE;
	int count = 0;
	for(int i=0; i<RIVSIZE; i++){
		if(!input.values[i]) continue;
		locations[count] = i;
		values[count]= input.values[i]*divisor;
		if(values[count])count++;
	}
	sparseRIV output;
	output.locations = (int*) malloc(count*2*sizeof(int));
	output.values = output.locations+count;
	memcpy(output.locations, locations, count*sizeof(int));
	memcpy(output.values, values, count*sizeof(int));
	strcpy(output.name, input.name);
	output.count = count;
	output.magnitude = getMagnitudeSparse(output);
	output.contextSize = *input.contextSize;
	output.frequency = *input.frequency;

	return output;
}

sparseRIV normalize(denseRIV input, int factor){
	float divisor = (float)factor/(*input.contextSize);
//	printf("in norm: %d, %d, %f\n", *input.contextSize, factor, divisor);
	int* locations = RIVKey.h_tempBlock;
	int* values = locations+RIVSIZE;
	int count = 0;
	for(int i=0; i<RIVSIZE; i++){
		if(!input.values[i]) continue;
		locations[count] = i;
		values[count]= roundMultiply(input.values[i], divisor);
		if(values[count])count++;
	}
	sparseRIV output;
	output.locations = (int*) malloc(count*2*sizeof(int));
	output.values = output.locations+count;
	memcpy(output.locations, locations, count*sizeof(int));
	memcpy(output.values, values, count*sizeof(int));
	strcpy(output.name, input.name);
	output.count = count;
	output.magnitude = getMagnitudeSparse(output);
	output.contextSize = *input.contextSize;
	output.frequency = *input.frequency;
	
	return output;
}

int roundMultiply(int base, float divisor){
	float temp = base*divisor;
	int output = temp*2;
	if (output%2){
		output/=2;
		output+=1;
	}else{
		output/=2;
	}
	return output;

}


#endif
