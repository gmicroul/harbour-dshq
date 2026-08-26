#include "dshclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QDebug>

static const QString kApiBase = QStringLiteral("http://127.0.0.1:3080/api");

DshClient::DshClient(QObject *parent)
    : QObject(parent)
{
    connect(&m_ws, &QWebSocket::connected, this, [this]() {
        qDebug() << "dsh: websocket connected";
        emit connectedChanged();
    });
    connect(&m_ws, &QWebSocket::disconnected, this, [this]() {
        qDebug() << "dsh: websocket disconnected";
        emit connectedChanged();
    });
    connect(&m_ws,
            static_cast<void (QWebSocket::*)(QAbstractSocket::SocketError)>(&QWebSocket::error),
            this, [this](QAbstractSocket::SocketError) {
        qDebug() << "dsh: websocket error:" << m_ws.errorString();
    });
    connect(&m_ws, &QWebSocket::textMessageReceived,
            this, [this](const QString &message) {
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject())
            return;
        handleFrame(doc.object());
    });
}

DshClient::~DshClient()
{
    m_ws.close();
}

bool DshClient::connected() const
{
    return m_ws.state() == QAbstractSocket::ConnectedState;
}

void DshClient::open()
{
    if (m_ws.state() != QAbstractSocket::UnconnectedState)
        return;
    m_ws.open(QUrl(QStringLiteral("ws://127.0.0.1:3080/api/events.mux")));
}

void DshClient::postRpc(const QString &method, const QJsonObject &payload,
                        const std::function<void(bool, const QJsonValue &)> &handler)
{
    QJsonObject envelope;
    envelope.insert(QStringLiteral("type"), QStringLiteral("client-request"));
    envelope.insert(QStringLiteral("rpcId"),
                    QStringLiteral("q%1").arg(++m_rpcSeq));
    envelope.insert(QStringLiteral("method"), method);
    envelope.insert(QStringLiteral("payload"), payload);

    QNetworkRequest request((QUrl(kApiBase + QLatin1String("/") + method)));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));

    QNetworkReply *reply = m_nam.post(
        request, QJsonDocument(envelope).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, method, handler]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(method, reply->errorString());
            handler(false, QJsonValue());
            return;
        }
        const QJsonDocument doc =
            QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit requestFailed(method, QStringLiteral("malformed response"));
            handler(false, QJsonValue());
            return;
        }
        const QJsonObject response = doc.object();
        const QJsonObject result = response.value(QStringLiteral("result")).toObject();
        const bool ok = result.value(QStringLiteral("ok")).toBool(false);
        const QJsonValue value = result.value(QStringLiteral("value"));
        if (!ok) {
            const QJsonObject error = result.value(QStringLiteral("error")).toObject();
            emit requestFailed(method, error.value(QStringLiteral("message")).toString());
        }
        handler(ok, value);
    });
}

void DshClient::handleFrame(const QJsonObject &frame)
{
    const QJsonObject payload = frame.value(QStringLiteral("payload")).toObject();
    if (payload.value(QStringLiteral("type")).toString()
            != QLatin1String("session/event"))
        return;

    const QString sessionId = payload.value(QStringLiteral("sessionId")).toString();
    const QJsonObject event = payload.value(QStringLiteral("event")).toObject();
    const QString type = event.value(QStringLiteral("type")).toString();

    if (type == QLatin1String("assistant/chunk")) {
        const QJsonObject data = event.value(QStringLiteral("data")).toObject();
        const QJsonObject chunk = data.value(QStringLiteral("chunk")).toObject();
        const QString chunkType = chunk.value(QStringLiteral("type")).toString();
        const QString text = chunk.value(QStringLiteral("text")).toString();
        if (chunkType == QLatin1String("text-delta"))
            emit textDelta(sessionId, text);
        else if (chunkType == QLatin1String("reasoning-delta"))
            emit reasoningDelta(sessionId, text);
    } else if (type == QLatin1String("turn/end")) {
        const QJsonObject data = event.value(QStringLiteral("data")).toObject();
        const QJsonObject reason = data.value(QStringLiteral("reason")).toObject();
        if (reason.value(QStringLiteral("kind")).toString() == QLatin1String("error")) {
            const QString message = reason.value(QStringLiteral("error")).toObject()
                                        .value(QStringLiteral("message")).toString();
            emit turnFailed(sessionId,
                            message.isEmpty() ? QStringLiteral("unknown error") : message);
        }
        emit turnEnded(sessionId);
    }
}

