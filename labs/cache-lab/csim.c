#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#define ToStderr(format, ...) fprintf(stderr, format, __VA_ARGS__)
typedef struct {
  int32_t hits;
  int32_t misses;
  int32_t evictions;
} Stats;

typedef struct {
  uint32_t S;
  uint32_t E;
  uint32_t s_bits;
  uint32_t b_bits;
  uint32_t t_bits;
  uint64_t time_stamp;
} LRUCacheParams;

typedef struct {
  bool valid_bit;
  uint64_t tag;
  uint64_t time_stamp;
} LRUCacheLine;

typedef LRUCacheLine LRUCache;

typedef struct {
  bool h;
  bool v;
  uint32_t s;
  uint32_t E;
  uint32_t b;
  const char* trace_file;
} Args;

typedef struct {
  char op;
  uint64_t address;
  uint64_t size;
} MemoryOperation;

extern char *optarg;
extern int optind;

const char* help_str =\
"Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
"Options:\n"
"  -h         Print this help message.\n"
"  -v         Optional verbose flag.\n"
"  -s <num>   Number of set index bits.\n"
"  -E <num>   Number of lines per set.\n"
"  -b <num>   Number of block offset bits.\n"
"  -t <file>  Trace file.\n"
"\n"
"Examples:\n"
"  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
"  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n";

void ArgsToString(const Args* args) {
  if (args->h) printf("-h\n");
  if (args->v) printf("-v\n");
  printf("-s %d\n", args->s);
  printf("-E %d\n", args->E);
  printf("-b %d\n", args->b);
  printf("-t %s\n", args->trace_file);
}

