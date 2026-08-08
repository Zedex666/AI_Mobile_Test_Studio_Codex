#include "core/workspace_catalog.h"

const QList<WorkspaceDescriptor> &workspaceCatalog()
{
    static const QList<WorkspaceDescriptor> catalog = {
        {QStringLiteral("overview"), QStringLiteral("概览"), 0},
        {QStringLiteral("display"), QStringLiteral("显示"), 1},
        {QStringLiteral("mirroring"), QStringLiteral("镜像"), 2},
        {QStringLiteral("terminal"), QStringLiteral("终端"), 3},
        {QStringLiteral("automation"), QStringLiteral("自动化"), 4},
        {QStringLiteral("device-control"), QStringLiteral("设备控制"), 5},
        {QStringLiteral("packages"), QStringLiteral("软件包管理器"), 6},
        {QStringLiteral("apps"), QStringLiteral("应用"), 7},
        {QStringLiteral("files"), QStringLiteral("文件"), 8},
        {QStringLiteral("recovery"), QStringLiteral("恢复"), 9},
        {QStringLiteral("performance"), QStringLiteral("性能"), 10},
        {QStringLiteral("layout"), QStringLiteral("布局"), 11},
        {QStringLiteral("logcat"), QStringLiteral("日志"), 12},
        {QStringLiteral("other"), QStringLiteral("其它"), 13},
        {QStringLiteral("process"), QStringLiteral("进程"), 14},
        {QStringLiteral("settings"), QStringLiteral("设置"), 15},
    };
    return catalog;
}

int workspaceIndexForId(const QString &id)
{
    for (const WorkspaceDescriptor &workspace : workspaceCatalog()) {
        if (workspace.id == id) {
            return workspace.index;
        }
    }
    return -1;
}

QString workspaceIdForIndex(int index)
{
    for (const WorkspaceDescriptor &workspace : workspaceCatalog()) {
        if (workspace.index == index) {
            return workspace.id;
        }
    }
    return QString();
}
