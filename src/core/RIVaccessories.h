#ifndef RIVACCESS_H_
#define RIVACCESS_H_


#include "stemconfig/stemset.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALPHAFILTER 31

typedef struct RIVtreenode{
	void* data;
	struct RIVtreenode* links[32];
	int downstream;
	
}RIVtree;
RIVtree *nextNode;

/* treeInsert, with a memory save for a tree of known size */
void stemInsert(RIVtree* tree, char* word, void* data);

/* removes one element of a tree, along with any now dead trailing branches */
int treecut(RIVtree* tree, char* word);

/* inserts one entry into the word tree, creating a path to itself as necessary */
void treeInsert(RIVtree* tree, char* word, void* data);

/* retrieves a piece of data corresponding to one word */ 
void* treeSearch(RIVtree* tree, char* word);

/* builds a stemtree, accepts an argument:
NULL to indicate a complete and exhaustive tree
non-null, should be a list of stems to include in the tree
*/
RIVtree* stemTreeSetup(char*);

/* used temporarily by the stemtree to optimize shrunken tree creation */
RIVtree* dummyTree(char*);

void destroyTree(RIVtree* tree);

/* creates a standard seed from the characters in a word, hopefully unique */
int wordtoSeed(char* word);


int wordtoSeed(char* word){
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
int clean(char* word){
	
	char* letter = word;
	char* outLetter = word;
	
	while(*letter){
		if(*letter >= 'A' && *letter <= 'Z'){
			*outLetter = *letter + 32;
			outLetter++;
		}else if( *letter >= 'a' && *letter <= 'z'){
			*outLetter = *letter;
			outLetter++;
		}
		letter++;
	}*outLetter = 0;
	
	if(outLetter == word) return 0;
	else return 1;
}
int cleanLine(RIVtree* searchRoot, char* textLine){
	int wordCount = 0;
	char word[199];
	int displacement;
	char* stem;
	char temp[100000] = {0};
	char* textBase = textLine;
	char* textEnd = textLine + strlen(textLine);
	while(textLine<textEnd){
		
		displacement=0;
		sscanf(textLine, "%99s%n", word, &displacement);
		
		textLine += displacement +1;
		if(!displacement)continue;
		
		if(!clean(word))continue;
		
	
		
		if(searchRoot){
			stem = treeSearch(searchRoot, word);
			
		}else{
			stem = word;
		}
		if(!stem){
			textBase[0] = 0;
			continue;
		}
		wordCount++;
		strcat(temp, stem);
		strcat(temp, " ");
		
	}
	
	if(wordCount < 5){
		return 0;
	}
	strcpy(textBase, temp);
	return strlen(textBase);
}


RIVtree* stemTreeSetup(char* selectionFile){
	#include "stemconfig/leafset.h"
	
	RIVtree* rootNode = calloc(treesize, sizeof(RIVtree));
	
	
	RIVtree* referenceTree = dummyTree(selectionFile);
	nextNode = rootNode+1;

	char* stem = stemset;
	char* leaf = leafset;
	int stemDisplacement;
	int leafDisplacement;
	for(int i=0; i<wordCount; i++){
		
		sscanf(leaf, "%*s%n",&leafDisplacement);
		sscanf(stem, "%*s%n", &stemDisplacement);
		stem[stemDisplacement] = '\0';
		leaf[leafDisplacement] = '\0';
		if((!selectionFile) || treeSearch(referenceTree, stem)){
			stemInsert(rootNode, leaf, stem);
		}
		
		stem += stemDisplacement+1;
		leaf += leafDisplacement+1;;
	}
	destroyTree(referenceTree);
	return rootNode;
}

RIVtree* dummyTree(char* fileName){
	
	RIVtree* rootNode = calloc(1, sizeof(RIVtree));
	if(!fileName) return rootNode;
	
	FILE* file = fopen(fileName, "r");
	if(!file) return rootNode;
	int* dummyValue = (int*)2;
	char line[1000];
	char word[100];
	do{
		if(!fgets(line, 1000, file))break;
		sscanf(line, "%s", word);
		treeInsert(rootNode, word, dummyValue);
		
		
	}while(!feof(file));
	fclose(file);
	return rootNode;
}
	
void* treeSearch(RIVtree* node, char* letter){
	
	
	
	if(*(letter)){
		if(!node->links[*(letter)&ALPHAFILTER]){
			return NULL;
		}
		
		return treeSearch(node->links[*(letter)&ALPHAFILTER], letter+1);
	}else{
		
		return node->data;
	}
}
void stemInsert(RIVtree* node, char* letter, void* data){
	
	node->downstream++;
	if(*(letter)){
		if(!node->links[(*letter)&ALPHAFILTER]){
			node->links[(*letter)&ALPHAFILTER] = nextNode++;
			
		}
		treeInsert(node->links[(*letter)&ALPHAFILTER], letter+1, data);
		
	}else{
		
		if(node->data) return;
		node->data = data;
		

		
	}
}
void treeInsert(RIVtree* node, char* letter, void* data){
	node->downstream++;
	if(*(letter)){
		
		if(!node->links[(*letter)&ALPHAFILTER]){
			node->links[(*letter)&ALPHAFILTER] = calloc(1, sizeof(RIVtree));
			
		}
		treeInsert(node->links[(*letter)&ALPHAFILTER], letter+1, data);
		
	}else{
		
		if(node->data) return;
		node->data = data;
		

		
	}
}

int treecut(RIVtree* node, char* letter){
	node->downstream--;
	int flag;
	//continue searching downstream if there is a letter
	if(*(letter)){
		if(node->links[*(letter) | ALPHAFILTER]){
			//propagate to next section
			flag = treecut(node->links[*(letter) &ALPHAFILTER], letter+1);
			//if next section returned a "cut" flag, 0 it out
			if(flag){
				node->links[*(letter)&ALPHAFILTER] = NULL;
			}
		}
	//there are no more letters, we've reached our destination
	}else{
		
		node->data = NULL;
	}
	//this is on a branch that leads nowhere, free it and return "cut" flag
	if(!node->downstream){
			
		free(node);
		return 1;
	}
	return 0;
	
	
}
void destroyTree(RIVtree* node){
	//if(node->data) free(node->data);
	for(int i=0; i<32; i++){
		if(node->links[i]){
			
			destroyTree(node->links[i]);
		}
		
	}
	free(node);
	
}

#endif
