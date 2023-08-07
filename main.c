
/* Constants, DEFAULT_CAPACITY can be set to higher values to eliminate extra _Buffer and _Output realocations, so that will make program faster but will increase minimal memory usage.*/
#define BUFFER_SIZE 1024
#define DEFAULT_CAPACITY 1
#define PRECISSION 6
/*This include is needed in order to process the i/o*/
#include <stdio.h>
/* This is here now mainly to declare the NULL pointer */
#include <stdlib.h>
/* This include is needed in order to process the arguments, it declares the getopt_long() function and "struct option" structure. */
#include <getopt.h>
/*This is here now mainly to declare bool type*/
#include <stdbool.h>
/*This include is needed in order to declare the mathematical functions*/
#include <math.h>
/*This include is needed to declare len() function*/
#include <string.h>
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

/*This is a struct of array of uint32_t's with capacity of capacity, and with amount of used cells of array in size, something like std::vector<uint32_t> in c++ */
struct _Buffer
{
	uint32_t* arr;
	size_t size;
	size_t capcity;
};

/*This is a struct of array of char* with capacity of capacity, and with amount of used cells of array in size, something like std::string or std::vector<char> in c++ */
struct _Output
{
	char* string;
	size_t size;
	size_t capcity;
};

/* Those variables are global not to parse a lot of variables used as "readonly(once set a valid value won't ever get changed)", may be considered as settings*/

static bool gotStatsType = false;
static enum StatsType givenStatsType = None;
static int n = -1;
// static float preAmpGain = 0.f;
// static float ampGain = 0.f;


static inline uint16_t getLengthBeforeDot(double in)
/* 	may be unsafe because the double type can represent values higher than long, but it will never happen because the theoretical max input is (2*2^16)^2 == 2^34
	where 2^16 is maximal integer number can be repesented by 17 bit signed int, 2* and ^2 stands for situation where used dispersion and average is -2^16 and x_i is 2^16
	2^34 can be represented by long, so in current circumstances, it's safe and fast.*/
{
	uint16_t counter = 0;
	int64_t num = (int64_t)(in);
	while (num)
	{
		counter++;
		num = num / 10;
	}
	return counter;
	/*-----------------------------------------------------------------------------------------------------------------------
	second varaition, may be slower because it involves dividing decimal numbers, but 100% safe.
		short counter=0;
		while (in>=1||in<=-1)
		{
			counter++;
			in=in/10;
		}
		return counter;
	--------------------------------------------------------------------------------------------------------------------------
	third variation, 100%, safe but slow.
		 return floor(log10(abs(x))) + 1
	--------------------------------------------------------------------------------------------------------------------------
	anyway its impossible to notice the execution time difference between these variations
	*/
}

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
				n = atoi(optarg); // if the first char in string is not an integer n will be 0, so the exception will be handeled later
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
	if (!gotStatsType || n <= 0 && !error)
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

/*Basicly constructor of _Buffer, returns pointer to empty _Buffer with *capacity* of allocated cells in array,if something went wrong, returns NULL.
 capacity has to be > 0*/
static inline struct _Buffer* createBuffer(size_t capacity)
{
	struct _Buffer* buf = (struct _Buffer*)malloc(sizeof(struct _Buffer));
	if (buf && capacity)
	{
		buf->capcity = capacity;
		buf->size = 0;
		buf->arr = (uint32_t*)malloc(capacity * sizeof(uint32_t));
		if (buf->arr)
		{
			return buf; /* If the array with given capacity was allocated we return pointer to prepaired _Buffer*/
		}
	}
	return NULL;
}

/*Function used to add val to buf*/
bool addElementToBuffer(struct _Buffer* buf, uint32_t val)
{

	if (buf->size >= buf->capcity)
	{
		uint32_t* safetyBuf = (uint32_t*)realloc(buf->arr, (buf->capcity << 1) * sizeof(uint32_t));  // if the amount of used cells == capacity array gets safely realocated, if it can't be realoacated we won't lose any data.
		if (safetyBuf)
		{
			buf->arr = safetyBuf;
			buf->capcity = buf->capcity << 1;

		}
		else
		{
			return false;
		}

	}
	buf->arr[buf->size] = val;
	buf->size++;
	return true;
}

