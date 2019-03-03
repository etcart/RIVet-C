#ifndef RIV_LEXICON_H
#define RIV_LEXICON_H

#include "RIVlower.h"
#include "RIVmath.h"
#include "RIVaccessories.h"


#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>



/* these flags will be used by the lexicon to know its permissions and states */
#ifndef READFLAG
#define READFLAG 0x01
#endif

#ifndef WRITEFLAG
#define WRITEFLAG 0x02
#endif

#ifndef INCFLAG 
#define INCFLAG 0x04
#endif

#ifndef CACHEFLAG
#define CACHEFLAG 0x08
#endif

/* if user has specified neither hashed nor sorted cache we assume sorted
 * hashed strategy is extremely CPU and memory light, but very inneffective 
 * at ensuring the most important vectors are cached. as such it is better
 * optimized for RAMdisks and unusually fast SSDs.  the sorted strategy
 * is much more expensive for the CPU, but ensures the minimum possible 
 * hard-drive read writes far more effectively */

#ifndef SORTCACHE
	#ifndef HASHCACHE
		#define SORTCACHE
	#endif
#endif


typedef char RIVword[100];

/* the LEXICON struct will be used similar to a FILE (as a pointer) which
 * contains all metadata that a lexicon needs in order to be read and written to safely*/
typedef struct{
	char lexName[100];
	denseRIV* *cache;
	struct cacheList* listPoint;
	char flags;
	#ifdef SORTCACHE
	/* if our cache is sorted, we will need a search tree and a saturation */
	RIVtree* treeRoot;
	int cacheSaturation;
	denseRIV* *cache_slider;
	#endif /* SORTCACHE */
}LEXICON;
/* this will form a linked list of caches, so that all data can be safely dumped
 * in event of an error, no matter how many or how strangely lexica have
 * been opened and closed */
struct cacheList{
	denseRIV* *cache;
	struct cacheList* next;
	struct cacheList* prev;
}*rootCache = NULL;

/* IOstagingSlot is used by fLexPush to preformat data to be written in a single
 * fwrite() call.  it has room for RIVSIZE integers behind it and 2*RIVSIZE
 * integers ahead of it, which the function saturationForStaging() will need */
int* IOstagingSlot = tempBlock+RIVSIZE;

/* lexOpen is called to "open the lexicon", setting up for later calls to
 * lexPush and lexPull. if the lexicon has not been opened before calls
 * to these functions, their behavior can be unpredictable, most likely crashing
 * lexOpen accepts flags: r, w, x.
 * r: for reading, currently meaningless, it wont stop you reading if you don't have this
 * w: for writing. if a readonly lexicon is "written to" no data will be saved in hardcopy
 * although it will be cached if possible, so that later pulls will be optimized
 * x: exclusive. will not accept new words, lexPull returns a NULL pointer
 * and lexPush simply frees any word which is not already in the lexicon
 */
LEXICON* lexOpen(const char* lexName, const char* flags);

/* lexClose should always be called after the last lex push or lex pull call
 * if the lexicon is left open, some vector data may be lost due to 
 * un-flushed RIV cache.  also frees up data, memory leaks if lexicon is not closed
 */
void lexClose(LEXICON*);

/* both lexPush and lexPull must be called *after* the lexOpen() function
 * and after using them the lexClose() function must be called to ensure
 * data security (only after the final push or pull, not regularly during operation */
 
/* lexPush writes a denseRIV to the lexicon for permanent storage */
int lexPush(LEXICON* lexicon, denseRIV* RIVout);

/* lexPull reads a denseRIV from the lexicon, under "word"
 * if the file does not exist, it creates a 0 vector with the name of word
 * lexPull returns a denseRIV *pointer* because its data must be tracked 
 * globally for key optimizations
 */
denseRIV* lexPull(LEXICON* lexicon, char* word);

/* lexCount returns the number of words in the lexicon */
int lexCount(LEXICON* lp);

/* lexList returns a list of all words in the lexicon 
 * when called, there must already be a wordList ready to hold the list
 */
int lexList(LEXICON* lp, int wordCount, RIVword list[wordCount]);

/* lexInterrelations returns a square matrix of all cosines of the lexicon
 * (cosines of vectors with themselves are "identicalAction");
 */
