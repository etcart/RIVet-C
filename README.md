# RIVet


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
* Get started!

### Examples

In this code, we will add the context data of one text file to each word
that it contains, adding that to the knowledge pool of the lexicon
```C
#include "RIVtools.h"

	//we open the lexicon
	lexOpen("lexicon");
	
	FILE* testFile = fopen("testFile.txt", "r");
	//we create a context Vector
	sparseRIV context = file2L2(testFile);
	
	int wordCount = context.frequency;
	char word[100];
	
	//create an array of dense vectors
	denseRIV fileRIVs[wordCount];
	
	for( int i=0; i<wordCount; i++){
		
		//read the next word in the document
		fscanf(testFile, "%99s", word);
		
		//pull a vector from the lexicon
		fileRIVs[i] = lexPull(word);
		
		//add context to the lexicon vector
		addS2D(fileRIVs[i].values, context);
		
		//push the vector back to the lexicon for permanent storage
		lexPush(word);
	}
	//always the lexicon must be closed
	lexClose();
	fclose(testFile);
```

in this code we compare two words in the lexicon, to see how the system understands them
(this requires that we have already built a lexicon)

```C
#include "RIVtools.h"

	//we open the lexicon
	lexOpen("lexicon");
	
	//we pull the two words we want to compare from the lexicon
	denseRIV firstWordDense = lexPull("denmark");
	denseRIV secondWordDense = lexPull("norway");
	
	//we convert one of the two to a sparse RIV
	sparseRIV firstWordSparse = consolidateD2S(firstWordDense);
	
	//we take the cosine of the angle between these two
	double cosine = cosCompare(secondWordDense, firstWordSparse);
	
	//from most well-formed lexica, we find that these are 
	//>0,99 cosine similarity
	printf("similarity of these two words is: %lf", cosine);
	
	//always we must close the lexicon
	lexClose();
```	
