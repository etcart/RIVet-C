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

### Example

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
	
	lexClose();
	fclose(testFile);



```
