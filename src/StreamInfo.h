#pragma once

#include <qdbusargument.h>
#include <qtypes.h>

struct StreamInfo {
    uint nodeId;
    QVariantMap props;
};
Q_DECLARE_METATYPE(StreamInfo)
Q_DECLARE_METATYPE(QList<StreamInfo>)

inline const QDBusArgument &operator>>(const QDBusArgument &arg, StreamInfo &s)
{
    arg.beginStructure();
    arg >> s.nodeId >> s.props;
    arg.endStructure();
    return arg;
}

inline QDBusArgument &operator<<(QDBusArgument &arg, const StreamInfo &s)
{
    arg.beginStructure();
    arg << s.nodeId << s.props;
    arg.endStructure();
    return arg;
}
