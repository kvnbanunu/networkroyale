#include "../include/test_msg.h"
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

void convert_to_stream(test_message *msg, uint8_t stream[])
{
    uint32_t net_course = htonl(msg->course);
    memcpy(&stream[0], &net_course, sizeof(net_course));
    memcpy(&stream[sizeof(net_course)], msg->msg, sizeof(msg->msg)/sizeof(uint8_t));
}

void convert_from_stream(test_message *msg, uint8_t stream[])
{
    uint32_t net_course;
    uint32_t home_course;
    memcpy(&net_course, &stream[0], sizeof(uint32_t));
    home_course = ntohl(net_course);
    memcpy(&msg->course, &home_course, sizeof(uint32_t));
    memcpy(&msg->msg, &stream[sizeof(uint32_t)], sizeof(test_message) - sizeof(uint32_t));
}