void DshClient::createSession(const QString &cwd)
{
    QJsonObject payload;
    if (!cwd.isEmpty())
        payload.insert(QStringLiteral("cwd"), cwd);
    postRpc(QStringLiteral("session.create"), payload,
            [this](bool ok, const QJsonValue &value) {
        if (ok)
            emit sessionCreated(value.toObject().value(QStringLiteral("sessionId")).toString());
    });
}

void DshClient::prompt(const QString &sessionId, const QString &text)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("sessionId"), sessionId);
    payload.insert(QStringLiteral("mode"), QStringLiteral("queue"));
    QJsonArray content;  // requires QT += network and include
    content.append(QJsonObject({
        { QStringLiteral("type"), QStringLiteral("text") },
        { QStringLiteral("text"), text }
    }));
    payload.insert(QStringLiteral("content"), content);
    payload.insert(QStringLiteral("clientTimeZone"),
                   QStringLiteral("Asia/Shanghai"));
    postRpc(QStringLiteral("session.prompt"), payload,
            [](bool, const QJsonValue &) {});
}

void DshClient::cancelSession(const QString &sessionId)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("sessionId"), sessionId);
    postRpc(QStringLiteral("session.cancel"), payload,
            [](bool, const QJsonValue &) {});
}

void DshClient::listSessions()
{
    if (m_loadingList)
        return;
    m_loadingList = true;
    emit loadingChanged();
    postRpc(QStringLiteral("session.list"), QJsonObject(),
            [this](bool ok, const QJsonValue &value) {
        m_loadingList = false;
        emit loadingChanged();
        if (!ok)
            return;
        QVariantList items;
        const QJsonArray array = value.toObject().value(QStringLiteral("items")).toArray();
        for (const QJsonValue &entry : array) {
            const QJsonObject item = entry.toObject();
            if (item.value(QStringLiteral("blank")).toBool(false))
                continue;  // never-used session: hidden like in the web UI
            // subagent sessions belong to their parent's trajectory, not this list
            if (item.contains(QStringLiteral("parentSessionId")))
                continue;
            QString title = item.value(QStringLiteral("projections")).toObject()
                                .value(QStringLiteral("values")).toObject()
                                .value(QStringLiteral("title")).toString();
            QVariantMap map;
            map.insert(QStringLiteral("sessionId"),
                       item.value(QStringLiteral("sessionId")).toString());
            map.insert(QStringLiteral("title"), title);
            map.insert(QStringLiteral("updatedAt"),
                       item.value(QStringLiteral("updatedAt")).toDouble());
            map.insert(QStringLiteral("running"),
                       item.value(QStringLiteral("running")).toBool(false));
            map.insert(QStringLiteral("cwd"), item.value(QStringLiteral("cwd")).toString());
            items.append(map);
        }
        emit sessionListReady(items);
    });
}

QVariantList DshClient::foldHistory(const QJsonArray &events)
{
    QVariantList messages;
    auto appendMessage = [&messages](bool isUser, const QString &text) {
        if (text.trimmed().isEmpty())
            return;
        QVariantMap map;
        map.insert(QStringLiteral("isUser"), isUser);
        map.insert(QStringLiteral("text"), text);
        messages.append(map);
    };

    for (const QJsonValue &entry : events) {
        const QJsonObject event = entry.toObject().value(QStringLiteral("event")).toObject();
        const QString type = event.value(QStringLiteral("type")).toString();
        const QJsonObject data = event.value(QStringLiteral("data")).toObject();

        if (type == QLatin1String("user/message")) {
            QStringList parts;
            const QJsonArray content = data.value(QStringLiteral("content")).toArray();
            for (const QJsonValue &block : content) {
                const QJsonObject obj = block.toObject();
                if (obj.value(QStringLiteral("type")).toString() == QLatin1String("text"))
                    parts << obj.value(QStringLiteral("text")).toString();
            }
            appendMessage(true, parts.join(QLatin1Char('\n')));
        } else if (type == QLatin1String("assistant/message")) {
            QStringList parts;
            const QJsonArray content = data.value(QStringLiteral("message")).toObject()
                                           .value(QStringLiteral("content")).toArray();
            for (const QJsonValue &block : content) {
                const QJsonObject obj = block.toObject();
                if (obj.value(QStringLiteral("type")).toString() == QLatin1String("text"))
                    parts << obj.value(QStringLiteral("text")).toString();
            }
            appendMessage(false, parts.join(QLatin1Char('\n')));
        }
    }

    return messages;
}

