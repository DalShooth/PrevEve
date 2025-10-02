#pragma once

#include <qdbusargument.h>
#include <QDBusInterface>
#include <QTemporaryFile>
#include "Data/ThumbnailPosition.h"

class KWinManager final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.example.EveWPreview")
public:
    explicit KWinManager(QObject* parent = nullptr);

    //= Singleton
    static KWinManager* Instance() {
        static KWinManager instance;
        return &instance;
    }
    // Empêche la copie
    KWinManager(const KWinManager&) = delete;
    KWinManager& operator=(const KWinManager&) = delete;
    //=

    static void GetThumbnailsPositions();
    static void setFocusedClient(const QString &clientToFocused);

static bool callKWin(const char* method, const QList<QVariant>& args)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.kde.KWin", "/KWin", "org.kde.KWin", method);
    msg.setArguments(args);

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 3000);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qCritical() << "[DBus] KWin" << method << "args=" << args
                    << "error=" << reply.errorName() << reply.errorMessage();
        return false;
    }

    qInfo() << "[DBus] KWin" << method << "OK";
    return true;
}


public slots:
    // Fonction réponse à la requête GetThumbnailsPositions du script KWin
    void GetThumbnailsPositionsResponse(const QVariantList& positions) {
        qInfo() << "[GetThumbnailsPositionsResponse]" << positions;

        if (positions.size() >= 3) {
            QList<ThumbnailPosition> thumbnailsPositions;
            QString raw = positions[0].toString();
            QString profile = raw.split("Thumbnail-").last();

            int x = positions[1].toInt();
            int y = positions[2].toInt();

            qInfo() << "Profile:" << profile << "X:" << x << "Y:" << y;

            thumbnailsPositions.append({ profile, QPoint(x, y) });

            emit onThumbnailsPositionsReceived(thumbnailsPositions); // Emet le signal de récéption
        } else {
            qWarning() << "[GetThumbnailsPositionsResponse] entry invalide:" << positions;
        }
    }

Q_SIGNALS:
    // Signal de récéption des positions de thumbnails/profile
    void onThumbnailsPositionsReceived(QList<ThumbnailPosition> thumbnailsPositions);
};
