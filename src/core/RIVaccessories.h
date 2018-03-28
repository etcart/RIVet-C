#ifndef RIVACCESS_H_
#define RIVACCESS_H_
/*isWordClean filters words that contain non-letter characters, and 
 * upperCase letters, allowing only the '_' symbol through
 */
int isWordClean(char* word);
/* used by wordClean */
int isLetter(char c);

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

#endif
