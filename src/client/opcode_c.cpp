#include "src/client/opcode_c.h"
#include "src/client/client_c.h"

namespace Client
{

OpCodeHandler OpCodeTable[NB_OPCODES] =
{
    {"CMSG_HELLO",                          NEVER,       &Client::handleClientSide},
    {"SMSG_HELLO",                          NOT_CHECKED, &Client::handleHello},
    {"SMSG_ERROR",                          ALWAYS,      &Client::handleError},
    {"SMSG_KICK",                           ALWAYS,      &Client::handleKick},
    {"CMSG_NEW_BACKUP",                     NEVER,       &Client::handleClientSide},
    {"CMSG_INITIATE_TRANSFER",              NEVER,       &Client::handleClientSide},
    {"CMSG_FINISH_TRANSFER",                NEVER,       &Client::handleClientSide},
    {"CMSG_CANCEL_TRANSFER",                NEVER,       &Client::handleClientSide},
    {"CMSG_FILE_DATA",                      NEVER,       &Client::handleClientSide},
    {"SMSG_WAITING_FOR_DATA",               AUTHED,      &Client::handleWaitingForData},
    {"SMSG_TRANSFER_COMPLETE",              TRANSFER,    &Client::handleTransferComplete},
    {"CMSG_DELETE_FILE",                    NEVER,       &Client::handleClientSide},
    {"SMSG_FILE_DELETED",                   AUTHED,      &Client::handleFileDeleted},
    {"CMSG_FILE_LIST",                      NEVER,       &Client::handleClientSide},
    {"SMSG_FILE_LIST",                      AUTHED,      &Client::handleFileList},
    {"CMSG_RECOVER_FILE",                   NEVER,       &Client::handleClientSide},
    {"SMSG_FINISH_TRANSFER",                TRANSFER,    &Client::handleFinishTransfer},
    {"SMSG_CANCEL_TRANSFER",                TRANSFER,    &Client::handleCancelTransfer},
    {"SMSG_FILE_DATA",                      TRANSFER,    &Client::handleFileData}
};

}
