# RIVet
(/ˈɹɪvət/)

RIVet is a low level natural language processing toolkit for the Random 
Index Vector system.  This system is for document labelling and other 
text processing with:

  - Intuitive word relations 
  - Extreme lexical flexibility
  - Long-term cumulative improvement

# Why C?
The above benefits looks nice, but they come at the cost of: 
- extreme dimensionalities

So we have, in C, the extreme power to match.  Don't worry though, this 
library strives to remove all mallocs, frees, and pointer magic from the 
coder's space, freeing you up to ignore the technicalities and get to 
work
# Coming Soon!

  - Full scale standard lexicon for general use in higher level 
comparisons


### Infrastructure
* Linux OS
* GCC compiler

### Installation

NONE!
* Clone the repo
* #include "pathTo/RIVtools.h"
* compile with -lm flag
* Get started!



### Examples

In this code, we will add the context data of one text file to each word
that it contains, adding that to the knowledge pool of the lexicon:
```C
include "RIVtools.h"
int main(){
	//we open the lexicon
	lexOpen("lexicon");
	
	
	FILE* testFile = fopen("testFile.txt", "r");
	if(!testFile){
		puts("testFile does not exist");
		return 1;
	}	
	//we create a context Vector
	sparseRIV context = fileToL2(testFile);
	int wordCount = context.frequency;
	char word[100];
	
	//create an array of dense vectors
	denseRIV *lexRIV;
	
	for( int i=0; i<wordCount; i++){
		
		//read the next word in the document
		fscanf(testFile, "%99s", word);
		
		//pull a vector from the lexicon
		lexRIV = lexPull(word);
		
		//add context to the lexicon vector
		addS2D(lexRIV->values, context);
		
		//push the vector back to the lexicon for permanent storage
		lexPush(lexRIV);
	}
	//always the lexicon must be closed
	lexClose();
	fclose(testFile);
	return 0;
}

```
For this style of reading, to get good, long-term accuracy, it's important
to feed it stemmed, cleaned plaintext to reduce noise.  There is an included
python script for this, under applications/lexiconBuilder


In this code we compare two words in the lexicon, to see how the system understands them
(this requires that we have already built a lexicon):

```C
#include "RIVtools.h"
int main(){
	//we open the lexicon
	lexOpen("/home/amberhosen/code/lexica/lexicon2-25");
	
	//we pull the two words we want to compare from the lexicon
	denseRIV* firstWordDense = lexPull("denmark");
	denseRIV* secondWordDense = lexPull("norway");
	
	//we convert one of the two to a sparse RIV
	sparseRIV firstWordSparse = consolidateD2S(firstWordDense->values);

	//we get the magnitudes of these two vectors
	firstWordSparse.magnitude = getMagnitudeSparse(firstWordSparse);
	secondWordDense->magnitude = getMagnitudeDense(secondWordDense);

	//we take the cosine of the angle between these two
	double cosine = cosCompare(*secondWordDense, firstWordSparse);
	
	//from most well-formed lexica, we find that these are 
	//>0,99 cosine similarity
	printf("similarity of these two words is: %lf", cosine);
	
	//always we must close the lexicon
	lexClose();
	return 0;
}	
```	
