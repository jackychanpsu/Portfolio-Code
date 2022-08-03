#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <map>
#include <math.h>
#include <fcntl.h>
#include <vector>
#include <iterator>

#include "431project.h"

unsigned int getil1size(std::string configuration);
unsigned int getdl1size(std::string configuration);
unsigned int getl2size(std::string configuration);


using namespace std;

/*
 * Enter your PSU IDs here to select the appropriate scanning order.
 */
#define PSU_ID_SUM (904351957 + 947352523)
//1940369139%24=3 so we choose order 3 ie BP core FPU cache

/*
 * Some global variables to track heuristic progress.
 * 
 * Feel free to create more global variables to track progress of your
 * heuristic.
 */
int currentlyExploringDim =0;
bool currentDimDone = false;
bool isDSEComplete = false;
//int myOrder[] = {12, 13, 14, 11, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 1};

int myOrder[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 1, 12, 13, 14, 11};

vector<int> explore;
int counter = 0;
int global_val = 0;

/*
 * Given a half-baked configuration containing cache properties, generate
 * latency parameters in configuration string. You will need information about
 * how different cache paramters affect access latency.
 * 
 * Returns a string similar to "1 1 1"
 */
std::string generateCacheLatencyParams(string halfBackedConfig)
{
	//
    //YOUR CODE BEGINS HERE
    //
	int il1lat;
	int dl1lat;
	int ul2lat;
	
	unsigned int dl1blocksize = getdl1size(halfBackedConfig);
    unsigned int il1blocksize = getil1size(halfBackedConfig);
    unsigned int ul2blocksize = getl2size(halfBackedConfig);
	int dl1cacheconfig = extractConfigPararm(halfBackedConfig, 4);
    int il1cacheconfig = extractConfigPararm(halfBackedConfig, 6);
    int ul2cacheconfig = extractConfigPararm(halfBackedConfig, 9);

	int dl1cachecycles;
	int il1cachecycles;
	int ul2cachecycles;
    
	//detecting config for level 1 instruction cache
    if (il1blocksize == 2048)
	{
		il1lat = 1;
	} else if (il1blocksize == 4096)
	{
		il1lat = 2;
	} else if (il1blocksize == 8192)
	{
		il1lat = 3;
	} else if (il1blocksize == 16384)
	{
		il1lat = 4;
	} else if (il1blocksize == 32768)
	{
		il1lat = 5;
	} else if (il1blocksize == 65536)
	{
		il1lat = 6;
	} else
	{
		il1lat = 0;
	}

	//detecting config for level 1 data cache
    if (dl1blocksize == 2048)
	{
		dl1lat = 1;
	} else if (dl1blocksize == 4096)
	{
		dl1lat = 2;
	} else if (dl1blocksize == 8192)
	{
		dl1lat = 3;
	} else if (dl1blocksize == 16384)
	{
		dl1lat = 4;
	} else if (dl1blocksize == 32768)
	{
		dl1lat = 5;
	} else if (dl1blocksize == 65536)
	{
		dl1lat = 6;
	} else
	{
		dl1lat = 0;
	}
	
	//detecting config for level 2 unified cache
	if (ul2blocksize == 32768)
	{
		ul2lat = 5;
	} else if (ul2blocksize == 65536)
	{
		ul2lat = 6;
	} else if (ul2blocksize == 131072)
	{
		ul2lat = 7;
	} else if (ul2blocksize == 262144)
	{
		ul2lat = 8;
	} else if (ul2blocksize == 524288)
	{
		ul2lat = 9;
	} else if (ul2blocksize == 1048576)
	{
		ul2lat = 10;
	} else
	{
		ul2lat = 0;
	}
	
	//detecting if cache is direct mapped, 2 way assoc, 4 way assoc, or 8 way assoc
	if (il1cacheconfig == 0)
	{
		il1cachecycles = 0;
	} else if (il1cacheconfig == 1)
	{
		il1cachecycles = 1;
	} else if (il1cacheconfig == 2)
	{
		il1cachecycles = 2;
	} else if (il1cacheconfig == 3)
	{
		il1cachecycles = 3;
	}
	
	//determining total latency for instruction level 1 cache
	int il1totallat;
	il1totallat = il1lat + il1cachecycles - 1;

	//detecting if cache is direct mapped, 2 way assoc, 4 way assoc, or 8 way assoc
	if (dl1cacheconfig == 0)
	{
		dl1cachecycles = 0;
	} else if (dl1cacheconfig == 1)
	{
		dl1cachecycles = 1;
	} else if (dl1cacheconfig == 2)
	{
		dl1cachecycles = 2;
	} else if (dl1cacheconfig == 3)
	{
		dl1cachecycles = 3;
	}

	//determining total latency for direct mapped cache
	int dl1totallat;
	dl1totallat = dl1lat + dl1cachecycles - 1;

	//detecting if cache is direct mapped, 2 way assoc, 4 way assoc, or 8 way assoc
	if (ul2cacheconfig == 0)
	{
		ul2cachecycles = 0;
	} else if (ul2cacheconfig == 1)
	{
		ul2cachecycles = 1;
	} else if (ul2cacheconfig == 2)
	{
		ul2cachecycles = 2;
	} else if (ul2cacheconfig == 3)
	{
		ul2cachecycles = 3;
	} else if (ul2cacheconfig == 4)
	{
		ul2cachecycles = 4;
	}
    
	//determining total latency for level 2 unified cache
    int ul2totallat;
	ul2totallat = ul2lat + ul2cachecycles - 5;

	stringstream latencySettings;
	latencySettings << dl1totallat << " " << il1totallat << " " << ul2totallat;

    //
    //YOUR CODE ENDS HERE
    //
	
    return latencySettings.str();
}

