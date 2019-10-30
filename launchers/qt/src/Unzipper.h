#pragma once

#include <QString>
#include <QDir>
#include <QObject>
#include <QRunnable>

class Unzipper : public QObject, public QRunnable {
    Q_OBJECT
public:
    Unzipper(const QString& zipFilePath, const QDir& outputDirectory);
    void run() override;

signals:
    void progress(float progress);
    void finished(bool error, QString errorMessage);

private:
    const QString _zipFilePath;
    const QDir _outputDirectory;
};