/*This function used to add *bytesRead* bytes of array of vals to to the end of buf the fastest way*/
bool mergeBuffers(struct _Buffer* buf, uint32_t* vals, size_t bytesRead)
{
	if (buf->capcity * sizeof(uint32_t) < buf->size * sizeof(uint32_t) + bytesRead) // If current amount of unused allocated bytes in array less than amount of bytes to be added, it gets doubled
	{
		size_t newByteCapacity = (buf->capcity << 1) * sizeof(uint32_t);
		if (newByteCapacity < buf->size * sizeof(uint32_t) + bytesRead) // If itsn't enough, capacity gets froced to just fit all new bytes
		{
			newByteCapacity = buf->size * sizeof(uint32_t) + bytesRead;
		}
		uint32_t* safetyBuf = (uint32_t*)realloc(buf->arr, newByteCapacity); // Safe reallocation array of buf to fit bytesRead of new data, and if it succeeded we set capacity to newByteCapacity/sizeof(uint32_t)
		if (safetyBuf)
		{
			buf->arr = safetyBuf;
			buf->capcity = newByteCapacity / sizeof(uint32_t);
		}
		else
		{
			return false;
		}
	}
	memcpy(buf->arr + buf->size, vals, bytesRead); // bytesRead from vals get added to end of buf array
	buf->size += bytesRead / sizeof(uint32_t); // adding right amount of added elements to size
	return true;

}


/*Basicly constructor of _Output, returns pointer to empty _Output with *capacity* of allocated cells in array,if something went wrong, returns NULL.
 capacity has to be > 0*/
static inline struct _Output* createOutput(size_t capacity)
{
	struct _Output* out = (struct _Output*)malloc(sizeof(struct _Output));
	if (out && capacity)
	{
		out->capcity = capacity;
		out->size = 0;
		out->string = (char*)malloc(capacity * sizeof(char));
		if (out->string)
		{
			return out;
		}
	}
	return NULL;

}

/*Function used to add char to out*/
bool addCharToOutput(struct _Output* out, char ch)
{
	if (out->size + 1 >= out->capcity)
	{
		char* safetyString = (char*)realloc(out->string, (out->capcity << 1) * sizeof(char)); // if the amount of used cells > capacity -1 (for \0, end of string) array gets safely realocated, if it can't be realoacated we won't lose any data.
		if (!safetyString)
		{
			return false;
		}
		else
		{
			out->string = safetyString;
			out->capcity = out->capcity << 1;
		}
	}
	out->string[out->size++] = ch; // previous \0 gets overwrited by new char and \0 sets on the position after new char
	out->string[out->size] = '\0';
	return true;
}

/*This function used to add *stringSize* chars of array of chars to to the end of out the fastest way*/
bool addStringToOutput(struct _Output* out, char* arr, size_t stringSize)
{

	if (out->capcity < out->size + stringSize) // If current amount of unused allocated cells in array less than amount of bytes to be added, it gets doubled
	{
		size_t newByteCapacity = (out->capcity << 1) * sizeof(char);
		if (newByteCapacity < out->size * sizeof(char) + stringSize * sizeof(char)) // If itsn't enough, capacity gets froced to just fit all new bytes
		{
			newByteCapacity = out->size * sizeof(char) + stringSize * sizeof(char);
		}
		char* safetyBuf = (char*)realloc(out->string, newByteCapacity); // Safe reallocation array of buf to fit *stringSize* of new chars, and if it succeeded we set capacity to newByteCapacity/sizeof(char)
		if (safetyBuf)
		{
			out->string = safetyBuf;
			out->capcity = newByteCapacity / sizeof(char);
		}
		else
		{
			return false;
		}
	}
	memcpy(out->string + out->size, arr, stringSize * sizeof(char)); // stringSize from arr get added to end of string
	out->size += stringSize; // size increases by amount of chars added
	if (out->string[out->size - 1] == '.')
	{
		addCharToOutput(out, '0'); // to ensure that the last char is not a dot to make output look better, probably will never be called.
	}
	out->string[out->size] = '\0'; // setting end of a string.
	return true;
}