int lexInterrelations(LEXICON* lp, int wordCount, float emptyRelations[wordCount][wordCount], float identicalAction);

 /*-----end external functions: here thar be dragons -------*/
 

/* cacheCheckOnPush tests the state of this vector in our lexicon cache
 * and returns 1 on "success" indicating cache storage and no need to push to file
 * or returns 0 on "failure" indicating that the vector need be pushed to file 
 */ 
int cacheCheckOnPush(LEXICON* lexicon, denseRIV* RIVout);

/* cacheCheckonPull checks if the word's vector is stored in cache,
 * and returns a pointer to that vector on success
 * or returns a NULL pointer if the word is not cached, indicating a need 
 * to pull from file
 */
denseRIV* cacheCheckOnPull(LEXICON* lexicon, char* word);

/* fLexPush pushes the data contained in a denseRIV out to a lexicon file,
 * saving it for long-term aggregation.  function is called by "lexPush",
 * which is what users should actually use.  lexPush, unlike fLexPush,
 * has cache logic under the hood for speed and harddrive optimization
 */
int fLexPush(LEXICON* lexicon, denseRIV* RIVout);

/* flexPull pulls data directly from a file and outputs it as a denseRIV.
 * function is called by "lexPull" which is what users 
 * should actually use.  lexPull, unlike FlexPull, has cache logic under
 * the hood for speed and harddrive optimization 
 */
denseRIV* fLexPull(FILE* lexWord);

/* redefines signal behavior to protect cached data against seg-faults etc*/
void signalSecure(int signum, siginfo_t *si, void* arg);
int cacheDump(denseRIV* *toDump);

/* used exclusively by flexpush to determine write-style (sparse or dense)
 * and also formats the "IOstagingSlot" for fwrite as a single block if sparse
 */
int saturationForStaging(denseRIV* output);
/* begin definitions */
LEXICON* lexOpen(const char* lexName, const char* flags){
	LEXICON* output = calloc(1, sizeof(LEXICON));
	/* identify the presence of read, write, and exclusive flags */
	char* r = strstr(flags, "r");
	char* w = strstr(flags, "w");
	char* x = strstr(flags, "x");
	struct stat st = {0};
	
	
	if(w){
		/* if set to write, we check and create if necessary, the lexicon */
		if (stat(lexName, &st) == -1) {
			mkdir(lexName, 0777);
		}
		/* flag for writing*/
		output->flags |= WRITEFLAG;
	}else if(r){
		/* if set to read and not write, return null if lexicon does not exist */
		if (stat(lexName, &st) == -1) {
			free(output);
			return NULL;
		}	
		/* flag for reading */
		output->flags |= READFLAG;
	}
		/* if not set to exclusive, set the inclusive flag */
	if(!x){
		/* flag inclusive (will return unknown words as 0 vector */
		output->flags |= INCFLAG;
	}
	/* record the name of the lexicon */
	strcpy(output->lexName, lexName);
	
	#if CACHESIZE > 0
	output->cache = calloc(CACHESIZE, sizeof(denseRIV*));


	#ifdef SORTCACHE
	/* a sorted cache needs a search tree for finding RIVs by name */
	output->treeRoot = calloc(1, sizeof(RIVtree));
	output->cacheSaturation = 0;
	output->cache_slider = output->cache+CACHESIZE;
	#endif /* SORTCACHE */
	
	/* flag cached ?? */ 
	output->flags |= CACHEFLAG;
	if(w){
		/* setup cache-list element for break dumping */
		struct cacheList* newCache = calloc(1, sizeof(struct cacheList));
		newCache->cache = output->cache;
		if(!rootCache){
			rootCache = calloc(1, sizeof(struct cacheList));
		}
		newCache->next = rootCache;
		
		rootCache->prev = newCache;
		rootCache = newCache;
		output->listPoint = newCache;

		struct sigaction action = {0};
		action.sa_sigaction = signalSecure;
		action.sa_flags = SA_SIGINFO;
		
		for(int i=1; i<27; i++){
			sigaction(i,&action,NULL);
		}
	}

	#endif /* CACHESIZE > 0 */

	return output;
}
void lexClose(LEXICON* toClose){
	
#if CACHESIZE>0 
	if(toClose->flags & WRITEFLAG){
		puts("about to do the dump");
		if(cacheDump(toClose->cache)){
			puts("cache dump failed, some lexicon data was lost");
		}
		struct cacheList* listPoint = toClose->listPoint;
		if(listPoint->prev){
			listPoint->prev->next = toClose->listPoint->next;
		}
		if(listPoint->next){
			listPoint->next->prev = toClose->listPoint->prev;
		}
		if(rootCache == listPoint){
			rootCache = listPoint->next;
		}
		free(listPoint);
		
			
		
	}else{
		denseRIV* *cache = toClose->cache;
		for(int i=0; i<CACHESIZE; i++){
			if(cache[i]){
				free(cache[i]);
			}
		}
	}
	//free(toClose->cache);
	#ifdef SORTCACHE
	//destroyTree(toClose->treeRoot);
	
	#endif /* SORTCACHE */
#endif
	free(toClose);
}



