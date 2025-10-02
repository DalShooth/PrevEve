#include "KWinManager.h"

#include <qdbusreply.h>
#include <qdir.h>
#include <qproperty.h>


KWinManager::KWinManager(QObject *parent) : QObject(parent)
{
    qInfo() << "CONSTRUCTOR [KWinManager]";

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerService("org.example.EveWPreview")) {
        qWarning() << "[DBus] registerService FAILED:"
                   << bus.lastError().name() << bus.lastError().message();
    }
    if (!bus.registerObject("/EveWPreview", this, QDBusConnection::ExportAllSlots)) {
        qWarning() << "[DBus] registerObject FAILED:"
                   << bus.lastError().name() << bus.lastError().message();
    }
}

void KWinManager::MakeThumbnailsKeepAbove()
{
    qInfo() << "[MakeThumbnailsKeepAbove]";

    QFile file(":/scripts/KWin_KeepAbove.js");
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "[MakeThumbnailsKeepAbove] -> Impossible d’ouvrir le script embarqué";
        return;
    }
    const QString script = QString::fromUtf8(file.readAll());
    //qInfo() << script.toUtf8().constData(); // <- DEBUG for print script
    file.close();

    QTemporaryFile temporaryFile(QDir::tempPath() + "/EveWPreview_XXXXXX.js");
    temporaryFile.setAutoRemove(true);
    if (!temporaryFile.open()) {
        qCritical() << "[MakeThumbnailsKeepAbove] -> Cannot open temporaryFile";
        return;
    }
    temporaryFile.write(script.toUtf8());
    temporaryFile.flush();
    temporaryFile.close();

    QDBusInterface DBusInterface(
        "org.kde.KWin",
        "/Scripting",
        "org.kde.kwin.Scripting",
        QDBusConnection::sessionBus());
    if (!DBusInterface.isValid()) {
        qCritical() << "[MakeThumbnailsKeepAbove] -> DBusInterface invalid";
        return;
    }

    QDBusReply<int> reply = DBusInterface.call("loadScript", temporaryFile.fileName());
    if (!reply.isValid()) {
        qCritical() << "[MakeThumbnailsKeepAbove] -> Erreur loadScript:" << reply.error().message();
        return;
    }

    int scriptId = reply.value();
    QString scriptPath = "/Scripting/Script" + QString::number(scriptId);

    qInfo() << "[MakeThumbnailsKeepAbove] -> Script loaded, scriptId:" << scriptId << " | path:" << scriptPath;

    // Interface vers le script
    QDBusInterface DBusScriptInterface(
        "org.kde.KWin",
        scriptPath,
        "org.kde.kwin.Script",
        QDBusConnection::sessionBus()
    );

    if (!DBusScriptInterface.isValid()) {
        qCritical() << "[MakeThumbnailsKeepAbove] -> Unable to create DBusScriptInterface";
        return;
    }

    // Lancer le script
    QDBusReply<void> runReply = DBusScriptInterface.call("run");
    if (!runReply.isValid()) {
        qCritical() << "[MakeThumbnailsKeepAbove] -> Error run:" << runReply.error().message();
        return;
    }


    qInfo() << "[MakeThumbnailsKeepAbove] -> Script used";
}

void KWinManager::SetWindowPosition(const QString &character, const QPoint newPosition)
{
    qInfo() << "SetWindowPosition()";

    QFile file(":/scripts/KWin_SetWindowPosition.js");
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "[SetWindowPosition] -> Impossible d’ouvrir le script embarqué";
        return;
    }
    QString caption = "Thumbnail-" + character;
    const QString script = QString::fromUtf8(file.readAll()).arg(caption).arg(newPosition.x()).arg(newPosition.y());
    //qInfo() << script.toUtf8().constData(); // <- DEBUG for print script
    file.close();

    QTemporaryFile temporaryFile(QDir::tempPath() + "/EveWPreview_XXXXXX.js");
    temporaryFile.setAutoRemove(true);
    if (!temporaryFile.open()) {
        qCritical() << "[SetWindowPosition] -> Cannot open temporaryFile";
        return;
    }
    temporaryFile.write(script.toUtf8());
    temporaryFile.flush();
    temporaryFile.close();

    QDBusInterface DBusInterface(
        "org.kde.KWin",
        "/Scripting",
        "org.kde.kwin.Scripting",
        QDBusConnection::sessionBus());
    if (!DBusInterface.isValid()) {
        qCritical() << "[SetWindowPosition] -> DBusInterface invalid";
        return;
    }

    QDBusReply<int> reply = DBusInterface.call("loadScript", temporaryFile.fileName());
    if (!reply.isValid()) {
        qCritical() << "[SetWindowPosition] -> Erreur loadScript:" << reply.error().message();
        return;
    }

    int scriptId = reply.value();
    QString scriptPath = "/Scripting/Script" + QString::number(scriptId);

    qInfo() << "[SetWindowPosition] -> Script loaded, scriptId:" << scriptId << " | path:" << scriptPath;

    // Interface vers le script
    QDBusInterface DBusScriptInterface(
        "org.kde.KWin",
        scriptPath,
        "org.kde.kwin.Script",
        QDBusConnection::sessionBus()
    );

    if (!DBusScriptInterface.isValid()) {
        qCritical() << "[SetWindowPosition] -> Unable to create DBusScriptInterface";
        return;
    }

    // Lancer le script
    QDBusReply<void> runReply = DBusScriptInterface.call("run");
    if (!runReply.isValid()) {
        qCritical() << "[SetWindowPosition] -> Error run:" << runReply.error().message();
        return;
    }

    qInfo() << "[SetWindowPosition] -> Script used for character:" << character;
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
    qInfo() << scriptRaw; // <- DEBUG for print script
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
