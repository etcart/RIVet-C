#include <stdio.h>
#include "../RIVtools.h"
#include <dirent.h>
#include <sys/types.h>
int main(){

	lexOpen("lexicon/");
	FILE *wordList = fopen("wordList.txt", "r");
	if(!wordList){
		puts("feed me a list of the words in your lexicon");
		return 1;
	}
	
	char word[100];
	denseRIV accept;
	sparseRIV analyzefloor;
	sparseRIV analyzerounded;
	sparseRIV other;
	while(fscanf(wordList, "%s", word)){
		if(!*word) break;
		if(feof(wordList))break;
		accept = lexPull(word);

		other = consolidateD2S(accept.values);
		other.magnitude = getMagnitudeSparse(other);
		accept.magnitude = other.magnitude;
		analyzerounded = normalize(accept, 2000);
		analyzefloor = normalizeFloored(accept, 2000);
		if(cosCompare(accept, analyzefloor)>1.00){
		printf("%s\nbase mag:%f\tunrounded: %f\trounded: %f\tcontextSize: %d\tfrequency: %d\tsaturationbase %d\tdiffround: %lf\tdiffflooor; %lf\n", word, other.magnitude, analyzefloor.magnitude, analyzerounded.magnitude, *(accept.contextSize), *(accept.frequency), other.count, cosCompare(accept,analyzerounded) , cosCompare(accept, analyzefloor));
}
		
		free(analyzefloor.locations);
		free(analyzerounded.locations);
		free(other.locations);
		
		free(accept.values);
	}
	lexClose();

}

