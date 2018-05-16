import dbtools
from subprocess import call

collection = dbtools.dbSetup()

preset = collection.find()
set = {}
for doc in preset:
	set[doc["from"]] = doc["to"]
words = [];
stems = [];
for key, value in set.iteritems():
	words.append(key);
	stems.append(value);
	
wordFILE = open("wordset.txt", "w")
wordFILE.write(' '.join(words));
wordFILE.close()
stemFILE = open("stemset.h", "w")
finalOut = 'char stemset[] = "' + ' '.join(stems) + ' ";'+'\nint treesize = '

stemFILE.write(finalOut + '0;')

stemFILE.close()

tempfile = open("tempfile.txt", "w")
call(["gcc", "stemconf.c","-o", "stemconfig"])
call(["./stemconfig"], stdout=tempfile)
tempfile.close()
tempfile = open("tempfile.txt", "r")
treesize = tempfile.read();
finalOut = finalOut + treesize + ';'
stemFile = open("stemset.h", "w")
stemFile.write(finalOut)
stemFile.close;

