#ifndef SYSTEMDCONTROL_H
#define SYSTEMDCONTROL_H

#include <QObject>
#include <QString>

// Controls dsh-web.service through the systemd user instance on the
// session bus. Replaces shelling out to systemctl, which the sailjail
// sandbox forbids (--private-bin leaves no systemctl inside).
class SystemdControl : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit SystemdControl(QObject *parent = nullptr);

    QString state() const;
    bool busy() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

signals:
    void stateChanged();
    void busyChanged();
    void failed(const QString &message);

private slots:
    void unitLookupFinished(class QDBusPendingCallWatcher *watcher);
    void stateReadFinished(class QDBusPendingCallWatcher *watcher);
    void jobFinished(class QDBusPendingCallWatcher *watcher);

private:
    void setBusy(int delta);
    void runJob(const QString &method);
    void scheduleRefresh();

    int m_busy = 0;
    QString m_state = QStringLiteral("unknown");
};

#endif
