import pymongo
from pymongo import MongoClient


def dbSetup():
    client = MongoClient("mongodb://etcart:Argelfraster1@ds261969.mlab.com:61969/rivwordnet")
    database = client.rivwordnet
    collection = database.stems

    collection.create_index("from")
    return collection


def dbPost(wordset, collection):
    if not len(wordset):
        return

    posts = []
    for key, value in wordset.iteritems():
        post = {"from": key, "to": value}
        posts.append(post)

    collection.insert_many(posts)

def cleanDbSetup():
    client = MongoClient("mongodb://etcart:Argelfraster1@ds163119.mlab.com:63119/rivetcleandocs")
    database = client.rivetcleandocs
    collection = database.cleaned
    collection.create_index("file")
    return collection

def dbPostCleaned(text, file, collection):
    if not len(text):
        return
    document = {
        "text": text,
        "file": file,
    }
    collection.insert_one(document)



def dbGet(words, collection):



    if mebewords:
        return mebeword["to"]
    else:
        return 0