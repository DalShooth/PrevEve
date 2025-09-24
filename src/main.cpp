#include "MainWindow.h"
#include <QtDBus/qdbusmetatype.h>
#include <QtDBus>
#include "PortalStreamInfo.h"
#include "StreamManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //= Enregistrement PortalStreamInfo
    qDBusRegisterMetaType<PortalStreamInfo>();
    qDBusRegisterMetaType<QList<PortalStreamInfo>>();
    //=

    MainWindow MainWindow; // Crée la fenêtre principale
    StreamManager::Instance().SetMainWindow(&MainWindow); // Crée StreamManager et Set sa ref à MainWindow (Singleton)
    MainWindow.show(); // Affiche MainWindow

    return app.exec();
}
