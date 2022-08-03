#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "check.h"

int x[5] = {1,2,3,4,5};

void allocate()
{
    int i;
    int *p;
    for(i =1 ; i<1000000 ; i++)
    {
      p = malloc(500 * sizeof(int));
      if(func(i)) { free (p);}
    }
}

void allocate1()
{
  int i;
  int *p;
  for (i=1 ; i<10000 ; i++)
  {
    p = malloc(1000 * sizeof(int));
    if(i & 1)
      free (p);
  }
}

void allocate2()
{
  int i;
  int *p;
  for (i=1 ; i<300000 ; i++)
  {
    p = malloc(10000 * sizeof(int));
    free (p);
  }
}


int main(int argc, char const *argv[]) {
  struct rusage usage;
  struct timeval startUser, endUser, startCPU, endCPU;
  long maxResidentStart;
  long maxResidentEnd;
  long sigRecievedStart;
  long sigRecievedEnd;
  long volSwitchStart;
  long volSwitchEnd;
  long involSwitchStart;
  long involSwitchEnd;
  int i;
  int *p;
  printf("Executing the code ......\n");
  allocate();
  getrusage(RUSAGE_SELF, &usage);
  startUser = usage.ru_utime;
  startCPU = usage.ru_stime;
  maxResidentStart = usage.ru_maxrss;
  sigRecievedStart= usage.ru_nsignals;
  volSwitchStart = usage.ru_nvcsw;
  involSwitchStart = usage.ru_nivcsw;
  for (i=0 ; i<10000 ; i++)
  {
    p = malloc(1000 * sizeof(int));
    free (p);
  }
  endUser = usage.ru_utime;
  endCPU = usage.ru_stime;
  maxResidentEnd = usage.ru_maxrss;
  sigRecievedEnd= usage.ru_nsignals;
  volSwitchEnd = usage.ru_nvcsw;
  involSwitchEnd = usage.ru_nivcsw;
  printf("CPU start time: %ld.%lds\n", startCPU.tv_sec, startCPU.tv_usec);
  printf("CPU end time: %ld.%lds\n", endCPU.tv_sec, endCPU.tv_usec);
  printf("User start time: %ld.%lds\n", startUser.tv_sec, startUser.tv_usec);
  printf("User start time: %ld.%lds\n", endUser.tv_sec, endUser.tv_usec);
  printf("Maximum Resident Start: %ld\n", maxResidentStart);
  printf("Maximum Resident End: %ld\n", maxResidentEnd);
  printf("Signals Recieved Start: %ld\n", sigRecievedStart);
  printf("Signals Recieved End: %ld\n", sigRecievedEnd);
  printf("Voluntary Swtiches Start: %ld\n", volSwitchStart);
  printf("Voluntary Swtiches End: %ld\n", volSwitchEnd);
  printf("Involuntary Swtiches Start: %ld\n", involSwitchStart);
  printf("Involuntary Swtiches End: %ld\n", involSwitchEnd);
  printf("Program execution successfull\n");
  return 0;
}
