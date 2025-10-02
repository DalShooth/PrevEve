/* Create by Wanek, for multibox EVE on Linux, fu** you Windows */

#include "MainWindow.h"

#include <qdatetime.h>
#include <QStandardPaths>
#include "ConfigManager.h"
#include "KWinManager.h"
#include "StreamManager.h"

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
    qInfo() << "installDesktopFile()";

    const QString appsDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    const QString desktopDestFile = appsDir + "/eve-w-preview.desktop";
    if (QFile::exists(desktopDestFile)) {
        return;
    }
    QFile src(":/scripts/eve-w-preview.desktop");
    if (!src.exists()) {
        return;
    }
    if (!src.copy(desktopDestFile)) {
        qWarning() << "[installDesktopFile] Failed to copy to:" << desktopDestFile;
    }
}

void installIcon()
{
    qInfo() << "installIcon()";

    const QString iconDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/icons/";
    const QString destFile = iconDir + "EveWPreview.png";

    if (QFile::exists(destFile)) {
        return;
    }

    if (QFile src(":/assets/EveWPreview.png"); !src.copy(destFile)) {
        qWarning() << "[installIcon] Failed to copy to:" << destFile;
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

    // Options de lancement
    qputenv("QT_QPA_PLATFORM", "xcb");
    qputenv("QSG_RHI_BACKEND", "opengl");
    qputenv("QSG_RENDER_LOOP", "basic");
    qputenv("QT_XCB_NO_XI2", "1");

    installIcon(); // Copie l'icone dans .local/share/icons
    installDesktopFile(); // Copie le .desktop dans .local/share/applications

    QApplication app(argc, argv);

    // Init
    ConfigManager::Instance();
    KWinManager::Instance();
    MainWindow MainWindow;
    StreamManager::GetInstance().init(&MainWindow);
    MainWindow.show(); // Affiche MainWindow

    return QApplication::exec();
}
