#ifndef AI_MOBILE_TEST_STUDIO_WORKSPACE_CATALOG_H
#define AI_MOBILE_TEST_STUDIO_WORKSPACE_CATALOG_H

#include <QList>
#include <QString>

struct WorkspaceDescriptor {
    QString id;
    QString title;
    int index = -1;
};

const QList<WorkspaceDescriptor> &workspaceCatalog();
int workspaceIndexForId(const QString &id);
QString workspaceIdForIndex(int index);

#endif // AI_MOBILE_TEST_STUDIO_WORKSPACE_CATALOG_H
