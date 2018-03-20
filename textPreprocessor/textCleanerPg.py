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

#cleanword will pull all to lowerCase and remove any non-letter characters
def cleanWord(word):
    
    word = word.lower();
    regex = re.compile('[^a-z]+')
    word = regex.sub('', word)
 
    return word

#nltk morphy will attempt a preliminary noise reduction, and flag if not a word
def morphyTest(word):
    morphyTemp = wn.morphy(word)

    if not morphyTemp:
        return 0

    return morphyTemp;

#blacklist lists words we remove as noise
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

#document will be split into paragraph sized documents, in the following directory
if not os.path.exists(pathString):
    os.makedirs(pathString)

call(["python", "blacklist.py"])
i=0
# to begin with, we expect a project gutenberg header, legal info that we skip
skip = 1
with open(sourceString, 'U') as fileIn:

    text = fileIn.read()
	#project gutenberg delimits paragraphs by double returns
    for paragraph in text.split(2*os.linesep):

        if not paragraph:
            continue
        #two different wordings for the begining of the actual content
        elif "*** START OF " in paragraph or "*END THE SMALL PRINT" in paragraph:
            skip = 0
            continue
		#three different wordings for the end of the actual content
        elif "*** END OF " in paragraph or "End of Project Gutenberg's" in paragraph or "End of the Project Gutenberg" in paragraph:
            fileIn.close()
            sys.exit()
		#if we have passed the header, begin processing
        if not skip:
            cleanString = ''
            i += 1
            #create a numbered text file to hold one paragraph
            fileOut = open("{}{}.txt".format(pathString, i), "w")
            
            
            for line in paragraph.split(os.linesep):#os.linesep may need to be changed for operating system

                for tempWord in line.split():
					#remove extraneous characters and lower-case
                    word=cleanWord(tempWord)

                    if not word:
                        continue
					#is it a word?
                    temp = morphyTest(word)
                    if temp:
						#if the word passes morphy test, stem it
                        stem = ps.stem(temp)
                        #if not blacklisted, add it to the accumulating paragraph with a space
                        if stem and not stem in blacklist:
                            cleanString = cleanString + ' ' + stem



                
                cleanString = cleanString + os.linesep
                #trim paragraphs too small to be meaningful
            if len(cleanString.split(' ')) > 5:
                
                fileOut.write(cleanString)
                fileOut.close()
            else:
                
                fileOut.close()
                os.remove("{}{}.txt".format(pathString, i))
                i -= 1


if skip==1:
    print(sourceString + " was badly parsed, no output");
