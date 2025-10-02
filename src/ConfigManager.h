#pragma once

#include <QMap>
#include <QObject>
#include <qpoint.h>
#include <QSizeF>
#include "Data/ThumbnailPosition.h"

class ConfigManager final : public QObject
{
    Q_OBJECT
public:
    //= Singleton
    static ConfigManager* Instance() {
        static ConfigManager instance;
        return &instance;
    }
    // Empêche la copie
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    //=

    QSize loadThumbnailsSize() const;
    QStringList loadThumbnailsProfiles();
    QPoint loadThumbnailPosition(const QString &profile) const;

    void saveThumnailsSize(int width, int height) const;
    void saveThumbnailsPositions(const QList<ThumbnailPosition>& thumbnailsPositions) const;
    void saveThumbnailsProfiles(const QStringList &ThumbnailsProfiles) const;

private:
    explicit ConfigManager(); // Constructor
};
