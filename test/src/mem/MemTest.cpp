#include "gtest/gtest.h"

#include "mem.h"

using namespace std;

class MemTest : public ::testing::Test
{
public:
    MemTest() {}
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
protected:

};

TEST_F(MemTest, CreateDeleteSystemV)  
{
    key_t key = ftok("CreateDelete", 0);
    int shmid = shm_create(key, 10, create_flag);
    int err = shm_remove(shmid);
    EXPECT_FALSE(err);

}

TEST_F(MemTest, CreateDeleteMMAP)  
{
    int fd = mmap_create("CreateDeleteMMAP", 10, mmap_create_flag);
    // void* addr = mmap_attach(fd, size, mmap_attach_flag);

    int err = mmap_remove("CreateDeleteMMAP");
    EXPECT_FALSE(err);

}

