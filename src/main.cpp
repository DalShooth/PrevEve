/* Create by Wanek, for playing EVE on Linux, fu** you Windows */

#include "MainWindow.h"
#include <QtDBus/qdbusmetatype.h>
#include <QtDBus>
#include "PortalStreamInfo.h"
#include "StreamManager.h"

// Fichier log global
QFile logFile;

void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QString logMessage = QString("[%1] %2")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
        .arg(msg);

    // 1. Écrit dans le fichier
    if (logFile.isOpen()) {
        QTextStream ts(&logFile);
        ts << logMessage << Qt::endl;
    }

    // 2. Continue aussi d'afficher dans la console CLion
    fprintf(stderr, "%s\n", logMessage.toLocal8Bit().constData());
    fflush(stderr);
}

int main(int argc, char *argv[])
{
    logFile.setFileName("PrevEve.log");
    if (QFile::exists(logFile.fileName())) {
        QFile::remove(logFile.fileName());
    }
    logFile.open(QIODevice::WriteOnly | QIODevice::Text);

    qInstallMessageHandler(myMessageHandler);

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
