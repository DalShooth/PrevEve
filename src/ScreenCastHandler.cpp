#include "ScreenCastHandler.h"
#include <QDBusPendingReply>
#include <qdbusunixfiledescriptor.h>
#include <QDebug>
#include <qimage.h>
#include <QSocketNotifier>

#include "StreamInfo.h"
#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#include <spa/debug/types.h>
#include <spa/debug/pod.h>
#include <spa/pod/pod.h>


ScreenCastHandler::ScreenCastHandler()
{
    qInfo() << "[ScreenCastHandler]";

    m_portal = new QDBusInterface(
        "org.freedesktop.portal.Desktop",       // service
        "/org/freedesktop/portal/desktop",      // chemin
        "org.freedesktop.portal.ScreenCast",    // interface
        QDBusConnection::sessionBus(),
        this);

    m_StreamProps = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Video",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Application",
        nullptr);

    pw_init(nullptr, nullptr);

    m_pwLoop = pw_loop_new(nullptr);
    if (!m_pwLoop) {
        qInfo() << "Failed to create loop";
        return;
    }

    m_pwContext = pw_context_new(m_pwLoop, nullptr, 0);
    if (!m_pwContext) {
        qInfo() << "Failed to create context";
        return;
    }

    const int fd = pw_loop_get_fd(m_pwLoop);
    if (fd == -1) {
        qInfo() << "Failed to get fd";
        return;
    }
    m_socketNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);

    const QMetaObject::Connection socketNotifierActivatedConnexion = connect(m_socketNotifier,
        &QSocketNotifier::activated,
        this,
        [this] { pw_loop_iterate(m_pwLoop, 0); });
    if (!socketNotifierActivatedConnexion) {
        qInfo() << "Failed to connect socket";
    }
}

ScreenCastHandler::~ScreenCastHandler()
{
    if (m_pwCore) {
        pw_core_disconnect(m_pwCore);
        m_pwCore = nullptr;
    }
    if (m_pwContext) {
        pw_context_destroy(m_pwContext);
        m_pwContext = nullptr;
    }
    if (m_pwLoop) {
        pw_loop_destroy(m_pwLoop);
        m_pwLoop = nullptr;
    }
    pw_deinit();
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
            setScreenCastState(ScreenCastState::Active);
            break;
        case ScreenCastState::Active:
            qInfo() << "screenCastState is Active";
            openThumbnailsPipe();
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
        qWarning() << "Échec ou annulé";
        return;
    }

    // récupérer la liste de streams en utilisant notre struct custom
    m_streams = qdbus_cast<QList<StreamInfo>>(results.value("streams"));

    if (m_streams.isEmpty()) {
        qWarning() << "Aucun stream reçu";
        return;
    }

    for (int i = 0; i < m_streams.length(); ++i) {
        qInfo() << "nodeId =" << m_streams[i].nodeId;

        if (m_streams[i].props.contains("source_type")) {
            qInfo() << "    source_type =" << m_streams[i].props.value("source_type").toUInt();
        }
        else {
            qInfo() << "stream[" << i << "].props not have source_type";
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
    qInfo() << "FD PipeWire reçu =" << m_pwFd;
    watcher->deleteLater();

    setScreenCastState(ScreenCastState::PipeWireRemoteCreated);
}

void ScreenCastHandler::openThumbnailsPipe()
{
    qInfo() << "[openThumbnailsPipe]";

    qInfo() << "[openThumbnailsPipe] -> pw_context_connect_fd avec FD" << m_pwFd;
    m_pwCore = pw_context_connect_fd(m_pwContext, m_pwFd, nullptr, 0);
    qInfo() << "[openThumbnailsPipe] -> pw_context_connect_fd renvoie core =" << m_pwCore;
    if (!m_pwCore) {
        qWarning() << "[openThumbnailsPipe] -> m_pwCore invalid";
        return;
    }

    m_pwStream = pw_stream_new(m_pwCore, "PrevEveScreenCast", m_StreamProps);
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


    qInfo() << "[openThumbnailsPipe] -> appel pw_stream_connect (ID cible =" << m_streams[0].nodeId << ")";
    int streamConnectResult = pw_stream_connect(
        m_pwStream,
        PW_DIRECTION_INPUT,
        m_streams[0].nodeId,
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
