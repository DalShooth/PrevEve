#include "StreamManager.h"
#include <QDBusPendingReply>
#include <qdbusunixfiledescriptor.h>
#include <QDebug>
#include <QSocketNotifier>
#include "PortalStreamInfo.h"
#include <pipewire/pipewire.h>
#include <spa/debug/types.h>
#include <spa/debug/pod.h>
#include "Thumbnail.h"


StreamManager::StreamManager()
{
    qInfo() << "CONSTRUCTOR [StreamManager]";

    // Canal de communication persistant entre l'app Qt et le service org.freedesktop.portal.ScreenCast
    m_QtDBusInterface = new QDBusInterface(
        "org.freedesktop.portal.Desktop",       // service
        "/org/freedesktop/portal/desktop",      // chemin
        "org.freedesktop.portal.ScreenCast",    // interface
        QDBusConnection::sessionBus(),
        this);

    pw_init(nullptr, nullptr); // Initialise PipeWire

    m_PipeWireLoop = pw_loop_new(nullptr); // Boucle d'événement PipeWire
    if (!m_PipeWireLoop) {
        qCritical() << "CONSTRUCTOR [StreamManager] -> m_PipeWireLoop invalide";
        return;
    }

    m_PipeWireContext = pw_context_new(m_PipeWireLoop, nullptr, 0); // Point d’entrée PipeWire
    if (!m_PipeWireContext) {
        qCritical() << "CONSTRUCTOR [StreamManager] -> m_PipeWireContext invalide";
        return;
    }

    // Notifier m_PipeWireLoop sur son FD->Read
    m_PipeWireLoopSocketNotifier = new QSocketNotifier(
        pw_loop_get_fd(m_PipeWireLoop),
        QSocketNotifier::Read,
        this);

    // Connecte m_PipeWireLoopSocketNotifier
    const QMetaObject::Connection socketNotifierActivatedConnexion = connect(m_PipeWireLoopSocketNotifier,
        &QSocketNotifier::activated,
        this,
        [this] { pw_loop_iterate(m_PipeWireLoop, 0); });
    if (!socketNotifierActivatedConnexion) {
        qCritical() << "CONSTRUCTOR [StreamManager] -> Failed to connect m_PipeWireLoopSocketNotifier";
        return;
    }

    // Abonnement à la préparation des streams
    connect(
        this,
        &StreamManager::onStreamsReady,
        this,
        [this] {
            qInfo() << "[onStreamsReady]";
            // Boucle sur les streams
            for (int i = 0; i < m_PortalStreamInfoList.length(); ++i) {
                Thumbnail* preview = new Thumbnail(m_MainWindow, m_PipeWireCore, &m_PortalStreamInfoList[i]);
                m_ThumbnailsList.append(preview);
                preview->show();
            }});
}

StreamManager::~StreamManager()
{
    // Todo -> refaire le deconstructeur
    m_PipeWireCore = nullptr;
    m_PipeWireContext = nullptr;
    m_PipeWireLoop = nullptr;
    pw_deinit();
}

void StreamManager::onChangeScreenCastState() // Linear State Machine
{
    switch (m_StreamState) {
        case ScreenCastState::Idle: // Cosmetique
            qInfo() << "[onChangeScreenCastState] -> Idle";
            break;
        case ScreenCastState::CreatingSession: // Cosmetique
            qInfo() << "[onChangeScreenCastState] -> CreatingSession...";
            break;
        case ScreenCastState::SessionCreated: // Actif
            qInfo() << "[onChangeScreenCastState] -> SessionCreated";
            DBusSelectSourcesRequest();
            break;
        case ScreenCastState::SelectingSources: // Cosmetique
            qInfo() << "[onChangeScreenCastState] -> SelectingSources...";
            break;
        case ScreenCastState::SourcesSelected: // Actif
            qInfo() << "[onChangeScreenCastState] -> SourcesSelected";
            StartScreensSharingRequest();
            break;
        case ScreenCastState::Starting: // Cosmetique
            qInfo() << "[onChangeScreenCastState] -> Starting...";
            break;
        case ScreenCastState::AppSelected: // Actif
            qInfo() << "[onChangeScreenCastState] -> AppSelected";
            OpenPipeWireConnexionRequest();
            break;
        case ScreenCastState::OpeningPipeWireRemote: // Cosmetique
            qInfo() << "[onChangeScreenCastState] -> OpeningPipeWireRemote...";
            break;
        case ScreenCastState::PipeWireRemoteCreated: // Actif
            qInfo() << "[onChangeScreenCastState] -> PipeWireRemoteCreated";
            setScreenCastState(ScreenCastState::Active);
            break;
        case ScreenCastState::Active: // Actif
            qInfo() << "[onChangeScreenCastState] -> Streams is Active";
            emit onStreamsReady(); // Signalé la préparation des streams
            break;
        default:
            break;
    }
}

