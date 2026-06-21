#include "hashmap.h"

static unsigned long hash_string(const char *str, size_t capacity) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash % capacity;
}

HashMap *hashmap_new(size_t capacity) {
  HashMap *map = malloc(sizeof(HashMap));
  if (!map)
    return NULL;

  map->capacity = capacity == 0 ? 16 : capacity;
  map->size = 0;
  map->buckets = calloc(map->capacity, sizeof(HMNode *));
  if (!map->buckets) {
    free(map);
    return NULL;
  }
  return map;
}

bool hashmap_insert(HashMap *map, const char *key, void *value) {
  if (!map || !key)
    return false;

  unsigned long index = hash_string(key, map->capacity);
  HMNode *curr = map->buckets[index];

  // Check for an update
  while (curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      curr->value = value;
      return true;
    }
    curr = curr->next;
  }

  // Allocate new node
  HMNode *new_node = malloc(sizeof(HMNode));
  if (!new_node)
    return false;

  new_node->key = mystrdup(key);
  if (!new_node->key) {
    free(new_node);
    return false;
  }
  new_node->value = value;
  new_node->next = map->buckets[index];
  map->buckets[index] = new_node;
  map->size++;

  return true;
}

void *hashmap_get(HashMap *map, const char *key) {
  if (!map || !key)
    return NULL;

  unsigned long index = hash_string(key, map->capacity);
  HMNode *curr = map->buckets[index];

  while (curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      return curr->value;
    }
    curr = curr->next;
  }
  return NULL; // Key not found
}

void hashmap_free(HashMap *map, ValueFreeFunc val_free) {
  if (!map)
    return;

  for (size_t i = 0; i < map->capacity; i++) {
    HMNode *curr = map->buckets[i];
    while (curr != NULL) {
      HMNode *temp = curr;
      curr = curr->next;

      free(temp->key); // Clean up hardcoded string key
      if (val_free) {
        val_free(temp->value); // Optional cleanup for the value pointer
      }
      free(temp);
    }
  }
  free(map->buckets);
  free(map);
}