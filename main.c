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
enum StatsType{None=0,Average=1,Dispersional=2};
	

	
/* This static array defines the arguments we want to use. One for each structure element. The array should be terminated by the all-zeroed element (remember, that NULL pointer is a pointer filled with zeros). */
static struct option _LongOptionList[] = {
	{ "arg-dispersional",	0, NULL, 'd' },     /*This is an example of just a flag argument, which has no parameter (second column is 0). Long variant of the argument would be "--arg-dispersional", short variant would be "-d".*/
	{ "arg-average", 		0, NULL, 'a' },		/* This is an example of just a flag argument, which has no parameter (second column is 0). Long variant of the argument would be "--arg-average", short variant would be "-a". */
	{ "arg-n_val",  		1, NULL, 'n' },		/* This is an example of argument with a value (second column is 1). Long variant of the argument would be "--arg-n_val=VAL", short variant would be "-n VAL". */
	{ NULL,       			0, NULL, 0   },		/* This is a zeroed element, which terminates the list of argument specifications. */
};

/*This is a struct of array of __uint32_t's with capacity of capacity, and with amount of used cells of array in size, something like std::vector<__uint32_t> in c++ */
struct _Buffer
{
	__uint32_t* arr;
	__u_long size;
	__u_long capcity;
};

/*This is a struct of array of char* with capacity of capacity, and with amount of used cells of array in size, something like std::string or std::vector<char> in c++ */
struct _Output
{
	char* string;
	__u_long size;
	__u_long capcity;
};

/* Those variables are global not to parse a lot of variables used as "readonly(once set a valid value won't ever get changed)", may be considered as settings*/

static bool gotStatsType=false;
static enum StatsType givenStatsType=None;
static int n=-1;
static float preAmpGain=0.f;
static float ampGain=0.f;

/* Constants, DEFAULT_CAPACITY can be set to higher values to eliminate extra _Buffer and _Output realocations, so that will make program faster but will increase minimal memory usage.*/

const int BUFFER_SIZE = 1024;
const __u_long DEFAULT_CAPACITY=1;
const int PRECISSION=6;

static inline __uint16_t getLengthBeforeDot(double in)
/* 	may be unsafe because the double type can represent values higher than __int64_t, but it will never happen because the theoretical max input is (2*2^16)^2 == 2^34
	where 2^16 is maximal integer number can be repesented by 17 bit signed int, 2* and ^2 stands for situation where used dispersion and average is -2^16 and x_i is 2^16
	2^34 can be represented by __int64_t, so in current circumstances, it's safe and fast.*/
{
	__uint16_t counter=0;
	__int64_t num=(__int64_t)(in);
	while (num)
	{
		counter++;
		num=num/10;
	}
	return counter;
/*-----------------------------------------------------------------------------------------------------------------------
second varaition, may be slower because it involves dividing decimal numbers, but 100% safe.
	__uint16_t counter=0;
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

static inline void handleParameters(int argc, char *argv[]){
	bool error=false;
	const char* errorCode=NULL;
	int _Ret;
	/* This is the main argument processing function-call. In the third argument string, there each of the arguments specified
	in the _LongOptionList array should have its short form letter. If the second columnt of the array is specified as 1 (having an argument),
	then the particular letter in this string should be followed by a ':' character. Also, if we want to process positional parameters
	(those not beginning with "-" or "--") we should put '-' as the the first letter of the string. */
	while ((_Ret = getopt_long (argc, argv, "-adn:", _LongOptionList, NULL)) != -1&&!error)
	{
			switch (_Ret) {
            case 'a':
                if(gotStatsType){
					error=true;
					errorCode="Only one flag for the type of stats can be used";
				}
				else{
					gotStatsType =true;
					givenStatsType=Average;
				}
                break;
            case 'd':
                if(gotStatsType){
					error=true;
					errorCode="Only one flag for the type of stats can be used";
				}
				else{
					gotStatsType =true;
					givenStatsType=Dispersional;
				}
                break;
            case 'n':
				if(optarg)
				{
                n = atoi(optarg); // if the first char in string is not an integer n will be 0, so the exception will be handeled later
				}
				else
				{
					error=true;
					errorCode="Invalid -n value";
				}
                break;
            default:
				error=true;
        }
    }
	if(!gotStatsType||n<=0&&!error)
	{
		error=true;
		errorCode="Not all necessary flags and values were given";
	}
	if(error){
		if(errorCode){fprintf(stderr,"%s\n",errorCode);} // if the error type has been determined it, the error code gets parsed to the standard error output
		fprintf(stderr,"Usage: %s [-a] [-d] [-n <number>]\n",argv[0]);	// the guide gets printed to stderr
		exit(EXIT_FAILURE);
	}
}

