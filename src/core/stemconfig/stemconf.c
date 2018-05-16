#include <stdio.h>
#include "../RIVaccessories.h"
int configInsert(struct treenode* node, char* letter, int treeSize);
int stemTreeConfig();
int main(){
	int count = stemTreeConfig();
	printf("%d", count);
	
}

int configInsert(struct treenode* node, char* letter, int treeSize){
	
	node->downstream++;
	if(*(letter)){
		if(!node->links[*(letter)-'a']){
			treeSize++;
			node->links[*(letter)-'a'] = calloc(1, sizeof(struct treenode));
			
		}
		return configInsert(node->links[*(letter)-'a'], letter+1, treeSize);
	}else{
		return treeSize;
	}
}

int stemTreeConfig(){
	int treeSize = 1;
	FILE* wordFile = fopen("wordset.txt", "r");
	if(!wordFile){
		printf("no wordnet file");
		return 0;
	}
	
	struct treenode* rootNode = calloc(1, sizeof(struct treenode));
	char word[100];
	char* stem = (char*)stemset;
	int displacement;
	while(fscanf(wordFile, "%s", word)){
		sscanf(stem, "%*s%n", &displacement);
		stem[displacement] = '\0';
		
		
		treeSize = configInsert(rootNode, word, treeSize);
		if(feof(wordFile)){
			break;
		}
		stem += displacement+1;
	}
	fclose(wordFile);
	return treeSize;
}
