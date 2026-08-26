#ifndef SESSIONMODEL_H
#define SESSIONMODEL_H

#include <QAbstractListModel>
#include <QVariantList>

struct SessionEntry
{
    QString sessionId;
    QString title;
    double updatedAt = 0;
    bool running = false;
    QString cwd;
};

class SessionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        RoleSessionId = Qt::UserRole + 1,
        RoleTitle,
        RoleUpdatedAt,
        RoleRunning,
        RoleCwd
    };

    explicit SessionModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;

    // items: QVariantList of {sessionId, title, updatedAt, running, cwd}
    Q_INVOKABLE void replaceAll(const QVariantList &items);

signals:
    void countChanged();

private:
    QList<SessionEntry> m_sessions;
};

#endif  // SESSIONMODEL_H