/*Basicly constructor of _Buffer, returns pointer to empty _Buffer with *capacity* of allocated cells in array,if something went wrong, returns NULL.
 capacity has to be > 0*/
static inline struct _Buffer* createBuffer(__u_long capacity)
{
	struct _Buffer* buf=malloc(sizeof(struct _Buffer));
	if(buf&&capacity)
	{
	buf->capcity=capacity;
	buf->size=0;
	buf->arr=malloc(capacity*sizeof(__uint32_t));
		if(buf->arr)
		{
			return buf; /* If the array with given capacity was allocated we return pointer to prepaired _Buffer*/
		}
	}
	return NULL;
}

/*Function used to add val to buf*/
bool addElementToBuffer(struct _Buffer* buf,__uint32_t val)
{

	if(buf->size>=buf->capcity) 
	{
		__uint32_t* safetyBuf=realloc(buf->arr,(buf->capcity<<1)*sizeof(__uint32_t));  // if the amount of used cells == capacity array gets safely realocated, if it can't be realoacated we won't lose any data. 
		if(safetyBuf)
		{
			buf->arr=safetyBuf;
			buf->capcity=buf->capcity<<1;
			
		}
		else
		{
			return false;
		}

	}
	buf->arr[buf->size]=val;
	buf->size++;
	return true;
}

/*This function used to add *bytesRead* bytes of array of vals to to the end of buf the fastest way*/
bool mergeBuffers(struct _Buffer* buf,__uint32_t* vals,__uint32_t bytesRead)
{
	if(buf->capcity*sizeof(__uint32_t)<buf->size*sizeof(__uint32_t)+bytesRead) // If current amount of unused allocated bytes in array less than amount of bytes to be added, it gets doubled
	{
		__u_long newByteCapacity=(buf->capcity<<1)*sizeof(__uint32_t); 
		if(newByteCapacity<buf->size*sizeof(__uint32_t)+bytesRead) // If itsn't enough, capacity gets froced to just fit all new bytes
		{
			newByteCapacity=buf->size*sizeof(__uint32_t)+bytesRead;
		}
		__uint32_t* safetyBuf=realloc(buf->arr,newByteCapacity); // Safe reallocation array of buf to fit bytesRead of new data, and if it succeeded we set capacity to newByteCapacity/sizeof(__uint32_t)
		if(safetyBuf)
		{
			buf->arr=safetyBuf;
			buf->capcity=newByteCapacity/sizeof(__uint32_t);
		}
		else
		{
			return false;
		}
	}
	memcpy(buf->arr+buf->size,vals,bytesRead); // bytesRead from vals get added to end of buf array 
	buf->size+=bytesRead/sizeof(__uint32_t); // adding right amount of added elements to size
	return true;

}


/*Basicly constructor of _Output, returns pointer to empty _Output with *capacity* of allocated cells in array,if something went wrong, returns NULL.
 capacity has to be > 0*/
static inline struct _Output* createOutput(__u_long capacity)
{
	struct _Output* out=malloc(sizeof(struct _Output));
	if(out&&capacity)
	{
	out->capcity=capacity;
	out->size=0;
	out->string=malloc(capacity*sizeof(char));
		if(out->string)
		{
			return out;
		}
	}
	return NULL;
	
} 
 
 /*Function used to add char to out*/
