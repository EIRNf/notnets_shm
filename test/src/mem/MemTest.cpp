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
    int shmid = shm_create(1, 10, create_flag);
    int err = shm_remove(shmid);
    EXPECT_FALSE(err);

}

