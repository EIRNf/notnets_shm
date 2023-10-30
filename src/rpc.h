#ifndef __COORD_H
#define __COORD_H

//Thread safe?
class notnets_user_agent {
    public:
        void* try_connect_shm(int key);

        int try_write(void* addr, void* message, int size);
        int try_read(void* addr, void** message);
    private:

};


//Thread safe?
class notnets_server {
    public:
        int coord_maintenance(int key);

        int try_write();
        int try_read();
    private:
};




#endif