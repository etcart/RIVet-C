import requests
import re
import string
import os
import sys
from subprocess import call
import nltk
from nltk.corpus import wordnet as wn
import pdb
from nltk.stem import PorterStemmer

def adverbFix(word):
    if not nltk.pos_tag(word)[0][1] == 'RB':
        return word

    adjective = word[:-2]
    if not nltk.pos_tag(word)[0][1] == 'JJ':
        return word;
    FILE = open("lexicon/" + word, "w")
    FILE.write("2" + temp)
    FILE.close()
    FILE = open("lexicon/" + adjective, "w")
    FILE.write("1")
    FILE.close()
    return adjective

def strip(word):
    for suffix in ['ing', 'ly', 'ed', 'ious', 'ies', 'ive', 'es', 's', 'ment']:
        if word.endswith(suffix):
            return word[:-len(suffix)]
        return word


def cleanWord(word):
    #if(len(word) == 0):
        #print("\n\n***************\n\n***************\n\n***************\n\n***************\n\n***************\n\n***************\n\n***************\n\n***************\n\n***************\n\n***************\n\n***************\n\n***************\n\n***************\n\n***************\n\n***************")
    word = word.lower();
    regex = re.compile('[^a-z]+')
    word = regex.sub('', word)
    #print(word)
    return word


def fileCheck(word):

    try:
        #print("trying")
        wordFile = open("lexicon/{}".format(word), "r")
        code = int(wordFile.read(1))
    except:
        #print("file does not exist")
        return 0
    #print("fileCode{}".format(code))

    if code == 2:
        word = wordFile.read()
        #print("file flipped to: " + word)
        wordFile.close()
        return word
    elif code == 1:
        #print("file accepted: " + word)
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
pathString = "cleanbooks/" + pathString + "clean/"
print(sourceString + "\n")

if not os.path.exists('cleanbooks'):
    os.makedirs('cleanbooks')
if not os.path.exists('lexicon'):
    os.makedirs('lexicon')

if not os.path.exists(pathString):
    os.makedirs(pathString)

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
            i += 1
            fileOut = open("{}{}.txt".format(pathString, i), "w")
            for line in paragraph.split(os.linesep):

                for tempWord in line.split():
                    word=cleanWord(tempWord)

                    if not word:
                        continue

                    # temp = fileCheck(word)
                    #
                    # if temp == -1:
                    #     continue
                    # if temp == 0:
                    temp = morphyTest(word)
                    if temp:
                        stem = ps.stem(temp)
                        if stem and not stem in blacklist:
                            cleanString = cleanString + ' ' + stem



                    #if temp == 0:
                    #    catchAll(word)
                cleanString = cleanString + os.linesep
            if len(cleanString.split(' ')) > 10:
                
                fileOut.write(cleanString)
                fileOut.close()
            else:
                
                fileOut.close()
                os.remove("{}{}.txt".format(pathString, i))
                i -= 1


if skip==1:
    print(sourceString + " was badly parsed, no output");
