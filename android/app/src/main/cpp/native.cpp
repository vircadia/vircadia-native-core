#include <functional>


#include <QtCore/QBuffer>
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

#include <shared/Storage.h>

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

AAssetManager* g_assetManager = nullptr;

void withAssetData(const char* filename, const std::function<void(off64_t, const void*)>& callback) {
    auto asset = AAssetManager_open(g_assetManager, filename, AASSET_MODE_BUFFER);
    if (!asset) {
        throw std::runtime_error("Failed to open file");
    }
    auto buffer = AAsset_getBuffer(asset);
    off64_t size = AAsset_getLength64(asset);
    callback(size, buffer);
    AAsset_close(asset);
}

QStringList readAssetLines(const char* filename) {
    QStringList result;
    withAssetData(filename, [&](off64_t size, const void* data){
        QByteArray buffer = QByteArray::fromRawData((const char*)data, size);
        {
            QTextStream in(&buffer);
            while (!in.atEnd()) {
                QString line = in.readLine();
                result << line;
            }
        }
    });
    return result;
}

void copyAsset(const char* sourceAsset, const QString& destFilename) {
    withAssetData(sourceAsset, [&](off64_t size, const void* data){
        QFile file(destFilename);
        if (!file.open(QFile::ReadWrite | QIODevice::Truncate)) {
            throw std::runtime_error("Unable to open output file for writing");
        }
        if (!file.resize(size)) {
            throw std::runtime_error("Unable to resize output file");
        }
        if (size != 0) {
            auto mapped = file.map(0, size);
            if (!mapped) {
                throw std::runtime_error("Unable to map output file");
            }
            memcpy(mapped, data, size);
        }
        file.close();
    });
}

void unpackAndroidAssets() {
    const QString DEST_PREFIX = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/";

    QStringList filesToCopy = readAssetLines("cache_assets.txt");
    QString dateStamp = filesToCopy.takeFirst();
    QString dateStampFilename = DEST_PREFIX + dateStamp;
    qDebug() << "Checking date stamp" << dateStamp;
    if (QFileInfo(dateStampFilename).exists()) {
        return;
    }

    auto rootDir = QDir::root();
    for (const auto& fileToCopy : filesToCopy) {
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

        qDebug() << "Copying asset " << fileToCopy << "to" << destFileName;
        copyAsset(fileToCopy.toStdString().c_str(), destFileName);
    }

    {
        qDebug() << "Writing date stamp" << dateStamp;
        QFile file(dateStampFilename);
        if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
            throw std::runtime_error("Can't write date stamp");
        }
        QTextStream(&file) << "touch" << endl;
        file.close();
    }
}

extern "C" {

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnCreate(JNIEnv* env, jobject obj, jobject instance, jobject asset_mgr) {
    qDebug() << "nativeOnCreate On thread " << QThread::currentThreadId();
    g_assetManager = AAssetManager_fromJava(env, asset_mgr);
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


