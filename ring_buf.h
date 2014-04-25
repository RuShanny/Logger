#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "typedefs.h"
#include "log.h"


struct logger_t;


struct ring_buf_t
{
    char* data;
    int size;    /* length of ring buffer */
    volatile int idx_in;  /* points to a place where one can write to */
    volatile int idx_out; /* points to a place where one can read from*/

};


void rbuf_read(logger_t* logger);
void* thread_reads_from_buf(void* _logger);
bool rbuf_init(int size, logger_t* logger);
void rbuf_write(ring_buf_t* rbuf, char* data, int length);


#endif