bool addCharToOutput(struct _Output* out,char ch)
{
	if(out->size+1>=out->capcity)
	{
		char* safetyString=realloc(out->string,(out->capcity<<1)*sizeof(char)); // if the amount of used cells > capacity -1 (for \0, end of string) array gets safely realocated, if it can't be realoacated we won't lose any data. 
		if(!safetyString)
		{
			return false;
		}
		else
		{
			out->string=safetyString;
			out->capcity=out->capcity<<1;
		}
	}
	out->string[out->size++]=ch; // previous \0 gets overwrited by new char and \0 sets on the position after new char 
	out->string[out->size]='\0';
	return true;
}

/*This function used to add *stringSize* chars of array of chars to to the end of out the fastest way*/
bool addStringToOutput(struct _Output* out, char* arr, unsigned stringSize)
{

	if(out->capcity<out->size+stringSize) // If current amount of unused allocated cells in array less than amount of bytes to be added, it gets doubled
		{
			__u_long newByteCapacity=(out->capcity<<1)*sizeof(char);
			if(newByteCapacity<out->size*sizeof(char)+stringSize*sizeof(char)) // If itsn't enough, capacity gets froced to just fit all new bytes
			{
				newByteCapacity=out->size*sizeof(char)+stringSize*sizeof(char);
			}
			char* safetyBuf=realloc(out->string,newByteCapacity); // Safe reallocation array of buf to fit *stringSize* of new chars, and if it succeeded we set capacity to newByteCapacity/sizeof(char)
			if(safetyBuf)
			{
				out->string=safetyBuf;
				out->capcity=newByteCapacity/sizeof(char); 
			}
			else
			{
				return false;
			}
		}
	memcpy(out->string+out->size,arr,stringSize*sizeof(char)); // stringSize from arr get added to end of string 
	out->size+=stringSize; // size increases by amount of chars added
	if(out->string[out->size-1]=='.')
	{
		addCharToOutput(out,'0'); // to ensure that the last char is not a dot to make output look better, probably will never be called.
	}
	out->string[out->size]='\0'; // setting end of a string.
	return true;	
}

/*Adds decimal number to new line in out*/
bool addRecordOfDoubleToOutput(struct _Output* out,double val)
{
	unsigned bufsize=PRECISSION+getLengthBeforeDot(val)+2; // probable amount of numbers after decimal point + amount of numbers before decimal point + 1 (for '.') + 1 (for '\0') as amount of char in string to be add to output
	char* buf=malloc(sizeof(char)*bufsize*2); // allocate string with bufsize*2 cells, *2 for extremal state where precission is too low so the number has to be represented in scientific way and "e+" or "e-" has to be added, wastes couple of bytes but ensures safety
	gcvt(val,bufsize-2,buf); // conversion from double to const char*
	if(buf)
	{
		if(!addStringToOutput(out,buf,strlen(buf))) // trying to add string to output
		{
			free(buf);
			return false;
		}
		free(buf);
		addCharToOutput(out,'\n'); // if succeeded, add end of a line to output
		return true;
	}
	else
	{
		return false;
	}

}

