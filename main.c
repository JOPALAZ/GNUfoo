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
const int BUFFER_SIZE = 1024;
const __u_long DEFAULT_CAPACITY=1;
const int precission=6;
unsigned char getLengthBeforeDot(float in)
{
	unsigned char counter=0;
	__uint32_t num=(__uint32_t)(in);
	while (num)
	{
		counter++;
		num=num/10;
	}
	return counter;
}
void handleParameters(int argc, char *argv[]){
	    int opt;
		bool error=false;
		char* errorCode=NULL;
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
struct Buffer* createBuffer(__u_long capacity)
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
		__u_long newByteCapacity=buf->capcity<<1*sizeof(__uint32_t);
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
struct Output* createOutput(__u_long capacity)
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
		char* safetyString=realloc(out->string,out->capcity<<1*sizeof(char));
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
	out->string[out->size]=ch;
}

bool addStringToOutput(struct Output* out, char* arr, unsigned stringSize)
{

	if(out->capcity<out->size+stringSize)
		{
			__u_long newByteCapacity=out->capcity<<1*sizeof(char);
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
	out->size+=stringSize-1;
	out->string[out->size]='\0'; 
	return true;

	
}

bool addRecordOfFloatToOutput(struct Output* out,float val)
{
	unsigned bufsize=precission+getLengthBeforeDot(val)+2;
	char* buf=malloc(sizeof(char)*bufsize);
	gcvt(val,bufsize-2,buf);
	if(buf)
	{
		if(!addStringToOutput(out,buf,bufsize))
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
float calculateAverage(__uint32_t* arr,__u_long startPos,__u_long range)
{
	__int32_t sum=0;
	for(__u_long i=0;i<range;++i)
	{
		sum+=((__int32_t)(arr[i+startPos]<<15))>>15;
	}
	return (float)(sum)/range;
}

float calculateDispersion(__uint32_t* arr,__u_long startPos,__u_long range,float average)
{
	float sum=0;
	for(int i=0;i<range;++i)
	{
		sum+=pow((((__int32_t)(arr[i+startPos]<<15))>>15)-average,2);
	}
	return sum/range;
}	
int processBuffer(struct Buffer* buf,struct Output* out,bool isFinished)
{
 	int rest=buf->size%n;
	__u_long i,j;
	__u_long sum;
	float recordValue=0;
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
		if(!addRecordOfFloatToOutput(out,recordValue))
		{
			fprintf(stdout,"%s",out->string);
			out->size=0;
			out->string[out->size]='\0';
			if(!addRecordOfFloatToOutput(out,recordValue))
			{
				fprintf(stdout,"%.*f\n",precission,recordValue);
			}
		}

	}
}

struct Buffer* getInput(struct Buffer* buf,struct Output* out)
{
    __uint32_t* input_buffer;
	input_buffer=malloc(sizeof(__uint32_t)*BUFFER_SIZE);
	if(!input_buffer)
	{
		fprintf(stderr,"Bad alloc, immposible to store necesarry resources");
		exit(EXIT_FAILURE);
	}
    // Читаем данные из стандартного ввода (stdin) в буфер
    __uint32_t bytesRead;
    while ((bytesRead = fread(input_buffer, sizeof(__uint32_t), BUFFER_SIZE, stdin)) > 0) 
	{
		if(bytesRead%sizeof(__uint32_t))
		{
			bytesRead+=sizeof(__uint32_t)-bytesRead%sizeof(__uint32_t);
		}
		if(!mergeBuffers(buf,input_buffer,bytesRead))
		{

			for(int i=0;i<bytesRead/sizeof(__uint32_t);++i)
			{
				if(!addElementToBuffer(buf,input_buffer[i]))
				{
					processBuffer(buf,out,false);
					buf->size=0;
					if(!addElementToBuffer(buf,input_buffer[i]))
					{
						fprintf(stderr,"Bad alloc, immposible to store necesarry resources");
						exit(EXIT_FAILURE);
					}
				}
			}
		}

    }

}

int main(int argc, char *argv[]) {

	//handleParameters(argc,argv);
	struct Buffer* buffer= createBuffer(DEFAULT_CAPACITY);
	struct Output* out = createOutput(DEFAULT_CAPACITY); 
	getInput(buffer,out);
    return 0; // Return 0 to indicate successful execution
}