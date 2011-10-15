#include "src/serveur/opcode_s.h"

#include "src/serveur/serveur.h"

namespace Serveur
{

OpCodeHandler OpCodeTable[NB_OPCODES] =
{
    {"CMSG_HELLO",                          NOT_CHECKED, &Serveur::handleHello},
    {"SMSG_HELLO",                          NEVER,       &Serveur::handleServerSide},
    {"SMSG_ERROR",                          ALWAYS,      &Serveur::handleServerSide},
    {"SMSG_KICK",                           ALWAYS,      &Serveur::handleServerSide}
};

}