/*Function to calculate average value of representation lower 17 bits as signed int in range of range starting from startPos*/
static inline double calculateAverage(__uint32_t* arr,__u_long startPos,__u_long range)
{
	__int32_t sum=0;
	for(__u_long i=0;i<range;++i)
	{
		sum+=((__int32_t)(arr[i+startPos]<<15))>>15; // for each 32 bits from arr in range from startPos lower 17 bits gets represented as signed integer and get added to sum
	}
	return (double)(sum)/range; // returning average value as double, its faster to convert sum to double here, instead of having it as double from start because amount of operations with decimal will drop to 1
}
/*TODO*/
static inline double calculateDispersion(__uint32_t* arr,__u_long startPos,__u_long range,double average)
{
	double sum=0;
	__int32_t xi;
	for(int i=0;i<range;++i)
	{
		xi=((__int32_t)(arr[i+startPos]<<15))>>15;
		sum+=pow(xi-average,2);
	}
	return sum/range;
}	
void processBuffer(struct _Buffer* buf,struct _Output* out,bool isFinished)
{
 	int rest=buf->size%n;
	__u_long i,j;
	__u_long sum;
	double recordValue=0;
	for(i=0;i<buf->size/n;++i)	
	{
		
		if(givenStatsType==Average)
		{
		 	recordValue=calculateAverage(buf->arr,i*n,n);
		}
		else if(givenStatsType==Dispersional)
		{
			recordValue=calculateDispersion(buf->arr,i*n,n,calculateAverage(buf->arr,i*n,n));
		}
		else
		{
			fprintf(stderr,"Bad memory integrity, tampering with an important variable was found or value has changed due to unknown reasons");
			exit(EXIT_FAILURE);
		}
		if(!addRecordOfDoubleToOutput(out,recordValue))
		{
			fprintf(stdout,"%s",out->string);
			out->size=0;
			out->string[out->size]='\0';
			if(!addRecordOfDoubleToOutput(out,recordValue))
			{
				fprintf(stdout,"%.*f\n",PRECISSION,recordValue);
			}
		}

	}
	for(int i=0;i<rest;++i)
	{
		buf->arr[i]=buf->arr[buf->size-rest+i];
	}
	buf->size=rest;
	
	if(isFinished)
	{   
		if(givenStatsType==Average)
		{
		 	recordValue=calculateAverage(buf->arr,buf->size-rest,rest);
		}
		else if(givenStatsType==Dispersional)
		{
			recordValue=calculateDispersion(buf->arr,buf->size-rest,rest,calculateAverage(buf->arr,buf->size-rest,rest));
		}
		else
		{
			fprintf(stderr,"Bad memory integrity, tampering with an important variable was found or value has changed due to unknown reasons");
			exit(EXIT_FAILURE);
		}
		if(!addRecordOfDoubleToOutput(out,recordValue))
		{
			fprintf(stdout,"%s",out->string);
			out->size=0;
			out->string[out->size]='\0';
			if(!addRecordOfDoubleToOutput(out,recordValue))
			{
				fprintf(stdout,"%.*f\n",PRECISSION,recordValue);
			}
		}
		if(out->size)
		{
			fprintf(stdout,"%s",out->string);
		}
		free(buf->arr);
		free(buf);
		free(out->string);
		free(out);
	}
}

void processInput(struct _Buffer* buf,struct _Output* out)
{
    __uint32_t* inputBuffer;
	inputBuffer=malloc(sizeof(__uint32_t)*BUFFER_SIZE);
	if(!inputBuffer)
	{
		fprintf(stderr,"Bad alloc, immposible to store necesarry resources");
		exit(EXIT_FAILURE);
	}
    __uint32_t elementsRead;
    while ((elementsRead = fread(inputBuffer, sizeof(__uint32_t), BUFFER_SIZE, stdin)) > 0) 
	{
		if(!mergeBuffers(buf,inputBuffer,elementsRead*sizeof(__uint32_t)))
		{

			for(int i=0;i<elementsRead;++i)
			{
				if(!addElementToBuffer(buf,inputBuffer[i]))
				{
					processBuffer(buf,out,false);
					if(!addElementToBuffer(buf,inputBuffer[i]))
					{
						fprintf(stderr,"Bad alloc, immposible to store necesarry resources");
						exit(EXIT_FAILURE);
					}
				}
			}
		}

    }
	free(inputBuffer);
	processBuffer(buf,out,true);
}

int main(int argc, char *argv[]) {

	handleParameters(argc,argv);
	struct _Buffer* buffer= createBuffer(DEFAULT_CAPACITY);
	struct _Output* out = createOutput(DEFAULT_CAPACITY);
	if(!buffer||!out)
	{
		fprintf(stderr,"Bad alloc, immposible to store necesarry resources");
		exit(EXIT_FAILURE);
	}
	processInput(buffer,out);

    return 0; // Return 0 to indicate successful execution
}