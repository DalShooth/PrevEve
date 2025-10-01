/* Create by Wanek, for playing EVE on Linux, fu** you Windows */

#include <qdatetime.h>
#include <qdir.h>
#include <QFile>
#include <QStandardPaths>
#include "MainWindow.h"
#include <QtDBus/qdbusmetatype.h>

#include "ConfigManager.h"
#include "KWinManager.h"
#include "PortalStreamInfo.h"
#include "StreamManager.h"

// Fichier log global
QFile logFile;

void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    const QString logMessage = QString("[%1] %2")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), msg);

    // 1. Écrit dans le fichier
    if (logFile.isOpen()) {
        QTextStream ts(&logFile);
        ts << logMessage << Qt::endl;
    }

    // 2. Continue aussi d'afficher dans la console CLion
    fprintf(stderr, "%s\n", logMessage.toLocal8Bit().constData());
    fflush(stderr);
}

void installDesktopFile()
{
    const QString appsDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    const QString desktopDestFile = appsDir + "/eve-w-preview.desktop";
    if (QFile::exists(desktopDestFile)) {
        qInfo() << "[DesktopFile] Already exists:" << desktopDestFile;
        return;
    }
    QFile src(":/scripts/eve-w-preview.desktop");
    if (!src.exists()) {
        qWarning() << "[DesktopFile] Resource not found!";
        return;
    }
    if (!src.copy(desktopDestFile)) {
        qWarning() << "[DesktopFile] Failed to copy to:" << desktopDestFile;
    } else {
        qInfo() << "[DesktopFile] Installed to:" << desktopDestFile;
    }
}

void installIcon()
{
    const QString iconDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                      + "/icons/";
    const QString destFile = iconDir + "EveWPreview.png";

    if (QFile::exists(destFile)) {
        qDebug() << "[Icon] Already exists:" << destFile;
        return;
    }

    // Ton PNG embarqué dans .qrc
    QFile src(":/assets/EveWPreview.png");
    if (!src.exists()) {
        qWarning() << "[Icon] Resource not found!";
        return;
    }

    if (!src.copy(destFile)) {
        qWarning() << "[Icon] Failed to copy to:" << destFile;
    } else {
        qDebug() << "[Icon] Installed to:" << destFile;
    }
}

int main(int argc, char *argv[])
{
    // Logs in file
    logFile.setFileName("PrevEve.log");
    if (QFile::exists(logFile.fileName())) {
        QFile::remove(logFile.fileName());
    }
    logFile.open(QIODevice::WriteOnly | QIODevice::Text);
    qInstallMessageHandler(myMessageHandler);

    QApplication app(argc, argv);

    installIcon(); // Copie l'icone dans .local/share/icons
    installDesktopFile(); // Copie le .desktop dans .local/share/applications

    //= Enregistrement PortalStreamInfo // todo -> voir s'il est possible de s'en débarrassé
    qDBusRegisterMetaType<PortalStreamInfo>();
    qDBusRegisterMetaType<QList<PortalStreamInfo>>();
    //=

    // Init Singleton
    ConfigManager::Instance();
    KWinManager::Instance();

    MainWindow MainWindow; // Crée la fenêtre principale
    StreamManager::Instance().SetMainWindow(&MainWindow); // Crée StreamManager et Set sa ref à MainWindow (Singleton)
    MainWindow.show(); // Affiche MainWindow

    return QApplication::exec();
}
