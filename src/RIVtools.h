#ifndef RIVTOOLS_H_
#define RIVTOOLS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "core/RIVLower.h"
#include "core/RIVlexicon.h"
#include "core/RIVaccessories.h"

#define OVERFLOWCHECK 1L<<63

/* separates a block of text into tokens, and then aggregates L1 (barcode) vectors
 * into a vector representation of the text */
sparseRIV textToL2(char *text);

/* the same as textToL2, but simplified for reading entire files
 */
sparseRIV fileToL2(FILE *data);

/*cosine determines the "similarity" between two RIVs. */
double cosCompare(denseRIV *baseRIV, sparseRIV* comparator);

/* used for analysis of lexicon vectors (not simply accumulation)
 * to avoid overflow of even a 64 bit integer, vectors must be normalized
 * this is not a true normal.  magnitude is assigned as "context specificity"
 * and no original magnitude needs to be calculated, which avoids the 
 * issue of integer overflow in magnitude calculation */
sparseRIV normalize(denseRIV input, int factor);

/* this takes a sparse-vector intake, and outputs a dense-vector representation
 * of the same.  it carries metadata and frees the memory used by the input
 * for a non-destructive version, use "addS2D" with a 0 dense Vector */
denseRIV* convertS2D(sparseRIV* input);

/* this normalizes a vector, scaling it to have a magnitude equal to 
 * the second argument, within a rounding error from the integers used */
sparseRIV trueNormalize(denseRIV* input, double magnitude);

/* this sine distance function uses cosine under the hood, and outputs all
 * angles greater than 90 degrees as "1", rather than looping back around
 * to near zero numbers at near 180 */
double sine(denseRIV* baseDense, sparseRIV* comparator);

/* calculates the magnitude of a sparseVector */ //TODO further test integer overflow measures
double getMagnitudeSparse(sparseRIV input);

/* same for denseVector */
double getMagnitudeDense(denseRIV *input); //TODO consolidate these into one function

/* creates a L3 sum of L2 vectors for a line of text, for use in bag of concepts
 * classification
 */
sparseRIV line2L3(LEXICON* lexicon, char* text);

/* these two function together produce a set of sine intercomparisons
 * for an array of sparseRIVs, and do so multithreaded for speed */
void* cosineSetThread (void* args);
double** cosIntercompare(sparseRIV* vectorSet, size_t vectorCount);
	
sparseRIV textToL2(char *text){
	int wordCount = 0;
	char word[100] = {0};
	
	/* barcodes are accumulated in a temp block, and moved 
	 * to permanent home in consolidation */
	int* denseTemp = h_tempBlock;
	memset(denseTemp, 0, RIVSIZE);

	int displacement = 0;;
	char* textEnd = text+strlen(text)-1;	
	while(text<textEnd){
		sscanf(text, "%99s%n", word, &displacement);
		text += displacement+1;
		
		/*check for unreadable text conditions and escape */
		if(!displacement){
			break;
		}
		if(!(*word)){
			break;
		}
		
		/*add the L1 for this word to the accumulating vector */
		addBarcode2Dense(denseTemp, word);
		wordCount++;
		
	}
	sparseRIV output = consolidateD2S(denseTemp);

	/* contextSize stores the number of words read */
	output.contextSize = wordCount;
	return output;
}

sparseRIV fileToL2(FILE *data){
	char word[100] = {0};

	/* locations (implicit RIV) are temporarily stored in temp block, 
	 * and moved to permanent home in consolidation */
	 
	int* denseTemp = h_tempBlock;
	memset(denseTemp, 0, RIVSIZE);

	int wordCount = 0;
	while(fscanf(data, "%99s", word)){

		if(feof(data)){
			break;
		}
		if(!(*word)){
			break;
		}

		/* if this word would overflow the locations block, map it to the denseVector */
	
		addBarcode2Dense(denseTemp, word);
		
		wordCount++;
	}
	/* map remaining locations to the denseTemp */
	sparseRIV output = consolidateD2S(denseTemp);

	/* contextSize records the number of words in this file */
	output.contextSize = wordCount;
	fseek(data, 0, SEEK_SET);
	return output;
}


sparseRIV line2L3(LEXICON* lexicon, char* text){
	
	/* will contain the vector for each component word */
	denseRIV* wordRIV;
	/* will accumulate the sum of all word vectors */
	denseRIV accumulate = {0};
	int wordCount = 0;
	char* textEnd = text+strlen(text);
	char word[200];
	int displacement;
	
	while(text<textEnd){
		displacement = 0;
		sscanf(text, "%199s%n", word, &displacement);
		text += displacement+1;
		if(!displacement){
			break;
		}
		if(!(*word)){
			break;
		}
		
		wordRIV = lexPull(lexicon, word);
			
		if(!wordRIV){
			continue;
		}
		/* at this point the word has passed cleaning, stemming,
		 * and is contained in the lexicon.  we add it to the total */
		addD2D(&accumulate, wordRIV);

		/* push again. this will cache the word if optimal for easy reaccess */		
		lexPush(lexicon, wordRIV);
		wordCount++;	
	
	
	
	}
	/* a larger context size equate to a smaller vector on normalization */
	accumulate.contextSize = wordCount;
	
	
	if(wordCount > 1000){
		/* normalization is used primarily to avoid integer overflow in magnitude calculation */
	
		return normalize(accumulate, 1);
	}else{
		
		return consolidateD2S(accumulate.values);
	}
}

double cosCompare(denseRIV* baseRIV, sparseRIV* comparator){

	long long int dot = 0;
	int* locations_stop = comparator->locations+comparator->count;
	int* locations_slider = comparator->locations;
	int* values_slider = comparator->values;
	while(locations_slider<locations_stop){

		/* we calculate the dot-product to derive the cosine 
		 * comparing sparse to dense by index*/
		dot += *values_slider * baseRIV->values[*locations_slider];
		locations_slider++;
		values_slider++;

	}
	/*dot divided by product of magnitudes */

	return dot/(baseRIV->magnitude*comparator->magnitude);
}