void DshClient::readHistory(const QString &sessionId, qint64 beforeSeq, bool olderPage)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("sessionId"), sessionId);
    if (beforeSeq >= 0)
        payload.insert(QStringLiteral("beforeSeq"), beforeSeq);
    // one page = up to 40 conversation messages worth of raw events
    payload.insert(QStringLiteral("maxMessages"), 40);
    m_loadingHistory = true;
    emit loadingChanged();
    postRpc(QStringLiteral("session.history"), payload,
            [this, sessionId, olderPage](bool ok, const QJsonValue &value) {
        m_loadingHistory = false;
        emit loadingChanged();
        if (!ok)
            return;
        const QJsonObject result = value.toObject();
        const QJsonArray events = result.value(QStringLiteral("events")).toArray();
        const bool hasMore = result.value(QStringLiteral("hasMore")).toBool(false);
        qint64 oldestSeq = -1;
        for (const QJsonValue &entry : events) {
            const qint64 seq = entry.toObject().value(QStringLiteral("event"))
                                   .toObject().value(QStringLiteral("seq")).toDouble();
            if (oldestSeq < 0 || seq < oldestSeq)
                oldestSeq = seq;
        }
        const QVariantList messages = foldHistory(events);
        if (olderPage)
            emit olderPageLoaded(sessionId, messages, hasMore);
        else
            emit historyLoaded(sessionId, messages, hasMore, oldestSeq);
    });
}

void DshClient::fetchHistory(const QString &sessionId)
{
    readHistory(sessionId, -1, false);
}

void DshClient::fetchHistoryBefore(const QString &sessionId, qint64 beforeSeq)
{
    readHistory(sessionId, beforeSeq, true);
}

void DshClient::fetchModels(const QString &sessionId)
{
    if (m_loadingModels)
        return;
    QJsonObject payload;
    payload.insert(QStringLiteral("sessionId"), sessionId);
    m_loadingModels = true;
    emit loadingChanged();
    postRpc(QStringLiteral("session.models"), payload,
            [this, sessionId](bool ok, const QJsonValue &value) {
        m_loadingModels = false;
        emit loadingChanged();
        if (!ok)
            return;
        const QJsonObject obj = value.toObject();
        QVariantList providers;
        const QJsonArray groups = obj.value(QStringLiteral("groups")).toArray();
        for (const QJsonValue &groupValue : groups) {
            const QJsonObject group = groupValue.toObject();
            QVariantList models;
            const QJsonArray modelArray = group.value(QStringLiteral("models")).toArray();
            for (const QJsonValue &modelValue : modelArray) {
                const QJsonObject mo = modelValue.toObject();
                QVariantMap map;
                map.insert(QStringLiteral("id"), mo.value(QStringLiteral("id")).toString());
                map.insert(QStringLiteral("name"), mo.value(QStringLiteral("name")).toString());
                models.append(map);
            }
            QVariantMap provider;
            provider.insert(QStringLiteral("id"), group.value(QStringLiteral("id")).toString());
            provider.insert(QStringLiteral("name"), group.value(QStringLiteral("name")).toString());
            provider.insert(QStringLiteral("models"), models);
            providers.append(provider);
        }
        const QJsonObject current = obj.value(QStringLiteral("current")).toObject();
        emit modelsLoaded(sessionId, providers,
                          current.value(QStringLiteral("provider")).toString(),
                          current.value(QStringLiteral("model")).toString(),
                          obj.value(QStringLiteral("routable")).toBool(false));
    });
}

void DshClient::selectModel(const QString &sessionId,
                            const QString &provider, const QString &model)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("sessionId"), sessionId);
    payload.insert(QStringLiteral("provider"), provider);
    payload.insert(QStringLiteral("model"), model);
    postRpc(QStringLiteral("session.selectModel"), payload,
            [this, sessionId, provider, model](bool ok, const QJsonValue &) {
        if (ok)
            emit modelSelected(sessionId, provider, model);
    });
}
