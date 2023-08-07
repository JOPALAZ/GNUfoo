
/*Constant known amount of useless upper bits in input values*/
#define NOISESHIFT 15
/*This include is needed in order to process the i/o*/
#include <stdio.h>
/* This is here now mainly to declare the NULL pointer */
#include <stdlib.h>
/* This include is needed in order to process the arguments, it declares the getopt_long() function and "struct option" structure. */
#include <getopt.h>
/*This is here now mainly to declare bool type*/
#include <stdbool.h>
/*This include is needed to define 32 bit integer*/
#include <stdint.h>
enum StatsType { None = 0, Average = 1, Dispersional = 2 };



/* This static array defines the arguments we want to use. One for each structure element. The array should be terminated by the all-zeroed element (remember, that NULL pointer is a pointer filled with zeros). */
static struct option _LongOptionList[] = {
	{ "arg-dispersional",	0, NULL, 'd' },     /*This is an example of just a flag argument, which has no parameter (second column is 0). Long variant of the argument would be "--arg-dispersional", short variant would be "-d".*/
	{ "arg-average", 		0, NULL, 'a' },		/* This is an example of just a flag argument, which has no parameter (second column is 0). Long variant of the argument would be "--arg-average", short variant would be "-a". */
	{ "arg-n_val",  		1, NULL, 'n' },		/* This is an example of argument with a value (second column is 1). Long variant of the argument would be "--arg-n_val=VAL", short variant would be "-n VAL". */
	{ NULL,       			0, NULL, 0   },		/* This is a zeroed element, which terminates the list of argument specifications. */
};

/* Those variables are global not to parse a lot of variables used as "readonly(once set a valid value won't ever get changed)", may be considered as settings*/

static bool gotStatsType = false;
static enum StatsType givenStatsType = None;
static uint64_t N = 0;


static inline void handleParameters(int argc, char* argv[]) {
	bool error = false;
	const char* errorCode = NULL;
	int _Ret;
	/* This is the main argument processing function-call. In the third argument string, there each of the arguments specified
	in the _LongOptionList array should have its short form letter. If the second columnt of the array is specified as 1 (having an argument),
	then the particular letter in this string should be followed by a ':' character. Also, if we want to process positional parameters
	(those not beginning with "-" or "--") we should put '-' as the the first letter of the string. */
	while ((_Ret = getopt_long(argc, argv, "-adn:", _LongOptionList, NULL)) != -1 && !error)
	{
		switch (_Ret) {
		case 'a':
			if (gotStatsType) {
				error = true;
				errorCode = "Only one flag for the type of stats can be used";
			}
			else {
				gotStatsType = true;
				givenStatsType = Average;
			}
			break;
		case 'd':
			if (gotStatsType) {
				error = true;
				errorCode = "Only one flag for the type of stats can be used";
			}
			else {
				gotStatsType = true;
				givenStatsType = Dispersional;
			}
			break;
		case 'n':
			if (optarg)
			{
				 N = atoi(optarg); // if the first char in string is not an integer N will be 0, so the exception will be handeled later
			}
			else
			{
				error = true;
				errorCode = "Invalid -n value";
			}
			break;
		default:
			error = true;
		}
	}
	if (!gotStatsType || N <= 0 && !error)
	{
		error = true;
		errorCode = "Not all necessary flags and values were given";
	}
	if (error) {
		if (errorCode) { fprintf(stderr, "%s\n", errorCode); } // if the error type has been determined it, the error code gets parsed to the standard error output
		fprintf(stderr, "Usage: %s [-a] [-d] [-n <number>]\n", argv[0]);	// the guide gets printed to stderr
		exit(EXIT_FAILURE);
	}
}
static inline int32_t cleanNoise(uint32_t val)
{
	return ((int32_t)(val<<NOISESHIFT))>>NOISESHIFT;
}
static inline void processInput()
{
	uint32_t inputValue;
	size_t i;
	int32_t currentValue;
	double average;
	int64_t sum=0;
	uint64_t counter=0;
	uint64_t sum_squared=0;

	while ((fread(&inputValue, sizeof(uint32_t), 1, stdin)) > 0) // until it's possible to read 4byte integers from standart input, they get added to _Buffer.
	{
			
			currentValue=cleanNoise(inputValue); // useless noise upper NOISESHIFT bites get trimmed 
			sum+=currentValue;
			if(gotStatsType==Dispersional)
			{
				sum_squared+=currentValue*currentValue; // doesn't have to be calculated, unless statsType is despersinal
			}
			counter++;
			if(counter == N)
			{
				average=(double)(sum)/N; // need it in both variants so, it's better to calculate it here.
				counter=0;
				if(gotStatsType==Average)
				{
					printf("%f\n",average); // outputing average
				}
				else if(givenStatsType==Dispersional)
				{
					printf("%f\n",((double)sum_squared/N-(average*average))); /*
					Calculating 1/N \sum_{i=1}^{N} (x_i-average), but 1/N \sum_{i=1}^{N} (x_i-average)^2 == 1/N \sum_{i=1}^{N} (x_i^2-2*x_i*average+average^2)
					
					\sum_{i=1}^{N} (x_i) = N*average, so 1/N \sum_{i=1}^{N} (x_i^2-2*x_i*average+average^2) ==  1/N(\sum_{i=1}^{N}x_i^2-2N*average^2+average^2) ==
					
					\sum_{i=1}^{N}x_i^2 / N - average^2 
					*/
				}
				else
				{
					fprintf(stderr, "Bad memory integrity, tampering with an important variable was found or value has changed due to unknown reasons"); // If arguments were processed, givenStatsType has to be average or dispersional. And if givenStatsType had changed, static data segment isn't intact
					exit(EXIT_FAILURE);
				}
				sum_squared=0;
				sum=0;
			}
	}
}
int main(int argc, char* argv[]) {
	handleParameters(argc, argv); //handling parameters
	processInput(); //Processing input

	return 0; // Return 0 to indicate successful execution
}
