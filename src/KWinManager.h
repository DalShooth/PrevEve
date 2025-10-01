#pragma once
#include <QDBusInterface>
#include <QTemporaryFile>


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

    static void MakeThumbnailsKeepAbove();
    static void SetWindowPosition(const QString &Caption, int X, int Y);
    static void GetThumbnailsPositions() ;

public slots:
    void GetThumbnailsPositionsResponse(const QString &caption, const int x, const int y) {
        qInfo() << "[GetThumbnailsPositionsResponse] Caption:" << caption << "X:" << x
            << "Y:" << y;
        emit onThumbnailPositionsReceived(caption, x, y);}

Q_SIGNALS:
    void onThumbnailPositionsReceived(const QString &caption, int x, int y);

private:
    //void StopTrackingAllThumbnails();
    /* todo -> pas encore testé, pas besoin pour l'instant grace à
     * todo -> script.setAutoRemove(true), valacary il est moch */
};
