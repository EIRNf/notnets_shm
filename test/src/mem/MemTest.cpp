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

TEST_F(MemTest, CreateDelete)  
{
    key_t key = ftok("CreateDelete", 0);
    int shmid = shm_create(key, 10, create_flag);
    int err = shm_remove(shmid);
    EXPECT_FALSE(err);

}

