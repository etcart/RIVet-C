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
work, along with linux OS interfacing to make lexicon generation 
secure and easy.

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
#include "RIVet-C/src/RIVtools.h"

//this program reads a file, and adds all context vectors (considering line as context)
//to all words found in this file. this is used to create a lexicon, or add to an existing one
void fileGrind(FILE* textFile);
void addContext(denseRIV* lexRIV, sparseRIV context);
void lineGrind(char* textLine);

LEXICON* lp;
struct treenode* stemRoot;

int main(int argc, char *argv[]){

	//we open the lexicon, if it does not yet exist, it will be created
	lp = lexOpen("sandboxLexicon", "rw");
	stemRoot = stemTreeSetup();
	//we format the root directory, preparing to scan its contents
	if(argc < 2){
		printf("no file given for reading");
		return 1;
	}

	FILE* textFile = fopen(argv[1], "r");
	if(! textFile){
		printf("invalid file to read");
		return 1;
	}
	/* the file is scanned, adding word data to each word vector */
	fileGrind(textFile);
	//we close the lexicon again, ensuring all data is secured
	lexClose(lp);
	return 0;
}

void fileGrind(FILE* textFile){
	
	
	char textLine[100000];
	// included python script separates paragraphs into lines
	while(fgets(textLine, 99999, textFile)){
		
		if(!strlen(textLine)) continue;
		if(feof(textFile)) break;
		/* the line is purged of non-letters and all words are stemmed */
		if(cleanLine(textLine, stemRoot)){
			
			//process each line as a context set
			
			lineGrind(textLine);
		}
	}
}
//form context vector from contents of text, then add that vector to
//all lexicon entries of the words contained
void lineGrind(char* textLine){
	
	//extract a context vector from this text set
	sparseRIV contextVector = textToL2(textLine);
	if(contextVector.contextSize <= 1){
		free(contextVector.locations);
		return;
	}
		
	denseRIV* lexiconRIV;
	//identify stopping point in line read
	char* textEnd = textLine + strlen(textLine)-1;
	int displacement = 0;
	char word[100] = {0};
	while(textLine<textEnd){
		sscanf(textLine, "%99s%n", word, &displacement);
		//we ensure that each word exists, and is free of unwanted characters
		
		textLine += displacement+1;
		if(!(*word))continue;

		//we pull the vector corresponding to each word from the lexicon
		//if it's a new word, lexPull returns a 0 vector
		lexiconRIV= lexPull(lp, word);
		if(!lexiconRIV){
			continue;
		}
		//we add the context of this file to this wordVector
		addContext(lexiconRIV, contextVector);
		
		//we remove the sub-vector corresponding to the word itself
		subtractThisWord(lexiconRIV);
		//we log that this word has been encountered one more time
		lexiconRIV->frequency += 1;
		
		//and finally we push it back to the lexicon for permanent storage
		lexPush(lp, lexiconRIV);
		
		
	}
	//free the heap allocated context vector data
	free(contextVector.locations);
}

void addContext(denseRIV* lexRIV, sparseRIV context){
		
		//add context to the lexRIV, (using sparse-dense vector comparison)
		sparseRIV thing = context;
		addS2D(lexRIV->values, thing);
		
		//log the "size" of the vector which was added
		//this is not directly necessary, but is useful metadata for some analises
		lexRIV->contextSize += context.contextSize;
		
}

```
For this style of reading, to get good, long-term accuracy, it's important
to feed it stemmed, cleaned plaintext to reduce noise.  There is an included
python script for this, under applications/lexiconBuilder


In this code we compare two words in the lexicon, to see how the system understands them
this code requires an already formed lexicon.  a form which can be reasonably downloaded is coming soon

```C
#include "RIVtools.h"
int main(){
	//we open the lexicon
	lexOpen("src/lexica/lexicon");
	
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
