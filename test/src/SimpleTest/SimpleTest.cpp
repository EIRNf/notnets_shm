
#include "gtest/gtest.h"

#include "mem.h"
#include "queue.h"

#include <iostream>

using namespace std;

class SimpleTest : public ::testing::Test
{
public:
    SimpleTest() {}
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
protected:

};

TEST_F(SimpleTest, Example)  
{
    key_t key = 1;
    int ipc_shmid = shm_create(key+1, sizeof(int), create_flag);
    EXPECT_EQ(ipc_shmid, 0);
}





