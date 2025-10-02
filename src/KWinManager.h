#pragma once

#include <QObject>

class KWinManager final : public QObject
{
    Q_OBJECT
public:

    static void setFocusedClient(const QString &clientToFocused);
};
