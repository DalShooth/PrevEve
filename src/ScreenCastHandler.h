#pragma once

#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QVariantMap>

class ScreenCastHandler : public QObject
{
    Q_OBJECT
public:
    explicit ScreenCastHandler(QObject* parent = nullptr);

    void createSession();

    signals:
        void sessionCreated(const QString& sessionHandle);
    void sourcesSelected(const QString& requestHandle);

private slots:
    void onCreateSessionFinished(QDBusPendingCallWatcher* watcher);
    void onCreateSessionResponse(uint responseCode, const QVariantMap& results);
    void onSelectSourcesFinished(QDBusPendingCallWatcher* watcher);
    void onSelectSourcesResponse(uint code, const QVariantMap &results);
    void onStartFinished(QDBusPendingCallWatcher* watcher);
    void onStartResponse(uint code, const QVariantMap &results);

private:
    QDBusInterface* m_portal;
    QString m_sessionHandle;
};
