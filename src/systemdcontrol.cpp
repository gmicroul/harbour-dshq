#include "systemdcontrol.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QTimer>

namespace {

const auto kBusService = QStringLiteral("org.freedesktop.systemd1");
const auto kBusPath = QStringLiteral("/org/freedesktop/systemd1");
const auto kManagerInterface = QStringLiteral("org.freedesktop.systemd1.Manager");
const auto kUnitInterface = QStringLiteral("org.freedesktop.systemd1.Unit");
const auto kUnitName = QStringLiteral("dsh-web.service");

} // namespace

SystemdControl::SystemdControl(QObject *parent)
    : QObject(parent)
{
}

QString SystemdControl::state() const
{
    return m_state;
}

bool SystemdControl::busy() const
{
    return m_busy > 0;
}

void SystemdControl::setBusy(int delta)
{
    m_busy += delta;
    if (m_busy < 0)
        m_busy = 0;
    emit busyChanged();
}

void SystemdControl::refresh()
{
    if (!QDBusConnection::sessionBus().isConnected()) {
        emit failed(QStringLiteral("session bus not available"));
        return;
    }

    QDBusMessage call = QDBusMessage::createMethodCall(
        kBusService, kBusPath, kManagerInterface, QStringLiteral("GetUnit"));
    call << kUnitName;

    setBusy(1);
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(call), this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &SystemdControl::unitLookupFinished);
}

void SystemdControl::unitLookupFinished(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    setBusy(-1);

    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        // an unloaded unit is not running; NoSuchUnit therefore means inactive
        if (reply.error().name() == QLatin1String("org.freedesktop.systemd1.NoSuchUnit"))
            m_state = QStringLiteral("inactive");
        else
            m_state = QStringLiteral("unknown");
        emit stateChanged();
        if (m_state == QStringLiteral("unknown"))
            emit failed(reply.error().message());
        return;
    }

    QDBusMessage call = QDBusMessage::createMethodCall(
        kBusService, reply.value().path(),
        QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"));
    call << kUnitInterface << QStringLiteral("ActiveState");

    setBusy(1);
    QDBusPendingCallWatcher *watcher2 =
        new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(call), this);
    connect(watcher2, &QDBusPendingCallWatcher::finished,
            this, &SystemdControl::stateReadFinished);
}

void SystemdControl::stateReadFinished(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    setBusy(-1);

    QDBusPendingReply<QVariant> reply = *watcher;
    if (reply.isError()) {
        emit failed(reply.error().message());
        return;
    }

    m_state = reply.value().toString();
    emit stateChanged();
}

void SystemdControl::start()
{
    runJob(QStringLiteral("StartUnit"));
}

void SystemdControl::stop()
{
    runJob(QStringLiteral("StopUnit"));
}

void SystemdControl::runJob(const QString &method)
{
    if (!QDBusConnection::sessionBus().isConnected()) {
        emit failed(QStringLiteral("session bus not available"));
        return;
    }

    QDBusMessage call = QDBusMessage::createMethodCall(
        kBusService, kBusPath, kManagerInterface, method);
    call << kUnitName << QStringLiteral("replace");

    setBusy(1);
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(call), this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &SystemdControl::jobFinished);
}

void SystemdControl::jobFinished(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    setBusy(-1);

    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        emit failed(reply.error().message());
        return;
    }

    // the job needs a moment; pick up the resulting state shortly
    scheduleRefresh();
}

void SystemdControl::scheduleRefresh()
{
    QTimer::singleShot(1200, this, [this]() { refresh(); });
}
