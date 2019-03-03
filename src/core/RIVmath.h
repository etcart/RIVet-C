#ifndef RIVMATH_H_
#define RIVMATH_H_
#include "math.h"
#include "RIVlower.h"



/* these macros allow cosine to be converted to a variety of related 
 * distance measurements, with their own uses and peculiarities
 */
/* this yields sine distance: 
 * if -90 < angle < 90: abs(sin)
 * if angle >90 or <-90: 1 */
#define COS2SIN(cos) (cos>1? 0: cos < 0? 1: sqrt(1-cos*cos)) 

/* this yields tan distance:
 * if if -90 < angle < 90: abs(tan)
 * if angle >90 or <-90: FLTMAX (approx. infinity) */
#define COS2TAN(cos) (cos>1? 0: cos < 0? FLTMAX: sqrt(1-cos*cos)/cos)

/* this yields secant distance:
 * if if -90 < angle < 90: sec-1
 * if angle >90 or <-90: FLTMAX (approx. infinity) */
#define COS2SEC(cos) (cos>1? 0: cos < 0? FLTMAX: 1/cos -1)


/* this macro is used to detect and compensate for integer overflows in 
 * the calculation of vector magnitudes 
 */
#define OVERFLOWCHECK 1L<<63

enum RIVTYPES{
	SPARSERIVP,
	DENSERIVP
};

#define TYPECHECK(x) _Generic((x),\
	struct sparseRIV* : SPARSERIVP,\
	struct denseRIV* : DENSERIVP\
)



/* calculates the magnitude of a sparseVector */
double getMagnitudeSparse(void* input);

/* same for denseVector */
double getMagnitudeDense(void* input);
double (*magP[2]) (void* input) = {getMagnitudeSparse,getMagnitudeDense};
	
/* selects the correct magnitude calculation */
#define RIVMagnitude(vector) \
	magP[TYPECHECK(vector)](vector)

double cosCompareS2S(void* vector1, void* vector2);
double cosCompareS2D(void* dense, void* sparse);
double cosCompareD2S(void* sparse, void* dense);
double cosCompareD2D(void* vector1, void* vector2);


double (*cosP[4]) (void*, void*) = {cosCompareS2S, cosCompareD2S, cosCompareS2D, cosCompareD2D};

#define RIVcosCompare(vectorA, vectorB)\
	cosP[TYPECHECK(vectorA)*2+TYPECHECK(vectorB)](vectorA, vectorB);


void addS2S(void* vector1, void* vector2);
void addS2D(void* destinationV, void* inputV);
void addD2S(void* sparse, void* dense);
void addD2D(void* vector1, void* vector2);

void (*addP[4]) (void*, void*) = {addS2S, addD2S, addS2D, addD2D};

#define addRIV(destination, source)\
	addP[TYPECHECK(destination)*2+TYPECHECK(source)](destination, source);

void addS2D(void* destinationV, void* inputV){
	sparseRIV* input = (sparseRIV*)inputV;
	int* destination = ((denseRIV*)destinationV)->values;
	int *locations_slider = input->locations;
	int *values_slider = input->values;
	int *locations_stop = locations_slider+input->count;
	
	/* apply values at an index based on locations */
	while(locations_slider<locations_stop){
		
		destination[*locations_slider] += *values_slider;
		locations_slider++;
		values_slider++;
	}
	
}
void addD2D(void* destinationV, void* inputV){
	int* input = ((denseRIV*)inputV)->values;
	int* destination = ((denseRIV*)destinationV)->values;
	/* apply values at an index based on locations */
	for(int i=0; i<RIVSIZE; i++){	
		destination[i] += input[i];
	}
	
}
void addS2S(void* destinationV, void* inputV){
	sparseRIV* input = (sparseRIV*)inputV;
	sparseRIV* destination = (sparseRIV*)destinationV;
	int *locations_slider = destination->locations;
	int *values_slider = destination->values;
	int *locations_stop = locations_slider+destination->count;
	
	/* apply values at an index based on locations */
	while(locations_slider<locations_stop){
		for(int i=0; i<input->count; i++){
			if(input->locations[i] == *locations_slider){
				*values_slider += input->values[i];
			}
		}
		locations_slider++;
		values_slider++;
	}

}
void addD2S(void* destinationV, void* inputV){
	sparseRIV* input = (sparseRIV*)destinationV;
	int* destination = ((denseRIV*)inputV)->values;
	int *locations_slider = input->locations;
	int *values_slider = input->values;
	int *locations_stop = locations_slider+input->count;
	
	/* apply values at an index based on locations */
	while(locations_slider<locations_stop){
		
		*values_slider += destination[*locations_slider] ;
		locations_slider++;
		values_slider++;
	}
	
	
}

