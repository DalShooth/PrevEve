#include "KWinManager.h"

#include <qdbusreply.h>
#include <qdir.h>
#include <qproperty.h>


KWinManager::KWinManager(QObject *parent) : QObject(parent)
{
    qInfo() << "KWinManager::KWinManager()";

    QDBusConnection::sessionBus().registerService("org.example.EveWPreview");
    QDBusConnection::sessionBus().registerObject("/EveWPreview", this,
                                                 QDBusConnection::ExportAllSlots);
}

void KWinManager::GetThumbnailsPositions() {
    qInfo() << "GetThumbnailsPositions()";

    QTemporaryFile script(QDir::tempPath() + "/EveWPreview_XXXXXX.js");
    script.setAutoRemove(true);
    if (!script.open()) {
        qCritical() << "[GetThumbnailsPositions] -> Cannot open temporaryFile";
        return;
    }

    QFile file(":/scripts/KWin_GetThumbnailsPositions.js");
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "[GetThumbnailsPositions] -> Impossible d’ouvrir le script embarqué";
        return;
    }
    const QString scriptRaw = QString::fromUtf8(file.readAll());
    //qInfo() << script.toUtf8().constData(); // <- DEBUG for print script
    script.write(scriptRaw.toUtf8());
    script.flush();
    script.close();

    // Interface pour load le script
    QDBusInterface scriptingInterface(
        "org.kde.KWin",
        "/Scripting",
        "org.kde.kwin.Scripting",
        QDBusConnection::sessionBus());
    if (!scriptingInterface.isValid()) {
        qCritical() << "[GetThumbnailsPositions] -> DBusInterface invalid";
        return;
    }

    // chargé le script via scriptingInterface
    QDBusReply<int> reply = scriptingInterface.call("loadScript", script.fileName());
    if (!reply.isValid()) {
        qCritical() << "[GetThumbnailsPositions] -> Erreur loadScript:" << reply.error().message();
        return;
    }

    const int scriptId = reply.value(); // Id du script
    const QString scriptPath = "/Scripting/Script" + QString::number(scriptId); // Path du script

    qInfo() << "[GetThumbnailsPositions] -> Script loaded, scriptId:" << scriptId << " | path:" << scriptPath;

    // Interface vers le script chargé
    QDBusInterface DBusScriptInterface(
        "org.kde.KWin",
        scriptPath,
        "org.kde.kwin.Script",
        QDBusConnection::sessionBus()
    );

    // Lancer le script
    if (QDBusReply<void> runReply = DBusScriptInterface.call("run"); !runReply.isValid()) {
        qCritical() << "[GetThumbnailsPositions] -> Error run:" << runReply.error().message();
        return;
    }

    qInfo() << "[GetThumbnailsPositions] -> Script used";
}

void KWinManager::setFocusedClient(const QString &clientToFocused)
{
    qInfo() << "setFocusedWindow()";

    QTemporaryFile script(QDir::tempPath() + "/EveWPreview_XXXXXX.js");
    script.setAutoRemove(true);
    if (!script.open()) {
        qCritical() << "[setFocusedWindow] -> Cannot open temporaryFile";
        return;
    }

    QFile file(":/scripts/KWin_SetFocusedClient.js");
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "[setFocusedWindow] -> Impossible d’ouvrir le script embarqué";
        return;
    }
    QString caption = "EVE - " + clientToFocused;
    const QString scriptRaw = QString::fromUtf8(file.readAll()).arg(caption);
    //qInfo() << scriptRaw.toUtf8().constData();; // <- DEBUG for print script
    script.write(scriptRaw.toUtf8());
    script.flush();
    script.close();

    // Interface pour load le script
    QDBusInterface scriptingInterface(
        "org.kde.KWin",
        "/Scripting",
        "org.kde.kwin.Scripting",
        QDBusConnection::sessionBus());
    if (!scriptingInterface.isValid()) {
        qCritical() << "[setFocusedWindow] -> DBusInterface invalid";
        return;
    }

    // chargé le script via scriptingInterface
    QDBusReply<int> reply = scriptingInterface.call("loadScript", script.fileName());
    if (!reply.isValid()) {
        qCritical() << "[setFocusedWindow] -> Erreur loadScript:" << reply.error().message();
        return;
    }

    const int scriptId = reply.value(); // Id du script
    const QString scriptPath = "/Scripting/Script" + QString::number(scriptId); // Path du script

    qInfo() << "[setFocusedWindow] -> Script loaded, scriptId:" << scriptId << " | path:" << scriptPath;

    // Interface vers le script chargé
    QDBusInterface DBusScriptInterface(
        "org.kde.KWin",
        scriptPath,
        "org.kde.kwin.Script",
        QDBusConnection::sessionBus()
    );

    // Lancer le script
    if (QDBusReply<void> runReply = DBusScriptInterface.call("run"); !runReply.isValid()) {
        qCritical() << "[setFocusedWindow] -> Error run:" << runReply.error().message();
        return;
    }

    qInfo() << "[setFocusedWindow] -> Script used";
}