#if CACHESIZE > 0
denseRIV* cacheCheckOnPull(LEXICON* lexicon, char* word){
	#ifdef HASHCACHE
	/* we find which cache entry this word belongs in by simple hashing */
	srand(wordtoSeed(word));
	int hash = rand()%CACHESIZE;
	if(lexicon->cache[hash]){
		if(!strcmp(word, lexicon->cache[hash]->name)){
			/* if word is cached, pull from cache and exit */
			return lexicon->cache[hash];
		}
	}
	return NULL;
	#endif
	#ifdef SORTCACHE
	/* use a treeSearch (found in RIVaccessories) to find the denseRIV* in the cache */
	return treeSearch(lexicon->treeRoot, word);

	#endif
}

int cacheCheckOnPush(LEXICON* lexicon, denseRIV* RIVout){
	
	/* if our RIV was cached already, no need to play with it */
	if(RIVout->cached == lexicon){
		/* return "success" the vector is already in cache and updated */
		return 1;
	}
	#ifdef HASHCACHE
	srand(wordtoSeed(RIVout->name));
	int hash = rand()%CACHESIZE;
	
	/* if there is no word in this cache slot */
	if(!lexicon->cache[hash]){
		/* push to cache instead of file */
		lexicon->cache[hash] = RIVout;
		lexicon->cache[hash]->cached = lexicon;
		/* return "success" */
		return 1;
	/*if the current RIV is more frequent than the RIV holding its slot */
	}
	if(RIVout->frequency > lexicon->cache[hash]->frequency ){
		/* push the lower frequency cache entry to a file */
		fLexPush(lexicon, lexicon->cache[hash]);
		/* replace this cache-slot with the current vector */

		lexicon->cache[hash] = RIVout;
		lexicon->cache[hash]->cached = lexicon;
		/* return "success" */
		return 1;
	}
	return 0;
	#endif /* HASHCACHE */
	#ifdef SORTCACHE
	
	/* if the cache is not yet full, append this vector to the accumulating list */
	if (lexicon->cacheSaturation < CACHESIZE){
		RIVout->cached = lexicon;
		lexicon->cache[lexicon->cacheSaturation] = RIVout;
		treeInsert(lexicon->treeRoot, RIVout->name, RIVout);	
		
		
		lexicon->cacheSaturation = lexicon->cacheSaturation+1;
		/* return "success" */
		return 1;
	}else{ /* if cache is full */
		
		RIVout->cached = lexicon;
		denseRIV* toCheck = RIVout;
		denseRIV* temp;
		
		while(1){
			if(lexicon->cache_slider == lexicon->cache){
				lexicon->cache_slider += CACHESIZE;
			}		
			(lexicon->cache_slider)--;
			if(toCheck->frequency > (*lexicon->cache_slider)->frequency){
				temp = (*lexicon->cache_slider);
				(*lexicon->cache_slider) = toCheck;
				toCheck = temp;
			}else{
				if(toCheck == RIVout){
					return 0;
				}else{
					treecut(lexicon->treeRoot, toCheck->name);
					fLexPush(lexicon, toCheck);
					treeInsert(lexicon->treeRoot, RIVout->name, RIVout);
					return 1;
				}
				break;
			}			
		}
	}
	/* return "failure" */
	return 0;
	
	#endif /* SORTCACHE */
}

