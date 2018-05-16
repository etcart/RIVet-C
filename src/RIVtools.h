#ifndef RIVTOOLS_H_
#define RIVTOOLS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "core/RIVLower.h"
#include "core/RIVaccessories.h"
#include "core/RIVlexicon.h"


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

/* like fileToL2 but takes a block of text */
sparseRIV textToL2(char *text);

/*cosine determines the "similarity" between two RIVs. */
double cosCompare(denseRIV baseRIV, sparseRIV comparator);

/*used for analysis of lexicon vectors (not simply accumulation)
 * to avoid overflow of even a 64 bit integer, vectors must be normalized
 * this is an experimental approximation of true normal, which should yield 
 * some extra data about the nature of this word's context
 */
sparseRIV normalize(denseRIV input, int factor);



/* calculates the magnitude of a sparseVector */ //TODO contain integer overflow in square process
double getMagnitudeSparse(sparseRIV input);
/* same for denseVector */
double getMagnitudeDense(denseRIV *input); //TODO consolidate these into one function
	
sparseRIV textToL2(char *text){
	int wordCount = 0;
	char word[100] = {0};

	int denseTemp[RIVSIZE] = {0};
	/* locations (implicit RIV) are temp stored in temp block, and moved 
	 * to permanent home in consolidation */
	int *locations = RIVKey.h_tempBlock;
	int locationCount = 0;
	int displacement = 0;;
	char* textEnd = text+strlen(text)-1;

	while(text<textEnd){
		sscanf(text, "%99s%n", word, &displacement);
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

	/* contextSize stores the number of words read */
	output.contextSize = wordCount;
	return output;
}

sparseRIV fileToL2(FILE *data){
	char word[100] = {0};

	/* locations (implicit RIV) are temporarily stored in temp block, 
	 * and moved to permanent home in consolidation */
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

	/* contextSize records the number of words in this file */
	output.contextSize = wordCount;
	fseek(data, 0, SEEK_SET);
	return output;
}

sparseRIV fileToL2Clean(FILE *data){

	int denseTemp[RIVSIZE] = {0};
	char word[100] = {0};
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
	output.contextSize = locationCount/NONZEROS;
	fseek(data, 0, SEEK_SET);
	return output;
}

double cosCompare(denseRIV baseRIV, sparseRIV comparator){

	int dot = 0;
	int* locations_stop = comparator.locations+comparator.count;
	int* locations_slider = comparator.locations;
	int* values_slider = comparator.values;
	while(locations_slider<locations_stop){

		/* we calculate the dot-product to derive the cosine 
		 * comparing sparse to dense by index*/
		dot += *values_slider * baseRIV.values[*locations_slider];
		locations_slider++;
		values_slider++;

	}
	/*dot divided by product of magnitudes */

	return dot/(baseRIV.magnitude*comparator.magnitude);
}

double getMagnitudeSparse(sparseRIV input){
	size_t temp = 0;
	int *values = input.values;
	int *values_stop = values+input.count;
	while(values<values_stop){
		/* we sum the squares of all elements */
		temp += (*values)*(*values);
		values++;
	}
	/* we take the root of that sum */
	return sqrt(temp);
}

double getMagnitudeDense(denseRIV *input){
	size_t temp = 0;
	int *values = input->values;
	int *values_stop = values+RIVSIZE;
	while(values<values_stop){
		if(*values){
			temp += (*values)*(*values);
		}
		values++;
	}
	return sqrt(temp);
}



sparseRIV normalize(denseRIV input, int factor){
	/* multiplier is the scaling factor we need to bring our vector to the right size */
	float multiplier = (float)factor/(input.contextSize);

	/* write to temp slot, data will go to a permanent home lower in function */
	int* locations = RIVKey.h_tempBlock+RIVSIZE;
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
