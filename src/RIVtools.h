#ifndef RIVTOOLS_H_
#define RIVTOOLS_H_

#include "core/RIVlower.h"
#include "core/RIVlexicon.h"
#include "core/RIVaccessories.h"
#include "core/RIVmath.h"



/* fileToL2 takes an input file, reads words (delimiting on " " and "\n") 
 * and returns a sparse RIV which is the vector sum of the base RIVs of each 
 * word contained
 */
sparseRIV* fileToL2(FILE *input);

/* like fileToL2 but takes a block of text */
sparseRIV* textToL2(char *text);

/*cosine determines the "similarity" between two RIVs. */
/*NOTE this legacy cosCompare is kept for simplicity, but the cos
 * defined in RIVmath.h RIVcosCompare(vectorA, vectorB) 
 * is RIV type agnostic for ease of use */
double cosCompare(denseRIV *baseRIV, sparseRIV* comparator);

/* used for analysis of lexicon vectors (not simply accumulation)
 * This normalization is secure against potential integer overflow
 * issues in extreme size lexica, however, it is only an approximation
 * of true normal.
 */
sparseRIV* normalize(denseRIV input, int factor);

/* used to set a vector magnitude. this is safe to use in all but the
 * most extremely large lexica, and get a true normal 
 */
sparseRIV* trueNormalize(denseRIV* input, double magnitude);

/* replaceable with the above macro, this calculates sine distance
 * not true sine, a periodically useful metric of angle
 */
double sine(denseRIV* baseDense, sparseRIV* comparator);

/* this converts a string of raw text into a vector format, based on 
 * the lexicon vectors of the words that form it.  this both stems and 
 * vectorizes the text based on the lexicon and stemroot it is given
 */
sparseRIV* line2L3(LEXICON* lexicon, char* text, RIVtree* stemRoot);


sparseRIV* textToBOWL2(char *text, RIVtree* root){
	int wordCount = 0;
	char word[100] = {0};

	int denseTemp[RIVSIZE] = {0};
	/* locations (implicit RIV) are temp stored in temp block, and moved 
	 * to permanent home in consolidation */
	
	int displacement = 0;
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
		
		/* add word's L1 RIV to the accumulating denseRIV */
		denseTemp[getBOWIndex(word, root)] += 1;
		
		
		wordCount++;
	}
	/* map remaining locations to the denseTemp */
	sparseRIV* output = consolidateD2S(denseTemp);

	/* contextSize stores the number of words read */
	output->contextSize = wordCount;
	return output;
}	


sparseRIV* textToL2(char *text){
	int wordCount = 0;
	char word[100] = {0};

	int denseTemp[RIVSIZE] = {0};
	/* locations (implicit RIV) are temp stored in temp block, and moved 
	 * to permanent home in consolidation */
	
	int displacement = 0;
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
		
		/* add word's L1 RIV to the accumulating denseRIV */
		addBarcodeToDense(denseTemp, word);
		
		
		wordCount++;
	}
	/*consolidate the dense form to a sparseRIV for output */
	sparseRIV* output = consolidateD2S(denseTemp);

	/* contextSize stores the number of words read */
	output->contextSize = wordCount;
	return output;
}

sparseRIV* fileToL2(FILE *data){
	char word[100] = {0};

	/* locations (implicit RIV) are temporarily stored in temp block, 
	 * and moved to permanent home in consolidation */

	int denseTemp[RIVSIZE] = {0};
	int wordCount = 0;
	while(fscanf(data, "%99s", word)){

		if(feof(data)){
			break;
		}
		if(!(*word)){
			break;
		}

		/* add the barcode form of this word to the accumulating denseRIV */
		addBarcodeToDense(denseTemp, word);
		
		
		wordCount++;
	}
	
	
	/* consolidate the denseTemp to a sparseRIV */
	sparseRIV* output = consolidateD2S(denseTemp);

	/* contextSize records the number of words in this file */
	output->contextSize = wordCount;
	fseek(data, 0, SEEK_SET);
	return output;
}

double cosCompare(denseRIV* baseRIV, sparseRIV* comparator){

	long long int dot = 0;
	int* locations_stop = comparator->locations+comparator->count;
	int* locations_slider = comparator->locations;
	int* values_slider = comparator->values;
	while(locations_slider<locations_stop){

		/* we calculate the dot-product to derive the cosine 
		 * comparing sparse to dense by index*/
		dot += (long long int)*values_slider * (long long int)baseRIV->values[*locations_slider];
		locations_slider++;
		values_slider++;

	}
	/*dot divided by product of magnitudes */

	return dot/(baseRIV->magnitude*comparator->magnitude);
}



sparseRIV* trueNormalize(denseRIV* input, double magnitude){
	
	input->magnitude = getMagnitudeDense(input);
	
	/* multiplier is the scaling factor we need to bring our vector to the right size */
	double multiplier = magnitude/(input->magnitude);
	
	/* write to temp slot, data will go to a permanent home lower in function */
	int* locations = tempBlock+RIVSIZE;
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
	
	sparseRIV* output;
	/* for memory conservation, both datasets are put inline with each other */
	output = sparseAllocate(count);
	
	/* copy the data from tempBlock into permanent home */
	memcpy(output->locations, locations, count*sizeof(int));
	memcpy(output->values, values, count*sizeof(int));
	
	/* carry metadata */
	strcpy(output->name, input->name);
	output->magnitude = getMagnitudeSparse(output);
	output->contextSize = input->contextSize;
	output->frequency = input->frequency;
	
	return output;
	
	
}


