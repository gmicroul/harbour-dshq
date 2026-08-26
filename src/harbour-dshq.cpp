#include <QCoreApplication>
#include <QGuiApplication>
#include <QFileInfo>
#include <QQmlEngine>
#include <QQuickView>
#include <QTimer>
#include <QImage>
#include <QDebug>
#include <QUrl>
#include <cstdio>

#include "chatmodel.h"
#include "dshclient.h"
#include "processrunner.h"
#include "sessionmodel.h"
#include "systemdcontrol.h"

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsSet("DSHQ_AUTOTEST")) {
        QCoreApplication app(argc, argv);

        if (qEnvironmentVariableIsSet("DSHQ_AUTOTEST_HISTORY")) {
            // Headless check of the history features: session.list + session.history
            DshClient client;
            QVariantList sessions;
            int probeIndex = 0;
            int failures = 0;
            bool sawAnyUser = false;

            auto finalize = [&]() {
                qInfo("self test %s", (failures == 0 && sawAnyUser) ? "PASSED" : "FAILED");
                QCoreApplication::exit(failures == 0 && sawAnyUser ? 0 : 4);
            };

            QObject::connect(&client, &DshClient::requestFailed,
                    [&failures](const QString &method, const QString &message) {
                qWarning("rpc %s failed: %s", qPrintable(method), qPrintable(message));
                failures++;
            });
            QObject::connect(&client, &DshClient::sessionListReady,
                    [&](const QVariantList &items) {
                sessions = items;
                qInfo("session.list: %lld non-blank entries", static_cast<long long>(items.size()));
                if (items.isEmpty()) {
                    QCoreApplication::exit(3);
                    return;
                }
                const QVariantMap first = items.first().toMap();
                qInfo("first: %s (%s)", qPrintable(first.value("title").toString()),
                      qPrintable(first.value("sessionId").toString()));
                client.fetchHistory(first.value(QStringLiteral("sessionId")).toString());
            });
            QObject::connect(&client, &DshClient::historyLoaded,
                    [&](const QString &sessionId, const QVariantList &messages,
                        bool hasMore, qint64 oldestSeq) {
                qInfo("history %s: %lld bubbles, hasMore=%d, oldestSeq=%lld",
                      qPrintable(sessionId), static_cast<long long>(messages.size()),
                      static_cast<int>(hasMore), static_cast<long long>(oldestSeq));
                if (messages.isEmpty())
                    failures++;
                for (const QVariant &m : messages) {
                    if (!m.toMap().value(QStringLiteral("text")).toString().isEmpty()
                            && m.toMap().value(QStringLiteral("isUser")).toBool())
                        sawAnyUser = true;
                }
                if (!hasMore || oldestSeq <= 0) {
                    // short session: try the next one for a paginated case
                    if (++probeIndex < sessions.size() && probeIndex < 8) {
                        client.fetchHistory(sessions.at(probeIndex)
                                            .toMap().value(QStringLiteral("sessionId")).toString());
                        return;
                    }
                    finalize();
                    return;
                }
                QObject::connect(&client, &DshClient::olderPageLoaded,
                        [&](const QString &sid, const QVariantList &older, bool more) {
                    qInfo("older page %s: %lld bubbles, hasMore=%d",
                          qPrintable(sid), static_cast<long long>(older.size()),
                          static_cast<int>(more));
                    if (older.isEmpty())
                        failures++;
                    finalize();
                });
                client.fetchHistoryBefore(sessionId, oldestSeq);
            });

            QTimer::singleShot(30000, []() {
                qWarning("history self test timeout");
                QCoreApplication::exit(2);
            });

            client.open();
            client.listSessions();
            return app.exec();
        }

        if (qEnvironmentVariableIsSet("DSHQ_AUTOTEST_MODELS")) {
            // Headless check of model switching: session.models + session.selectModel
            DshClient client;
            QString sessionId;
            QString provider;
            QString model;
            int failures = 0;

            QObject::connect(&client, &DshClient::requestFailed,
                    [&failures](const QString &method, const QString &message) {
                qWarning("rpc %s failed: %s", qPrintable(method), qPrintable(message));
                failures++;
            });
            QObject::connect(&client, &DshClient::sessionCreated,
                    [&](const QString &id) {
                sessionId = id;
                qInfo("session: %s", qPrintable(id));
                client.fetchModels(id);
            });
            QObject::connect(&client, &DshClient::modelsLoaded,
                    [&](const QString &sid, const QVariantList &groups,
                        const QString &curProvider, const QString &curModel, bool routable) {
                int total = 0;
                for (const QVariant &g : groups)
                    total += g.toMap().value(QStringLiteral("models")).toList().size();
                qInfo("models: routable=%d groups=%lld models=%d current=%s/%s",
                      static_cast<int>(routable), static_cast<long long>(groups.size()),
                      total, qPrintable(curProvider), qPrintable(curModel));
                if (!routable || groups.isEmpty() || curModel.isEmpty())
                    failures++;
                provider = curProvider;
                model = curModel;
                client.selectModel(sid, provider, model);
            });
            QObject::connect(&client, &DshClient::modelSelected,
                    [&](const QString &, const QString &selProvider, const QString &selModel) {
                if (selProvider != provider || selModel != model)
                    failures++;
                qInfo("selected: %s/%s", qPrintable(selProvider), qPrintable(selModel));
                qInfo("self test %s", failures == 0 ? "PASSED" : "FAILED");
                QCoreApplication::exit(failures == 0 ? 0 : 5);
            });

            QTimer::singleShot(30000, []() {
                qWarning("models self test timeout");
                QCoreApplication::exit(2);
            });

            client.open();
            client.createSession(QStringLiteral("/tmp/opencode"));
            return app.exec();
        }

        DshClient client;
        QString sessionId;
        QString collected;

        QObject::connect(&client, &DshClient::requestFailed,
                [](const QString &method, const QString &message) {
            qWarning("rpc %s failed: %s", qPrintable(method), qPrintable(message));
        });
        QObject::connect(&client, &DshClient::sessionCreated,
                [&](const QString &id) {
            sessionId = id;
            qInfo("session: %s", qPrintable(id));
            client.prompt(id, QStringLiteral("reply with exactly: ok"));
        });
        QObject::connect(&client, &DshClient::textDelta,
                [&](const QString &, const QString &delta) {
            collected += delta;
        });
        QObject::connect(&client, &DshClient::turnEnded, [&](const QString &) {
            qInfo("final text: %s", qPrintable(collected));
            QCoreApplication::exit(collected.contains(QStringLiteral("ok")) ? 0 : 3);
        });

        QTimer::singleShot(60000, [&]() {
            qWarning("self test timeout");
            QCoreApplication::exit(2);
        });

        client.open();
        client.createSession(QStringLiteral("/tmp/opencode"));
        return app.exec();
    }

    QGuiApplication application(argc, argv);

    qmlRegisterType<ChatModel>("Dsh", 1, 0, "ChatModel");
    qmlRegisterType<DshClient>("Dsh", 1, 0, "DshClient");
    qmlRegisterType<ProcessRunner>("Dsh", 1, 0, "ProcessRunner");
    qmlRegisterType<SessionModel>("Dsh", 1, 0, "SessionModel");
    qmlRegisterType<SystemdControl>("Dsh", 1, 0, "SystemdControl");

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);

    const QString shotDir = qgetenv("DSHQ_SHOT");
    if (!shotDir.isEmpty()) {
        static int shotSeq = 0;
        auto *shotTimer = new QTimer(&view);
        QObject::connect(shotTimer, &QTimer::timeout, &view, [&view, shotDir]() {
            const QImage image = view.grabWindow();
            static int failures = 0;
            if (!image.isNull()) {
                const QString path = QStringLiteral("%1/shot-%2.png")
                                         .arg(shotDir).arg(++shotSeq, 3, 10, QLatin1Char('0'));
                const bool ok = image.save(path);
                qDebug() << "dsh: shot" << path << (ok ? "saved" : "save FAILED");
            } else if (++failures % 5 == 1) {
                qDebug() << "dsh: grabWindow returned null";
            }
        });
        shotTimer->start(2000);
    }

    QString qmlMain = QStringLiteral("/usr/share/%1/qml/harbour-dshq.qml")
                          .arg(application.applicationName());
    if (!QFileInfo::exists(qmlMain))
        qmlMain = QStringLiteral("qml/harbour-dshq.qml");
    view.setSource(QUrl::fromLocalFile(qmlMain));

    view.show();
    return application.exec();
}