double getMagnitudeDense(void *inputV){
	size_t temp = 0;
	size_t accumulate = 0;
	double divisor = 1;
	int *values = ((denseRIV*)inputV)->values;
	int *values_stop = values+RIVSIZE;
	while(values<values_stop){
		if(*values){
			temp +=((size_t)*values)*((size_t)*values);
		}
		values++;
		
		/* integer overflow is a concern.  this checks for overflows, sacrificing small amounts of accuracy */
		if(OVERFLOWCHECK & temp){
			/* accumulate holds an increasingly compact version of the sum of squares, as necessary */
			accumulate*= divisor/(divisor+1);
			divisor+=1;
			accumulate = temp/divisor;
			temp=0;	
			
		}
	}
	
	/* add the temp holder, scaled in line with the accumulate value */
	accumulate += temp/divisor;
	/* take the square root, and scale it back to its original form */
	return sqrt(accumulate)*sqrt(divisor);
}
double getMagnitudeSparse(void* inputV){
	size_t temp = 0;
	size_t accumulate = 0;
	double divisor = 1;
	int *values = ((sparseRIV*)inputV)->values;
	int *values_stop = values+((sparseRIV*)inputV)->count;
	while(values<values_stop){
		/* we sum the squares of all elements */
		temp += ((size_t)*values)*((size_t)*values);
		values++;
		if(OVERFLOWCHECK & temp){
			accumulate*= divisor/(divisor+1);
			divisor+=1;
			accumulate = temp/divisor;
			temp=0;	
			
		}
	}
	/* we take the root of that sum */
	accumulate += temp/divisor;
	return sqrt(accumulate)*sqrt(divisor);
}

double cosCompareS2D(void* dense, void* sparse){

	sparseRIV* comparator = (sparseRIV*)sparse;
	denseRIV* baseRIV = (denseRIV*)dense;
	long long int dot = 0;
	int* locations_stop = comparator->locations+comparator->count;
	int* locations_slider = comparator->locations;
	int* values_slider = comparator->values;
	while(locations_slider<locations_stop){

		/* we calculate the dot-product to derive the cosine 
		 * comparing sparse to dense by index*/
		dot += (long long int)*values_slider * (long long int)baseRIV->values[*locations_slider];
		locations_slider++;
		values_slider++;

	}
	/*dot divided by product of magnitudes */

	return dot/(baseRIV->magnitude*comparator->magnitude);
}
double cosCompareD2S(void* sparse, void* dense){
		
	sparseRIV* comparator = (sparseRIV*)sparse;
	denseRIV* baseRIV = (denseRIV*)dense;
	long long int dot = 0;
	int* locations_stop = comparator->locations+comparator->count;
	int* locations_slider = comparator->locations;
	int* values_slider = comparator->values;
	while(locations_slider<locations_stop){

		/* we calculate the dot-product to derive the cosine 
		 * comparing sparse to dense by index*/
		dot += (long long int)*values_slider * (long long int)baseRIV->values[*locations_slider];
		locations_slider++;
		values_slider++;

	}
	/*dot divided by product of magnitudes */

	return dot/(baseRIV->magnitude*comparator->magnitude);
}
double cosCompareD2D(void* vector1, void* vector2){
	denseRIV* comparator = (denseRIV*)vector2;
	denseRIV* baseRIV = (denseRIV*)vector1;
	long long int dot = 0;
	for(int i=0; i<RIVSIZE; i++){
		dot+= baseRIV->values[i]*comparator->values[i];		
		
	}
	/*dot divided by product of magnitudes */

	return dot/(baseRIV->magnitude*comparator->magnitude);
}
double cosCompareS2S(void* vector1, void* vector2){
	sparseRIV* comparator = (sparseRIV*)vector2;
	denseRIV baseRIV = {0};
	addS2D(baseRIV.values, (sparseRIV*)vector1);
	baseRIV.magnitude = ((sparseRIV*)vector2)->magnitude;
	long long int dot = 0;
	int* locations_stop = comparator->locations+comparator->count;
	int* locations_slider = comparator->locations;
	int* values_slider = comparator->values;
	while(locations_slider<locations_stop){

		/* we calculate the dot-product to derive the cosine 
		 * comparing sparse to dense by index*/
		dot += (long long int)*values_slider * (long long int)baseRIV.values[*locations_slider];
		locations_slider++;
		values_slider++;

	}
	/*dot divided by product of magnitudes */

	return dot/(baseRIV.magnitude*comparator->magnitude);
}
void addS2I(int* destination, sparseRIV* input){
	
	int *locations_slider = input->locations;
	int *values_slider = input->values;
	int *locations_stop = locations_slider+input->count;
	
	/* apply values at an index based on locations */
	while(locations_slider<locations_stop){
		
		destination[*locations_slider] += *values_slider;
		locations_slider++;
		values_slider++;
	}
	
}

#endif /*RIVMATH_H*/
