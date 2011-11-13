#-------------------------------------------------
#
# Project created by QtCreator 2011-10-12T19:24:31
#
#-------------------------------------------------

QT += core network
TARGET = Timeline
TEMPLATE = app

QT += gui

SOURCES  += src/main.cpp \
    src/client/opcode_c.cpp \
    src/client/client_c.cpp \
    src/serveur/serveur.cpp \
    src/serveur/opcode_s.cpp \
    src/serveur/client_s.cpp \
    src/shared/paquet.cpp \
    src/ui/fenprincipale.cpp \
    src/serveur/cache.cpp \
    src/serveur/defines.cpp

HEADERS  += src/client/opcode_c.h \
    src/client/client_c.h \
    src/serveur/serveur.h \
    src/serveur/opcode_s.h \
    src/serveur/client_s.h \
    src/shared/shared.h \
    src/shared/paquet.h \
    src/ui/fenprincipale.h \
    src/serveur/cache.h \
    src/serveur/defines.h

FORMS    += src/ui/fenprincipale.ui




