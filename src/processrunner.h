#ifndef PROCESSRUNNER_H
#define PROCESSRUNNER_H

#include <QObject>
#include <QProcess>

class ProcessRunner : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString output READ output NOTIFY outputChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    explicit ProcessRunner(QObject *parent = nullptr);
    ~ProcessRunner() override;

    QString output() const;
    bool running() const;

    Q_INVOKABLE void run(const QStringList &command);
    Q_INVOKABLE void cancel();

signals:
    void outputChanged();
    void runningChanged();
    void finished(int exitCode, const QString &output);

private slots:
    void onFinished(int exitCode, QProcess::ExitStatus status);

private:
    QProcess *takeProcess();
    void cleanupProcess(QProcess *process);

    QProcess *m_process = nullptr;
    QString m_output;
};

#endif