void StreamManager::setScreenCastState(::ScreenCastState NewScreenCastState) {
    //qInfo() << "[setScreenCastState] " << static_cast<int>(m_StreamState) << " -> " << static_cast<int>(NewScreenCastState);
    m_StreamState = NewScreenCastState;
    onChangeScreenCastState();
}

void StreamManager::DBusCreateSessionRequest() // Requête de création de session D-Bus
{
    qInfo() << "[DBusCreateSessionRequest]";

    QVariantMap SessionRequestOptions; // Options de la requête (const don't work)
    SessionRequestOptions["session_handle_token"] = "PrevEveSessionHandleToken";
    SessionRequestOptions["handle_token"] = "PrevEveHandleToken";

    const QDBusPendingCall call = m_QtDBusInterface->asyncCall("CreateSession", SessionRequestOptions);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(
        watcher,
        &QDBusPendingCallWatcher::finished,
        this,
        &StreamManager::onDBusCreateSessionRequestFinished);

    setScreenCastState(ScreenCastState::CreatingSession); // Passe à l'état 'en cours de création' (cosmetique)
}

void StreamManager::onDBusCreateSessionRequestFinished(QDBusPendingCallWatcher* watcher)
{
    qInfo() << "[onDBusCreateSessionRequestFinished]";

    const QDBusPendingReply<QDBusObjectPath> reply = *watcher;

    if (reply.isError()) {
        qWarning() << "[onDBusCreateSessionRequestFinished] -> Erreur:" << reply.error().message();
        watcher->deleteLater();
        return;
    }

    const QDBusObjectPath requestHandle = reply.value();
    qInfo() << "[onDBusCreateSessionRequestFinished] -> Request send, handle =" << requestHandle.path();

    const bool ok = QDBusConnection::sessionBus().connect(
        "org.freedesktop.portal.Desktop",
        requestHandle.path(),
        "org.freedesktop.portal.Request",
        "Response",
        this,
        SLOT(onDBusCreateSessionRequestResponse(uint, QVariantMap))
        );
    if (!ok) {
        qCritical() << "[onDBusCreateSessionRequestFinished] -> Can't connect to response signal";
    }
    watcher->deleteLater();
}

void StreamManager::onDBusCreateSessionRequestResponse(const uint responseCode, const QVariantMap &results)
{
    qInfo() << "[onDBusCreateSessionRequestResponse]";

    if (responseCode != 0) {
        qCritical() << "Fail, code =" << responseCode;
        if (results.isEmpty()) {
            qCritical() << "[onDBusCreateSessionRequestResponse] -> Result is empty";
        } else {
            qCritical() << "[onDBusCreateSessionRequestResponse] -> results dump:";
            for (auto it = results.begin(); it != results.end(); ++it) {
                qCritical() << "   " << it.key() << "=" << it.value();
            }
        }
        return;
    }

    m_DBusSessionHandle = QDBusObjectPath(results.value("session_handle").toString());

    if (m_DBusSessionHandle.path().isEmpty()) {
        qCritical() << "[onDBusCreateSessionRequestResponse] -> m_DBusSessionHandle is empty";
        return;
    }

    setScreenCastState(ScreenCastState::SessionCreated); // Passage à l'état suivant
}