#endif
denseRIV* lexPull(LEXICON* lexicon, char* word){
	
	denseRIV* output = NULL;
	
	#if CACHESIZE > 0
	if(lexicon->flags & CACHEFLAG){
		/* if there is a cache, first check if the word is cached */
		if((output = cacheCheckOnPull(lexicon, word))){
			return output;
		}
	}
	#endif /* CACHESIZE > 0 */

	/* if not, attempt to pull the word data from lexicon file */
	char pathString[200];

	sprintf(pathString, "%s/%s", lexicon->lexName, word);

	FILE *lexWord = fopen(pathString, "rb");

	/* if this lexicon file already exists */
	if(lexWord){
		/* pull data from file */
		
		output = fLexPull(lexWord);
		if(!output){
			return NULL;
		}
		/* record the "name" of the vector, as the word */
		strcpy(output->name, word);
		fclose(lexWord);
	}else{
		/* if lexicon is set to inclusive (can gain new words) */
		if(lexicon->flags & INCFLAG){
			
			/*if file does not exist, return a 0 vector (word is new to the lexicon) */
			output = calloc(1, sizeof(denseRIV));
			/* record the "name" of the vector, as the word */
			strcpy(output->name, word);
		}else{
			/*if lexicon is set to exclusive, will return a NULL pointer instead of a 0 vector */
			return NULL;
		}
		
	}
	return output;
}

int lexPush(LEXICON* lexicon, denseRIV* RIVout){
	
	#if CACHESIZE > 0
	if(lexicon->flags & CACHEFLAG){
	/* check the cache to see if it belongs in cache */
		if(cacheCheckOnPush(lexicon, RIVout)){
			/* if the cache check returns 1, it has been dealt with in cache */
			return 0;
		}
	}
	#endif
	if(lexicon->flags & WRITEFLAG){
		/* push to the lexicon */
		return fLexPush(lexicon, RIVout);
	}else{
		/* free and return */
		free(RIVout);
		return 0;
	}
}

int saturationForStaging(denseRIV* output){
	
	/* IOstagingSlot is a reserved block of stack memory used for this (and other)
	 * purposes. in this function, all of the metadata to be written along with a
	 * sparse representation of the vector, will be laid into the IOstagingSlot
	 * in the necessary format for writing and reading again */	
	int* count = IOstagingSlot;
	/* count, requires an 8 byte slot for reasons of compatibility between 
	 * dense and sparse. it takes up two integers (int* count and count+1); */
	*count = 0;
	*(count+1) = 0;
	*(count+2) = output->frequency;
	*(count+3) = output->contextSize;
	/* TODO fix this to allow magnitude to be changed to double easily */
	*(float*)(count+4) = output->magnitude;
	
	/* locations will be laid in immediately after the metadata */
	int* locations = IOstagingSlot+5;
	/* values will be laid in *before* metadata, to be copied after locations,
	 * once the size of the values and locations arrays are known.  there is,
	 * by description of the stagingSlot, enough room for a 
	 * completely saturated vector without conflict */
	int* values = IOstagingSlot-RIVSIZE;;
	int* locations_slider = locations;
	int* values_slider = values;
	for(int i=0; i<RIVSIZE; i++){
		
		/* act only on non-zeros */
		if(output->values[i]){
			/* assign index to locations */
			*(locations_slider++) = i;
			/* assign value to values */
			*(values_slider++) = output->values[i];
			/* track size of forming sparseRIV */
			*count += 1;
		}
	}
	/* copy values into slot immediately after locations */
	memcpy(locations+*count, values, (*count)*sizeof(int));
	
	/* return number of non-zeros */
	return *count;
}
int fLexPush(LEXICON* lexicon, denseRIV* output){	
	char pathString[200] = {0};
	
	/* word data will be placed in a (new?) file under the lexicon directory
	 * in a file named after the word itself */
	sprintf(pathString, "%s/%s", lexicon->lexName, output->name);
	
	/* saturationForStaging returns the number of non-zero elements in the vector
	 * and, in the process, places the data of the vector, in sparse format, in the
	 * preallocated "IOstagingSlot" */
	int saturation = saturationForStaging(output);
	
	/* if our vector is less than half full, it is lighter to save it as a sparseRIV */
	if( saturation < RIVSIZE/2){
		
		FILE *lexWord = fopen(pathString, "wb");
		if(!lexWord){
			fprintf(stderr,"lexicon push has failed for word: %s\n", output->name);
			return 1;
		}
		/* IOstagingSlot is formatted for immediate writing */
		fwrite(IOstagingSlot, (saturation*2)+5, sizeof(int), lexWord);
		fclose(lexWord);
	}else{
		/* the "cached" datapoint will be erased, a typecheck flag (0) for
		 * the fLexPull function to know that this is a denseVector put 
		 * in its place */
		output->cached = 0;
		FILE *lexWord = fopen(pathString, "wb");
		if(!lexWord){
			fprintf(stderr, "lexicon push has failed for word: %s\n", output->name);
			return 1;
		}
		/* from the type flag forward, all metadata is preformatted, we simpy write */
		fwrite(((int*)&output->cached), sizeof(int), RIVSIZE+5, lexWord);
		
		fclose(lexWord);
	}
	
	/* and free the memory */
	free(output);

	return 0;
}

