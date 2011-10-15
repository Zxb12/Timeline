#include "src/client/opcode_c.h"
#include "src/client/client_c.h"

namespace Client
{

OpCodeHandler OpCodeTable[NB_OPCODES] =
{
    {"CMSG_HELLO",                          NEVER,       &Client::handleClientSide},
    {"SMSG_HELLO",                          NOT_CHECKED, &Client::handleHello},
    {"SMSG_ERROR",                          NOT_CHECKED, &Client::handleError},
    {"SMSG_KICK",                           ALWAYS,      &Client::handleKick}
};

}