/*
 * Returns 1 if configuration is valid, else 0
 */

int validateConfiguration(std::string configuration)
{
    unsigned int il1size = getil1size(configuration);
	unsigned int dl1size = getdl1size(configuration);
    unsigned int ul2size = getl2size(configuration);
	
	int l1cbyteconfig = extractConfigPararm(configuration, 2);
    int ul2byteconfig = extractConfigPararm(configuration, 8);

	int l1cbytesize;
    int ul2bytesize;
    
	
	//checking l1 cache block width
	if (l1cbyteconfig == 0)
	{
		l1cbytesize = 8;
	} else if (l1cbyteconfig == 1)
	{
		l1cbytesize = 16;
	} else if (l1cbyteconfig == 2)
	{
		l1cbytesize = 32;
	} else if (l1cbyteconfig == 3)
	{
		l1cbytesize = 64;
	}

	//checking unified cache block width
	if (ul2byteconfig == 0)
	{
		ul2bytesize = 16;
	} else if (ul2byteconfig == 1)
	{
		ul2bytesize = 32;
	} else if (ul2byteconfig == 2)
	{
		ul2bytesize = 64;
	} else if (ul2byteconfig == 3)
	{
		ul2bytesize = 128;
	}
    
	//checking any remaining constraints
    
    if (il1size < 2048)
	{
        return 0;
	}
	if (dl1size < 2048)
	{
        return 0;
	}
    if (il1size > 65536)
	{
        return 0;
	}
	if (dl1size > 65536)
	{
        return 0;
	}
    if (ul2size < 32768)
	{
        return 0;
	}
	if (ul2size > 1048576)
	{
        return 0;
	}
    if ((2 * l1cbytesize) > ul2bytesize)
	{
        return 0;
	}
	
	if (ul2size < 2 * (il1size + dl1size)) 
	{
		return 0;
	}
	
	
	
	/*
	if (!isNumDimConfiguration(configuration))
	{
        return 0;
	}
	*/
    return 1;
    
}

/*
 * Given the current best known configuration, the current configuration,
 * and the globally visible map of all previously investigated configurations,
 * suggest a previously unexplored design point. You will only be allowed to
 * investigate 1000 design points in a particular run, so choose wisely.
 *
 * In the current implementation, we start from the leftmost dimension and
 * explore all possible options for this dimension and then go to the next
 * dimension until the rightmost dimension.
 */
