////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_cache.c
//  Description    : This is the implementation of the cache for the 
//                   FS3 filesystem interface.
//
//  Author         : Justin Cote
//  Last Modified  : Nov 19th 2021
//

// Includes
#include <cmpsc311_log.h>
#include <stdlib.h>
#include <string.h>


// Project Includes
#include <fs3_cache.h>

//
// Support Macros/Data
cacheItem * cacheArray;
int * cacheSize;
int itemsInCache;
int cachePuts = 0;
int cacheGets = 0;
int cacheHits = 0;
int cacheMiss = 0;
//
// Implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_init_cache
// Description  : Initialize the cache with a fixed number of cache lines
//
// Inputs       : cachelines - the number of cache lines to include in cache
// Outputs      : 0 if successful, -1 if failure

int fs3_init_cache(uint16_t cachelines) {
    if (cachelines >= 0){
        cacheArray = (cacheItem *) calloc(cachelines, sizeof(cacheItem));
        if (cacheArray == NULL){logMessage(FS3DriverLLevel, "could not allocate cacheArray");return -1 ;}
        cacheSize = (int*)calloc(1,sizeof(int));
        if (cacheSize == NULL){logMessage(FS3DriverLLevel,"could not allocate cacheSize");return -1;}
        *cacheSize = cachelines;
        itemsInCache = 0;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_close_cache
// Description  : Close the cache, freeing any buffers held in it
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int fs3_close_cache(void)  {
    free(cacheArray);
    cacheArray = NULL;
    free(cacheSize);
    cacheSize = NULL;
    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_put_cache
// Description  : Put an element in the cache
//
// Inputs       : trk - the track number of the sector to put in cache
//                sct - the sector number of the sector to put in cache
// Outputs      : 0 if inserted, -1 if not inserted

int fs3_put_cache(FS3TrackIndex trk, FS3SectorIndex sct, void *buf) {

    int i;
    int lruIndex;
    int replaceIndex;
    int exists = 0;
    for (i=0; i< itemsInCache; i++){
        if (cacheArray[i].sector == sct && cacheArray[i].track == trk){
        exists = 1;
        break;
        }
    }

    //if cache is full
    if (cacheSize[0] == itemsInCache){
        // getting position in cache to replace
        
        //if not already in cache 
        if (exists == 0){
            for (i=0;i<cacheSize[0];i++){
                if (cacheArray[i].posInCache == cacheSize[0]){
                    lruIndex = i;
                    break;
                }
            }
            cacheArray[lruIndex].sector = sct;
            cacheArray[lruIndex].track = trk;
            memcpy(cacheArray[lruIndex].data,buf,FS3_SECTOR_SIZE);
            for (i = 0;i<cacheSize[0];i++){
                cacheArray[i].posInCache += 1;
            }
            cacheArray[lruIndex].posInCache = 1;
        }
        //if already in cache must replace that position
        else {
            for (i=0;i<cacheSize[0];i++){
                if (cacheArray[i].sector == sct && cacheArray[i].track == trk){
                    lruIndex = i;
                    break;
                }
            }
            memcpy(cacheArray[lruIndex].data,buf,FS3_SECTOR_SIZE);
            replaceIndex = cacheArray[lruIndex].posInCache;
            for (i =0;i<cacheSize[0];i++){
                if (cacheArray[i].posInCache < replaceIndex){
                    cacheArray[i].posInCache += 1;
                }
            }
            cacheArray[lruIndex].posInCache = 1;
            
        }
    }
    //if cache isn't full or empty
    else{
        // if already in cache must replace that position 
        if (exists == 1){
            for (i=0;i<itemsInCache;i++){
                if (cacheArray[i].sector == sct && cacheArray[i].track == trk){
                    lruIndex=i;
                    break;
                }
            }
            memcpy(cacheArray[lruIndex].data,buf,FS3_SECTOR_SIZE);
            replaceIndex = cacheArray[lruIndex].posInCache;
            if (replaceIndex != 1){
                for (i = 0; i<itemsInCache;i ++){
                    if (cacheArray[i].posInCache < replaceIndex){
                        cacheArray[i].posInCache += 1;
                    }
                }
                cacheArray[lruIndex].posInCache = 1;
            }
        }

        else{
            cacheArray[itemsInCache].sector = sct;
            cacheArray[itemsInCache].track = trk;
            memcpy(cacheArray[itemsInCache].data,buf,FS3_SECTOR_SIZE);
            for (i = 0; i<=itemsInCache; i++){
                cacheArray[i].posInCache += 1;
            }
            itemsInCache += 1;
        }
    }
    cachePuts += 1;
    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_get_cache
// Description  : Get an element from the cache (
//
// Inputs       : trk - the track number of the sector to find
//                sct - the sector number of the sector to find
// Outputs      : returns NULL if not found or failed, pointer to buffer if found

void * fs3_get_cache(FS3TrackIndex trk, FS3SectorIndex sct)  {
    cacheGets +=1;
    int i;
    int replaceIndex;
    char *outptr;
    for (i=0; i< cacheSize[0]; i++){
        if (cacheArray[i].sector == sct && cacheArray[i].track == trk){
            outptr = cacheArray[i].data;
            replaceIndex = cacheArray[i].posInCache;
            int j;
            for (j=0;j<itemsInCache;j++){
                if (cacheArray[j].posInCache < replaceIndex){
                    cacheArray[j].posInCache += 1;
                }
            }
            cacheArray[i].posInCache = 1;
            cacheHits += 1;
            return outptr;
        }
    }
    cacheMiss += 1;
    return(NULL);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_log_cache_metrics
// Description  : Log the metrics for the cache 
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int fs3_log_cache_metrics(void) {
    double cachePerformance;
    cachePerformance = (double)cacheHits/(double)cacheGets; 
    logMessage(LOG_OUTPUT_LEVEL,"Cache inserts      [      %d]",cachePuts);
    logMessage(LOG_OUTPUT_LEVEL,"Cache gets         [      %d]",cacheGets);
    logMessage(LOG_OUTPUT_LEVEL,"Cache hits         [      %d]",cacheHits);
    logMessage(LOG_OUTPUT_LEVEL,"Cache misses       [      %d]",cacheMiss);
    logMessage(LOG_OUTPUT_LEVEL,"Cache hit ratio    [      %.2f%%]",cachePerformance*100);
    return(0);
}

