#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "log.h"

void* thread_test(void* log_file) {
    int i;
    int size = 3;
    logger_t* logger = get_instance(true, DEBUG, (FILE*)log_file, append_human_time);
    unsigned int usecs = 1;


    for (i = 0; i < 3; i++) {
        print_log(logger, LOG_DEBUG, "I'm thread with tid = %d, iteration = %d\n", pthread_self()%10, i);
        usleep(usecs);
    }

    pthread_exit((void *)0);

}


int main() 
{
    int i;
    int num_of_threads = 3;
    pthread_t thread[num_of_threads];
  
    FILE* log_file = fopen("log_file.txt", "w");
    //FILE* log_file = stdout;
    if (log_file == NULL)
    {
        perror ("fopen(log_file)");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < num_of_threads; i++) 
        pthread_create(&thread[i], NULL, thread_test, log_file);
      

    for (i = 0; i < num_of_threads; i++) 
        pthread_join(thread[i], NULL);

    deinit_logger(logger);

    return 0;
}