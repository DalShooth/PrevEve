#pragma once

#include <QMap>
#include <QObject>
#include <QSizeF>

class ConfigManager final : public QObject
{
    Q_OBJECT
public:
    //= Singleton
    static ConfigManager* Instance() {
        static ConfigManager instance;
        return &instance;
    }
    //=
    //= Singleton - Empêche la copie
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    //=

    void saveThumnailsSize(int width, int height) const;
    QSize loadThumbnailsSize() const;
    QPoint loadThumbnailPosition(const QString &caption) const;
    void saveThumbnailsPositions() const;

private:
    explicit ConfigManager(); // Constructor
};
