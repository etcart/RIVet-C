#ifndef RIVACCESS_H_
#define RIVACCESS_H_



/*isWordClean filters words that contain non-letter characters, and 
 * upperCase letters, allowing only the '_' symbol through
 */
int isWordClean(char* word);

/* used by wordClean */
int isLetter(char c);

/* creates a standard seed from the characters in a word, hopefully unique */
int wordtoSeed(unsigned char* word);

int isLetter(char c){
	
	if((c>96 && c<123)||(c == 32) || (c == '_')) return 1;
	else return 0;
}
int isWordClean(char* word){
	char *letter = word;
	char *word_stop = word+99;
	while(letter<word_stop){
		if(!(*letter)) break;
		if(!(isLetter(*letter))){
			
			return 0;
		}
		letter++;
	}
	return 1;
		
}
int wordtoSeed(unsigned char* word){
	int i=0;
	int seed = 0;
	while(*word){
		/* left-shift 5 each time *should* make seeds unique to words
		 * this means letters are taken as characters counted in base 32, which
		 * should be large enough to hold all english characters plus a few outliers
		 * */
		seed += (*(word))<<(i*5);
		word++;
		i++;
	}
	return seed;
}

#endif
