
import re
import string
import os
import sys
from subprocess import call
import nltk
from nltk.corpus import wordnet as wn
import pdb
from nltk.stem import PorterStemmer

#here we log this word-stem relationship in a file lexicon for future knowledge
def writeWord(cleanString, word, stem, blacklist):
    #if our original word was a valid stem-form (was already checked in the beginning for blacklist)
    if word == stem:
        #log in the file "1" for valid word
        FILE = open("lexicon/" + word, "w")
        FILE.write("1");
        FILE.close();
        return (cleanString + " " + word)
    #if our stem is not blacklisted
    elif stem not in blacklist:
        #to reduce noise, no valid words are less than 3 letters
        if len(stem) > 2:
            #log in the word"s file a "2" for real but deformed
            FILE = open("lexicon/" + word, "w")
            #and write the target stem to that file
            FILE.write("2"+stem);
            FILE.close();
            #log the stem as a valid word
            FILE = open("lexicon/" + stem, "w")
            FILE.write("1")
            FILE.close();
            return (cleanString + stem + " ")

    return cleanString
	
#adjectives ending in "li" are problematic alternate forms of the same root word
def liFix(word):
    #if the stem doens't end with "li"
    if not word[len(word)-2:] == "li":
        return word
    #if the truncated form has a valid stem
    temp = ps.stem(word[:-2])
    if temp:
        return temp
    #otherwise, use the original word
    return word

#remove all non-letter characters and reduce all caps to lower-case
def cleanWord(word):
    word = word.lower();
    regex = re.compile('[^a-z]+')
    word = regex.sub('', word)
    return word


#as we read, we form a lexicon of word-relations for easy machine learning
def fileCheck(word):
    try:
	#if the file for this word exists: carry on
        wordFile = open("lexicon/{}".format(word), "r")
	#grab a code from the first character
        code = int(wordFile.read(1))
    except:

        #if the file does not exist exit and return 0
        return 0
    
    #code 2 indicates valid word in invalid form
    if code == 2:
        #correct word form is found in the remainder of the file
        word = wordFile.read()
  	
        wordFile.close()
        return word
    #code 1 indicates valid word-form
    elif code == 1:
	
        wordFile.close()
        return word
    #code 0 indicates valid word blacklisted (articles, etc.)
    elif code == 0:
	 
        wordFile.close()
        return -1


#wn.morphy does a good job of corrected words and some base forms
def morphyTest(word):
    morphyTemp = wn.morphy(word)
    if not morphyTemp:
        #returning 0 indicates a non-word
        return 0

    return morphyTemp;


#begin mainfunction

#a shortlist of common "valid" words that we want to remove
blacklist = ["a", "an", "the", "so", "as", "how",
             "i", "me", "we", "they", "you", "it", "he", "she",
             "but", "have", "had",
             "for", "by", "in", "out", "as", "not"
             "be", "were", "was", "am", "are", "is",
             "mr", "mrs", "mr", "and"]
word = {}
ps = PorterStemmer()
sourceString = sys.argv[1]

#parse our argv for path and name
cutDirectories = sourceString.split('/')[-1]
pathString = cutDirectories.split('.')[0]
pathString = "cleanbooks/" + pathString + "clean.txt"

print(sourceString)

#ensure necessary directories exist
if not os.path.exists('cleanbooks'):
    os.makedirs('cleanbooks')
if not os.path.exists('lexicon'):
    os.makedirs('lexicon')

#top of the file will be a legal header, skipped for noise reduction
skip = 1
with open(sourceString, 'U') as fileIn:
    
    text = fileIn.read()
    #project gutenberg separates paragraphs by double-returns
    for paragraph in text.split(2*os.linesep):
	#skip empty paragraphs
        if not paragraph:
            continue
	#these strings indicate the end of the header, begining of body
        elif "*** START OF " in paragraph or "*END THE SMALL PRINT" in paragraph:
            skip = 0once
            fileOut = open(pathString, "w")
            continue
	#these strings indicate the beginning of footer, again skipped for noise
        elif "*** END OF " in paragraph:
            fileIn.close()
            sys.exit()
        elif "End of Project Gutenberg's" in paragraph:
            fileIn.close()
            sys.exit()
        elif "End of the Project Gutenberg" in paragraph:
            fileIn.close()
            sys.exit()
	#
        if not skip:
            cleanString = ''
            
            for line in paragraph.split(os.linesep):

                for tempWord in line.split():
                    word=cleanWord(tempWord)
                    if not word:
                        continue
                    if len(word) < 3:
						continue;
                    if word in blacklist:
						continue;


                    temp = fileCheck(word)
                    if temp == -1:
                        continue
                    if temp:
                        cleanString = (cleanString + temp + " " );
                        continue
						
                    else:
                        morphy = morphyTest(word)
                        if morphy:
                            stem = ps.stem(morphy)
                            if stem:
				stem = liFix(stem)
                                cleanString = writeWord(cleanString, word, stem, blacklist)

                cleanString = cleanString
            if len(cleanString.split(' ')) > 4:
                
                fileOut.write(cleanString+os.linesep)
           

if skip==1:
    print(sourceString + " was badly parsed, no output");