sparseRIV* normalize(denseRIV input, int factor){
	/* multiplier is the scaling factor we need to bring our vector to the right size */
	float multiplier = (float)factor/(input.contextSize);

	/* write to temp slot, data will go to a permanent home lower in function */
	int* locations = tempBlock+RIVSIZE;
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
	sparseRIV* output = sparseAllocate(count);
	
	/* copy the data from tempBlock into permanent home */
	memcpy(output->locations, locations, count*sizeof(int));
	memcpy(output->values, values, count*sizeof(int));
	
	/* carry metadata */
	strcpy(output->name, input.name);
	output->magnitude = getMagnitudeSparse(output);
	output->contextSize = input.contextSize;
	output->frequency = input.frequency;
	return output;
}

double sine(denseRIV* baseDense, sparseRIV* comparator){
	double cos = cosCompare(baseDense, comparator);
	return COS2SIN(cos);
}
		
sparseRIV* line2L3(LEXICON* lexicon, char* text, RIVtree* stemRoot){
	
	/* wordRIV will be used as temporary storage for lexPull output */
	denseRIV* wordRIV;
	
	/* vector will be summed in preparation for consolidation and output */
	denseRIV accumulate = {0};
	
	/* pointer to null terminator */
	char* textEnd = text+strlen(text);
	char word[100];

	/* will log the number of characters read in each word */
	int displacement;
	while(text<textEnd){
		sscanf(text, "%99s%n", word, &displacement);
		
		/* shift forward by the number of characters read (+ a space!) */
		text += displacement+1;
		
		/* if the word parses down to nothing, skip */
		if(!clean(word)) continue;
		char* stem;
		if(stemRoot){
			stem = treeSearch(stemRoot, word);
		}else{
			stem = word;
		}
		/* if the word stems to nothing (has no valid stem in the lexicon)
		 * skip */
		if(!stem) continue;
	
		/* retrieve the vector form of this word from the lexicon */
		wordRIV = lexPull(lexicon, stem);
		
		if(!wordRIV) continue;
		
		/* add this vector to the accumulating text vector */
		addRIV(&accumulate, wordRIV) 
		
		/* send the vector back to the lexicon */
		lexPush(lexicon, wordRIV);
	
		
	}

	/* reduce this vector to a sparse form and return */
	return consolidateD2S(accumulate.values);
}



/*defunct, need to be shifted around to meet new standards, mid debugging
struct threadargs{
	int begin;
	int end;
	sparseRIV* set;
};
#define THREADCOUNT 6
volatile int threadsInProgress = 0;
pthread_mutex_t counterMut = PTHREAD_MUTEX_INITIALIZER;
double** sines;

double** cosIntercompare(sparseRIV* vectorSet, size_t vectorCount){
	
	
	int cosSize = (vectorCount*vectorCount-vectorCount)/2;
	
	sines = malloc(vectorCount*sizeof(double*));
	*sines = calloc(cosSize, sizeof(double));
	double* sines_slider = *sines;
	for(int i=0; i<vectorCount; i++){
		sines[i] = sines_slider;
		sines_slider += i;
	}
	
	
	
	
	pthread_t threadID[THREADCOUNT];
	struct threadargs arguments[THREADCOUNT];

	int jump = (vectorCount*vectorCount)/THREADCOUNT;
	for(int i=0; i<THREADCOUNT; i++){
		arguments[i].begin = sqrt(jump*i);
		arguments[i].end = sqrt(jump*(i+1));
		arguments[i].set = vectorSet;
	}arguments[THREADCOUNT-1].end = vectorCount;
	
	for(int i=0; i<THREADCOUNT; i++){
		
		
		pthread_create(&threadID[i], NULL, cosineSetThread, &arguments[i]);
		

	}
	for(int i=0; i<THREADCOUNT; i++){
		
		
		pthread_join(threadID[i], NULL);
		

	}
	return sines;
}

void* cosineSetThread (void* args){
	int begin = ((struct threadargs*)args)->begin;
	int end = ((struct threadargs*)args)->end;
	sparseRIV* set = ((struct threadargs*)args)->set;
	denseRIV baseDense = {0};
	
	for(int i=begin; i<end; i++){
		
		memset(baseDense.values, 0, RIVSIZE*sizeof(int));
		addRIV(baseDense, &set[i]);
		baseDense.magnitude = set[i].magnitude;
		
		for(int j=0; j<i; j++){
			
			sines[i][j] = sine(&baseDense,&set[j]);
			
		}
	}
	
	pthread_mutex_lock(&counterMut);
		threadsInProgress--;
	pthread_mutex_unlock(&counterMut);
	return NULL;
	
}
*/

#endif
