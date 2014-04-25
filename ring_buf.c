#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>

#include "ring_buf.h"

void rbuf_read(logger_t* logger) 
{
    int part_length, idx_in_local;

    /* we save idx_in locally, because real idx_in can be changed while reading from buf, so reading will become inconsistent */
    idx_in_local = logger->_rbuf->idx_in;

    /* if idx_in > idx_out, then we can read the message at a time without dividing this process by 2 parts */
    if ( idx_in_local > (logger->_rbuf->idx_out) ) 
        while ( (logger->_rbuf->idx_out) < idx_in_local ) {
            fprintf(logger->_log_file, "%c", logger->_rbuf->data[logger->_rbuf->idx_out]);
            logger->_rbuf->idx_out++;
        }
    /* if idx_in > idx_out, then the written message is divided in two parts */ 
    if ( idx_in_local < (logger->_rbuf->idx_out) ) {
         /* we read the first part of message till the end of ring buffer */
        while ( (logger->_rbuf->idx_out) < (logger->_rbuf->size) ) {
            fprintf(logger->_log_file, "%c", logger->_rbuf->data[logger->_rbuf->idx_out]);
            logger->_rbuf->idx_out++;
        }
        /* we read the rest of the message from the beginning of ring buffer*/
        logger->_rbuf->idx_out = 0;
        while ( (logger->_rbuf->idx_out) < idx_in_local ) {
            fprintf(logger->_log_file, "%c", logger->_rbuf->data[logger->_rbuf->idx_out]);
            logger->_rbuf->idx_out++;
        }

    } 
        
}

void* thread_reads_from_buf(void* _logger) 
{
    logger_t* logger = (logger_t*)_logger;
    unsigned int usecs = 1;
    

    while(1) {
        rbuf_read(logger);
        if (logger->_rbuf->idx_in == logger->_rbuf->idx_out) {
            /* we're waiting for writing threads to fill ring buffer */
            usleep(usecs);
            /* if "stop" field was set to "true", then we should finish reading from ring buf and exit */
            if (logger->_stop) {
                if (fflush(logger->_log_file)) perror("Failed to flush buffer in reading thread");
                fclose(logger->_log_file);
                pthread_exit((void *)0);
            }
        } 
    }

}


bool rbuf_init(int size, logger_t* logger) 
{
    logger->_rbuf = (ring_buf_t*)calloc(1, sizeof(ring_buf_t));
    if (logger->_rbuf == NULL) {
        printf("Failed to allocate memory for ring buffer's structure\n");
        return false;
    }

    logger->_rbuf->size = size;
    logger->_rbuf->data = (char*)calloc(size, sizeof(char));

    if (logger->_rbuf->data == NULL) {
        printf("Failed to allocate memory for ring buffer of size %d bytes\n", size);
        return false;
    }
    logger->_rbuf->idx_out = logger->_rbuf->idx_in = 0;

    /* Create a supplementary thread which will read messages from ring buffer and print them */
    pthread_create(&logger->_reading_thread, NULL, thread_reads_from_buf, logger);
    return true;
}

/* 
    we don't check if there is available space in ring buffer to write data, 
    because reading thread is supposed to read faster than other threads write to ring buffer.
*/
void rbuf_write(ring_buf_t* rbuf, char* data, int length) 
{
    int idx_in_old, idx_in_new, part_length, tmp_idx_in;

    while(1) {
        idx_in_old = rbuf->idx_in;
        idx_in_new = (idx_in_old + length)%rbuf->size;
        if (__sync_bool_compare_and_swap(&(rbuf->idx_in), idx_in_old, idx_in_new)) 
        {
            break;
        } 
    }

    /* in case if the word should be divided into 2 parts to be written to ring buf */
    if (idx_in_new - idx_in_old < 0) {
        part_length = rbuf->size - idx_in_old;
        /* we write the first part of message until we reach the end of ring buffer */
        memcpy(&rbuf->data[idx_in_old], data, part_length);
        /* we write the second part of message from the beginning of ring buffer */
        memcpy(&rbuf->data[0], &data[part_length], length - part_length);

    } else /* if not, then we can write the whole message at a time */
        memcpy(&rbuf->data[idx_in_old], data, length);

}


