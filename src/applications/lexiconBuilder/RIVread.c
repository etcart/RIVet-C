#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <error.h>
#include <string.h>
#include "../../RIVtools.h"

//this program reads a directory full of files, and adds all context vectors (considering file as context)
//to all words found in these files. this is used to create a lexicon, or add to an existing one

void fileGrind(FILE* textFile);
void addContext(denseRIV* lexRIV, sparseRIV context);
void directoryGrind(char *rootString);
void lineGrind(char* textLine);

int main(int argc, char *argv[]){
	char pathString[1000];

	//we open the lexicon, if it does not yet exist, it will be created
	lexOpen("lexicon");
	
	//we format the root directory, preparing to scan its contents
	
	strcpy(pathString, argv[1]);
	strcat(pathString, "/");
	//ensure that the targeted root directory exists
	
	struct stat st;
	if(stat(pathString, &st) == -1) {
		printf("directory doesn't seem to exist");
		return 1;
	}
	//we will scan the directory, adding all data to our lexicon, as seen inside
	directoryGrind(pathString);

	//we close the lexicon again, ensuring all data is secured
	lexClose();
	return 0;
}

//mostly a standard Dirent-walk
void directoryGrind(char *rootString){
/* *** begin Dirent walk *** */
	char pathString[2000];
	DIR *directory;
	struct dirent *files = 0;

	if(!(directory = opendir(rootString))){
		printf("location not found, %s\n", rootString);
		return;
	}

	while((files=readdir(directory))){
		
		if(files->d_type == DT_DIR){

			continue;
		}
			
		sprintf(pathString, "%s/%s", rootString, files->d_name);
/* *** end dirent walk, begin meat of function  *** */
		
		//check for non-txt files
		char *fileEnding = pathString+strlen(pathString)-4;
		if(strcmp(fileEnding, ".txt")){
			printf("skipped: %s\n", files->d_name); 
			continue;
		}
		
		//open a file within root directory
		FILE *input = fopen(pathString, "r");
		if(input){
			//process this file and add it's data to lexicon
			fileGrind(input);
			
			fclose(input);
		}
	}
}


void fileGrind(FILE* textFile){
	char textLine[5000];
	// included python script separates paragraphs into lines
	
	while(fgets(textLine, 4999, textFile)){
	
		if(!strlen(textLine)) continue;
		if(feof(textFile)) break;
	
		//process each line as a context set
		lineGrind(textLine);
	}
}
//form context vector from contents of text, then add that vector to
//all lexicon entries of the words contained
void lineGrind(char* textLine){
	//extract a context vector from this text set
	sparseRIV contextVector = textToL2(textLine);
	
	denseRIV* lexiconRIV;
	//identify stopping point in line read
	char* textEnd = textLine + strlen(textLine)-1;
	int displacement = 0;
	char word[100] = {0};
	while(textLine<textEnd){
		sscanf(textLine, "%99s%n", word, &displacement);
		//we ensure that each word exists, and is free of unwanted characters
		
		if(!(*word))continue;

		if(!isWordClean((char*)word)){
			continue;
		}
		
		
		//we pull the vector corresponding to each word from the lexicon
		//if it's a new word, lexPull returns a 0 vector
		lexiconRIV= lexPull(word);

		//we add the context of this file to this wordVector
		addContext(lexiconRIV, contextVector);
		
		//we remove the sub-vector corresponding to the word itself
		subtractThisWord(lexiconRIV);
		
		//we log that this word has been encountered one more time
		lexiconRIV->frequency += 1;
		
		//and finally we push it back to the lexicon for permanent storage
		lexPush(lexiconRIV);
		textLine += displacement+1;
		
	}
	//free the heap allocated context vector data
	free(contextVector.locations);
	
	
	
	
	
	
	
	
}

void addContext(denseRIV* lexRIV, sparseRIV context){
		
		//add context to the lexRIV, (using sparse-dense vector comparison)
		addS2D(lexRIV->values, context);
		
		//log the "size" of the vector which was added
		//this is not directly necessary, but is useful metadata for some analises
		lexRIV->contextSize += context.contextSize;
		
}


	
