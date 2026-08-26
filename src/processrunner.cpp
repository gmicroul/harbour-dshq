#include "processrunner.h"

ProcessRunner::ProcessRunner(QObject *parent)
    : QObject(parent)
{
}

ProcessRunner::~ProcessRunner()
{
    cancel();
    if (m_process) {
        m_process->waitForFinished(1000);
        delete m_process;
        m_process = nullptr;
    }
}

QString ProcessRunner::output() const
{
    return m_output;
}

bool ProcessRunner::running() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void ProcessRunner::run(const QStringList &command)
{
    if (running())
        return;

    delete m_process;
    m_process = new QProcess(this);
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(onFinished(int,QProcess::ExitStatus)));
    connect(m_process, &QProcess::stateChanged, this, [this]() {
        emit runningChanged();
    });

    m_output.clear();
    emit outputChanged();
    emit runningChanged();

    m_process->start(command.first(), command.mid(1));
    if (!m_process->waitForStarted(3000)) {
        m_output = QStringLiteral("failed to start: ") + command.join(QLatin1Char(' '));
        emit outputChanged();
        emit finished(-1, m_output);
        cleanupProcess(m_process);
    }
}

void ProcessRunner::cancel()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(2000);
    }
}

void ProcessRunner::onFinished(int exitCode, QProcess::ExitStatus status)
{
    if (status == QProcess::CrashExit) {
        m_output = tr("process crashed");
    } else {
        m_output = QString::fromUtf8(m_process->readAllStandardOutput()).trimmed();
        if (m_output.isEmpty())
            m_output = QString::fromUtf8(m_process->readAllStandardError()).trimmed();
    }
    emit outputChanged();
    emit finished(exitCode, m_output);
    cleanupProcess(m_process);
}

QProcess *ProcessRunner::takeProcess()
{
    QProcess *p = m_process;
    m_process = nullptr;
    emit runningChanged();
    return p;
}

void ProcessRunner::cleanupProcess(QProcess *process)
{
    if (m_process == process) {
        m_process = nullptr;
        emit runningChanged();
    }
    process->deleteLater();
}
