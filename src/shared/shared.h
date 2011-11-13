#ifndef SHARED_H
#define SHARED_H

#define VERSION "0.0.1a"
#define TIMELINE_PORT 50182

#define nbr(x) QString::number(x)

enum Error
{
    SERR_INVALID_VERSION = 0,
    SERR_COULDNT_CREATE_FILE,
    SERR_TRANSFER_IN_PROGESS
};

enum OpCodeValues
{
    CMSG_HELLO = 0,
    SMSG_HELLO,
    SMSG_ERROR,
    SMSG_KICK,
    CMSG_NEW_BACKUP,
    CMSG_INITIATE_TRANSFER,
    CMSG_FINISH_TRANSFER,
    CMSG_CANCEL_TRANSFER,
    CMSG_FILE_DATA,
    SMSG_WAITING_FOR_DATA,
    SMSG_TRANSFER_COMPLETE,
    CMSG_DELETE_FILE,
    SMSG_FILE_DELETED
};

#define NB_OPCODES 13

#endif // SHARED_H
