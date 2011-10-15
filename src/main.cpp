#include <QtGui/QApplication>
#include "src/ui/fenprincipale.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FenPrincipale w;
    w.show();

    return a.exec();
}
