#include "ScreenCastHandler.h"
#include <QDBusPendingReply>
#include <QDebug>
#include "StreamInfo.h"

ScreenCastHandler::ScreenCastHandler(QObject* parent)
    : QObject(parent)
{
    m_portal = new QDBusInterface(
        "org.freedesktop.portal.Desktop",       // service
        "/org/freedesktop/portal/desktop",      // chemin
        "org.freedesktop.portal.ScreenCast",    // interface
        QDBusConnection::sessionBus(), this);
}

void ScreenCastHandler::createSession()
{
    QVariantMap opts;
    opts["session_handle_token"] = "mysessiontoken";
    opts["handle_token"] = "createreq";

    QDBusPendingCall call = m_portal->asyncCall("CreateSession", opts);

    auto* watcher = new QDBusPendingCallWatcher(call, this);
    connect(
        watcher,
        &QDBusPendingCallWatcher::finished,
        this,
        &ScreenCastHandler::onCreateSessionFinished);
}

void ScreenCastHandler::onCreateSessionFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "[onCreateSessionFinished] Erreur:" << reply.error().message();
    }
    else {
        const QDBusObjectPath requestHandle = reply.value();
        qInfo() << "[onCreateSessionFinished] Requête envoyée, handle =" << requestHandle.path();

        const bool ok = QDBusConnection::sessionBus().connect(
            "org.freedesktop.portal.Desktop",
            requestHandle.path(),
            "org.freedesktop.portal.Request",
            "Response",
            this,
            SLOT(onCreateSessionResponse(uint, QVariantMap))
            );
        if (!ok) {
            qCritical() << "[onCreateSessionFinished] Impossible de se connecter au signal Response.";
        }
    }
}

void ScreenCastHandler::onCreateSessionResponse(uint responseCode, const QVariantMap &results)
{
    if (responseCode != 0) {
        qCritical() << "[onCreateSessionResponse] Échec, code =" << responseCode;
        return;
    }

    QString sessionHandleStr = results.value("session_handle").toString();
    if (sessionHandleStr.isEmpty()) {
        qCritical() << "[CreateSession] Pas de session_handle dans results";
        return;
    }
    m_sessionHandle = sessionHandleStr;

    qInfo() << "[onCreateSessionResponse] Session créée, handle =" << m_sessionHandle;

    // Maintenant on lance SelectSources
    QVariantMap opts;
    opts["types"] = (uint)3;        // fenêtres
    opts["multiple"] = true;        // autoriser plusieurs choix
    opts["cursor_mode"] = (uint)2;  // curseur intégré
    opts["handle_token"] = "selectreq";

    QDBusObjectPath sessionPath(m_sessionHandle);
    QDBusPendingCall call = m_portal->asyncCall("SelectSources", sessionPath, opts);

    auto* watcher = new QDBusPendingCallWatcher(call, this);
    connect(
        watcher,
        &QDBusPendingCallWatcher::finished,
        this,
        &ScreenCastHandler::onSelectSourcesFinished);
}

void ScreenCastHandler::onSelectSourcesFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "[onSelectSourcesFinished] Erreur:" << reply.error().message();
    } else {
        const QDBusObjectPath requestHandle = reply.value();
        qDebug() << "[onSelectSourcesFinished] Request handle =" << requestHandle.path();

        // Écoute du signal Response sur ce request
        QDBusConnection::sessionBus().connect(
            "org.freedesktop.portal.Desktop",       // service
            requestHandle.path(),                          // chemin de l'objet Request
            "org.freedesktop.portal.Request",       // interface
            "Response",                             // signal
            this,
            SLOT(onSelectSourcesResponse(uint, QVariantMap))
        );
    }
    watcher->deleteLater();
}

void ScreenCastHandler::onSelectSourcesResponse(uint code, const QVariantMap &results)
{
    qInfo() << "[onSelectSourcesResponse] code=" << code << " results=" << results;

    if (code != 0) {
        qWarning() << "[onSelectSourcesResponse] Annulé ou échoué";
        return;
    }

    QVariantMap opts;
    opts["handle_token"] = "startreq";

    QDBusObjectPath sessionPath(m_sessionHandle);
    QDBusPendingCall call = m_portal->asyncCall("Start", sessionPath, QString(""), opts);

    auto* watcher = new QDBusPendingCallWatcher(call, this);
    connect(
        watcher,
        &QDBusPendingCallWatcher::finished,
        this,
        &ScreenCastHandler::onStartFinished);
}



void ScreenCastHandler::onStartFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "[onStartFinished] Erreur:" << reply.error().message();
    } else {
        const QDBusObjectPath requestHandle = reply.value();
        qInfo() << "[onStartFinished] Request handle =" << requestHandle.path();

        // Écoute du signal Response sur ce request
        QDBusConnection::sessionBus().connect(
            "org.freedesktop.portal.Desktop",       // service
            requestHandle.path(),                          // chemin de l'objet Request
            "org.freedesktop.portal.Request",       // interface
            "Response",                             // signal
            this,
            SLOT(onStartResponse(uint, QVariantMap))
        );
    }
    watcher->deleteLater();
}

void ScreenCastHandler::onStartResponse(uint code, const QVariantMap &results)
{
    if (code != 0) {
        qWarning() << "[Start Response] Échec ou annulé";
        return;
    }

    // récupérer la liste de streams en utilisant notre struct custom
    QList<StreamInfo> streams = qdbus_cast<QList<StreamInfo>>(results.value("streams"));

    if (streams.isEmpty()) {
        qWarning() << "[Start Response] Aucun stream reçu";
        return;
    }

    for (const auto &s : streams) {
        qInfo() << "[Start Response] nodeId =" << s.nodeId;

        // si tu veux un peu plus d'infos :
        if (s.props.contains("source_type"))
            qInfo() << "    source_type =" << s.props.value("source_type").toUInt();
        if (s.props.contains("size")) {
            auto size = s.props.value("size").toList();
            if (size.size() == 2)
                qInfo() << "    size =" << size[0].toInt() << "x" << size[1].toInt();
        }
    }
}

