#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

// Traits of the address and numbers to track for whole cache
typedef struct{
  // {s, b, S, B, E}
  int track[5];

  //number to track the solution
  int verbose;	
  int hit_count;
  int miss_count;
  int eviction_count;
} cacheTraits;

// The cacheLine struct holds all the info in a cache line
typedef struct {
  int numUses;
  int isValid;
  unsigned long long tagBits;
} cacheLine;

// The cacheSet struct holds all the lines in a given set
typedef struct {
    cacheLine *lineList;
} cacheSet;

// The cache struct holds all the sets
typedef struct {
    cacheSet *setList;
} cache;

int power(int base, int exp);
cache createCache(long long S, int E);
void createCacheLines(int E, cacheSet set, cacheLine line);
cacheTraits mySimulator(cache newCache, cacheTraits traits, unsigned long long tagBits, unsigned long long setBits);
int getMostUsed(cacheSet set, int E);
int getOpenLine(cacheSet set, int E);
int getLeastUsed(cacheSet set, int E);
void printUsage();

int main(int argc, char** argv) {
  // Declare a cache with its properties
  cache newCache;
  cacheTraits traits;
  int setAndBlock;
  int tagSize;

  // Variables for the writing file
  FILE *wrt;
  char ins; // instruction
  unsigned long long address;
  int length;
  
  // Variables to go follow input
  char* trace;
  char in = getopt(argc,argv,"v:s:b:t:E:h");
  traits.verbose = 0;

  // Parse information fromt the input
  while(in != -1) {
    // If input regards the traits of the cache and address
    if(in == 's' || in == 'b' || in == 'E') {
      // Index default is 0, for 's'
      int index = 0;

      // Change index if input is 'b' or 'E'
      if(in != 's') {
        index = in == 'b' ? 1 : 4;
      }
      
      // Instantiate value in traits' array
      traits.track[index] = atoi(optarg);

    // Instatiate verbose
    } else if(in == 'v') {
      traits.verbose = 1;

    // Instatntiate trace
    } else if(in == 't') {
      trace = optarg;

    // Print useful information and exit
    } else {
      printUsage();

      if(in == 'h') {
        exit(0);
      } else {
        exit(-1);
      }
    }
    in = getopt(argc,argv,"v:s:b:t:E:h");
  }
    
  // Get values of the total number of Sets and Block size of each line, and tag length
  traits.track[2] = power(2, traits.track[0]);
  traits.track[3] = power(2, traits.track[1]);
  setAndBlock = traits.track[0] + traits.track[1];
  tagSize = 64 - setAndBlock;
  
  // Instantiate the cache with number of sets and lines, and set counters
  newCache = createCache(traits.track[2], traits.track[4]);
  traits.miss_count = 0;
  traits.hit_count = 0;
  traits.eviction_count = 0;
    
  // Write information into the wrt File
  wrt = fopen(trace, "r");
  while(fscanf(wrt, " %c %llx,%d", &ins, &address, &length) == 3){
    // Run through the cache if inputs states so
    if(ins == 'L' || ins == 'S' || ins == 'M') {
      // Isolate the tag and set bits the address
      unsigned long long tagBits = address >> setAndBlock;
      unsigned long long setBits = (address << tagSize) >> (tagSize + traits.track[1]);

      traits = mySimulator(newCache, traits, tagBits, setBits);

      if(ins == 'M') {
        traits = mySimulator(newCache, traits, tagBits, setBits);
      }
    }
  }
  
  printSummary(traits.hit_count, traits.miss_count, traits.eviction_count);
  fclose(wrt);
  return 0;
}

// Gives result of base^exp
int power(int base, int exp) {
  int result = 1;
  while(exp-- > 0) {
    result *= base;
  }
  return result;
}

