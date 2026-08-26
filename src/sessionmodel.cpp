#include "sessionmodel.h"

SessionModel::SessionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SessionModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_sessions.size();
}

QVariant SessionModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_sessions.size())
        return QVariant();
    const SessionEntry &entry = m_sessions.at(index.row());
    switch (role) {
    case RoleSessionId:
        return entry.sessionId;
    case RoleTitle:
        return entry.title.isEmpty() ? QStringLiteral("(untitled)") : entry.title;
    case RoleUpdatedAt:
        return entry.updatedAt;
    case RoleRunning:
        return entry.running;
    case RoleCwd:
        return entry.cwd;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> SessionModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(RoleSessionId, "sessionId");
    roles.insert(RoleTitle, "title");
    roles.insert(RoleUpdatedAt, "updatedAt");
    roles.insert(RoleRunning, "running");
    roles.insert(RoleCwd, "cwd");
    return roles;
}

int SessionModel::count() const
{
    return m_sessions.size();
}

void SessionModel::replaceAll(const QVariantList &items)
{
    beginResetModel();
    m_sessions.clear();
    for (const QVariant &item : items) {
        const QVariantMap map = item.toMap();
        SessionEntry entry;
        entry.sessionId = map.value(QStringLiteral("sessionId")).toString();
        entry.title = map.value(QStringLiteral("title")).toString();
        entry.updatedAt = map.value(QStringLiteral("updatedAt")).toDouble();
        entry.running = map.value(QStringLiteral("running")).toBool();
        entry.cwd = map.value(QStringLiteral("cwd")).toString();
        m_sessions.append(entry);
    }
    endResetModel();
    emit countChanged();
}
