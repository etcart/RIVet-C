#ifndef RIVACCESS_H_
#define RIVACCESS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stemconfig/stemset.h"

/* for making a word-search tree.  26 links is because it expects only 
 * lower-case, english language characters */
struct treenode{
	void* data;
	struct treenode* links[26];
	int downstream;
	
}*nextNode;

/* used in creating  a stemTree, this passes a pointer to a stack-held stem,
 * to be found by searching its leaf */
void stemInsert(struct treenode* node, char* letter, void* data);

/* removes the chosen word from the search tree, along with closing 
 * pruning the tree as far back as possible without interfering with other
 * entries
 */
int treecut(struct treenode* node, char* letter);


/* general insert into word-search tree, accepts void-pointers for generic
 * use. used internally in the lex-cache logic.
 */
void treeInsert(struct treenode* node, char* letter, void* data);

/* used both in stem searching and also cache-searching, returns a pointer
 * to memory or a null pointer if the entered word is linked to nothing
 */
void* treeSearch(struct treenode* node, char* letter);

/* a wrapper for treeSearch() for easier reading in code */
char* getStem(struct treenode* searchRoot, char* word);

/* builds a tree of english language word-stems, using files and tools in 
 * the stemconfig folder.  included python script: stemconfig.py can be run
 * to sync the stemconfig folder with a stem-network database
 */
struct treenode* stemTreeSetup();

/* creates a standard seed from the characters in a word, hopefully unique */
int wordToSeed(char* word);

int cleanLine(char* textLine, struct treenode* stemRoot);

int wordToSeed(char* word){
	int i=0;
	int seed = 0;
	while(*word){
		/* left-shift 5 each time *should* make seeds unique to words
		 * this means letters are taken as characters counted in base 32, which
		 * is large enough to hold all english characters plus a few outliers
		 * additionally, it is caps-insensitive */
		seed += (*(word)%32)<<(i*5);
		word++;
		i++;
	}
	return seed;
}

char* getStem(struct treenode* searchRoot, char* word){
	return treeSearch(searchRoot , word);
}

int clean(char* word){
	
	char* letter = word;
	char* outLetter = word;
	
	/* scan through the word */
	while(*letter){
		/* reduce upper case to lower case */
		if(*letter >= 'A' && *letter <= 'Z'){
			*outLetter = *letter + 32;
			outLetter++;
		/* keep lower case */
		}else if( *letter >= 'a' && *letter <= 'z'){
			*outLetter = *letter;
			outLetter++;
		}
		letter++;
		/* null terminator at end */
	}*outLetter = 0;
	
	/* if no characters made it through the filter, return 0 for "empty" */
	if(outLetter == word) return 0;
	else return 1;
}

int cleanLine(char* textLine, struct treenode* stemRoot){
	
	char word[100];
	int displacement;
	char temp[100000] = {0};
	
	char* textBase = textLine;
	char* textEnd = textLine + strlen(textLine);
	while(textLine<textEnd){
		displacement = 0;
		sscanf(textLine, "%99s%n", word, &displacement);
		
		textLine += displacement+1;
		//we ensure that each word exists, and is free of unwanted characters
		
		if(!(*word))continue;
		clean(word);
		/* we seek the stem of the word (if it exists) */
		char* stem = treeSearch(stemRoot, word);
		if(!stem) continue;
		
		/*the stem is added to the end of the line, along with a space*/
		strcat(temp, stem);
		strcat(temp, " ");
		
		
	}	
	/* copy the cleaned text back into the old textSlot */
	strcpy(textBase, temp);
	
	return strlen(textBase);
}

struct treenode* stemTreeSetup(){
	/* a stack-temporary set of original words, to be linked to their stems */
	#include "stemconfig/leafset.h"
	struct treenode* rootNode = calloc(treesize, sizeof(struct treenode));
	nextNode = rootNode;
	char* stem = stemset;
	char* leaf = leafset;
	int stemDisplacement;
	int leafDisplacement;
	
	
	for(int i=0; i<wordCount; i++){
		/* scan through both super-strings full of words */
		sscanf(leaf, "%*s%n",&leafDisplacement);
		sscanf(stem, "%*s%n", &stemDisplacement);
		stem[stemDisplacement] = '\0';
		leaf[leafDisplacement] = '\0';
		
		/* insert each stem under its corresponding leaf in the tree */
		stemInsert(rootNode, leaf, stem);
	
		stem += stemDisplacement+1;
		leaf += leafDisplacement+1;;
	}
	return rootNode;
}

	
void* treeSearch(struct treenode* node, char* letter){
	
	/* if the null-terminator has been reached, return whatever data is contained */
	if(!*(letter)){
		return node->data;
	}else{
		/* if we will try to search a non-lower-case letter, quit.  accepts
		 * only clean words */
		if(*letter > 'z' || *letter < 'a'){
			
			return NULL;
		}
		/* if there is nothing down that path, return NULL */
		if(!node->links[*(letter)-'a']){
			
			return NULL;
		}
		/* search deeper down the tree */
		return treeSearch(node->links[*(letter)-'a'], letter+1);
		
	}
}

void treeInsert(struct treenode* node, char* letter, void* data){
	/* flag that there are entries down this path, for use by treecut()*/
	node->downstream++;
	if(!*(letter)){
		/* if the null terminator is reached, insert data at this point */
		node->data = data;
		
		
	}else{
		/* otherwise, allocate the next link if necessary */
		if(!node->links[*(letter)-'a']){
			node->links[*(letter)-'a'] = calloc(1, sizeof(struct treenode));
			
		}
		/* and dive further down the tree, checking the next letter */
		treeInsert(node->links[*(letter)-'a'], letter+1, data);
	}
}
/* stemInsert is memory optimized because we can know exactly how big it is
 * while compiling.  the key difference is in "nextNode++" which "allocates"
 * the next link of the tree as the next, tightly packed treenode in order
 * */
void stemInsert(struct treenode* node, char* letter, void* data){
	node->downstream++;
	if(!*(letter)){
		node->data = data;

	}else{

		if(!node->links[*(letter)-'a']){
			node->links[*(letter)-'a'] = nextNode++;
			
		}
		stemInsert(node->links[*(letter)-'a'], letter+1, data);
				
		

		
	}
}
int treecut(struct treenode* node, char* letter){
	node->downstream--;
	int flag;
	//continue searching downstream if there is a letter
	if(*(letter)){
		if(node->links[*(letter)-'a']){
			//propagate to next section
			flag = treecut(node->links[*(letter)-'a'], letter+1);
			//if next section returned a "cut" flag, 0 it out
			if(flag){
				node->links[*(letter)-'a'] = NULL;
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
void destroyTree(struct treenode* node){
	//if(node->data) free(node->data);
	for(int i=0; i<26; i++){
		if(node->links[i]){
			
			destroyTree(node->links[i]);
		}
		
	}
	free(node);
	
}

#endif
