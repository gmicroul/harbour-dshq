#ifndef DSHCLIENT_H
#define DSHCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QNetworkAccessManager>
#include <functional>

class DshClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool loadingList READ loadingList NOTIFY loadingChanged)
    Q_PROPERTY(bool loadingHistory READ loadingHistory NOTIFY loadingChanged)
    Q_PROPERTY(bool loadingModels READ loadingModels NOTIFY loadingChanged)

public:
    explicit DshClient(QObject *parent = nullptr);
    ~DshClient() override;

    bool connected() const;
    bool loadingList() const { return m_loadingList; }
    bool loadingHistory() const { return m_loadingHistory; }
    bool loadingModels() const { return m_loadingModels; }

    Q_INVOKABLE void open();
    Q_INVOKABLE void createSession(const QString &cwd);
    Q_INVOKABLE void prompt(const QString &sessionId, const QString &text);
    Q_INVOKABLE void cancelSession(const QString &sessionId);
    Q_INVOKABLE void listSessions();
    Q_INVOKABLE void fetchHistory(const QString &sessionId);
    Q_INVOKABLE void fetchHistoryBefore(const QString &sessionId, qint64 beforeSeq);
    Q_INVOKABLE void fetchModels(const QString &sessionId);
    Q_INVOKABLE void selectModel(const QString &sessionId,
                                 const QString &provider, const QString &model);

signals:
    void connectedChanged();
    void loadingChanged();
    void sessionCreated(const QString &sessionId);
    void textDelta(const QString &sessionId, const QString &delta);
    void reasoningDelta(const QString &sessionId, const QString &delta);
    void turnEnded(const QString &sessionId);
    void turnFailed(const QString &sessionId, const QString &message);
    void requestFailed(const QString &method, const QString &message);
    void sessionListReady(const QVariantList &items);
    // messages: QVariantList of {isUser: bool, text: QString}
    void historyLoaded(const QString &sessionId, const QVariantList &messages,
                       bool hasMore, qint64 oldestSeq);
    void olderPageLoaded(const QString &sessionId, const QVariantList &messages,
                         bool hasMore);
    // providers: [{id, name, models: [{id, name}]}]
    void modelsLoaded(const QString &sessionId, const QVariantList &providers,
                      const QString &curProvider, const QString &curModel, bool routable);
    void modelSelected(const QString &sessionId, const QString &provider,
                       const QString &model);

private:
    void postRpc(const QString &method, const QJsonObject &payload,
                 const std::function<void(bool, const QJsonValue &)> &handler);
    void handleFrame(const QJsonObject &frame);
    void readHistory(const QString &sessionId, qint64 beforeSeq, bool olderPage);
    static QVariantList foldHistory(const QJsonArray &events);

    QWebSocket m_ws;
    QNetworkAccessManager m_nam;
    int m_rpcSeq = 0;
    bool m_loadingList = false;
    bool m_loadingHistory = false;
    bool m_loadingModels = false;
};

#endif
