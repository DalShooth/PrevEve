#pragma once

#include <QMap>
#include <QObject>
#include <QSizeF>

class ConfigManager final : public QObject
{
    Q_OBJECT
public:
    //= Singleton
    static ConfigManager& Instance() {
        static ConfigManager Instance;
        return Instance;
    }
    //=

    void saveThumnailsSize(int width, int height) const;
    QSize loadThumbnailsSize() const;


    // todo -> fonction ecris les position de thumbnails dans le .conf
    QPoint loadThumbnailPosition(const QString &caption) const;

private:
    explicit ConfigManager(); // Constructor

    //= Singleton - Empêche la copie
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    //=
};