/*Adds decimal number to new line in out*/
bool addRecordOfDoubleToOutput(struct _Output* out, double val)
{
	uint32_t bufsize = PRECISSION + getLengthBeforeDot(val) + 2; // probable amount of numbers after decimal point + amount of numbers before decimal point + 1 (for '.') + 1 (for '\0') as amount of char in string to be add to output
	char* buf = (char*)malloc(sizeof(char) * bufsize * 2); // allocate string with bufsize*2 cells, *2 for extremal state where precission is too low so the number has to be represented in scientific way and "e+" or "e-" has to be added, wastes couple of bytes but ensures safety
	if (buf)
	{
		gcvt(val, bufsize - 2, buf);// conversion from double to const char*
		if (!addStringToOutput(out, buf, strlen(buf))) // trying to add string to output
		{
			free(buf);
			return false;
		}
		free(buf);
		addCharToOutput(out, '\n'); // if succeeded, add end of a line to output
		return true;
	}
	else
	{
		return false;
	}

}

/*Function to calculate average value of representation lower 17 bits as signed int in range of range starting from startPos*/
static inline double calculateAverage(uint32_t* arr, size_t startPos, size_t range)
{
	int64_t sum = 0;
	for (size_t i = 0; i < range; ++i)
	{
		sum += ((int32_t)(arr[i + startPos] << 15)) >> 15; // for each 32 bits from arr in range from startPos lower 17 bit gets represented as signed integer and get added to sum
	}
	return (double)(sum) / range; // returning average value as double, its faster to convert sum to double here, instead of having it as double from start because amount of operations with decimal will drop to 1
}
/*Function to calculate dispersion between value of representation lower 17 bits as signed int in range of range starting from startPos and average value*/
static inline double calculateDispersion(uint32_t* arr, size_t startPos, size_t range, double average)
{
	double sum = 0;
	int xi;
	for (size_t i = 0; i < range; ++i)
	{
		xi = ((int32_t)(arr[i + startPos] << 15)) >> 15; // for each 32 bits from arr in range from startPos lower 17 bits get represented as signed integer and squared diffence between it and average gets added to sum
		sum += (xi - average) * (xi - average); // Its faster to do it that way istead of pow(xi-average,2) according to this topic https://stackoverflow.com/questions/2940367/what-is-more-efficient-using-pow-to-square-or-just-multiply-it-with-itself
	}
	return sum / range;
}

