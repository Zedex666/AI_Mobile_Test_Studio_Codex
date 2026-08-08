#ifndef AI_MOBILE_TEST_STUDIO_CONTROL_PROTOCOL_H
#define AI_MOBILE_TEST_STUDIO_CONTROL_PROTOCOL_H

#include <QByteArray>
#include <QJsonObject>
#include <QString>

namespace studio_control {

constexpr qsizetype kMaximumFrameBytes = 1024 * 1024;

enum class DecodeResult {
    Incomplete,
    Complete,
    Invalid
};

QByteArray encodeFrame(const QJsonObject &message);
DecodeResult decodeNextFrame(QByteArray *buffer, QJsonObject *message, QString *error);

} // namespace studio_control

#endif // AI_MOBILE_TEST_STUDIO_CONTROL_PROTOCOL_H
