#include "simulator.h"
#include <string.h>
int *pagetable;
int addressNumber = 0;
int init_TLB(int frames)
{
    int i;
    timeCounter = 0;
    cacheHit = 0;
    cacheMiss = 0;
    cacheHitRatio = 0;
    char empty;
    empty = 0;
    
    for (i = 0; i < 16; i++)
    {
        TLB[i].timeOfUse = 0;
        TLB[i].vAddress = 0;
        TLB[i].pAddress = 0;
        //memset(TLB[i].data, empty, pow(2,20)); //pow(2,20) = size of data
    }

    cachePointer = (int *) calloc(frames, sizeof(TLBentry)); 

    if (cachePointer == NULL) return (-1);

    return( 0 );
}

int get_TLB(char* addr)
{
    int i, addrFound = 0;
    TLBentry *findAddr;

    timeCounter += 1;

    for (i = 0; i < 16; i++)
    {
        if (TLB[i].vAddress == (int) *addr) //strcmp(TLB[i].vAddress, addr) == 0
        {
            addrFound = 1;
            findAddr = &TLB[i];
            cacheHit += 1;
            break;
        }
    }

    if (addrFound == 1)
    {
        findAddr->timeOfUse = timeCounter;
        return (1);
    }
    else 
    {
        cacheMiss += 1;
        return (-1);
    }
}

int put_TLB(char *addr)
{
    int i, placed = 0, oldIndex = 0;

    TLBentry putFrame;
    TLBentry *oldest;

    timeCounter += 1;


    //memcpy(putFrame.data, data, pow(2,20));

    oldest = &TLB[0];

    for (i = 0; i < 16; i++)
    {
       // printf("PUT_TLB right before comparison");
        if (TLB[i].vAddress == (int) *addr) //strcmp(TLB[i].vAddress, addr) == 0
        {
            putFrame.timeOfUse = timeCounter;
            TLB[i] = putFrame; //this line might need to be fixed
            //memcpy(&TLB[i], &putFrame, sizeof(TLBentry));
            placed = 1;
            cacheHit += 1;

            break;
        }
    }
    if(!placed)
    {
        cacheMiss += 1;
        for (i = 0; i < cacheFrames; i++)
        {
            if (TLB[i].timeOfUse == 0)
            {
                putFrame.timeOfUse = timeCounter;
                TLB[i] = putFrame; //this might also give error
                //memcpy(&TLB[i], &putFrame, sizeof(TLBentry));
                placed = 1;

                break;
            }
            else
            {
                if (TLB[i].timeOfUse < oldest->timeOfUse)
                {
                    oldest = &TLB[i];
                    oldIndex = i;
                }
            }
        }
    }
    if (!placed)
    {
        putFrame.timeOfUse = timeCounter;
        TLB[oldIndex] = putFrame; // might give errors
        //memcpy(&TLB[oldIndex], &putFrame, sizeof(TLBentry));

        placed = 1;
    }

    return 1;
}


void init()
{
    current_time = 0;
    nextQuanta = current_time + quantum;
    readyProcess = gll_init();
    runningProcess= gll_init();
    blockedProcess = gll_init();

    processList = gll_init();
    traceptr = openTrace(traceFileName);

    sysParam = readSysParam(traceptr);

    //read traces from trace file and put them in the processList
    struct PCB* temp = readNextTrace(traceptr);
    if(temp == NULL)
    {
        printf("No data in file. Exit.\n");
        exit(1);
    }

    while(temp != NULL)
    {
        gll_pushBack(processList, temp);
        temp = readNextTrace(traceptr);
    }

    //transfer ready processes from processList to readyProcess list
    temp = gll_first(processList);
    
    while((temp!= NULL) && ( temp->start_time <= current_time))
    {
        struct NextMem* tempAddr;
        temp->memoryFile = openTrace(temp->memoryFilename);
        temp->numOfIns = readNumIns(temp->memoryFile);
        tempAddr = readNextMem(temp->memoryFile);
        while(tempAddr!= NULL)
        {
            gll_pushBack(temp->memReq, tempAddr);
            tempAddr = readNextMem(temp->memoryFile);
        }
        gll_pushBack(readyProcess, temp);
        gll_pop(processList);

        temp = gll_first(processList);
    }

    //TODO: Initialize what you need
    cacheFrames = sysParam->TLB_size_in_entries;
    
    init_TLB(cacheFrames);
    int addressNumber = 0;
    int dramSize = 0;

    pagetable = calloc(16777215, sizeof( int ));
}

void finishAll()
{
    if((gll_first(readyProcess)!= NULL) || (gll_first(runningProcess)!= NULL) || (gll_first(blockedProcess)!= NULL) || (gll_first(processList)!= NULL))
    {
        printf("Something is still pending\n");
    }
    gll_destroy(readyProcess);
    gll_destroy(runningProcess);
    gll_destroy(blockedProcess);
    gll_destroy(processList);

//TODO: Anything else you want to destroy

    closeTrace(traceptr);
}