denseRIV* fLexPull(FILE* lexWord){
	denseRIV *output = calloc(1,sizeof(denseRIV));
	size_t typeCheck;
	/* the first 8 byte value in the file will be either 0 (indicating storage as a dense vector)
	 * or a positive number, the number of values in a sparse-vector */
	if(!fread(&typeCheck, 1, sizeof(size_t), lexWord)){
		return NULL;
	}
	
	/* first value stored is the value count if sparse, and 0 if dense */
	if (typeCheck){ /* pull as sparseVector */
		
		/*create a sparseVector pointer, pointing to a prealloccated slot */
		sparseRIV* temp = (sparseRIV*)tempBlock;
		/* typecheck, non-zero, is the number of values in our vector */
		temp->count = typeCheck;
		/* locations slot comes immediately after the magnitude */
		
		/* and values slot comes immediately after locations */
		temp->values = temp->locations+temp->count;		
		
		if (fread(&(temp->frequency), sizeof(int), (typeCheck* 2)+3, lexWord) != typeCheck*2 + 3){
			printf("vector read failure");
			return NULL;
		}
		
		/* add our temporary sparseVector to the empty denseVector, for output */
		addRIV(output, temp);
		output->contextSize = temp->contextSize;
		output->frequency = temp->frequency;
		output->magnitude = temp ->magnitude;
	}else{ /* typecheck is thrown away, just a flag in this case */
	
		/*  read into our denseVector pre-formatted to fit */
		if(fread(&output->frequency, sizeof(int), RIVSIZE+3, lexWord) != RIVSIZE+3){
			printf("vector read failure");
			return NULL;
		}
	}

	return output;
}
/* if our data is cached, it cannot be allowed to be lost in event of an issue */
void signalSecure(int signum, siginfo_t *si, void* arg){
	/* descend linked list */
	while(rootCache->next){
		/* dumping all caches contained */
		if(cacheDump(rootCache->cache)){
			fprintf(stderr, "cache dump failed, some lexicon data lost");
		}
		rootCache = rootCache->next;
		
	}
	/* end with normal behavior of error */
	signal(signum, SIG_DFL);
	kill(getpid(), signum);
}
int cacheDump(denseRIV* *toDump){
	/* flag will record if there are any errors and alert */
	int flag = 0;
	#ifdef SORTCACHE
	//printf("saturation: %d\n\n", ((LEXICON*)((*toDump)->cached))->cacheSaturation);
	#endif
	
	//int i=0;
	/* iterate through the elements of our cache */
	denseRIV* *toDump_slider = toDump;
	denseRIV* *toDump_stop = toDump+CACHESIZE;
	while(toDump_slider<toDump_stop){
		
		#ifdef HASHCACHE
		/* if our cache is hashed, there may be null vectors to be skipped */
		if(*toDump_slider){
			
			flag += fLexPush((LEXICON*)(*toDump_slider)->cached,*toDump_slider);
		}
		#else /* HASHCACHE */
		#ifdef SORTCACHE
		/* if our cache is sorted, a null vector represents the end of the cache */
		if(!*toDump_slider)break;
			//printf("%d: %s, ", i++, (*toDump_slider)->name);
		flag += fLexPush((LEXICON*)(*toDump_slider)->cached,*toDump_slider);
		
		#endif /* SORTCACHE */
		#endif
		
		toDump_slider++;
	}
	
	free(toDump);
	
	return flag;
}

