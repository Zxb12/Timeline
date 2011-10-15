#ifndef SHARED_H
#define SHARED_H

#define VERSION "0.0.1a"
#define TIMELINE_PORT 50182

#define nbr(x) QString::number(x)

enum Error
{
    SERR_INVALID_VERSION = 0
};

enum OpCodeValues
{
    CMSG_HELLO = 0,
    SMSG_HELLO,
    SMSG_ERROR,
    SMSG_KICK
};

#define NB_OPCODES 0x04

#endif // SHARED_H