void statsinit()
{
    // statsList = gll_init();
    resultStats.perProcessStats = gll_init();
    resultStats.executionOrder = gll_init();
    resultStats.start_time = current_time;
    
}

void statsUpdate()
{
    resultStats.OSModetime = OSTime;
    resultStats.userModeTime  = userTime;   
    resultStats.numberOfContextSwitch = numberContextSwitch;
    resultStats.end_time = current_time;
}

//returns 1 on success, 0 if trace ends, -1 if page fault
int readPage(struct PCB* p, uint64_t stopTime)
{
    struct NextMem* addr = gll_first(p->memReq);
    uint64_t timeAvailable = stopTime - current_time;
    
    if(addr == NULL)
    {
        return 0;
    }
    if(debug == 1)
    {
        printf("Request::%s::%s::\n", addr->type, addr->address);
    }

    if(strcmp(addr->type, "NONMEM") == 0)
    {
        uint64_t timeNeeded = (p->fracLeft > 0)? p->fracLeft: sysParam->non_mem_inst_length;
    
        if(timeAvailable < timeNeeded)
        {
            current_time += timeAvailable;
            userTime += timeAvailable;
            p->user_time += timeAvailable;
            p->fracLeft = timeNeeded - timeAvailable;
        }
        else{
            gll_pop(p->memReq);
            current_time += timeNeeded; 
            userTime += timeNeeded;
            p->user_time += timeNeeded;
            p->fracLeft = 0;
        }

        if(gll_first(p->memReq) == NULL)
        {
            return 0;
        }
        return 1;
    }
    else{
        //TODO: for MEM traces
        char *justAddress = addr->address;

        int intAddress = (int)strtol(addr->address, NULL, 0);
        int pageNumber = ((intAddress & 0xFFFFF000)>>3);
        int offset = (intAddress & 0x00000FFF);
        int frame = -1;
        int i;
        if (get_TLB(justAddress)==1) //justAddress changed from intAddress
        {
        	//printf("READPAGE:MEM - in first if statement");
        	frame=1;
        	return 1;
        }

        if (frame == -1 && pagetable[pageNumber] == 0)
        {
        	   //printf("READPAGE:MEM - in second if statement");
        		//printf("READPAGE:MEM - pagetable[pn] was zero \n");
        		int pAddress = dramCheck(addr->address);
        		//printf("pAddress: %f\n", pAddress);
        		pagetable[pageNumber] = pAddress;
        		p->hitCount++;
        		frame = pageNumber;
        		put_TLB ( addr->address );
        		return 0;
        }
        else
        {
        	//printf("READPAGE:MEM - in else statement");
        	put_TLB ( addr->address );
        	p->missCount++;
        	return -1;
        }
        printf("Mem trace not handled\n");
        exit(1);
    }
}

void schedulingRR(int pauseCause)
{
    //move first readyProcess to running
    gll_push(runningProcess, gll_first(readyProcess));
    gll_pop(readyProcess);

    if(gll_first(runningProcess) != NULL)
    {
        current_time = current_time + contextSwitchTime;
        OSTime += contextSwitchTime;
        numberContextSwitch++;
        struct PCB* temp = gll_first(runningProcess);
        gll_pushBack(resultStats.executionOrder, temp->name);
    }
}

/*runs a process. returns 0 if page fault, 1 if quanta finishes, -1 if traceFile ends, 2 if no running process, 4 if disk Interrupt*/
int processSimulator()
{
    uint64_t stopTime = nextQuanta;
    int stopCondition = 1;
    if(gll_first(runningProcess)!=NULL)
    {
        
        if (gll_first(blockedProcess) != NULL)
        	{
        		struct PCB* temp = gll_first(blockedProcess);
        		stopTime = temp->start_time;
        		stopCondition = 4;
            }
        

        while(current_time < stopTime)
        {
            
            int read = readPage(gll_first(runningProcess), stopTime);
            if(debug == 1){
                printf("Read: %d\n", read);
                printf("Current Time %" PRIu64 ", Next Quanta Time %" PRIu64 " %" PRIu64 "\n",current_time, nextQuanta, stopTime);
            }
            if(read == 0)
            {
                return -1;
                break;
            }
            else if(read == -1) //page fault
            {
                if(gll_first(runningProcess) != NULL)
                {
                    gll_pushBack(blockedProcess, gll_first(runningProcess));
                    gll_pop(runningProcess);

                    return 0;
                }
                
            }
        }
        if(debug == 1)
        {
            printf("Stop condition found\n");
            printf("Current Time %" PRIu64 ", Next Quanta Time %" PRIu64 "\n",current_time, nextQuanta);
        }
        return stopCondition;
    }
    if(debug == 1)
    {
        printf("No running process found\n");
    }
    return 2;
}

