#ifndef HASHMAP_H
#define HASHMAP_H

#include "vm.h"

typedef void (*ValueFreeFunc)(void *ptr);

typedef struct HMNode {
  char *key;   // Hardcoded to string
  void *value; // Generic value payload
  struct HMNode *next;
} HMNode;

typedef struct {
  HMNode **buckets;
  size_t capacity;
  size_t size;
} HashMap;

char *mystrdup(
    const char *s); // wait — this is already declared in vm.h, see note below
HashMap *hashmap_new(size_t capacity);
void *hashmap_get(HashMap *map, const char *key);

// hashmap.h
typedef void (*ValueFreeFunc)(void *);

bool hashmap_insert(HashMap *map, const char *key, void *value);
void hashmap_free(HashMap *map, ValueFreeFunc val_free);

#endif