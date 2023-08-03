#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
enum StatsType{None=0,Average=1,Dispersional=2};
struct Buffer
{
	__uint32_t* arr;
	__u_long size;
	__u_long capcity;
};
struct Output
{
	char* string;
	__u_long size;
	__u_long capcity;
};
static bool gotStatsType=false;
static enum StatsType givenStatsType=None;
static int n=-1;
static float preAmpGain=0.f;
static float ampGain=0.f;
const int BUFFER_SIZE = 1024;
const __u_long DEFAULT_CAPACITY=1;
const int precission=6;
static inline unsigned char getLengthBeforeDot(double in)
{
	unsigned char counter=0;
	__int32_t num=(__int32_t)(in);
	while (num)
	{
		counter++;
		num=num/10;
	}
	return counter;
}
static inline void handleParameters(int argc, char *argv[]){
	    int opt;
		bool error=false;
		const char* errorCode=NULL;
    while ((opt = getopt(argc, argv, "adn:")) != -1&&!error) {
        switch (opt) {
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
                n = atoi(optarg);
				}
				else
				{
					error=true;
					errorCode="Invalid -n value\n";
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
		if(errorCode){fprintf(stderr,"%s\n",errorCode);}
		fprintf(stderr,"Usage: %s [-a] [-d] [-n <number>]\n",argv[0]);
		exit(EXIT_FAILURE);
	}
}
static inline struct Buffer* createBuffer(__u_long capacity)
{
	struct Buffer* buf=malloc(sizeof(struct Buffer));
	if(buf)
	{
	buf->capcity=capacity;
	buf->size=0;
	buf->arr=malloc(capacity*sizeof(__uint32_t));
		if(buf->arr)
		{
			return buf;
		}
	}
	return NULL;
}
bool addElementToBuffer(struct Buffer* buf,__uint32_t val)
{

	if(buf->size==buf->capcity)
	{
		__uint32_t* safetyBuf=realloc(buf->arr,(buf->capcity<<1)*sizeof(__uint32_t)); 
		if(safetyBuf)
		{
			buf->arr=safetyBuf; // doesn't acctually make sense, cuz they would point to same memory adress, but it makes code more readable
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
 bool mergeBuffers(struct Buffer* buf,__uint32_t* vals,__uint32_t bytesRead)
{
	if(buf->capcity*sizeof(__uint32_t)<buf->size*sizeof(__uint32_t)+bytesRead)
	{
		__u_long newByteCapacity=(buf->capcity<<1)*sizeof(__uint32_t);
		if(newByteCapacity<buf->size*sizeof(__uint32_t)+bytesRead)
		{
			newByteCapacity=buf->size*sizeof(__uint32_t)+bytesRead;
		}
		__uint32_t* safetyBuf=realloc(buf->arr,newByteCapacity); 
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
	memcpy(buf->arr+buf->size,vals,bytesRead);
	buf->size+=bytesRead/sizeof(__uint32_t);
	return true;

}
static inline struct Output* createOutput(__u_long capacity)
{
	struct Output* out=malloc(sizeof(struct Output));
	if(out)
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
 bool addCharToOutput(struct Output* out,char ch)
{
	if(out->size+1>=out->capcity)
	{
		char* safetyString=realloc(out->string,(out->capcity<<1)*sizeof(char));
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
	out->string[out->size++]=ch;
	out->string[out->size]='\0';
	return true;
}
bool addStringToOutput(struct Output* out, char* arr, unsigned stringSize)
{

	if(out->capcity<out->size+stringSize)
		{
			__u_long newByteCapacity=(out->capcity<<1)*sizeof(char);
			if(newByteCapacity<out->size*sizeof(char)+stringSize*sizeof(char))
			{
				newByteCapacity=out->size*sizeof(char)+stringSize*sizeof(char);
			}
			char* safetyBuf=realloc(out->string,newByteCapacity); 
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
	memcpy(out->string+out->size,arr,stringSize*sizeof(char));
	out->size+=stringSize;
	if(out->string[out->size-1]=='.')
	{
		addCharToOutput(out,'0');
	}
	out->string[out->size]='\0'; 
	return true;

	
}

 bool addRecordOfDoubleToOutput(struct Output* out,double val)
{
	unsigned bufsize=precission+getLengthBeforeDot(val)+2;
	char* buf=malloc(sizeof(char)*bufsize*2);
	gcvt(val,bufsize-2,buf);
	if(buf)
	{
		if(!addStringToOutput(out,buf,strlen(buf)))
		{
			return false;
		}
		addCharToOutput(out,'\n');
		return true;
	}
	else
	{
		return false;
	}

}
static inline double calculateAverage(__uint32_t* arr,__u_long startPos,__u_long range)
{
	__int32_t sum=0;
	for(__u_long i=0;i<range;++i)
	{
		sum+=((__int32_t)(arr[i+startPos]<<15))>>15;
	}
	return (double)(sum)/range;
}

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
void processBuffer(struct Buffer* buf,struct Output* out,bool isFinished)
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
				fprintf(stdout,"%.*f\n",precission,recordValue);
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
				fprintf(stdout,"%.*f\n",precission,recordValue);
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

void processInput(struct Buffer* buf,struct Output* out)
{
    __uint32_t* inputBuffer;
	 FILE* inputFile = fopen("data.dat", "r");
	inputBuffer=malloc(sizeof(__uint32_t)*BUFFER_SIZE);
	if(!inputBuffer)
	{
		fprintf(stderr,"Bad alloc, immposible to store necesarry resources");
		exit(EXIT_FAILURE);
	}
    __uint32_t elementsRead;
    while ((elementsRead = fread(inputBuffer, sizeof(__uint32_t), BUFFER_SIZE, inputFile)) > 0) 
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
	processBuffer(buf,out,true);
}

int main(int argc, char *argv[]) {

	handleParameters(argc,argv);
	struct Buffer* buffer= createBuffer(DEFAULT_CAPACITY);
	struct Output* out = createOutput(DEFAULT_CAPACITY);
	if(!buffer||!out)
	{
		fprintf(stderr,"Bad alloc, immposible to store necesarry resources");
		exit(EXIT_FAILURE);
	}
	processInput(buffer,out);

    return 0; // Return 0 to indicate successful execution
}