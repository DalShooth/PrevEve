#pragma once

#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QVariantMap>

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

    void onChangeScreenCastState(ScreenCastState NewScreenCastState);

    void createSession();
    void selectSources();
    void start();
    void openPipeWireRemote();

    ScreenCastState m_screenCastState = ScreenCastState::Idle;
    QDBusInterface* m_portal;
    QString m_sessionHandle;
    int m_pwFd = -1;
};
