#ifndef RIVACCESS_H_
#define RIVACCESS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stemconfig/stemset.h"

struct treenode{
	void* data;
	
	struct treenode* links[26];
	int downstream;
	
}*nextNode;
void stemInsert(struct treenode* node, char* letter, void* data);
int treecut(struct treenode* node, char* letter);

void treeInsert(struct treenode* node, char* letter, void* data);
void* treeSearch(struct treenode* node, char* letter);
struct treenode* stemTreeSetup();

/*isWordClean filters words that contain non-letter characters, and 
 * upperCase letters, allowing only the '_' symbol through
 */
int isWordClean(char* word);

/* used by wordClean */
int isLetter(char c);

/* creates a standard seed from the characters in a word, hopefully unique */
int wordtoSeed(char* word);

int isLetter(char c){
	
	if((c>96 && c<123)||(c == 32)) return 1;
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
struct treenode* stemTreeSetup(){
	FILE* wordFile = fopen("stemconfig/wordset.txt", "r");
	if(!wordFile){
		printf("no wordnet file");
		return 0;
	}
	
	struct treenode* rootNode = calloc(treesize, sizeof(struct treenode));
	nextNode = rootNode+1;
	char word[100];
	char* stem = (char*)stemset;
	int displacement;
	while(fscanf(wordFile, "%s", word)){
		
		sscanf(stem, "%*s%n", &displacement);
		stem[displacement] = '\0';
		
		
		stemInsert(rootNode, word, stem);
		if(feof(wordFile)){
			break;
		}
		stem += displacement+1;
	}
	fclose(wordFile);
	return rootNode;
}

	
void* treeSearch(struct treenode* node, char* letter){
	
	
	
	if(*(letter)){
		if(*letter > 'z' || *letter < 'a'){
			printf("bad letter %c :: %s", *letter, letter);
			return NULL;
		}
		if(!node->links[*(letter)-'a']){
			return NULL;
		}
		
		return treeSearch(node->links[*(letter)-'a'], letter+1);
	}else{
		
		return node->data;
	}
}
void stemInsert(struct treenode* node, char* letter, void* data){
	
	node->downstream++;
	if(*(letter)){
		if(!node->links[*(letter)-'a']){
			node->links[*(letter)-'a'] = nextNode++;
			
		}
		treeInsert(node->links[*(letter)-'a'], letter+1, data);
		
	}else{
		
		if(node->data) return;
		node->data = data;
		

		
	}
}
void treeInsert(struct treenode* node, char* letter, void* data){
	
	node->downstream++;
	if(*(letter)){
		if(!node->links[*(letter)-'a']){
			node->links[*(letter)-'a'] = calloc(1, sizeof(struct treenode));
			
		}
		treeInsert(node->links[*(letter)-'a'], letter+1, data);
		
	}else{
		
		if(node->data) return;
		node->data = data;
		

		
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
