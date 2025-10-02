/* Create by Wanek, for multibox EVE on Linux, fu** you Windows */

#include "EveWPreviewWindow.h"

#include <qdatetime.h>
#include <QFile>
#include <QStandardPaths>
#include "KWinManager.h"
#include "StreamManager.h"

QFile logFile;

int main(int argc, char *argv[])
{
    // Logs in file
    logFile.setFileName("PrevEve.log");
    if (QFile::exists(logFile.fileName())) {
        QFile::remove(logFile.fileName());
    }
    logFile.open(QIODevice::WriteOnly | QIODevice::Text);

    // Options de lancement
    qputenv("QT_QPA_PLATFORM", "xcb");
    qputenv("QSG_RHI_BACKEND", "opengl");
    qputenv("QSG_RENDER_LOOP", "basic");
    qputenv("QT_XCB_NO_XI2", "1");

    QApplication app(argc, argv);
    QApplication::setWindowIcon(QIcon(":/assets/EveWPreview.png"));

    EveWPreviewWindow* eveWPreviewWindow = new EveWPreviewWindow();
    eveWPreviewWindow->show(); // Affiche MainWindow

    return QApplication::exec();
}
