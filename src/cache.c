#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include "cache.h"

/**
 * Allocate a cache entry
 */
struct cache_entry *alloc_entry(char *path, char *content_type, void *content, int content_length)
{
    struct cache_entry *new_cache_entry = malloc(sizeof(struct cache_entry));

    new_cache_entry->path = strdup(path);
    new_cache_entry->content_type = strdup(content_type);
    new_cache_entry->content = malloc(content_length);
    memcpy(new_cache_entry->content, content, content_length);
    new_cache_entry->content_length = content_length;

    new_cache_entry->prev = NULL;
    new_cache_entry->next = NULL;

    return new_cache_entry;
}

/**
 * Deallocate a cache entry
 */
void free_entry(struct cache_entry *entry)
{
    free(entry->path);
    free(entry->content_type);
    free(entry->content);
    free(entry);
}

/**
 * Insert a cache entry at the head of the linked list
 */
void dllist_insert_head(struct cache *cache, struct cache_entry *ce)
{
    // Insert at the head of the list
    if (cache->head == NULL)
    {
        cache->head = cache->tail = ce;
        ce->prev = ce->next = NULL;
    }
    else
    {
        cache->head->prev = ce;
        ce->next = cache->head;
        ce->prev = NULL;
        cache->head = ce;
    }
    cache->cur_size++;
}

/**
 * Move a cache entry to the head of the list
 */
void dllist_move_to_head(struct cache *cache, struct cache_entry *ce)
{
    if (ce != cache->head)
    {
        if (ce == cache->tail)
        {
            // We're the tail
            cache->tail = ce->prev;
            cache->tail->next = NULL;
        }
        else
        {
            // We're neither the head nor the tail
            ce->prev->next = ce->next;
            ce->next->prev = ce->prev;
        }

        ce->next = cache->head;
        cache->head->prev = ce;
        ce->prev = NULL;
        cache->head = ce;
    }
}

/**
 * Removes the tail from the list and returns it
 * 
 * NOTE: does not deallocate the tail
 */
struct cache_entry *dllist_remove_tail(struct cache *cache)
{
    struct cache_entry *oldtail = cache->tail;

    cache->tail = oldtail->prev;
    cache->tail->next = NULL;

    cache->cur_size--;

    return oldtail;
}

/**
 * Create a new cache
 * 
 * max_size: maximum number of entries in the cache
 * hashsize: hashtable size (0 for default)
 */
struct cache *cache_create(int max_size, int hashsize)
{
    struct cache *new_cache = malloc(sizeof(struct cache));

    new_cache->index = hashtable_create(max_size, NULL);
    new_cache->head = NULL;
    new_cache->tail = NULL;
    new_cache->max_size = max_size;
    new_cache->cur_size = 0;

    return new_cache;
}

void cache_free(struct cache *cache)
{
    struct cache_entry *cur_entry = cache->head;

    hashtable_destroy(cache->index);

    while (cur_entry != NULL)
    {
        struct cache_entry *next_entry = cur_entry->next;

        free_entry(cur_entry);

        cur_entry = next_entry;
    }

    free(cache);
}

/**
 * Store an entry in the cache
 *
 * This will also remove the least-recently-used items as necessary.
 * 
 * NOTE: doesn't check for duplicate cache entries
 */
void cache_put(struct cache *cache, char *path, char *content_type, void *content, int content_length)
{
    // allocate a new cache entry with the passed parameters
    struct cache_entry *new_cache_entry = alloc_entry(path, content_type, content, content_length);
    // insert the entry at the head of the doubly-linked list
    dllist_insert_head(cache, new_cache_entry);
    // store the entry in the hashtable indexed by the entry's path
    hashtable_put(cache->index, new_cache_entry->path, new_cache_entry);

    if (cache->cur_size > cache->max_size)
    {
        // remove cache entry at the tail of the linked list
        struct cache_entry *old_tail = dllist_remove_tail(cache);
        // remove the same entry from the hashtable
        hashtable_delete(cache->index, old_tail->path);
        // free the cache entry
        free_entry(old_tail);
        // ensure the size counter for the number of entries in the cache is correct
        cache->cur_size--;
    }
}

/**
 * Retrieve an entry from the cache
 */
struct cache_entry *cache_get(struct cache *cache, char *path)
{
    // attempt to find the cache entry pointer by path in the hashtable
    struct cache_entry *entry = hashtable_get(cache->index, path);
    // if not found, return null
    if (entry == NULL)
    {
        return NULL;
    }
    // move the cache entry to the head of the dll
    dllist_move_to_head(cache, entry);
    // return the cache entry pointer
    return entry;
}