// Create the cache's sets and lines
cache createCache(long long S, int E) {
  cache newCache;	
  cacheSet set;
  cacheLine line;

  // Confirm there is enough memory for the number of sets
  size_t setSize = sizeof(cacheSet) * S;
  newCache.setList = (cacheSet*)malloc(setSize);

  // Create S sets
  for (int i = 0; i < S; i++) {
    // Confirm there is enough memory for the number of lines in each set
    size_t lineSize = sizeof(cacheLine) * E;
    set.lineList = (cacheLine*)malloc(lineSize);
    newCache.setList[i] = set;

    // Create the lines that go into the set
    createCacheLines(E, set, line);
  } 
  return newCache;
}

// Function creates E lines and instantiate the lines
void createCacheLines(int E, cacheSet set, cacheLine line) {
  for (int j = 0; j < E; j++) {
    line.isValid = 0; 
    line.tagBits = 0; 
    set.lineList[j] = line;
    line.numUses = 0;
  }
}

cacheTraits mySimulator(cache newCache, cacheTraits traits, unsigned long long tagBits, unsigned long long setBits) {
  // Instantiate set with the set in the cache at the index indicated by setBits
  cacheSet set = newCache.setList[setBits];

  // Variables for the hit detection and the number of lines
  int E = traits.track[4];
  int hasHit = 0;
  int max = getMostUsed(set, E);

  // Iterate over the lines in the set
  for (int i = 0; i < E; i++) {
    cacheLine curLine = set.lineList[i];

    // Enter if-statement if we detect a hit
    if(curLine.isValid && curLine.tagBits == tagBits) {
      // Update infor to show there is a hit
      hasHit = 1;
      traits.hit_count++;

      // Increase the number of uses for the line in the cache
      // Doesn't work with curLine++, not sure why
      newCache.setList[setBits].lineList[i].numUses++;
    }
  }

  // If there is a cache miss, determine process to insert memory into cache
  if(!hasHit) {
    // Check if there is a free spot and increase the number of misses
    int index = getOpenLine(set, E);
    traits.miss_count++;

    // A free spots were found
    if(index != -1) {
      // Change the free spot's status to show it has usable data
      set.lineList[index].isValid = 1;

    // No free spot was found
    } else {
      // Find the least used line
      traits.eviction_count++;
      index = getLeastUsed(set, E);
    }

    // Update the tagBits of the line with the new address info and update line
    set.lineList[index].tagBits = tagBits;
    int increment = newCache.setList[setBits].lineList[max].numUses + 1;
    newCache.setList[setBits].lineList[index].numUses = increment;
  }
  return traits;
}

// Function finds the line in the set with the most number of uses
int getMostUsed(cacheSet set, int E) {
  // Make default max to be the first line
  int max = set.lineList[0].numUses;
  int index = 0;

  // Iterate set starting with the second line
  for(int i = 1; i < E ; i++) {
    // Update if current line has been used more than current line
    if(set.lineList[i].numUses > max){
      max = set.lineList[i].numUses;
      index = i;
    }
  }
  return index;
}

// Verify if a particular set has an open spot
int getOpenLine(cacheSet set, int E) {
  // Iterate through the lines in the set
  for(int i = 0; i < E; i++){
    // Exit function in the first instance a line is not valid
    if(!set.lineList[i].isValid){
      return i;
    }
  }
  return -1;
}

// Function finds the line in the set with the least number of uses
int getLeastUsed(cacheSet set, int E) {
  // Make default min to be the first line
  int min = set.lineList[0].numUses;
  int index = 0;

  // Iterate set starting with the second line
  for(int i = 0; i < E; i++){
    // Update if current line has been used less than current line
    if(min > set.lineList[i].numUses) {
      min = set.lineList[i].numUses;
      index = i;
    }
  }
  return index;
}

// Print usage message
void printUsage() {
  printf("Usage: ./csim [-h] [-v] -s <s> -E <E> -b <b> -t <tracefile>\n");
  printf("-s: number of set index(2^s sets)\n");
  printf("-E: number of lines per set\n");
  printf("-b: number of block offset bits\n");
  printf("-t: trace file name\n");
}