/*Function used to process data from buf and add results to out*/
void processBuffer(struct _Buffer* buf, struct _Output* out, bool isFinished)
{
	uint32_t rest = buf->size % n; // Amount of cells that cant be processed if the procession isn't final
	double recordValue = 0;
	for (size_t i = 0; i < buf->size / n; ++i)	//for each cluster of n elements we calculate either average or dispersional statistics according to chosen type
	{

		if (givenStatsType == Average)
		{
			recordValue = calculateAverage(buf->arr, i * n, n);
		}
		else if (givenStatsType == Dispersional)
		{
			recordValue = calculateDispersion(buf->arr, i * n, n, calculateAverage(buf->arr, i * n, n));
		}
		else
		{
			fprintf(stderr, "Bad memory integrity, tampering with an important variable was found or value has changed due to unknown reasons"); // If arguments were processed, givenStatsType has to be average or dispersional. And if givenStatsType had changed, static data segment isn't intact
			exit(EXIT_FAILURE);
		}
		if (!addRecordOfDoubleToOutput(out, recordValue)) // if it's impossible to store new value in output buffer, all buffer gets sent to stdout and size sets to 0, so new values will overwrite old.
		{
			fprintf(stdout, "%s", out->string);
			out->size = 0;
			out->string[out->size] = '\0';
			if (!addRecordOfDoubleToOutput(out, recordValue)) // if it's impossible to add new value to empty output buffer, it gets directly sent to stdout, we don't need to throw error, because program can still work, but very much slower.
			{
				fprintf(stdout, "%.*f\n", PRECISSION, recordValue); // if it happens, out is empty.
			}
		}

	}
	for (size_t i = 0; i < rest; ++i)
	{
		buf->arr[i] = buf->arr[buf->size - rest + i]; // we place unprocessed values to start so new values can be added to not processed
	}
	buf->size = rest;

	if (isFinished) //If unprocessed values exist, process them. in the end, output buffer gets sent, and memory gets freed.
	{
		if (rest) {
			if (givenStatsType == Average)
			{
				recordValue = calculateAverage(buf->arr, buf->size - rest, rest);
			}
			else if (givenStatsType == Dispersional)
			{
				recordValue = calculateDispersion(buf->arr, buf->size - rest, rest, calculateAverage(buf->arr, buf->size - rest, rest));
			}
			else
			{
				fprintf(stderr, "Bad memory integrity, tampering with an important variable was found or value has changed due to unknown reasons");
				exit(EXIT_FAILURE);
			}
			if (!addRecordOfDoubleToOutput(out, recordValue))
			{
				fprintf(stdout, "%s", out->string);
				out->size = 0;
				out->string[out->size] = '\0';
				if (!addRecordOfDoubleToOutput(out, recordValue))
				{
					fprintf(stdout, "%.*f\n", PRECISSION, recordValue);
				}
			}
		}
		if (out->size)
		{
			fprintf(stdout, "%s", out->string); // whole output buffer get sended to stdout
		}
		//memory gets freed
		free(buf->arr);
		free(buf);
		free(out->string);
		free(out);
	}
}

/*Function that reads stdin, processes it, and outputs results to stdout using buffers to make program faster*/
void processInput(struct _Buffer* buf, struct _Output* out)
{
	uint32_t* inputBuffer;
	inputBuffer = (uint32_t*)malloc(sizeof(uint32_t) * BUFFER_SIZE); // we create buffer to store there input.
	if (!inputBuffer)
	{
		fprintf(stderr, "Bad alloc, immposible to store necesarry resources"); //if it's impossible to create buffer, throw error.
		exit(EXIT_FAILURE);
	}
	size_t elementsRead;
	while ((elementsRead = fread(inputBuffer, sizeof(uint32_t), BUFFER_SIZE, stdin)) > 0) // until it's possible to read 4byte integers from standart input, they get added to _Buffer.
	{
		if (!mergeBuffers(buf, inputBuffer, elementsRead * sizeof(uint32_t))) // if it's impossible to add whole input buffer to buf, it gets added by element
		{

			for (uint32_t i = 0; i < elementsRead; ++i)
			{
				if (!addElementToBuffer(buf, inputBuffer[i])) // if it's impossible to add an element, buffer gets processed and some of part of if gets freed
				{
					processBuffer(buf, out, false);
					if (!addElementToBuffer(buf, inputBuffer[i]))//if it's impossible to add an element to processed buffer, doesn't make sense to proceed
					{
						fprintf(stderr, "Bad alloc, imposible to store necesarry resources");
						exit(EXIT_FAILURE);
					}
				}
			}
		}

	}
	free(inputBuffer); // freeing the inputBuffer.
	processBuffer(buf, out, true); // and processing the buffer.
}

int main(int argc, char* argv[]) {

	handleParameters(argc, argv); //handling parameters
	struct _Buffer* buffer = createBuffer(DEFAULT_CAPACITY); //cunstructing buffers
	struct _Output* out = createOutput(DEFAULT_CAPACITY);
	if (!buffer || !out) // if buffers weren't constructed, throw error
	{
		fprintf(stderr, "Bad alloc, immposible to store necesarry resources");
		exit(EXIT_FAILURE);
	}
	//processing input
	processInput(buffer, out);

	return 0; // Return 0 to indicate successful execution
}
