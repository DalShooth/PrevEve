#include "ScreenCastHandler.h"
#include <QDBusPendingReply>
#include <qdbusunixfiledescriptor.h>
#include <QDebug>
#include <qimage.h>
#include <QSocketNotifier>
#include "PortalStreamInfo.h"
#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#include <spa/debug/types.h>
#include <spa/debug/pod.h>
#include <spa/pod/pod.h>


ScreenCastHandler::ScreenCastHandler()
{
    qInfo() << "CONSTRUCTOR [ScreenCastHandler]";

    // Canal de communication persistant entre l'app Qt et le service org.freedesktop.portal.ScreenCast
    m_QtDBusInterface = new QDBusInterface(
        "org.freedesktop.portal.Desktop",       // service
        "/org/freedesktop/portal/desktop",      // chemin
        "org.freedesktop.portal.ScreenCast",    // interface
        QDBusConnection::sessionBus(),
        this);

    // Dictionnaire de propriétés PipeWire qui décrit le flux comme vidéo de capture
    m_PipeWireProperties = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Video",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Application",
        nullptr);

    pw_init(nullptr, nullptr); // Initialise PipeWire

    m_PipeWireLoop = pw_loop_new(nullptr); // Boucle d'événement PipeWire
    if (!m_PipeWireLoop) {
        qCritical() << "CONSTRUCTOR [ScreenCastHandler] -> m_PipeWireLoop invalide";
        return;
    }

    m_PipeWireContext = pw_context_new(m_PipeWireLoop, nullptr, 0); // Point d’entrée PipeWire
    if (!m_PipeWireContext) {
        qCritical() << "CONSTRUCTOR [ScreenCastHandler] -> m_PipeWireContext invalide";
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
        qCritical() << "CONSTRUCTOR [ScreenCastHandler] -> Failed to connect m_PipeWireLoopSocketNotifier";
        return;
    }
}

ScreenCastHandler::~ScreenCastHandler()
{
    // Todo -> refaire le deconstructeur
    if (m_PipeWireCore) {
        pw_core_disconnect(m_PipeWireCore);
        m_PipeWireCore = nullptr;
    }
    if (m_PipeWireContext) {
        pw_context_destroy(m_PipeWireContext);
        m_PipeWireContext = nullptr;
    }
    if (m_PipeWireLoop) {
        pw_loop_destroy(m_PipeWireLoop);
        m_PipeWireLoop = nullptr;
    }
    pw_deinit();
}

ScreenCastHandler * ScreenCastHandler::instance() // SingleTon
{
    static ScreenCastHandler s_instance;
    return &s_instance;
}

void ScreenCastHandler::onChangeScreenCastState() // Linear State Machine
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
            openThumbnailsPipe();
            // Changement ici
            break;
        default:
            break;
    }
}

void ScreenCastHandler::setScreenCastState(::ScreenCastState NewScreenCastState) {
    //qInfo() << "[setScreenCastState] " << static_cast<int>(m_StreamState) << " -> " << static_cast<int>(NewScreenCastState);
    m_StreamState = NewScreenCastState;
    onChangeScreenCastState();
}

void ScreenCastHandler::init() // Todo -> changement complet de lancement ici
{
    qInfo() << "[init]";

    DBusCreateSessionRequest();
}

void ScreenCastHandler::DBusCreateSessionRequest() // Requête de création de session D-Bus
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
        &ScreenCastHandler::onDBusCreateSessionRequestFinished);

    setScreenCastState(ScreenCastState::CreatingSession); // Passe à l'état 'en cours de création' (cosmetique)
}

void ScreenCastHandler::onDBusCreateSessionRequestFinished(QDBusPendingCallWatcher* watcher)
{
    qInfo() << "[onDBusCreateSessionRequestFinished]";

    const QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "[onDBusCreateSessionRequestFinished] -> Erreur:" << reply.error().message();
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
}

void ScreenCastHandler::onDBusCreateSessionRequestResponse(const uint responseCode, const QVariantMap &results)
{
    qInfo() << "[onDBusCreateSessionRequestResponse]";

    if (responseCode != 0) {
        qCritical() << "Échec, code =" << responseCode;
        return;
    }

    m_DBusSessionHandle = QDBusObjectPath(results.value("session_handle").toString());

    if (m_DBusSessionHandle.path().isEmpty()) {
        qCritical() << "[onDBusCreateSessionRequestResponse] -> m_DBusSessionHandle is empty";
        return;
    }

    setScreenCastState(ScreenCastState::SessionCreated); // Passage à l'état suivant
}