double sine(denseRIV* baseDense, sparseRIV* comparator){
	double cos = cosCompare(baseDense, comparator); // true cosine, not 1-cosine
	if(cos < 0.0) return 1.0; //anything further than 90 degrees distance is "far"
	if(cos >= 1.0) return 0; //floating point errors need to be rounded off to prevent "-nan"
	return sqrt(1.0-(cos*cos));
}

double getMagnitudeSparse(sparseRIV input){
	size_t temp = 0;
	size_t accumulate = 0;
	double divisor = 1;
	int *values = input.values;
	int *values_stop = values+input.count;
	
	while(values<values_stop){
		/* we sum the squares of all elements */
		temp += ((size_t)*values)*((size_t)*values);
		values++;
		if(OVERFLOWCHECK & temp){
			/* if integer overflow is threatened, we accumulate overflow in
			 * another variable.  this costs accuracy, but not as much as
			 * an integer overflow. best to not let this happen at all */
			accumulate*= divisor/(divisor+1);
			divisor+=1;
			accumulate = temp/divisor;
			temp=0;	
			
		}
	}
	/* we take the root of that sum */
	accumulate += temp/divisor;
	return sqrt(accumulate)*sqrt(divisor);
}
denseRIV* convertS2D(sparseRIV* input){
	/* create a 0'd dense vector */
	denseRIV* output = calloc(1,sizeof(denseRIV));
	
	/* add the sparse-vector to the 0'd dense */
	addS2D(output->values, *input);
	
	/* carry metadata */
	strcpy(output->name, input->name);
	output->magnitude = input->magnitude;
	output->frequency = input->frequency;
	output->contextSize = input->contextSize;
	
	/* free (and record null pointers) the sparse data */
	free(input->locations);
	input->count =0;
	input->locations = NULL;
	input->values = NULL;
	
	/* and return a pointer to the dense vector */
	return output;
	
}	
double getMagnitudeDense(denseRIV *input){
	size_t temp = 0;
	size_t accumulate = 0;
	double divisor = 1;
	int *values = input->values;
	int *values_stop = values+RIVSIZE;
	while(values<values_stop){
		if(*values){
			temp += ((size_t)*values)*((size_t)*values);
		}
		values++;
		
		/* integer overflow is a concern.  this checks for overflows, sacrificing small amounts of accuracy */
		if(OVERFLOWCHECK & temp){
			
			/* accumulate holds an increasingly compact version of the sum of squares, as necessary */
			accumulate*= divisor/(divisor+1);
			divisor+=1;
			accumulate = temp/divisor;
			temp=0;	
			
		}
	}
	
	/* add the temp holder, scaled in line with the accumulate value */
	accumulate += temp/divisor;
	/* take the square root, and scale it back to its original form */
	return sqrt(accumulate)*sqrt(divisor);
}
sparseRIV trueNormalize(denseRIV* input, double magnitude){
	
	input->magnitude = getMagnitudeDense(input);
	
	/* multiplier is the scaling factor we need to bring our vector to the right size */
	double multiplier = magnitude/(input->magnitude);
	
	/* write to temp slot, data will go to a permanent home lower in function */
	int* locations = h_tempBlock+RIVSIZE;
	int* values = locations+RIVSIZE;
	
	int count = 0;
	for(int i=0; i<RIVSIZE; i++){
		/* if this point is 0, skip it */
		if(!input->values[i]) continue;
		
		/* record position and value in the forming sparse vector */
		locations[count] = i;
		
		values[count]= round(input->values[i]*multiplier);
		
		/* drop any 0 values */
		if(values[count])count++; 
	}
	
	sparseRIV output;
	output.count = count;
	/* for memory conservation, both datasets are put inline with each other */
	output.locations = (int*) malloc(count*2*sizeof(int));
	output.values = output.locations+count;
	
	/* copy the data from tempBlock into permanent home */
	memcpy(output.locations, locations, count*sizeof(int));
	memcpy(output.values, values, count*sizeof(int));
	
	/* carry metadata */
	strcpy(output.name, input->name);
	output.magnitude = getMagnitudeSparse(output);
	output.contextSize = input->contextSize;
	output.frequency = input->frequency;
	
	return output;
	
	
}


sparseRIV normalize(denseRIV input, int factor){
	/* multiplier is the scaling factor we need to bring our vector to the right size */
	float multiplier = (float)factor/(input.contextSize);

	/* write to temp slot, data will go to a permanent home lower in function */
	int* locations = h_tempBlock+RIVSIZE;
	int* values = locations+RIVSIZE;
	
	int count = 0;
	for(int i=0; i<RIVSIZE; i++){
		/* if this point is 0, skip it */
		if(!input.values[i]) continue;
		
		/* record position and value in the forming sparse vector */
		locations[count] = i;
		values[count]= round(input.values[i]*multiplier);
		
		/* drop any 0 values */
		if(values[count])count++; 
	}
	sparseRIV output;
	output.count = count;
	/* for memory conservation, both datasets are put inline with each other */
	output.locations = (int*) malloc(count*2*sizeof(int));
	output.values = output.locations+count;
	
	/* copy the data from tempBlock into permanent home */
	memcpy(output.locations, locations, count*sizeof(int));
	memcpy(output.values, values, count*sizeof(int));
	
	/* carry metadata */
	strcpy(output.name, input.name);
	output.magnitude = getMagnitudeSparse(output);
	output.contextSize = input.contextSize;
	output.frequency = input.frequency;
	return output;
}



#endif