void cleanUpProcess(struct PCB* p)
{
    //struct PCB* temp = gll_first(runningProcess);
    struct PCB* temp = p;
   //TODO: Adjust the amount of available memory as this process is finishing
    
    struct Stats* s = malloc(sizeof(stats));
    s->processName = temp->name;
    s->hitCount = temp->hitCount;
    s->missCount = temp->missCount;
    s->user_time = temp->user_time;
    s->OS_time = temp->OS_time;

    s->duration = current_time - temp->start_time;
    
    gll_pushBack(resultStats.perProcessStats, s);
    
    gll_destroy(temp->memReq);
    closeTrace(temp->memoryFile);

}

void printPCB(void* v)
{
    struct PCB* p = v;
    if(p!=NULL){
        printf("%s, %" PRIu64 "\n", p->name, p->start_time);
    }
}

void printStats(void* v)
{
    struct Stats* s = v;
    if(s!=NULL){
        double hitRatio = s->hitCount / (1.0* s->hitCount + 1.0 * s->missCount);
        printf("\n\nProcess: %s: \nHit Ratio = %lf \tProcess completion time = %" PRIu64 
                "\tuser time = %" PRIu64 "\tOS time = %" PRIu64 "\n", s->processName, hitRatio, s->duration, s->user_time, s->OS_time) ;
    }
}

void printExecOrder(void* v)
{
    char* c = v;
    if(c!=NULL){
        printf("%s\n", c) ;
    }
}



void diskToMemory()
{
    // TODO: Move requests from disk to memory
    // TODO: move appropriate blocked process to ready process

    //simpause = 0 ---- mmu detects a page fault 
    //		go into OS, put process to blocked state (from running), 
    //		issue IO request to swap device

    //simpause = 4 ---- disk int. has arrived
    //		pause running process, running page eviction, update page tables
    //		move process at the top of blocked list into ready list (now that page in mem)
    //		diskIO should take a fixed amount of time
    //		if disk int arrives, should be for the process at head of blockedProcess
    struct PCB* temp = gll_pop(blockedProcess);
 
    
    resultStats.numberOfDiskInt++;
    gll_pushBack(readyProcess, temp);
    if(debug == 1)
    {
        printf("Done diskToMemory\n");
    }
}


