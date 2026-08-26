#ifndef CHATMODEL_H
#define CHATMODEL_H

#include <QAbstractListModel>
#include <QStringList>

struct ChatMessage
{
    QString role;
    QString text;
};

class ChatModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        RoleText = Qt::UserRole + 1,
        RoleIsUser,
        RoleStreaming
    };

    explicit ChatModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;

    Q_INVOKABLE void addUserMessage(const QString &text);
    Q_INVOKABLE void beginAssistantTurn();
    Q_INVOKABLE void appendAssistantDelta(const QString &delta);
    Q_INVOKABLE void endAssistantTurn();
    // items: QVariantList of {isUser: bool, text: QString}
    Q_INVOKABLE void loadMessages(const QVariantList &items);
    Q_INVOKABLE void prependMessages(const QVariantList &items);
    Q_INVOKABLE void clear();

signals:
    void countChanged();
    void assistantUpdated();

private:
    QList<ChatMessage> m_messages;
    int m_activeRow = -1;  // assistant row currently receiving streamed deltas
};

#endif  // CHATMODEL_H
