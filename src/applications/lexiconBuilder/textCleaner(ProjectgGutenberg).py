
import re
import string
import os
import sys
from subprocess import call
import nltk
from nltk.corpus import wordnet as wn
import pdb
from nltk.stem import PorterStemmer


def writeWord(cleanString, word, stem, blacklist):
    if word == stem:
        FILE = open("lexicon/" + word, "w")
        FILE.write("1");
        FILE.close();
        return (cleanString + " " + word)
        
    elif stem not in blacklist:
        if len(stem) > 2:
            FILE = open("lexicon/" + word, "w")
            FILE.write("2"+stem);
            FILE.close();
            FILE = open("lexicon/" + stem, "w")
            FILE.write("1")
            FILE.close();
            return (cleanString + " " + stem)

    return cleanString
	

def liFix(word):
    if not word[len(word)-2:] == "li":
        return word
    
    temp = ps.stem(word[:-2])
    if temp:
        return temp
    return word

def cleanWord(word):
    word = word.lower();
    regex = re.compile('[^a-z]+')
    word = regex.sub('', word)
    #print(word)
    return word


def fileCheck(word):

    try:
        
        wordFile = open("lexicon/{}".format(word), "r")
        code = int(wordFile.read(1))
    except:
        return 0

    if code == 2:
        word = wordFile.read()
   
        wordFile.close()
        return word
    elif code == 1:

        wordFile.close()
        return word
    elif code == 0:
        wordFile.close()
        return -1

def morphyTest(word):
    morphyTemp = wn.morphy(word)

    if not morphyTemp:
        return 0

    return morphyTemp;


#begin mainfunction

blacklist = ["a", "an", "the", "so", "as", "how",
             "i", "me", "we", "they", "you", "it", "he", "she",
             "but", "have", "had",
             "for", "by", "in", "out", "as", "not"
             "be", "were", "was", "am", "are", "is",
             "mr", "mrs", "mr", "and"]
word = {}
ps = PorterStemmer()
sourceString = sys.argv[1]
cutDirectories = sourceString.split('/')[-1]
pathString = cutDirectories.split('.')[0]
pathString = "cleanbooks/" + pathString + "clean.txt"
print(sourceString + "\n")

if not os.path.exists('cleanbooks'):
    os.makedirs('cleanbooks')
if not os.path.exists('lexicon'):
    os.makedirs('lexicon')

call(["python", "blacklist.py"])
i=0
skip = 1
with open(sourceString, 'U') as fileIn:

    text = fileIn.read()

    for paragraph in text.split(2*os.linesep):

        if not paragraph:
            continue
        elif "*** START OF " in paragraph or "*END THE SMALL PRINT" in paragraph:
            skip = 0
            fileOut = open(pathString, "w")
            continue
        elif "*** END OF " in paragraph:
            fileIn.close()
            sys.exit()
        elif "End of Project Gutenberg's" in paragraph:
            fileIn.close()
            sys.exit()
        elif "End of the Project Gutenberg" in paragraph:
            fileIn.close()
            sys.exit()
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
