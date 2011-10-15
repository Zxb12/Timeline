#ifndef OPCODE_H
#define OPCODE_H

#include "src/shared/shared.h"

#include <QByteArray>

class Paquet;

namespace Serveur
{

class Serveur;
class Client;

enum SessionState
{
    NOT_CHECKED = 0x1,
    CHECKED     = 0x2,
    ALWAYS      = NOT_CHECKED | CHECKED,
    AUTHED      = CHECKED | 0x4,
    TRANSFER    = AUTHED | 0x8,
    NEVER       = 0x10
};

struct OpCodeHandler
{
    const QByteArray nom;
    SessionState state;
    void (Serveur::*f)(Paquet *, Client *);
};

//On ajoute le mot-clé extern pour que la table puisse être utilisée ailleurs.
extern OpCodeHandler OpCodeTable[NB_OPCODES];
}

#endif // OPCODE_H
