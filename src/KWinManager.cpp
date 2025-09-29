#include "KWinManager.h"

#include <qdbusreply.h>
#include <qdir.h>
#include <QFile>
#include <qproperty.h>
#include <QTemporaryFile>


void KWinManager::MakeThumbnailsAlwaysOnTop(const QString &CharacterName)
{
    qInfo() << "[MakeThumbnailsAlwaysOnTop]";

    QFile file(":/scripts/KWin_KeepAbove.js");
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "[MakeThumbnailsAlwaysOnTop] -> Impossible d’ouvrir le script embarqué";
        return;
    }
    const QString script = QString::fromUtf8(file.readAll()).arg(CharacterName);
    //qInfo() << script.toUtf8().constData(); // <- DEBUG for print script
    file.close();

    QTemporaryFile temporaryFile(QDir::tempPath() + "/kwin_script_XXXXXX.js");
    temporaryFile.setAutoRemove(true);
    if (!temporaryFile.open()) {
        qCritical() << "[MakeThumbnailsAlwaysOnTop] -> Cannot open temporaryFile";
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
        qCritical() << "[MakeThumbnailsAlwaysOnTop] -> DBusInterface invalid";
        return;
    }

    QDBusReply<int> reply = DBusInterface.call("loadScript", temporaryFile.fileName());
    if (!reply.isValid()) {
        qCritical() << "[MakeThumbnailsAlwaysOnTop] -> Erreur loadScript:" << reply.error().message();
        return;
    }

    int scriptId = reply.value();
    QString scriptPath = "/Scripting/Script" + QString::number(scriptId);

    qInfo() << "[MakeThumbnailsAlwaysOnTop] -> Script loaded, scriptId:" << scriptId << " | path:" << scriptPath;

    // Interface vers le script
    QDBusInterface DBusScriptInterface(
        "org.kde.KWin",
        scriptPath,
        "org.kde.kwin.Script",
        QDBusConnection::sessionBus()
    );

    if (!DBusScriptInterface.isValid()) {
        qCritical() << "[MakeThumbnailsAlwaysOnTop] -> Unable to create DBusScriptInterface";
        return;
    }

    // Lancer le script
    QDBusReply<void> runReply = DBusScriptInterface.call("run");
    if (!runReply.isValid()) {
        qCritical() << "[MakeThumbnailsAlwaysOnTop] -> Error run:" << runReply.error().message();
        return;
    }


    qInfo() << "[MakeThumbnailsAlwaysOnTop] -> Script used for Thumbnail:" << CharacterName;
}

void KWinManager::SetWindowPosition(const QString &Caption, const int X, const int Y)
{
    qInfo() << "[SetWindowPosition]";

    QFile file(":/scripts/KWin_SetPosition.js");
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "[SetWindowPosition] -> Impossible d’ouvrir le script embarqué";
        return;
    }
    const QString script = QString::fromUtf8(file.readAll()).arg(Caption).arg(X).arg(Y);
    //qInfo() << script.toUtf8().constData(); // <- DEBUG for print script
    file.close();

    QTemporaryFile temporaryFile(QDir::tempPath() + "/kwin_script_XXXXXX.js");
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


    qInfo() << "[SetWindowPosition] -> Script used for Thumbnail:" << Caption;
}