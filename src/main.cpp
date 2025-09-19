#include <QApplication>
#include "MainWindow.h"
#include <QtDBus/qdbusmetatype.h>
#include <QtDBus>
#include "StreamInfo.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Enregistrement DBus
    qDBusRegisterMetaType<StreamInfo>();
    qDBusRegisterMetaType<QList<StreamInfo>>();

    // IMPORTANT côté KDE/Wayland : expose un DesktopFileName cohérent avec ton AppID/.desktop
    QGuiApplication::setDesktopFileName(QStringLiteral("PrevEve"));

    // (Optionnel mais utile) vérifier l'AppID passé par l'env
    const QByteArray appId = qgetenv("XDG_DESKTOP_PORTAL_APPLICATION_ID");
    qInfo() << "XDG_DESKTOP_PORTAL_APPLICATION_ID =" << (appId.isEmpty() ? "<non défini>" : appId);

    // Afficher une fenêtre pour que le portail puisse associer la requête à ton appli
    MainWindow w;
    w.setWindowTitle(QStringLiteral("PrevEve"));
    w.resize(400, 200);
    w.show();

    return app.exec();
}
