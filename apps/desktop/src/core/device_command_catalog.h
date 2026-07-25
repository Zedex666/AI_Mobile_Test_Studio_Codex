#ifndef AI_MOBILE_TEST_STUDIO_DEVICE_COMMAND_CATALOG_H
#define AI_MOBILE_TEST_STUDIO_DEVICE_COMMAND_CATALOG_H

#include <QString>
#include <QVector>

enum class DeviceCommandType {
    KeyEvent,
    Reboot,
    PowerOff
};

struct DeviceCommand {
    QString label;
    QString value;
    DeviceCommandType type = DeviceCommandType::KeyEvent;
};

struct DeviceCommandCategory {
    QString title;
    QString icon;
    QString commandTemplate;
    QVector<DeviceCommand> commands;
};

QVector<DeviceCommandCategory> createDeviceCommandCatalog();

#endif // AI_MOBILE_TEST_STUDIO_DEVICE_COMMAND_CATALOG_H