std::string generateNextConfigurationProposal(std::string currentconfiguration,
                                              std::string bestEXECconfiguration, std::string bestEDPconfiguration,
                                              int optimizeforEXEC, int optimizeforEDP)
{

    //
    // Some interesting variables in 431project.h include:
    //
    // 1. GLOB_dimensioncardinality
    // 2. GLOB_baseline
    // 3. NUM_DIMS
    // 4. NUM_DIMS_DEPENDENT
    // 5. GLOB_seen_configurations

    	std::string nextconfiguration = currentconfiguration;
	
		int count[NUM_DIMS - NUM_DIMS_DEPENDENT];
		for (int i = 0; i < (NUM_DIMS - NUM_DIMS_DEPENDENT); i++)
		{
		count[i] = 0;
		}
	// Continue if proposed configuration is invalid or has been seen/checked before.
	while (!validateConfiguration(nextconfiguration) ||
		GLOB_seen_configurations[nextconfiguration]) {
		
		//currentlyExploringDim = dim_order[count_order];
		counter++;
		// Check if DSE has been completed before and return current
		// configuration.
		if(isDSEComplete) {
			return currentconfiguration;
		}

		std::stringstream ss;

		string bestConfig;
		if (optimizeforEXEC == 1){
			bestConfig = bestEXECconfiguration;
		}
			

		if (optimizeforEDP == 1){
			bestConfig = bestEDPconfiguration;
		}
			

		
		// Fill in the dimensions already-scanned with the already-selected best
		// value.
		for (int dim = 0; dim < myOrder[currentlyExploringDim]; ++dim) {
			ss << extractConfigPararm(bestConfig, dim) << " ";
		}

		
		// Handling for currently exploring dimension. This is a very dumb
		// implementation.
		
		
		/*if(explore.size()>0){
			int nextValue = global_val;
			global_val++;
			explore.push_back(nextValue);
			ss << nextValue << " ";
		}

		else { 
			int nextValue = 0;
			global_val++;
			explore.push_back(nextValue);
			ss << nextValue << " ";
		}
		

		if (explore.size() >= GLOB_dimensioncardinality[myOrder[currentlyExploringDim]]) {
			currentDimDone = true;
			global_val = 0;
			explore.clear();	
		}
		*/
		if (count[currentlyExploringDim] == 0) {
			//The value in the first iteration should begin with 0 at that dimension
			ss << "0 ";
		}
		else{
			int nextValue = extractConfigPararm(nextconfiguration,
					myOrder[currentlyExploringDim]) + 1;
			
			//std::clog << "The Next Value is :" << nextValue << "\n";
			if (nextValue >= GLOB_dimensioncardinality[myOrder[currentlyExploringDim]]) {
				nextValue = GLOB_dimensioncardinality[myOrder[currentlyExploringDim]] - 1;
				currentDimDone = true;
			}
			
			ss << nextValue << " ";
			
		}
		
		count[currentlyExploringDim]++;
		// Fill in remaining independent params with 0.
		for (int dim = myOrder[currentlyExploringDim] + 1;
				dim < (NUM_DIMS - NUM_DIMS_DEPENDENT); ++dim) {
					ss << extractConfigPararm(bestConfig, dim) << " ";
		}


		// Last NUM_DIMS_DEPENDENT3 configuration parameters are not independent.
		// They depend on one or more parameters already set. Determine the
		// remaining parameters based on already decided independent ones.
		//
		string configSoFar = ss.str();

		// Populate this object using corresponding parameters from config.
		ss << generateCacheLatencyParams(configSoFar);

		// Configuration is ready now.
		nextconfiguration = ss.str();

		// Make sure we start exploring next dimension in next iteration.
		if (currentDimDone) {
			currentlyExploringDim++;
			currentDimDone = false;
			
		}

		if(counter == 1000)
			isDSEComplete = true;
 
		// Signal that DSE is complete after this configuration.
		if (currentlyExploringDim == (NUM_DIMS - NUM_DIMS_DEPENDENT))
		{
			
			currentlyExploringDim = 0;
			for (int i = 0; i < (NUM_DIMS - NUM_DIMS_DEPENDENT); i++)
			{
				count[i] = 0;
			}
		}
			
	}
	return nextconfiguration;
}