bool ArgParser(int argc, char* argv[], Args* args) {

  if (argc == 1) {
    ToStderr("%s: Missing required command line argument\n", argv[0]);
    ToStderr("%s", help_str);
    return false;
  }

  args->h = false;
  args->v = false;
  args->s = 0;
  args->E = 0;
  args->b = 0;
  args->trace_file = NULL;

  // Parse Option Arguments
  char c;
  while ((c = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
    switch (c) {
      case 'h':
        args->h = true;
        break;
      case 'v':
        args->v = true;
        break;
      case 's':
        args->s = atoi(optarg);
        break;
      case 'E':
        args->E = atoi(optarg);
        break;
      case 'b':
        args->b = atoi(optarg);
        break;
      case 't':
        args->trace_file = optarg;
        break;
      default:
        return false;
    }
  }

  if (args->h) {
    ToStderr("%s", help_str);
    return false;
  }

  if (args->s == 0 || args->E == 0 ||
      args->b == 0 || args->trace_file == NULL) {
    ToStderr("Arguments are no complete.\n%s", help_str);
    return false;
  }

  return true;
}

LRUCache** InitLRUCache(LRUCacheParams* params) {
  uint32_t S = params->S;
  uint32_t E = params->E;
  LRUCache** cache = NULL;

  cache = (LRUCache**)malloc(S * sizeof(LRUCacheLine*));
  if (cache == NULL) {
    ToStderr("Error in allocate memory size: %lu bytes\n",
             S * sizeof(LRUCacheLine));
    return NULL;
  }

  // allocate and init all to 0
  cache[0] = (LRUCache*)calloc(S * E, sizeof(LRUCacheLine));
  if (cache[0] == NULL) {
    ToStderr("Error in allocate memory size: %lu bytes\n",
             S * E * sizeof(LRUCacheLine));
    return NULL;
  }

  size_t stride = E * sizeof(LRUCacheLine);
  for (int i = 1; i < S; ++i) {
    cache[i] = cache[i-1] + stride;
  }
  return cache;
}

void DeallocateLRUCache(LRUCache** cache) {
  if (cache == NULL) return;
  free(cache[0]);
  free(cache);
}

uint64_t AddressToTag(const LRUCacheParams* params, uint64_t address) {
  return (address >> (64 - params->t_bits));
}

uint64_t AddressToSetIndex(const LRUCacheParams* params, uint64_t address) {
  uint64_t mask = (1 << (64 - params->t_bits)) - 1; // clear tag bits
  return (address & mask) >> params->b_bits; // get index
}

void LRUCacheSimulate(const MemoryOperation* memory_op,
                      LRUCache** cache,
                      LRUCacheParams* params,
                      Stats* stats,
                      bool verbose) {
  uint64_t address = memory_op->address;
  uint64_t set_index = AddressToSetIndex(params, address);
  uint64_t tag = AddressToTag(params, address);
  // printf("Tag = %lld, Set Index = %lld\n", tag, set_index);

  uint32_t E = params->E;
  uint32_t i = 0;
  uint64_t min_op_time_idx = 0;

  bool miss = true;
  for (i = 0; i < E; ++i) {
    uint64_t min_op_time = cache[set_index][min_op_time_idx].time_stamp;
    uint64_t curr_op_time = cache[set_index][i].time_stamp;
    min_op_time_idx = min_op_time > curr_op_time ? i : min_op_time_idx;
    if (cache[set_index][i].valid_bit && cache[set_index][i].tag == tag) {
      miss = false;
      break;
    }
  }

  ++params->time_stamp;

  // Use LRU Replacement Policy
  bool eviction = false;
  if (miss) {
    // printf("min_op_time_idx %llu\n", min_op_time_idx);
    eviction = cache[set_index][min_op_time_idx].valid_bit;
    cache[set_index][min_op_time_idx].valid_bit = 1;
    cache[set_index][min_op_time_idx].tag = tag;
    cache[set_index][min_op_time_idx].time_stamp = params->time_stamp;
  } else {
    cache[set_index][i].time_stamp = params->time_stamp; // update time stamp
  }

  if (miss) {
    ++stats->misses;
  } else {
    ++stats->hits;
  }

  if (eviction) {
    ++stats->evictions;
  }

  if (memory_op->op == 'M') {
    ++stats->hits; // write hit
  }

  if (verbose) {
    printf("%c %llx,%llu", memory_op->op, memory_op->address, memory_op->size);
    if (miss) {
      printf(" miss");
    } else {
      printf(" hit");
    }
    if (eviction) {
      printf(" eviction");
    }
    if (memory_op->op == 'M') {
      printf(" hit");
    }
    printf("\n");
  }
}

void LRUCacheParamsToString(const LRUCacheParams* params) {
  printf("Number of set bits: %u\n", params->s_bits);
  printf("Number of block bits: %u\n", params->b_bits);
  printf("Number of tag bits: %u\n", params->t_bits);
  printf("Number of sets: S = %u\n", params->S);
  printf("Number of Lines: E = %u\n", params->E);
  printf("Init Time Stamp: %llu\n", params->time_stamp);
}

void MemoryOperationToString(const MemoryOperation* ops) {
  printf("%c %llx,%llu\n", ops->op, ops->address, ops->size);
}

int main(int argc, char* argv[]) {
  Args args;
  if (!ArgParser(argc, argv, &args)) return -1;
  ArgsToString(&args);
  if (args.s + args.b > 64) {
    ToStderr("%s\n", "Error sum of set bits and block bits > 64");
    return -1;
  }

  LRUCacheParams cache_params;
  cache_params.s_bits = args.s;
  cache_params.b_bits = args.b;
  cache_params.t_bits = 64 - (args.s + args.b);
  cache_params.S = 1 << args.s;
  cache_params.E = args.E;
  cache_params.time_stamp = 0;

  Stats stats;
  stats.hits = 0;
  stats.misses = 0;
  stats.evictions = 0;

  LRUCacheParamsToString(&cache_params);

  LRUCache** cache = InitLRUCache(&cache_params);

  MemoryOperation memory_op;
  freopen(args.trace_file, "r", stdin);
  char* line = NULL;
  size_t line_cap = 0;
  ssize_t line_len = 0;
  while ( (line_len = getline(&line, &line_cap, stdin)) != -1 ) {
    if (line[0] != ' ') continue;
    sscanf(line+1, "%c %llx,%llu", &memory_op.op, &memory_op.address, &memory_op.size);
    // MemoryOperationToString(&memory_op);
    LRUCacheSimulate(&memory_op, cache, &cache_params, &stats, args.v);
  }
  DeallocateLRUCache(cache);
  printSummary(stats.hits, stats.misses, stats.evictions);
  return 0;
}