void ScreenCastHandler::DBusSelectSourcesRequest()
{
    qInfo() << "[DBusSelectSourcesRequest]";

    QVariantMap DBusSourcesRequestOptions;
    DBusSourcesRequestOptions["types"] = (uint)2;        // Application seule (don't work without uint)
    DBusSourcesRequestOptions["multiple"] = true;        // autoriser plusieurs choix
    DBusSourcesRequestOptions["cursor_mode"] = (uint)2;  // curseur intégré (don't work without uint)
    DBusSourcesRequestOptions["handle_token"] = "DBusSourcesHandleToken";

    const QDBusPendingCall call = m_QtDBusInterface->asyncCall("SelectSources", m_DBusSessionHandle, DBusSourcesRequestOptions);
    auto* watcher = new QDBusPendingCallWatcher(call, this);
    connect(
        watcher,
        &QDBusPendingCallWatcher::finished,
        this,
        &ScreenCastHandler::onDBusSelectSourcesRequestFinished);

    setScreenCastState(ScreenCastState::SelectingSources); // Passage sur l'état cosmetique
}

void ScreenCastHandler::onDBusSelectSourcesRequestFinished(QDBusPendingCallWatcher* watcher)
{
    qInfo() << "[onDBusSelectSourcesRequestFinished]";

    const QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "[onDBusSelectSourcesRequestFinished] -> Erreur:" << reply.error().message();
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
}

void ScreenCastHandler::onDBusSelectSourcesRequestResponse(const uint responseCode, const QVariantMap &results)
{
    qInfo() << "[onDBusSelectSourcesRequestResponse]";

    if (responseCode != 0) {
        qWarning() << "Annulé ou échoué";
        return;
    }

    setScreenCastState(ScreenCastState::SourcesSelected); // Passage sur l'état suivant
}

void ScreenCastHandler::StartScreensSharingRequest()
{
    qInfo() << "[StartScreensSharingRequest]";

    QVariantMap ScreensSharingOptions;
    ScreensSharingOptions["handle_token"] = "ScreensSharingHandleToken";

    const QDBusPendingCall call = m_QtDBusInterface->asyncCall("Start", m_DBusSessionHandle, QString(""), ScreensSharingOptions);

    auto* watcher = new QDBusPendingCallWatcher(call, this);
    connect(
        watcher,
        &QDBusPendingCallWatcher::finished,
        this,
        &ScreenCastHandler::onStartScreensSharingRequestFinished);

    setScreenCastState(ScreenCastState::Starting);
}

void ScreenCastHandler::onStartScreensSharingRequestFinished(QDBusPendingCallWatcher *watcher)
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

void ScreenCastHandler::onStartScreensSharingRequestResponse(const uint code, const QVariantMap &results)
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

void ScreenCastHandler::OpenPipeWireConnexionRequest()
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
        &ScreenCastHandler::onOpenPipeWireConnexionRequestFinished);

    setScreenCastState(ScreenCastState::OpeningPipeWireRemote); // État cosmetique
}

void ScreenCastHandler::onOpenPipeWireConnexionRequestFinished(QDBusPendingCallWatcher* watcher)
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

    qInfo() << "[onOpenPipeWireConnexionRequestFinished] -> PipeWire File Descriptor =" << m_PipeWireFileDescriptor;

    setScreenCastState(ScreenCastState::PipeWireRemoteCreated); // Passage à l'état suivant
}