void simulate()
{
    init();
    statsinit();

    //get the first ready process to running state
    struct PCB* temp = gll_first(readyProcess);
    gll_pushBack(runningProcess, temp);
    gll_pop(readyProcess);

    struct PCB* temp2 = gll_first(runningProcess);
    gll_pushBack(resultStats.executionOrder, temp2->name);

    while(1)
    {
        int simPause = processSimulator();
        if(current_time == nextQuanta)
        {
            nextQuanta = current_time + quantum;
        }

        //transfer ready processes from processList to readyProcess list
        struct PCB* temp = gll_first(processList);
        
        while((temp!= NULL) && ( temp->start_time <= current_time))
        {
            temp->memoryFile = openTrace(temp->memoryFilename);
            temp->numOfIns = readNumIns(temp->memoryFile);

            struct NextMem* tempAddr = readNextMem(temp->memoryFile);

	        while(tempAddr!= NULL)
            {
                gll_pushBack(temp->memReq, tempAddr);
                tempAddr = readNextMem(temp->memoryFile);
            }
            gll_pushBack(readyProcess, temp);
            gll_pop(processList);

            temp = gll_first(processList);
        }

        //move elements from disk to memory
        diskToMemory();

        //This memory trace done
        if(simPause == -1)
        {
            //finish up this process
            cleanUpProcess(gll_first(runningProcess));
            gll_pop(runningProcess);
        }

        //move running process to readyProcess list
        int runningProcessNUll = 0;
        if(simPause == 1 || simPause == 4)
        {
            if(gll_first(runningProcess) != NULL)
            {
                gll_pushBack(readyProcess, gll_first(runningProcess));
                gll_pop(runningProcess);
            }
            else{
                runningProcessNUll = 1;
            }
            if(simPause == 1)
            {
                nextQuanta = current_time + quantum;
            }
        }

        schedulingRR(simPause);

        //Nothing in running or ready. need to increase time to next timestamp when a process becomes ready.
        if((gll_first(runningProcess) == NULL) && (gll_first(readyProcess) == NULL))
        {
            if(debug == 1)
            {
                printf("\nNothing in running or ready\n");
            }
            if((gll_first(blockedProcess) == NULL) && (gll_first(processList) == NULL))
            {

                    if(debug == 1)
                    {
                        printf("\nAll done\n");
                    }
                    break;
            }
            struct PCB* tempProcess = gll_first(processList);
            struct PCB* tempBlocked = gll_first(blockedProcess);

            //TODO: Set correct value of timeOfNextPendingDiskInterrupt
            uint64_t timeOfNextPendingDiskInterrupt = 0;

            if(tempBlocked == NULL)
            {
                if(debug == 1)
                {
                    printf("\nGoing to move from proess list to ready\n");
                }
                struct NextMem* tempAddr;
                tempProcess->memoryFile = openTrace(tempProcess->memoryFilename);
                tempProcess->numOfIns = readNumIns(tempProcess->memoryFile);
                tempAddr = readNextMem(tempProcess->memoryFile);
                while(tempAddr!= NULL)
                {
                    gll_pushBack(tempProcess->memReq, tempAddr);
                    tempAddr = readNextMem(tempProcess->memoryFile);
                }
                gll_pushBack(readyProcess, tempProcess);
                gll_pop(processList);
                
                while(nextQuanta < tempProcess->start_time)
                {   
                    
                    current_time = nextQuanta;
                    nextQuanta = current_time + quantum;
                }
                OSTime += (tempProcess->start_time-current_time);
                current_time = tempProcess->start_time; 
            }
            else
            {
                if(tempProcess == NULL)
                {
                    if(debug == 1)
                    {
                        printf("\nGoing to move from blocked list to ready\n");
                    }
                    OSTime += (timeOfNextPendingDiskInterrupt-current_time);
                    current_time = timeOfNextPendingDiskInterrupt;
                    while (nextQuanta < current_time)
                    {
                        nextQuanta = nextQuanta + quantum;
                    }
                    diskToMemory();
                }
                else if(tempProcess->start_time >= timeOfNextPendingDiskInterrupt)
                {
                    if(debug == 1)
                    {
                        printf("\nGoing to move from blocked list to ready\n");
                    }
                    OSTime += (timeOfNextPendingDiskInterrupt-current_time);
                    current_time = timeOfNextPendingDiskInterrupt;
                    while (nextQuanta < current_time)
                    {
                        nextQuanta = nextQuanta + quantum;
                    }
                    diskToMemory();
                }
                else{
                    struct NextMem* tempAddr;
                    if(debug == 1)
                    {
                        printf("\nGoing to move from proess list to ready\n");
                    }
                    tempProcess->memoryFile = openTrace(tempProcess->memoryFilename);
                    tempProcess->numOfIns = readNumIns(tempProcess->memoryFile);
                    tempAddr = readNextMem(tempProcess->memoryFile);
                    while(tempAddr!= NULL)
                    {
                        gll_pushBack(tempProcess->memReq, tempAddr);
                        tempAddr = readNextMem(tempProcess->memoryFile);
                    }
                    gll_pushBack(readyProcess, tempProcess);
                    gll_pop(processList);
                    
                    while(nextQuanta < tempProcess->start_time)
                    {   
                        current_time = nextQuanta;
                        nextQuanta = current_time + quantum;
                    }
                    OSTime += (tempProcess->start_time-current_time);
                    current_time = tempProcess->start_time; 
                }
            }   
        }
    }
}

int main(int argc, char** argv)
{
    if(argc == 1)
    {
        printf("No file input\n");
        exit(1);
    }
    traceFileName = argv[1];
    outputFileName = argv[2];

    simulate();
    finishAll();
    statsUpdate();

    if(writeToFile(outputFileName, resultStats) == 0)
    {
        printf("Could not write output to file\n");
    }
    printf("User time = %" PRIu64 "\nOS time = %" PRIu64 "\n", resultStats.userModeTime, resultStats.OSModetime);
    printf("Context switched = %d\n", resultStats.numberOfContextSwitch);
    printf("Start time = 0\nEnd time =%llu", current_time);
    gll_each(resultStats.perProcessStats, &printStats);

    // printf("\nExec Order:\n");
    // gll_each(resultStats.executionOrder, &printExecOrder);
    printf("\n");
}

int dramCheck(char* address) {
	
	
	int dramEmpty = 0; 
	int entryFound = 0;

    if (dramSize == 0) {
    	dramEmpty = 1;
        addressNumber += 1;
        dramSize += 1;
        dram = malloc(dramSize * sizeof(char*));
        dram[0] = *address;
        return dramSize;
    }
    else {
        int i;
        for (i = 0; i < addressNumber; i++) {
            if (dram[i] == *address) {
            	entryFound = 1;
                return i;
            }
        }
    }

    if (!dramEmpty && !entryFound)
    {
    	dramSize += 1;
    	addressNumber += 1;
        dram = realloc(dram, dramSize * sizeof(char*));
    	dram[addressNumber] = address;
    	return addressNumber;
    }
}