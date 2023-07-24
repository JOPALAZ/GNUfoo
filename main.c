#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
enum StatsType{None=0,Average=1,Dispersional=2};
struct Buffer
{
	__uint32_t* arr;
	__u_long size;
	__u_long capcity;
};
static bool gotStatsType=false;
static enum StatsType givenStatsType=None;
static int n=-1;
const int BUFFER_SIZE = 1024;
char *outputFilename = "values.txt";

void handleInput(int argc, char *argv[]){
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
struct Buffer* createBuffer(int capacity)
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
		__uint32_t* safetyBuf=malloc((buf->capcity<<1)*sizeof(__uint32_t)); 
		if(safetyBuf)
		{
			memcpy(safetyBuf,buf->arr,buf->capcity*sizeof(__uint32_t));
			free(buf->arr);
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
bool mergeBuffers(struct Buffer* buf,__uint32_t* vals,__uint32_t bytesRead)
{
	if(buf->capcity*sizeof(__uint32_t)<buf->size*sizeof(__uint32_t)+bytesRead)
	{
		__u_long newByteCapacity=buf->capcity<<1*sizeof(__uint32_t);
		if(newByteCapacity<buf->size*sizeof(__uint32_t)+bytesRead)
		{
			int lostBytes=(buf->size*sizeof(__uint32_t)+bytesRead)%sizeof(__uint32_t);
			newByteCapacity=buf->size*sizeof(__uint32_t)+bytesRead+lostBytes;
		}
		__uint32_t* safetyBuf=malloc(newByteCapacity); 
		if(safetyBuf)
		{
			memcpy(safetyBuf,buf->arr,buf->capcity*sizeof(__uint32_t));
			free(buf->arr);
			buf->arr=safetyBuf;
			buf->capcity=newByteCapacity/sizeof(__uint32_t);
		}
		else
		{
			return false;
		}
	}
	memcpy(buf->arr+buf->size,vals,bytesRead);
	buf->size+=(bytesRead+bytesRead%sizeof(__uint32_t))/sizeof(__uint32_t);
	return true;

}
void outputBuffer(struct Buffer* buf)
{
	__uint32_t* output;
	FILE *fp = fopen(outputFilename, "w");
	    if (fp == NULL)
    {
        fprintf(stderr,"Error opening the file %s", outputFilename);
		exit(EXIT_FAILURE);
    }
	int divisionParts=0;
	do
	{
		if(!divisionParts)
		{
			divisionParts=1;
		}
		else
		{
			divisionParts=divisionParts<<1;
		}
		if(buf->size%divisionParts!=0)
		{
			divisionParts=-1;
			break;
		}
		output=malloc(buf->size*sizeof(__uint32_t)/divisionParts);
	}
	while(!output);
	if(divisionParts>0)
	{
		for(int shift=0;shift<divisionParts;++shift)
		{
		for(int i=buf->size*shift/divisionParts;i<buf->size*(shift+1)/divisionParts;++i)
		{
			output[i%(buf->size*shift/divisionParts)]=(__int32_t)(buf->arr[i]<<15)>>15;
		}
		for(int i=0;i<buf->size/divisionParts;++i)
		{
			fprintf(fp,"%i\n",output[i]);
		}
		}
	}

}
struct Buffer* getInput(struct Buffer* buf)
{
    __uint32_t* input_buffer;
	input_buffer=malloc(sizeof(__uint32_t)*BUFFER_SIZE);
	if(!input_buffer)
	{
		fprintf(stderr,"Bad alloc, immposible to allocate necesarry resources");
		exit(EXIT_FAILURE);
	}
    // Читаем данные из стандартного ввода (stdin) в буфер
    __uint32_t bytesRead;
    while ((bytesRead = fread(input_buffer, sizeof(__uint32_t), BUFFER_SIZE, stdin)) > 0) 
	{
		if(!mergeBuffers(buf,input_buffer,bytesRead))
		{
			for(int i=0;i<(bytesRead+bytesRead%sizeof(__uint32_t))/sizeof(__uint32_t);++i)
			{
				if(!addElementToBuffer(buf,input_buffer[i]))
				{
					outputBuffer(buf);
					buf->size=0;
					if(!addElementToBuffer(buf,input_buffer[i]))
					{
						fprintf(stderr,"Bad alloc, immposible to allocate necesarry resources");
						exit(EXIT_FAILURE);
					}
				}
			}
		}

    }
	outputBuffer(buf);

}

int main(int argc, char *argv[]) {

	//handleInput(argc,argv);
	struct Buffer* buffer= createBuffer(1);
	getInput(buffer);
    return 0; // Return 0 to indicate successful execution
}