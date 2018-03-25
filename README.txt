This library is a toolkit for The RIV system.  this is a machine learning
system based fundamentally on the principles of the better known bag-of-words,
but which uses long-term aggregated learning to model inter-word relationships
in more detail.  
its key benefits are, in theory:
-little to no pre-knowledge necessary of the subject data
-wide ranging input vocabulary
-"human" treatment of synonyms and other relations in model building

its key downside is:
-massive size

RIV stands for Random Index Vector, referring to the method of generating
the basic vectors that correspond to each word.  each word has an algorithmically
generated vector which represents it in this mathematical model, such that a word
will produce the same vector each time it is encountered*[1]. this base
vector will be referred to as a L1 vector or a barcode vector

by summing these vectors, we can get a mathematical representation of
a set of text.  this summed vector will be referred to as an L2 vector
or aggregate vector.  in its simplest implimentation, an L2 vector
representation of a document contains a model of the contents of the 
document, enabling us to compare direction and magnitude of document 
vectors to understand their relationships to each other.

but the system we are really interested in is the ability to form 
context vectors
a context vector is the sum of all (L1?) vectors that the word
has been encountered in context with. from these context vectors
certain patterns and relationships between words should emerge. 
what patterns? that is the key question we will try to answer

[1] a word produces the same vector each time it is encountered only 
if the environment is the same, ie. RIVs are the same dimensionality
nonzero count is the same.  comparing vectors produced in different 
dimesionalities yields meaningless drivel and should be avoided

[2] what exactly "context" means remains a major stumbling point.
paragraphs?  sentences?  some potential analyses would expect a static
sized context (the nearest 10 words?) in order to be sensible, but 
it may be that some other definition of context is the most valid for
this model.  we will have to find out.

 Why C?
  this system is inherently fairly simple, without much need for 
High level abstraction.  it is, however, of extraordinary 
computational complexity. in order to have differentiability
between any possible word, instead of a selected set of words
(as in the bag of words system) it must operate at extreme 
dimensionalities.  C language gives us the control we need 
to optimize this system into viability without extreme hardware
 
some notes:
  
SparseRIV vs. denseRIV (sparse vector vs. dense vector):
the two primary data structures we will use to analyze these vectors
each vector type is packed with some metadata 
(name, magnitude, frequency, flags)

-denseRIV is a standard vector representation.  
each array index corresponds to a dimension
each value corresponds to a measurement in that dimension
-in this library denseRIV is used mainly for accumulating data
in the lexicon

-sparseRIV is vector representation optimized for largely empty vectors
each data point is a location/value pair where the
location represents array index 
value represents value in that array index
-in this library sparseRIV is the standard go-to representation

if we have a sparsely populated dense vector (mostly 0s) such as:

|0|0|5|0|0|0|0|0|4|0|

there are only 2 values in a ten element array. this could, instead
be represented as

|2|8| array indexes
|5|4| array values
|2|   record of size

and so, a 10 element vector has been represented in only 5 integers
