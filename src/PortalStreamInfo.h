#pragma once

#include <qdbusargument.h>
#include <QMetaType>


/* Structure pour analyser le contenu de QDBusArgument sur les stream (results.value <- onStartResponse)
 *
 *
 */

struct PortalStreamInfo {
    uint nodeId;
    QVariantMap props;
};
Q_DECLARE_METATYPE(PortalStreamInfo)
Q_DECLARE_METATYPE(QList<PortalStreamInfo>)

inline const QDBusArgument &operator>>(const QDBusArgument &arg, PortalStreamInfo &s)
{
    arg.beginStructure();
    arg >> s.nodeId >> s.props;
    arg.endStructure();
    return arg;
}

inline QDBusArgument &operator<<(QDBusArgument &arg, const PortalStreamInfo &s)
{
    arg.beginStructure();
    arg << s.nodeId << s.props;
    arg.endStructure();
    return arg;
}
