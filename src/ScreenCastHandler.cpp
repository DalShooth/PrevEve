#include "ScreenCastHandler.h"
#include <QDBusPendingReply>
#include <qdbusunixfiledescriptor.h>
#include <QDebug>
#include "StreamInfo.h"

ScreenCastHandler::ScreenCastHandler()
{
    m_portal = new QDBusInterface(
        "org.freedesktop.portal.Desktop",       // service
        "/org/freedesktop/portal/desktop",      // chemin
        "org.freedesktop.portal.ScreenCast",    // interface
        QDBusConnection::sessionBus(), this);
}

ScreenCastHandler * ScreenCastHandler::instance()
{
    static ScreenCastHandler s_instance;
    return &s_instance;
}

void ScreenCastHandler::onChangeScreenCastState(ScreenCastState NewScreenCastState)
{
    qInfo() << "[onChangeScreenCastState] " << static_cast<int>(NewScreenCastState);

    m_screenCastState = NewScreenCastState;

    switch (m_screenCastState) {
        case ScreenCastState::Idle:
            qInfo() << "screenCastState is Idle";
            break;
        case ScreenCastState::CreatingSession:
            qInfo() << "CreatingSession...";
            break;
        case ScreenCastState::SessionCreated:
            qInfo() << "SessionCreated";
            selectSources();
            break;
        case ScreenCastState::SelectingSources:
            qInfo() << "SelectingSources...";
            break;
        case ScreenCastState::SourcesSelected:
            qInfo() << "SourcesSelected";
            start();
            break;
        case ScreenCastState::Starting:
            qInfo() << "Starting...";
            break;
        case ScreenCastState::AppSelected:
            qInfo() << "AppSelected";
            openPipeWireRemote();
            break;
        case ScreenCastState::OpeningPipeWireRemote:
            qInfo() << "OpeningPipeWireRemote...";
            break;
        case ScreenCastState::PipeWireRemoteCreated:
            qInfo() << "PipeWireRemoteCreated";
        case ScreenCastState::Active:
            qInfo() << "screenCastState is Active";
            break;
        default:
            break;
    }
}

void ScreenCastHandler::setScreenCastState(::ScreenCastState NewScreenCastState) {
    qInfo() << "[setScreenCastState] " << static_cast<int>(m_screenCastState) << " -> " << static_cast<int>(NewScreenCastState);

    if (m_screenCastState == NewScreenCastState) {
        return;
    }

    m_screenCastState = NewScreenCastState;
    onChangeScreenCastState(m_screenCastState);
}

void ScreenCastHandler::init()
{
    qInfo() << "[init]";

    createSession();
}

void ScreenCastHandler::createSession()
{
    qInfo() << "[createSession]";

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

    setScreenCastState(ScreenCastState::CreatingSession);
}

void ScreenCastHandler::onCreateSessionFinished(QDBusPendingCallWatcher* watcher)
{
    qInfo() << "[onCreateSessionFinished]";

    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "Erreur:" << reply.error().message();
    }
    else {
        const QDBusObjectPath requestHandle = reply.value();
        qInfo() << "Requête envoyée, handle =" << requestHandle.path();

        const bool ok = QDBusConnection::sessionBus().connect(
            "org.freedesktop.portal.Desktop",
            requestHandle.path(),
            "org.freedesktop.portal.Request",
            "Response",
            this,
            SLOT(onCreateSessionResponse(uint, QVariantMap))
            );
        if (!ok) {
            qCritical() << "Impossible de se connecter au signal Response.";
        }
    }
}

void ScreenCastHandler::onCreateSessionResponse(uint responseCode, const QVariantMap &results)
{
    qInfo() << "[onCreateSessionResponse]";

    if (responseCode != 0) {
        qCritical() << "Échec, code =" << responseCode;
        return;
    }

    QString sessionHandleStr = results.value("session_handle").toString();
    if (sessionHandleStr.isEmpty()) {
        qCritical() << "Pas de session_handle dans results";
        return;
    }

    m_sessionHandle = sessionHandleStr;

    setScreenCastState(ScreenCastState::SessionCreated);
}

void ScreenCastHandler::selectSources()
{
    qInfo() << "[selectSources]";

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

    setScreenCastState(ScreenCastState::SelectingSources);
}

void ScreenCastHandler::onSelectSourcesFinished(QDBusPendingCallWatcher* watcher)
{
    qInfo() << "[onSelectSourcesFinished]";

    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "Erreur:" << reply.error().message();
    } else {
        const QDBusObjectPath requestHandle = reply.value();
        qInfo() << "Request handle =" << requestHandle.path();

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
    qInfo() << "[onSelectSourcesResponse]";

    if (code != 0) {
        qWarning() << "Annulé ou échoué";
        return;
    }

    setScreenCastState(ScreenCastState::SourcesSelected);
}

void ScreenCastHandler::start()
{
    qInfo() << "[start]";

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

    setScreenCastState(ScreenCastState::Starting);
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
    qInfo() << "[onStartResponse]";

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

    setScreenCastState(ScreenCastState::AppSelected);
}

void ScreenCastHandler::openPipeWireRemote()
{
    qInfo() << "[openPipeWireRemote]";

    if (m_sessionHandle.isEmpty()) {
        qWarning() << "Pas de session handle";
        return;
    }

    QDBusObjectPath sessionPath(m_sessionHandle);

    QDBusPendingCall call = m_portal->asyncCall("OpenPipeWireRemote", sessionPath, QVariantMap());
    auto* watcher = new QDBusPendingCallWatcher(call, this);
    connect(
        watcher,
        &QDBusPendingCallWatcher::finished,
        this,
        &ScreenCastHandler::onOpenPipeWireRemoteFinished);

    setScreenCastState(ScreenCastState::OpeningPipeWireRemote);
}

void ScreenCastHandler::onOpenPipeWireRemoteFinished(QDBusPendingCallWatcher* watcher)
{
    qInfo() << "[onOpenPipeWireRemoteFinished]";

    QDBusPendingReply<QDBusUnixFileDescriptor> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "Erreur:" << reply.error().message();
        return;
    }

    QDBusUnixFileDescriptor file_descriptor = reply.value();
    m_pwFd = dup(file_descriptor.fileDescriptor());
    qInfo() << "Remote FD =" << m_pwFd;
    watcher->deleteLater();

    setScreenCastState(ScreenCastState::PipeWireRemoteCreated);
}
