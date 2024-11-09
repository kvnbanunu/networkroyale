#ifndef TEST_MSG_H
#define TEST_MSG_H

#include <stdint.h>

typedef struct Test_Message {
    uint32_t course;
    uint8_t msg[5];
} test_message;

void convert_to_stream(test_message *msg, uint8_t stream[]);
void convert_from_stream(test_message *msg, uint8_t stream[]);

#endif // TEST_MSG_H
