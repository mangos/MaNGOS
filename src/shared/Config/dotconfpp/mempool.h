#ifndef ASYNC_DNS_MEMPOOL_H
#define ASYNC_DNS_MEMPOOL_H

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

class AsyncDNSMemPool
{
    private:

        struct PoolChunk
        {
            void* pool;
            size_t pos;
            size_t size;

            PoolChunk(size_t _size);
            ~PoolChunk();
        };

        PoolChunk** chunks;
        size_t chunksCount;
        size_t defaultSize;

        size_t poolUsage;
        size_t poolUsageCounter;

        void addNewChunk(size_t size);

    public:

        AsyncDNSMemPool(size_t _defaultSize = 4096);
        virtual ~AsyncDNSMemPool();

        bool initialize();
        void Free();
        void* Alloc(size_t size);
        void* Calloc(size_t size);
        char* Strdup(const char *str);
};

#endif
