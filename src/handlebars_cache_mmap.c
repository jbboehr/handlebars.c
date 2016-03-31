
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_cache.h"
#include "handlebars_map.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_opcode_serializer.h"


#define HANDLE_FD_ERROR(fd) if( fd == -1 ) { handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Failed to open shm: %s", strerror(errno)); }
#define DEFAULT_SHM_SIZE 1024 * 1024 * 100
#define DEFAULT_TABLE_SIZE 1024 * 1024 * 10

const char head[] = "handlebars shared opcode cache";

enum handlebars_cache_mmap_table_entry_state {
    STATE_EMPTY = 0,
    STATE_INIT = 1,
    STATE_READY = 2
};

struct cache_header {
    //! Header
    char head[32];

    //! The size of the memory block
    size_t size;

    //! The version of handlebars this block was initialized with
    int version;

    //! The offset in bytes from the beginning of the block at which the hash table exists
    size_t table_offset;

    //! The size in bytes of the hash table
    size_t table_size;

    //! The number of slots in the hash table
    size_t table_count;

    //! The offset in bytes from the beginning of the block at which the data segment exists
    size_t data_offset;

    //! The size in bytes of the data segment
    size_t data_size;

    //! The length in bytes of the currently used part of the data segment
    size_t data_length;
};

struct table_entry {
    //! The state of the entry
    int state;

    //! The offset from the beginning of the memory block at which the data for this entry resides
    size_t data_offset;
};

struct handlebars_cache_mmap {
    //! The file descriptor for the memory block - used for flock
    int fd;

    //! The mmap'd memory address
    void * addr;

    //! Information about the cache
    struct cache_header * h;
};



#undef CONTEXT
#define CONTEXT HBSCTX(cache)

static void cache_dtor(struct handlebars_cache * cache)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    if( intern->addr ) {
        munmap(intern->addr, DEFAULT_SHM_SIZE);
    }
    if( intern->fd > -1 ) {
        close(intern->fd);
    }
}

static int cache_gc(struct handlebars_cache * cache)
{
    return 0;
}


static struct handlebars_module * cache_find(struct handlebars_cache * cache, struct handlebars_string * tmpl)
{
    return NULL;
}

static void cache_add(
        struct handlebars_cache * cache,
        struct handlebars_string * tmpl,
        struct handlebars_module * module
) {

}

#undef CONTEXT
#define CONTEXT context

struct handlebars_cache * handlebars_cache_mmap_ctor(
    struct handlebars_context * context,
    const char * name
) {
    struct handlebars_cache * cache = MC(handlebars_talloc_zero(context, struct handlebars_cache));
    cache->add = &cache_add;
    cache->find = &cache_find;
    cache->gc = &cache_gc;

    struct handlebars_cache_mmap * intern = MC(handlebars_talloc_zero(cache, struct handlebars_cache_mmap));
    cache->internal = intern;

    talloc_set_destructor(cache, cache_dtor);

    bool create = false;
    intern->fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0644);
    if( errno == EEXIST ) {
        // Use existing
        intern->fd = shm_open(name, O_RDWR, 0644);
        HANDLE_FD_ERROR(intern->fd);
    } else {
        // Create
        HANDLE_FD_ERROR(intern->fd);
        ftruncate(intern->fd, DEFAULT_SHM_SIZE);
        create = true;
    }

    intern->addr = mmap(NULL, DEFAULT_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, intern->fd, 0);
    intern->h = intern->addr;

    if( create ) {
        memcpy(intern->h->head, head, sizeof(head));
        intern->h->size = DEFAULT_SHM_SIZE;
        intern->h->version = handlebars_version();
        intern->h->table_offset = sizeof(struct cache_header);
        intern->h->table_size = DEFAULT_TABLE_SIZE;
        intern->h->data_offset = intern->h->table_offset + intern->h->table_size;
        intern->h->data_size = intern->h->size - intern->h->data_offset;
        intern->h->table_count = (intern->h->data_size / sizeof(struct table_entry));
    }

    return cache;
}