void StreamManager::DBusSelectSourcesRequest()
{
    qInfo() << "[DBusSelectSourcesRequest]";

    QVariantMap DBusSourcesRequestOptions;
    DBusSourcesRequestOptions["types"] = (uint)2;        // Application seule (don't work without uint)
    DBusSourcesRequestOptions["multiple"] = true;        // autoriser plusieurs choix
    DBusSourcesRequestOptions["cursor_mode"] = (uint)2;  // curseur intégré (don't work without uint)
    DBusSourcesRequestOptions["handle_token"] = "DBusSourcesHandleToken";

    const QDBusPendingCall call = m_QtDBusInterface->asyncCall("SelectSources", m_DBusSessionHandle, DBusSourcesRequestOptions);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(
        watcher,
        &QDBusPendingCallWatcher::finished,
        this,
        &StreamManager::onDBusSelectSourcesRequestFinished);

    setScreenCastState(ScreenCastState::SelectingSources); // Passage sur l'état cosmetique
}

void StreamManager::onDBusSelectSourcesRequestFinished(QDBusPendingCallWatcher* watcher)
{
    qInfo() << "[onDBusSelectSourcesRequestFinished]";

    const QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "[onDBusSelectSourcesRequestFinished] -> Erreur:" << reply.error().message();
        watcher->deleteLater();
        return;
    }

    const QDBusObjectPath requestHandle = reply.value();
    qInfo() << "[onDBusSelectSourcesRequestFinished] -> Request handle path =" << requestHandle.path();

    // Écoute du signal Response sur ce request
    QDBusConnection::sessionBus().connect(
        "org.freedesktop.portal.Desktop",       // service
        requestHandle.path(),                          // chemin de l'objet Request
        "org.freedesktop.portal.Request",       // interface
        "Response",                             // signal
        this,
        SLOT(onDBusSelectSourcesRequestResponse(uint, QVariantMap))
    );
    watcher->deleteLater();
}

void StreamManager::onDBusSelectSourcesRequestResponse(const uint responseCode, const QVariantMap &results)
{
    qInfo() << "[onDBusSelectSourcesRequestResponse]";

    if (responseCode != 0) {
        qWarning() << "Annulé ou échoué";
        return;
    }

    setScreenCastState(ScreenCastState::SourcesSelected); // Passage sur l'état suivant
}

void StreamManager::StartScreensSharingRequest()
{
    qInfo() << "[StartScreensSharingRequest]";

    QVariantMap ScreensSharingOptions;
    ScreensSharingOptions["handle_token"] = "ScreensSharingHandleToken";

    const QDBusPendingCall call = m_QtDBusInterface->asyncCall("Start", m_DBusSessionHandle, QString(""), ScreensSharingOptions);

    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(
        watcher,
        &QDBusPendingCallWatcher::finished,
        this,
        &StreamManager::onStartScreensSharingRequestFinished);

    setScreenCastState(ScreenCastState::Starting);
}

void StreamManager::onStartScreensSharingRequestFinished(const QDBusPendingCallWatcher *watcher)
{
    qInfo() << "[onStartScreensSharingRequestFinished]";

    const QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        qCritical() << "[onStartScreensSharingRequestFinished] Erreur:" << reply.error().message();
        return;
    }

    const QDBusObjectPath requestHandle = reply.value();
    qInfo() << "[onStartScreensSharingRequestFinished] Request handle =" << requestHandle.path();

    // Écoute du signal Response sur ce request
    QDBusConnection::sessionBus().connect(
        "org.freedesktop.portal.Desktop",       // service
        requestHandle.path(),                          // chemin de l'objet Request
        "org.freedesktop.portal.Request",       // interface
        "Response",                             // signal
        this,
        SLOT(onStartScreensSharingRequestResponse(uint, QVariantMap))
    );
}

