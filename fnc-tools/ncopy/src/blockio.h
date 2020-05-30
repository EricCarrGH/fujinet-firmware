/**
 * Simple CIO GETCHR/PUTCHR wrappers
 */

#ifndef BLOCKIO_H
#define BLOCKIO_H

void open(char* buf, unsigned short len);
void close(void);
void get(char* buf, unsigned short len);
void put(char* buf, unsigned short len);

#endif /* BLOCKIO_H */