void ScreenCastHandler::openThumbnailsPipe()
{
    qInfo() << "[openThumbnailsPipe]";

    qInfo() << "[openThumbnailsPipe] -> pw_context_connect_fd avec FD" << m_PipeWireFileDescriptor;
    m_PipeWireCore = pw_context_connect_fd(m_PipeWireContext, m_PipeWireFileDescriptor, nullptr, 0);
    qInfo() << "[openThumbnailsPipe] -> pw_context_connect_fd renvoie core =" << m_PipeWireCore;
    if (!m_PipeWireCore) {
        qWarning() << "[openThumbnailsPipe] -> m_pwCore invalid";
        return;
    }

    m_pwStream = pw_stream_new(m_PipeWireCore, "PrevEveScreenCast", m_PipeWireProperties);
    qInfo() << "[openThumbnailsPipe] -> pw_stream_new renvoie stream =" << m_pwStream;
    if (!m_pwStream) {
        qWarning() << "[openThumbnailsPipe] -> m_pwStream invalid";
        return;
    }

    static pw_stream_events streamEvents = {};
    streamEvents.version = PW_VERSION_STREAM_EVENTS;
    streamEvents.state_changed = [](
        void* data,
        pw_stream_state oldState,
        pw_stream_state newState,
        const char* error) {
            qInfo() << "[streamEvents.state_changed] -> Stream state changed from "
                    << streamStateToStr(oldState) << "to" << streamStateToStr(newState) << (error ? error : "");
    };
    streamEvents.param_changed = [](
        void* data,
        const uint32_t id,
        const spa_pod* param) {
        auto* handler = static_cast<ScreenCastHandler*>(data);

        if (!param) {
            qWarning() << "[streamEvents.param_changed] -> param == nullptr";
            return;
        }

        qInfo() << "[streamEvents.param_changed] -> id ="
                << spa_debug_type_find_short_name(spa_type_param, id);

        spa_debug_pod(2, nullptr, param);

        if (id == SPA_PARAM_Format) {
            spa_video_info_raw info = {};
            uint32_t media_type, media_subtype;

            if (spa_format_parse(param, &media_type, &media_subtype) < 0) {
                qWarning() << "[param_changed:Format] -> impossible de parser le format";
                return;
            }

            if (media_type == SPA_MEDIA_TYPE_video &&
                media_subtype == SPA_MEDIA_SUBTYPE_raw) {

                if (spa_format_video_raw_parse(param, &info) < 0) {
                    qWarning() << "[param_changed:Format] -> impossible de parser les infos vidéo";
                    return;
                }

                handler->m_videoWidth  = info.size.width;
                handler->m_videoHeight = info.size.height;

                qInfo() << "[param_changed:Format] -> dimensions vidéo ="
                        << handler->m_videoWidth << "x" << handler->m_videoHeight;
                }
        }
    };

    streamEvents.add_buffer = [](void* data, pw_buffer* buffer) {
        qInfo() << "[streamEvents.add_buffer] -> Buffer ajouté";
    };
    streamEvents.remove_buffer = [](void* data, pw_buffer* buffer) {
        qInfo() << "[streamEvents.remove_buffer] -> Buffer supprimé";
    };
    streamEvents.destroy = [](void* data) {
        qInfo() << "[streamEvents.destroy] Stream destroyed";
    };
    streamEvents.process = [](void* data) {
        qInfo() << "[streamEvents.process]";
        auto* handler = static_cast<ScreenCastHandler*>(data);

        pw_buffer* buffer = pw_stream_dequeue_buffer(handler->m_pwStream);
        if (!buffer) {
            qWarning() << "[streamEvents.process] -> pw_buffer invalid";
            return;
        }

        spa_buffer* spaBuf = buffer->buffer;
        if (!spaBuf) {
            qWarning() << "[streamEvents.process] -> spa_buffer invalid";
            pw_stream_queue_buffer(handler->m_pwStream, buffer);
            return;
        }

        if (!spaBuf->datas[0].data || !spaBuf->datas[0].chunk) {
            qWarning() << "[streamEvents.process] -> spaBuf->datas[0] invalid";
            pw_stream_queue_buffer(handler->m_pwStream, buffer);
            return;
        }

        void* src = spaBuf->datas[0].data;
        //uint32_t size   = spaBuf->datas[0].chunk->size;
        //const uint32_t stride = spaBuf->datas[0].chunk->stride;

        // Construire QImage
        const QImage image(
            static_cast<uchar *>(src),
            handler->m_videoWidth,
            handler->m_videoHeight,
            spaBuf->datas[0].chunk->stride,
            QImage::Format_RGB32);

        // Faire une copie pour ne pas dépendre du buffer PipeWire
        const QImage copy = image.scaled(450, 380, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        // Émettre le signal Qt
        emit handler->videoFrameAvailable(copy);

        // Rendre le buffer à PipeWire
        pw_stream_queue_buffer(handler->m_pwStream, buffer);
    };



    pw_stream_add_listener(m_pwStream, &m_streamListener, &streamEvents, this);

    // Demande un format vidéo (ici BGRA arbitraire)
    spa_pod_builder b = SPA_POD_BUILDER_INIT(m_buffer, sizeof(m_buffer));

    const spa_pod* params[1];
    params[0] = static_cast<const spa_pod *>(spa_pod_builder_add_object(&b,
        SPA_TYPE_OBJECT_Format,
        SPA_PARAM_EnumFormat,
        SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
        SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
        SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(6,
            SPA_VIDEO_FORMAT_BGRA,
            SPA_VIDEO_FORMAT_BGRx,
            SPA_VIDEO_FORMAT_RGBx,
            SPA_VIDEO_FORMAT_RGB,
            SPA_VIDEO_FORMAT_ARGB,
            SPA_VIDEO_FORMAT_YUY2)
    ));


    qInfo() << "[openThumbnailsPipe] -> appel pw_stream_connect (ID cible =" << m_PortalStreamInfoList[0].nodeId << ")";
    int streamConnectResult = pw_stream_connect(
        m_pwStream,
        PW_DIRECTION_INPUT,
        m_PortalStreamInfoList[0].nodeId,
        static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
        params,
        1);
    qInfo() << "[openThumbnailsPipe] -> pw_stream_connect renvoie" << streamConnectResult;
    if (streamConnectResult < 0) {
        qWarning() << "[openThumbnailsPipe] -> Stream connect failed";
        return;
    }

    qInfo() << "[openThumbnailsPipe] -> PipeWire stream créé";
}

const char* ScreenCastHandler::streamStateToStr(const pw_stream_state state)
{
    switch (state) {
        case PW_STREAM_STATE_ERROR:        return "ERROR";
        case PW_STREAM_STATE_UNCONNECTED:  return "UNCONNECTED";
        case PW_STREAM_STATE_CONNECTING:   return "CONNECTING";
        case PW_STREAM_STATE_PAUSED:       return "PAUSED";
        case PW_STREAM_STATE_STREAMING:    return "STREAMING";
        default:                           return "UNKNOWN";
    }
}