int lexCount(LEXICON* lp){
	char path[100];
	sprintf(path, "%s/.meta-count", lp->lexName);
	FILE* countFile = fopen(path, "r");
	if(countFile){
		int count;
		fscanf(countFile, "%d", &count);
		fclose(countFile);
		return count;
	}

	DIR *directory;
    struct dirent *files = NULL;
	directory = opendir(lp->lexName);
	int wordCount = 0;
	while((files=readdir(directory))){
		if(*(files->d_name) == '.') continue;

		if(files->d_type == DT_DIR){
			/* the lexicon should not have valid sub-directories */
			continue;
		}
	
		wordCount++;

	}
	closedir(directory);
	countFile = fopen(path, "w");
	fprintf(countFile, "%d", wordCount);
	fclose(countFile);
	return wordCount;

}

int lexList(LEXICON* lp, int wordCount, RIVword list[wordCount]){
	char path[100];
	sprintf(path, "%s/.meta-list", lp->lexName);
	FILE* listFile = fopen(path, "r");
	if(listFile){
		for(int i=0; i<wordCount; i++){
			fscanf(listFile, "%s", list[i]);
			if(feof(listFile)) return i+1;
		}
		return wordCount;
	}
	DIR *directory;
    struct dirent *files = NULL;
	directory = opendir(lp->lexName);
	int i=0;
	while((files=readdir(directory))){
		if(*(files->d_name) == '.') continue;

		if(files->d_type == DT_DIR){
			/* the lexicon should not have valid sub-directories */
			continue;
		}
	
		strcpy(list[i], files->d_name);
		i++;

	}

	listFile = fopen(path, "w");
	for(int j=0; j<i; j++){
		fprintf(listFile, "%s\n", list[j]);
	}
	closedir(directory);
	fclose(listFile);
	return i;




}
int lexInterrelations(LEXICON* lp, int wordCount, float emptyRelations[wordCount][wordCount], float identicalAction){
	char path[100];
	sprintf(path, "%s/.meta-relations", lp->lexName);
	FILE* relationsFile = fopen(path, "r");
	if(relationsFile){
		for(int i=0; i<wordCount; i++){
			emptyRelations[i][i] = identicalAction;
			for(int j=0; j<i; j++){
				fscanf(relationsFile, "%f", &emptyRelations[i][j]);
				emptyRelations[j][i] = emptyRelations[i][j];
			}
		}
		return wordCount;
	}
	sparseRIV* dataset[wordCount];
	printf("calculatingThing");
	DIR *directory;
    struct dirent *files = NULL;
	directory = opendir(lp->lexName);
	int count=0;
	while((files=readdir(directory))){
		if(*(files->d_name) == '.') continue;

		if(files->d_type == DT_DIR){
			/* the lexicon should not have valid sub-directories */
			continue;
		}
		denseRIV* temp = lexPull(lp, files->d_name);
		if(!temp) continue;
		dataset[count] = consolidateD2S(temp->values);
		free(temp);
		dataset[count]->magnitude = RIVMagnitude(dataset[count]);
		count++;

	}
	relationsFile = fopen(path, "w");

	for(int i=0; i<count; i++){
		denseRIV temp = {0};
		addS2D(&temp, dataset[i]);
		temp.magnitude = dataset[i]->magnitude;
		emptyRelations[i][i] = identicalAction;
		for(int j=0; j<i; j++){
			emptyRelations[i][j] = RIVcosCompare(&temp, dataset[j]);
			emptyRelations[j][i] = emptyRelations[i][j];
			fprintf(relationsFile, "%f ", emptyRelations[i][j]);
		}
		fprintf(relationsFile, "\n");
	}



	return count;
	
}


#endif /* RIV_LEXICON_H */
