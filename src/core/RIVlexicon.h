#ifndef RIV_LEXICON_H
#define RIV_LEXICON_H

#include "RIVLower.h"
#include "RIVaccessories.h"

/* lexOpen is called to "open the lexicon", setting up for later calls to
 * lexPush and lexPull. if the lexicon has not been opened before calls
 * to these functions, their behavior can be unpredictable, most likely crashing
 */
void lexOpen();

/* lexClose should always be called after the last lex push or lex pull call
 * if the lexicon is left open, some vector data may be lost due to 
 * un-flushed RIV cache
 */
void lexClose();


/* both lexPush and lexPull must be called *after* the lexOpen() function
 * and after using them the lexClose() function must be called to ensure
 * data security */
 
/* lexPush writes a denseRIV to the lexicon for permanent storage */
int lexPush(denseRIV* RIVout);

int cacheCheckOnPush(denseRIV* RIVout);
/* lexPull reads a denseRIV from the lexicon, under "word"
 * if the file does not exist, it creates a 0 vector with the name of word
 * lexPull returns a denseRIV *pointer* because its data must be tracked 
 * globally for key optimizations
 */
denseRIV* lexPull(char* word);

denseRIV* cacheCheckOnPull(char* word);

/* fLexPush pushes the data contained in a denseRIV out to a lexicon file,
 * saving it for long-term aggregation.  function is called by "lexPush",
 * which is what users should actually use.  lexPush, unlike fLexPush,
 * has cache logic under the hood for speed and harddrive optimization
 */
int fLexPush(denseRIV* RIVout);

/* flexPull pulls data directly from a file and converts it (if necessary)
 * to a denseRIV.  function is called by "lexPull" which is what users 
 * should actually use.  lexPull, unlike FlexPull, has cache logic under
 * the hood for speed and harddrive optimization 
 */
denseRIV* fLexPull(FILE* lexWord);

/* redefines signal behavior to protect cached data against seg-faults etc*/
void signalSecure(int signum, siginfo_t *si, void* arg);

/* begin definitions */
void lexOpen(char* lexName){
	
	struct stat st = {0};
	if (stat(lexName, &st) == -1) {
		mkdir(lexName, 0777);
	}	
	strcpy(RIVKey.lexName, lexName);
	/* open a slot at least large enough for ;worst case handling of
	 * sparse to dense conversion.  may be enlarged by filetoL2 functions */
	struct sigaction action = {0};
	action.sa_sigaction = signalSecure;
	action.sa_flags = SA_SIGINFO;
	for(int i=1; i<27; i++){
		sigaction(i,&action,NULL);
	}
	 

	/* open a slot for a cache of dense RIVs, optimized for frequent accesses */
	memset(RIVKey.RIVCache, 0, sizeof(denseRIV*)*CACHESIZE);
}
void lexClose(){
	
	 
	if(cacheDump()){
		puts("cache dump failed, some lexicon data was lost");
	}
}
#if CACHESIZE > 0
denseRIV* cacheCheckOnPull(char* word){
	srand(wordtoSeed(word));
	int hash = rand()%CACHESIZE;
	if(RIVKey.RIVCache[hash]){
		if(!strcmp(word, RIVKey.RIVCache[hash]->name)){

			/* if word is cached, pull from cache and exit */
			return RIVKey.RIVCache[hash];
		}
	}
	return NULL;
}
#endif
denseRIV* lexPull(char* word){
	denseRIV* output;
	
	#if CACHESIZE > 0

	/* if there is a cache, first check if the word is cached */
	if((output = cacheCheckOnPull(word))){
		return output;
	}
	#endif /* CACHESIZE > 0 */

	/* if not, attempt to pull the word data from lexicon file */


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
		output = calloc(1, sizeof(denseRIV));
	}

	strcpy(output->name, word);
	return output;
}
#if CACHESIZE > 0
int cacheCheckOnPush(denseRIV* RIVout){
	/* if our RIV was cached already, no need to play with it */
	if(RIVout->cached){
		return 1;
	}
	
	srand(wordtoSeed(RIVout->name));
	int hash = rand()%CACHESIZE;
	
	/* if there is no word in this cache slot */
	if(!RIVKey.RIVCache[hash]){
		/* push to cache instead of file */
		RIVKey.RIVCache[hash] = RIVout;
		RIVKey.RIVCache[hash]->cached = 1;
		return 1;
	/*if the current RIV is more frequent than the RIV holding its slot */
	}
	if(RIVout->frequency > RIVKey.RIVCache[hash]->frequency ){
		/* push the lower frequency cache entry to a file */
		fLexPush(RIVKey.RIVCache[hash]);
		/* replace this cache-slot with the current vector */

		RIVKey.RIVCache[hash] = RIVout;
		RIVKey.RIVCache[hash]->cached = 1;
		
		return 1;
	}
	return 0;
	
	
}
#endif
int lexPush(denseRIV* RIVout){
	#if CACHESIZE > 0

	if(cacheCheckOnPush(RIVout)){
		return 0;
	}
	
	#endif /* CACHESIZE != 0 */
	
	/* find the cache-slot where this word belongs */

	return fLexPush(RIVout);
	

}
int fLexPush(denseRIV* output){	
	char pathString[200] = {0};
	denseRIV RIVout = *output;
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
		fwrite(&RIVout.frequency, 1, sizeof(int), lexWord);
		fwrite(&RIVout.contextSize, 1, sizeof(int), lexWord);
		fwrite(&RIVout.magnitude, 1, sizeof(float), lexWord);
		fwrite(temp.locations, temp.count, sizeof(int), lexWord);
		fwrite(temp.values, temp.count, sizeof(int), lexWord);
	}else{
		/* saturation is too high, better to store dense */
		/* there's gotta be a better way to do this */
		temp.count = 0;
		fwrite(&temp.count, 1, sizeof(size_t), lexWord);
		fwrite(&RIVout.frequency, 1, sizeof(int), lexWord);
		fwrite(&RIVout.contextSize, 1, sizeof(int), lexWord);
		fwrite(&RIVout.magnitude, 1, sizeof(float), lexWord);
		fwrite(RIVout.values, RIVSIZE, sizeof(int), lexWord);
	}

	fclose(lexWord);
	free(output);
	free(temp.locations);

	return 0;
}

denseRIV* fLexPull(FILE* lexWord){
	denseRIV *output = calloc(1,sizeof(denseRIV));
	size_t typeCheck;
	/* get metadata for vector */
	fread(&typeCheck, 1, sizeof(size_t), lexWord);
	fread(&output->frequency, 1, sizeof(int), lexWord);
	fread(&output->contextSize, 1, sizeof(int), lexWord);
	fread(&output->magnitude, 1, sizeof(float), lexWord);

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

		addS2D(output->values, temp);
		free(temp.locations);
	}else{
		/* typecheck is thrown away, just a flag in this case */
		fread(output->values, RIVSIZE, sizeof(int), lexWord);
	}


	output->cached = 0;

	return output;

}



int cacheDump(){

	int flag = 0;

	for(int i = 0; i < CACHESIZE; i++){
		if(RIVKey.RIVCache[i]){

			flag += fLexPush(RIVKey.RIVCache[i]);
		}
	}
	return flag;
}


/*TODO add a simplified free function*/
void signalSecure(int signum, siginfo_t *si, void* arg){
  if(cacheDump()){
	  puts("cache dump failed, some lexicon data lost");
  }else{
	puts("cache dumped successfully");
  }
  signal(signum, SIG_DFL);
  kill(getpid(), signum);
}

#endif
