#pragma once

#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QVariantMap>
#include <pipewire/stream.h>
#include <spa/utils/hook.h>

#include "StreamInfo.h"

class QSocketNotifier;

enum class ScreenCastState {
    Idle,
    CreatingSession,
    SessionCreated,
    SelectingSources,
    SourcesSelected,
    Starting,
    AppSelected,
    OpeningPipeWireRemote,
    PipeWireRemoteCreated,
    Active,
    Error
};

class ScreenCastHandler : public QObject
{
    Q_OBJECT

public:
    // Singleton
    static ScreenCastHandler* instance();

    ScreenCastHandler(const ScreenCastHandler&) = delete;
    ScreenCastHandler& operator=(const ScreenCastHandler&) = delete;
    //

    void setScreenCastState(ScreenCastState NewScreenCastState);

    void init();

    static const char* streamStateToStr(pw_stream_state state);

    signals:
        void videoFrameAvailable(const QImage &image);

public slots:
    void onCreateSessionFinished(QDBusPendingCallWatcher* watcher);
    void onCreateSessionResponse(uint responseCode, const QVariantMap& results);
    void onSelectSourcesFinished(QDBusPendingCallWatcher* watcher);
    void onSelectSourcesResponse(uint code, const QVariantMap &results);
    void onStartFinished(QDBusPendingCallWatcher* watcher);
    void onStartResponse(uint code, const QVariantMap &results);
    void onOpenPipeWireRemoteFinished(QDBusPendingCallWatcher* watcher);

private:
    explicit ScreenCastHandler();
    ~ScreenCastHandler() override;

    void onChangeScreenCastState(ScreenCastState NewScreenCastState);

    void createSession();
    void selectSources();
    void start();
    void openPipeWireRemote();
    void openThumbnailsPipe();

    ScreenCastState m_screenCastState = ScreenCastState::Idle;

    QDBusInterface* m_portal = nullptr;
    QString m_sessionHandle;
    int m_pwFd;
    QList<StreamInfo> m_streams;

    pw_properties *m_StreamProps;
    pw_loop* m_pwLoop;
    pw_context* m_pwContext;
    pw_core* m_pwCore = nullptr;
    QSocketNotifier* m_socketNotifier;
    pw_stream* m_pwStream = nullptr;
    spa_hook m_streamListener;
    uint8_t m_buffer[1024] = {};
    uint32_t m_videoWidth;
    uint32_t m_videoHeight;
};