void StreamManager::onStartScreensSharingRequestResponse(const uint code, const QVariantMap &results)
{
    qInfo() << "[onStartScreensSharingRequestResponse]";

    if (code != 0) {
        qWarning() << "Échec ou annulé";
        return;
    }

    m_PortalStreamInfoList = qdbus_cast<QList<PortalStreamInfo>>(results.value("streams")); // Les stream de la selection

    if (m_PortalStreamInfoList.isEmpty()) {
        qWarning() << "[onStartScreensSharingRequestResponse] -> m_PortalStreamInfoList is empty";
        return;
    }

    for (int i = 0; i < m_PortalStreamInfoList.length(); ++i) {
        qInfo().noquote() << QString("[onStartScreensSharingRequestResponse] -> m_PortalStreamInfoList[%1].nodeId = %2")
            .arg(i).arg(m_PortalStreamInfoList[i].nodeId);

        if (m_PortalStreamInfoList[i].props.contains("source_type")) {
            qInfo() << "    source_type =" << m_PortalStreamInfoList[i].props.value("source_type").toUInt();
        }
        else {
            qInfo() << "stream[" << i << "].props not have source_type";
        }
    }

    setScreenCastState(ScreenCastState::AppSelected); // Passage à l'état suivant
}

void StreamManager::OpenPipeWireConnexionRequest()
{
    qInfo() << "[OpenPipeWireConnexionRequest]";

    if (m_DBusSessionHandle.path().isEmpty()) {
        qCritical() << "[OpenPipeWireConnexionRequest] -> Pas de session handle";
        return;
    }

    QDBusObjectPath sessionPath(m_DBusSessionHandle);

    const QDBusPendingCall call = m_QtDBusInterface->asyncCall("OpenPipeWireRemote", sessionPath, QVariantMap());
    auto* watcher = new QDBusPendingCallWatcher(call, this);
    connect(
        watcher,
        &QDBusPendingCallWatcher::finished,
        this,
        &StreamManager::onOpenPipeWireConnexionRequestFinished);

    setScreenCastState(ScreenCastState::OpeningPipeWireRemote); // État cosmetique
}

void StreamManager::onOpenPipeWireConnexionRequestFinished(const QDBusPendingCallWatcher* watcher)
{
    qInfo() << "[onOpenPipeWireConnexionRequestFinished]";

    const QDBusPendingReply<QDBusUnixFileDescriptor> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "[onOpenPipeWireConnexionRequestFinished] -> Erreur:" << reply.error().message();
        return;
    }

    //= File Descriptor Duping
    const QDBusUnixFileDescriptor file_descriptor = reply.value();
    const int fd = file_descriptor.fileDescriptor();
    if (fd == -1) {
        qCritical() << "[onOpenPipeWireConnectionFinished] -> invalid fd from portal";
        return;
    }
    m_PipeWireFileDescriptor = dup(fd);
    if (m_PipeWireFileDescriptor == -1) {
        qCritical() << "dup() failed:" << strerror(errno);
        return;
    }
    //=

    //= Crée le PipeWireCore
    qInfo() << "[onOpenPipeWireConnexionRequestFinished] -> m_PipeWireFileDescriptor:" << m_PipeWireFileDescriptor;
    m_PipeWireCore = pw_context_connect_fd(m_PipeWireContext, m_PipeWireFileDescriptor, nullptr, 0);
    if (!m_PipeWireCore) {
        qWarning() << "[openThumbnailsPipe] -> m_PipeWireCore invalid";
        return;
    }
    qInfo() << "[openThumbnailsPipe] -> m_PipeWireCore:" << m_PipeWireCore;
    //=

    setScreenCastState(ScreenCastState::PipeWireRemoteCreated); // Passage à l'état suivant
}

void StreamManager::SetupPreviews() {
    qInfo() << "[SetupPreviews]";

    // todo -> détruire les thumbnails, détruire le tableau, détruire PipeWire session

    DBusCreateSessionRequest();
}
