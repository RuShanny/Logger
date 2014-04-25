#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h> 
#include "typedefs.h"
#include "ring_buf.h"

#define RING_BUFFER_SIZE 255
#define TMP_BUFFER_SIZE 100

extern volatile logger_created;
extern volatile logger_initialized;

typedef enum 
{
    LOG_DEBUG   = 0,   /* debug level messages */
    LOG_INFO    = 1,   /* informational */
    LOG_WARNING = 2,   /* warning conditions */
    LOG_ERROR   = 3    /* action must be taken immediately */
} log_severity_t;

typedef enum 
{
    DEBUG   = 1,
    RELEASE = 2
} program_state_t;

struct ring_buf_t;


struct logger_t
{
    program_state_t _state;
    log_severity_t  _level;
    FILE*           _log_file;
    bool            _logging_enabled;
    volatile bool   _stop;
    ring_buf_t*     _rbuf;
    pthread_t       _reading_thread;
    char*           (*appender)(char* input_str); /* you can append whatever you want to your resulting message */

};

logger_t* logger;

logger_t* init_logger (bool logging_enabled, program_state_t state, FILE* log_file, char* (*append_func)(char* input));
logger_t* get_instance(bool logging_enabled, program_state_t state, FILE* log_file, char* (*append_func)(char* input));

void deinit_logger(logger_t* logger);
void print_log(logger_t* logger, log_severity_t severity, const char* format, ...);

/* You can create any appender, this one is just an example */
char* append_human_time(char* input_str); 

#endif 