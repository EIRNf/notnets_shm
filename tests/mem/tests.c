
void test_success_print(const char *message) {
    printf("SUCCESS:%s\n", message);
}


void test_failed_print(const char *message) {
    printf("FAILED:%s\n", message);
}

void test_basic_queue_multiprocess(){
    int shmid = shm_create(1, 10, create_flag);
    int err =shm_remove(shmid);


    //CONDITION TO VERIFY
    if(err != 0){
        test_failed_print(__func__);
    }
    test_success_print(__func__);
}

void tests_run_all(void){
    test_basic_queue_multiprocess();

}