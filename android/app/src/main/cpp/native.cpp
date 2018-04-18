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
#include <QObject>

#include <AddressManager.h>
#include "AndroidHelper.h"
#include <udt/PacketHeaders.h>

QAndroidJniObject __interfaceActivity;
QAndroidJniObject __loginActivity;
QAndroidJniObject __loadCompleteListener;

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
    g_assetManager = AAssetManager_fromJava(env, asset_mgr);
    qRegisterMetaType<QAndroidJniObject>("QAndroidJniObject");
    __interfaceActivity = QAndroidJniObject(instance);
    auto oldMessageHandler = qInstallMessageHandler(tempMessageHandler);
    unpackAndroidAssets();
    qInstallMessageHandler(oldMessageHandler);

    QObject::connect(&AndroidHelper::instance(), &AndroidHelper::androidActivityRequested, [](const QString& a) {
        QAndroidJniObject string = QAndroidJniObject::fromString(a);
        __interfaceActivity.callMethod<void>("openGotoActivity", "(Ljava/lang/String;)V", string.object<jstring>());
    });
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnDestroy(JNIEnv* env, jobject obj) {
    QObject::disconnect(&AndroidHelper::instance(), &AndroidHelper::androidActivityRequested,
                        nullptr, nullptr);

}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeGotoUrl(JNIEnv* env, jobject obj, jstring url) {
    QAndroidJniObject jniUrl("java/lang/String", "(Ljava/lang/String;)V", url);
    DependencyManager::get<AddressManager>()->handleLookupString(jniUrl.toString());
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnPause(JNIEnv* env, jobject obj) {
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnResume(JNIEnv* env, jobject obj) {
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnExitVr(JNIEnv* env, jobject obj) {
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeGoBackFromAndroidActivity(JNIEnv *env, jobject instance) {
    AndroidHelper::instance().goBackFromAndroidActivity();
}

// HifiUtils
JNIEXPORT jstring JNICALL Java_io_highfidelity_hifiinterface_HifiUtils_getCurrentAddress(JNIEnv *env, jobject instance) {
    QSharedPointer<AddressManager> addressManager = DependencyManager::get<AddressManager>();
    if (!addressManager) {
        return env->NewString(nullptr, 0);
    }

    QString str;
    if (!addressManager->getPlaceName().isEmpty()) {
        str = addressManager->getPlaceName();
    } else if (!addressManager->getHost().isEmpty()) {
        str = addressManager->getHost();
    }

    QRegExp pathRegEx("(\\/[^\\/]+)");
    if (!addressManager->currentPath().isEmpty() && addressManager->currentPath().contains(pathRegEx) && pathRegEx.matchedLength() > 0) {
        str += pathRegEx.cap(0);
    }

    return env->NewStringUTF(str.toLatin1().data());
}

JNIEXPORT jstring JNICALL Java_io_highfidelity_hifiinterface_HifiUtils_protocolVersionSignature(JNIEnv *env, jobject instance) {
    return env->NewStringUTF(protocolVersionsSignatureBase64().toLatin1().data());
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_LoginActivity_nativeLogin(JNIEnv *env, jobject instance,
                                                            jstring username_, jstring password_) {
    const char *c_username = env->GetStringUTFChars(username_, 0);
    const char *c_password = env->GetStringUTFChars(password_, 0);
    QString username = QString(c_username);
    QString password = QString(c_password);
    env->ReleaseStringUTFChars(username_, c_username);
    env->ReleaseStringUTFChars(password_, c_password);

    QSharedPointer<AccountManager> accountManager = AndroidHelper::instance().getAccountManager();

    __loginActivity = QAndroidJniObject(instance);

    QObject::connect(accountManager.data(), &AccountManager::loginComplete, [](const QUrl& authURL) {
        AndroidHelper::instance().notifyLoginComplete(true);
    });

    QObject::connect(accountManager.data(), &AccountManager::loginFailed, []() {
        AndroidHelper::instance().notifyLoginComplete(false);
    });

    QObject::connect(&AndroidHelper::instance(), &AndroidHelper::loginComplete, [](bool success) {
        jboolean jSuccess = (jboolean) success;
        __loginActivity.callMethod<void>("handleLoginCompleted", "(Z)V", jSuccess);
    });

    QMetaObject::invokeMethod(accountManager.data(), "requestAccessToken", Q_ARG(const QString&, username), Q_ARG(const QString&, password));
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_SplashActivity_registerLoadCompleteListener(JNIEnv *env,
                                                                               jobject instance) {

    __loadCompleteListener = QAndroidJniObject(instance);

    QObject::connect(&AndroidHelper::instance(), &AndroidHelper::qtAppLoadComplete, []() {

        __loadCompleteListener.callMethod<void>("onAppLoadedComplete", "()V");
        __interfaceActivity.callMethod<void>("onAppLoadedComplete", "()V");

        QObject::disconnect(&AndroidHelper::instance(), &AndroidHelper::qtAppLoadComplete, nullptr,
                            nullptr);
    });

}
JNIEXPORT jboolean JNICALL
Java_io_highfidelity_hifiinterface_HomeActivity_nativeIsLoggedIn(JNIEnv *env, jobject instance) {
    return AndroidHelper::instance().getAccountManager()->isLoggedIn();
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_HomeActivity_nativeLogout(JNIEnv *env, jobject instance) {
    AndroidHelper::instance().getAccountManager()->logout();
}

JNIEXPORT jstring JNICALL
Java_io_highfidelity_hifiinterface_HomeActivity_nativeGetDisplayName(JNIEnv *env,
                                                                     jobject instance) {
    QString username = AndroidHelper::instance().getAccountManager()->getAccountInfo().getUsername();
    return env->NewStringUTF(username.toLatin1().data());
}

}
