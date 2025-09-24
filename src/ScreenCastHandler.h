#pragma once

#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QVariantMap>
#include <pipewire/stream.h>
#include <spa/utils/hook.h>

#include "PortalStreamInfo.h"

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

class ScreenCastHandler final : public QObject
{
    Q_OBJECT

public:
    //= Singleton
    static ScreenCastHandler* instance();
    ScreenCastHandler(const ScreenCastHandler&) = delete;
    ScreenCastHandler& operator=(const ScreenCastHandler&) = delete;
    //=

    void setScreenCastState(ScreenCastState NewScreenCastState); // StateMachine Setter

    void init(); // Todo -> changer cette merde

    signals:
        void videoFrameAvailable(const QImage &image);

public slots:
    void onDBusCreateSessionRequestFinished(QDBusPendingCallWatcher* watcher); // Réponse de création de requete de création de session
    void onDBusCreateSessionRequestResponse(uint responseCode, const QVariantMap& results); // Résultat de la requête de création de session
    void onDBusSelectSourcesRequestFinished(QDBusPendingCallWatcher* watcher); // Réponse de création de requête de sélection des sources
    void onDBusSelectSourcesRequestResponse(uint responseCode, const QVariantMap &results); // Résultat de la requête de selection des sources
    void onStartScreensSharingRequestFinished(QDBusPendingCallWatcher* watcher); // Réponse de la création de la requête de démarrage de stream
    void onStartScreensSharingRequestResponse(uint code, const QVariantMap &results); // Résultat de la requête de démarrage de stream (pop-up)
    void onOpenPipeWireConnexionRequestFinished(QDBusPendingCallWatcher* watcher); // Réponse de la création de requête de création de connexion PipeWire

private:
    explicit ScreenCastHandler(); // Constructor
    ~ScreenCastHandler() override; // De-Constructor

    //= State Machine Linéaire
    void onChangeScreenCastState();
    ScreenCastState m_StreamState = ScreenCastState::Idle;
    //=

    void DBusCreateSessionRequest(); // Requête de création de session
    void DBusSelectSourcesRequest(); // Requête de selection des sources (not the pop-up, only sources settings*)
    void StartScreensSharingRequest(); // Requête de démarrage (open the pop-up)
    void OpenPipeWireConnexionRequest(); // Requête de création de la connexion PipeWire

    void openThumbnailsPipe(); // Todo

    QDBusInterface* m_QtDBusInterface; // Interface D-Bus
    pw_properties *m_PipeWireProperties; // Propriétés PipeWire

    QDBusObjectPath m_DBusSessionHandle; // Session D-Bus
    int m_PipeWireFileDescriptor; // Canal de communication vers le serveur PipeWire
    QList<PortalStreamInfo> m_PortalStreamInfoList;

    pw_loop* m_PipeWireLoop; // Boucle d’événements PipeWire
    QSocketNotifier* m_PipeWireLoopSocketNotifier; // Notifier PipeWireLoop
    pw_context* m_PipeWireContext; /* Représente PrevEve côté serveur PipeWire,
    nécessaire pour la connexion au démon PipeWire (pw-core),
    l’authentification et les permissions, les ressources (streams, nodes, etc.) */
    pw_core* m_PipeWireCore = nullptr; // Connexion unique au démon PipeWire

    //= Todo
    pw_stream* m_pwStream = nullptr;
    spa_hook m_streamListener;
    uint8_t m_buffer[1024] = {};
    uint32_t m_videoWidth;
    uint32_t m_videoHeight;
    //

    //= Debug Tools
    static const char* streamStateToStr(pw_stream_state state); // Convert pw_stream_state to String
    //
};
