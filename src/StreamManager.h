#pragma once

#include <pipewire/pipewire.h>
#include "Data/PortalStreamInfo.h"
#include "Thumbnail.h"


class QDBusInterface;
class EveWPreviewWindow;
class QDBusPendingCallWatcher;
class Thumbnail;
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

class StreamManager final : public QObject
{
    Q_OBJECT
public:
    explicit StreamManager(EveWPreviewWindow* parent);
    ~StreamManager() override {
        qInfo() << "DE-CONSTRUCTOR [StreamManager]";
        ClosePreviews();
    }

    ScreenCastState getScreenCastState() const { return m_StreamState; } // StateMachine

    void SetupPreviews(); /* enclenche la sequence de création de
        la session PipeWire -> commence le stream des applications séléctionnées -> crée les Thumbnails */
    void ClosePreviews(); // Nétoie la session si présente

    QList<QPointer<Thumbnail>> m_ThumbnailsList; // Liste des thumbnails

public slots:
    void onDBusCreateSessionRequestFinished(QDBusPendingCallWatcher* watcher); // Réponse de création de requete de création de session
    void onDBusCreateSessionRequestResponse(uint responseCode, const QVariantMap& results); // Résultat de la requête de création de session
    void onDBusSelectSourcesRequestFinished(QDBusPendingCallWatcher* watcher); // Réponse de création de requête de sélection des sources
    void onDBusSelectSourcesRequestResponse(uint responseCode, const QVariantMap &results); // Résultat de la requête de selection des sources
    void onStartScreensSharingRequestFinished(QDBusPendingCallWatcher* watcher); // Réponse de la création de la requête de démarrage de stream
    void onStartScreensSharingRequestResponse(uint code, const QVariantMap &results); // Résultat de la requête de démarrage de stream (pop-up)
    void onOpenPipeWireConnexionRequestFinished(QDBusPendingCallWatcher* watcher); // Réponse de la création de requête de création de connexion PipeWire

private:
    //= State Machine Linéaire
    ScreenCastState m_StreamState = ScreenCastState::Idle;
    void onChangeScreenCastState();
    void setScreenCastState(ScreenCastState NewScreenCastState); // StateMachine Setter
    //=

    void DBusCreateSessionRequest(); // Requête de création de session
    void DBusSelectSourcesRequest(); // Requête de selection des sources (not the pop-up, only sources settings*)
    void StartScreensSharingRequest(); // Requête de démarrage (open the pop-up)
    void OpenPipeWireConnexionRequest(); // Requête de création de la connexion PipeWire

    QDBusInterface* m_QtDBusScreenCastInterface = nullptr; // Interface D-Bus pour ScreenCast

    QDBusObjectPath m_DBusSessionHandle = {}; // Session D-Bus
    int m_PipeWireFileDescriptor = -1; // Canal de communication vers le serveur PipeWire
    QList<PortalStreamInfo> m_PortalStreamInfoList = {}; // Liste des streams sélectionnés

    pw_loop* m_PipeWireLoop = nullptr; // Boucle d’événements PipeWire
    QSocketNotifier* m_PipeWireLoopSocketNotifier = nullptr; // Notifier PipeWireLoop
    pw_context* m_PipeWireContext = nullptr; /* Représente PrevEve côté serveur PipeWire,
        nécessaire pour la connexion au démon PipeWire (pw-core),
        l’authentification et les permissions, les ressources (streams, nodes, etc.) */
    pw_core* m_PipeWireCore = nullptr; // Connexion unique au démon PipeWire

    signals:
    void onStreamsReady();
};
