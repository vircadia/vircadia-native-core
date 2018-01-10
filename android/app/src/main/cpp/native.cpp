#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QThread>
#include <QtCore/QStringList>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>

#include <QtAndroidExtras/QAndroidJniObject>

#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

void tempMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (!message.isEmpty()) {
        const char * local=message.toStdString().c_str();
        switch (type) {
            case QtDebugMsg:
                __android_log_write(ANDROID_LOG_DEBUG,"Interface",local);
                break;
            case QtInfoMsg:
                __android_log_write(ANDROID_LOG_INFO,"Interface",local);
                break;
            case QtWarningMsg:
                __android_log_write(ANDROID_LOG_WARN,"Interface",local);
                break;
            case QtCriticalMsg:
                __android_log_write(ANDROID_LOG_ERROR,"Interface",local);
                break;
            case QtFatalMsg:
            default:
                __android_log_write(ANDROID_LOG_FATAL,"Interface",local);
                abort();
        }
    }
}

void unpackAndroidAssets() {
    const QString SOURCE_PREFIX { "assets:/" };
    const QString DEST_PREFIX = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/";
    QStringList filesToCopy;
    QString dateStamp;
    {
        QFile file("assets:/cache_assets.txt");
        if (!file.open(QIODevice::ReadOnly)) {
            throw std::runtime_error("Failed to open cache file list");
        }
        QTextStream in(&file);
        dateStamp = DEST_PREFIX + "/" + in.readLine();
        while (!in.atEnd()) {
            QString line = in.readLine();
            filesToCopy << line;
        }
        file.close();
    }
    qDebug() << "Checking date stamp" << dateStamp;

    if (QFileInfo(dateStamp).exists()) {
        return;
    }

    auto rootDir = QDir::root();
    for (const auto& fileToCopy : filesToCopy) {
        auto sourceFileName = SOURCE_PREFIX + fileToCopy;
        auto destFileName = DEST_PREFIX + fileToCopy;
        auto destFolder = QFileInfo(destFileName).absoluteDir();
        if (!destFolder.exists()) {
            qDebug() << "Creating folder" << destFolder.absolutePath();
            if (!rootDir.mkpath(destFolder.absolutePath())) {
                throw std::runtime_error("Error creating output path");
            }
        }
        if (QFile::exists(destFileName)) {
            qDebug() << "Removing old file" << destFileName;
            if (!QFile::remove(destFileName)) {
                throw std::runtime_error("Failed to remove existing file");
            }
        }

        qDebug() << "Copying from" << sourceFileName << "to" << destFileName;
        if (!QFile(sourceFileName).copy(destFileName)) {
            throw std::runtime_error("Failed to unpack cache files");
        }
    }

    {
        qDebug() << "Writing date stamp" << dateStamp;
        QFile file(dateStamp);
        if (file.open(QIODevice::ReadWrite)) {
            QTextStream(&file) << "touch" << endl;
            file.close();
        }
    }
}

extern "C" {

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnCreate(JNIEnv* env, jobject obj, jobject instance, jobject asset_mgr) {
    qDebug() << "nativeOnCreate On thread " << QThread::currentThreadId();
    auto oldMessageHandler = qInstallMessageHandler(tempMessageHandler);
    unpackAndroidAssets();
    qInstallMessageHandler(oldMessageHandler);
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnPause(JNIEnv* env, jobject obj) {
    qDebug() << "nativeOnPause";
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnResume(JNIEnv* env, jobject obj) {
    qDebug() << "nativeOnResume";
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnExitVr(JNIEnv* env, jobject obj) {
    qDebug() << "nativeOnCreate On thread " << QThread::currentThreadId();
}

}


