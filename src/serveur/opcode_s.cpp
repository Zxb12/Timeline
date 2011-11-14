#include "src/serveur/opcode_s.h"

#include "src/serveur/serveur.h"

namespace Serveur
{

OpCodeHandler OpCodeTable[NB_OPCODES] =
{
    {"CMSG_HELLO",                          NOT_CHECKED, &Serveur::handleHello},
    {"SMSG_HELLO",                          NEVER,       &Serveur::handleServerSide},
    {"SMSG_ERROR",                          NEVER,       &Serveur::handleServerSide},
    {"SMSG_KICK",                           NEVER,       &Serveur::handleServerSide},
    {"CMSG_NEW_BACKUP",                     AUTHED,      &Serveur::handleNewBackup},
    {"CMSG_INITIATE_TRANSFER",              AUTHED,      &Serveur::handleInitiateTransfer},
    {"CMSG_FINISH_TRANSFER",                TRANSFER,    &Serveur::handleFinishTransfer},
    {"CMSG_CANCEL_TRANSFER",                TRANSFER,    &Serveur::handleCancelTransfer},
    {"CMSG_FILE_DATA",                      TRANSFER,    &Serveur::handleFileData},
    {"SMSG_WAITING_FOR_DATA",               NEVER,       &Serveur::handleServerSide},
    {"SMSG_TRANSFER_COMPLETE",              NEVER,       &Serveur::handleServerSide},
    {"CMSG_DELETE_FILE",                    AUTHED,      &Serveur::handleDeleteFile},
    {"SMSG_FILE_DELETED",                   NEVER,       &Serveur::handleServerSide},
    {"CMSG_FILE_LIST",                      AUTHED,      &Serveur::handleFileList},
    {"SMSG_FILE_LIST",                      NEVER,       &Serveur::handleServerSide}
};

}
