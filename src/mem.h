#ifndef __MEM_H
#define __MEM_H


//Includes


//Definitions

class shared_memory_region {
    public:
        shared_memory_region();
        void create();
        void attach();
        void remove();
        void detach();

    private:

};




void HashShmKeys(char* fullServiceName);


void* InitializeAndAttachShmRegion();
void* InitializeShmRegion();
void* AttachShmRegion();
void RemoveShmRegion();
void Detach();


#endif
