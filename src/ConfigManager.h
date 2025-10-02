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
    QStringList loadCharacters();
    QPoint loadThumbnailPosition(const QString &character) const;

    void saveThumnailsSize(int width, int height) const;
    void saveThumbnailsPositions(const QList<ThumbnailPosition>& thumbnailsPositions) const;
    void saveCharacters(const QStringList &saveCharacters) const;

private:
    explicit ConfigManager(); // Constructor
};
