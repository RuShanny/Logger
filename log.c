#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <stdarg.h>  
#include <string.h>
#include <sys/types.h>
#include <pthread.h> 
#include <unistd.h>
#include <time.h>

#include "log.h"
#include "ring_buf.h"

volatile int logger_created = 0;
volatile int logger_initialized = 0;


logger_t* init_logger (bool logging_enabled, program_state_t state, FILE* log_file, char* (*append_func)(char* input)) 
{   
    logger_t* logger = (logger_t*)malloc(sizeof(logger_t));
    if (logger == NULL) {
        perror("Cannot create logger");
        return NULL;
    }
    logger->_logging_enabled = logging_enabled;

    /* If logging is disabled, it makes no sense to fill all fields */
    if (!logging_enabled) return;

    logger->_stop = false;
    logger->_state = state; 
    logger->appender = append_func;

    switch(state) {
        case DEBUG:   
            logger->_level = LOG_DEBUG; 
            break;
        case RELEASE: 
            logger->_level = LOG_WARNING; 
            break;
        default: printf("State is incorrect, default level = DEBUG\n"); 
                 logger->_level = LOG_DEBUG;
    }

    logger->_log_file = log_file;

    /* if we failed to create ring buffer, then we cannot use logger further, because ring buffer is its essential part */
    if (rbuf_init(RING_BUFFER_SIZE, logger) == false) {
        free(logger);
        logger = NULL;
    }

    return logger;
}

logger_t* get_instance(bool logging_enabled, program_state_t state, FILE* log_file, char* (*append_func)(char* input)){
    if (__sync_bool_compare_and_swap(&(logger_created), 0, 1)) {
        /* if logger is not created yet, do it */
        logger = init_logger(logging_enabled, state, log_file, append_func);
        logger_initialized = 1;
    } else { 
        /* logger is already created, but we should check if it is initialized. If not, we should wait its initialization */
        while (!logger_initialized) {}
    }
    
    return logger;
}

void deinit_logger(logger_t* logger) 
{   
    if (logger == NULL) return;
    
    /* we should notify our reading thread that it is time to end up*/
    logger->_stop = true; 
    /* we're waiting our reading thread to finish in order to prevent its force "killing" */
    pthread_join(logger->_reading_thread, NULL);


    free(logger->_rbuf->data);
    free(logger->_rbuf);
    free(logger);
}

void print_log(logger_t* logger, log_severity_t severity, const char* format, ...) 
{
    char tmp_buf[TMP_BUFFER_SIZE];
    char* res_buf;

    int length;

    if (logger == NULL) {
        return;
    }

    /* if logger is disabled, no sense to fill other fields */
    if (!logger->_logging_enabled) {
        printf("Logging is disabled\n");
        return;
    }

    /* if severity of log_msg is lower than defined level, it should be discarded */
    if (severity < logger->_level) {
        return;
    }

    va_list args;
    va_start(args, format);
    vsprintf (tmp_buf, format, args);
    /* in case user does not neet appender function, we write unmodified message to ring buffer */
    if (logger->appender != NULL) res_buf = logger->appender(tmp_buf);
    else res_buf = tmp_buf;
    va_end(args);

    length = strlen(res_buf);
    rbuf_write(logger->_rbuf, res_buf, length);

}


char* append_human_time(char* input_str) {
    char tmp_buf[TMP_BUFFER_SIZE];

    time_t curr_time = time(NULL);
    struct tm *local_time = localtime(&curr_time);
    char* realtime = asctime(local_time);

    /* we don't want the nextline which is caused by '\n' */
    realtime[strlen(realtime) - 1] = ' ';

    /* concatenate real time with input string*/
    snprintf(tmp_buf, TMP_BUFFER_SIZE, "[ %s] %s", realtime, input_str); 
    strncpy(input_str,  tmp_buf, TMP_BUFFER_SIZE);

    return input_str;
}
