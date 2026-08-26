#include "chatmodel.h"

ChatModel::ChatModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ChatModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_messages.size();
}

QVariant ChatModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_messages.size())
        return QVariant();
    const ChatMessage &message = m_messages.at(index.row());
    switch (role) {
    case RoleText:
        return message.text;
    case RoleIsUser:
        return message.role == QStringLiteral("user");
    case RoleStreaming:
        return index.row() == m_activeRow;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ChatModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(RoleText, "text");
    roles.insert(RoleIsUser, "isUser");
    roles.insert(RoleStreaming, "streaming");
    return roles;
}

int ChatModel::count() const
{
    return m_messages.size();
}

void ChatModel::addUserMessage(const QString &text)
{
    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append({ QStringLiteral("user"), text });
    endInsertRows();
    emit countChanged();
}

void ChatModel::beginAssistantTurn()
{
    if (m_activeRow >= 0)
        return;
    const int row = m_messages.size();
    beginInsertRows(QModelIndex(), row, row);
    m_messages.append({ QStringLiteral("assistant"), QString() });
    endInsertRows();
    m_activeRow = row;
    emit countChanged();
}

void ChatModel::appendAssistantDelta(const QString &delta)
{
    if (m_activeRow < 0)
        beginAssistantTurn();
    m_messages[m_activeRow].text += delta;
    const QModelIndex modelIndex = index(m_activeRow);
    emit dataChanged(modelIndex, modelIndex,
                     QVector<int>() << RoleText << RoleStreaming);
    emit assistantUpdated();
}

void ChatModel::endAssistantTurn()
{
    if (m_activeRow < 0)
        return;
    const QModelIndex modelIndex = index(m_activeRow);
    m_activeRow = -1;
    emit dataChanged(modelIndex, modelIndex, QVector<int>() << RoleStreaming);
}

void ChatModel::clear()
{
    beginResetModel();
    m_messages.clear();
    m_activeRow = -1;
    endResetModel();
    emit countChanged();
}

void ChatModel::loadMessages(const QVariantList &items)
{
    beginResetModel();
    m_messages.clear();
    for (const QVariant &item : items) {
        const QVariantMap map = item.toMap();
        m_messages.append({
            map.value(QStringLiteral("isUser")).toBool()
                    ? QStringLiteral("user") : QStringLiteral("assistant"),
            map.value(QStringLiteral("text")).toString()
        });
    }
    m_activeRow = -1;
    endResetModel();
    emit countChanged();
}

void ChatModel::prependMessages(const QVariantList &items)
{
    if (items.isEmpty())
        return;
    const int insertCount = items.size();
    beginInsertRows(QModelIndex(), 0, insertCount - 1);
    for (int i = insertCount - 1; i >= 0; --i) {
        const QVariantMap map = items.at(i).toMap();
        m_messages.prepend({
            map.value(QStringLiteral("isUser")).toBool()
                    ? QStringLiteral("user") : QStringLiteral("assistant"),
            map.value(QStringLiteral("text")).toString()
        });
    }
    endInsertRows();
    emit countChanged();
}
