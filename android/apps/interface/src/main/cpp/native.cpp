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
#include <SettingHandle.h>

#define AUTO_LOGOUT_SETTING_NAME "wallet/autoLogout"
#define WALLET_USERNAME_SETTING_NAME "wallet/savedUsername"

QAndroidJniObject __interfaceActivity;
QAndroidJniObject __loginCompletedListener;
QAndroidJniObject __signupCompletedListener;
QAndroidJniObject __loadCompleteListener;
QAndroidJniObject __usernameChangedListener;
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
        QTextStream(&file) << "touch" << Qt::endl;
        file.close();
    }
}

extern "C" {

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnCreate(JNIEnv* env, jobject instance, jobject asset_mgr) {
    g_assetManager = AAssetManager_fromJava(env, asset_mgr);
    qRegisterMetaType<QAndroidJniObject>("QAndroidJniObject");
    __interfaceActivity = QAndroidJniObject(instance);
    auto oldMessageHandler = qInstallMessageHandler(tempMessageHandler);
    unpackAndroidAssets();
    qInstallMessageHandler(oldMessageHandler);

    JavaVM* jvm;
    env->GetJavaVM(&jvm);

    QObject::connect(&AndroidHelper::instance(), &AndroidHelper::androidActivityRequested, [jvm](const QString& a, const bool backToScene, QMap<QString, QString> args) {
        JNIEnv* myNewEnv;
        JavaVMAttachArgs jvmArgs;
        jvmArgs.version = JNI_VERSION_1_6; // choose your JNI version
        jvmArgs.name = NULL; // you might want to give the java thread a name
        jvmArgs.group = NULL; // you might want to assign the java thread to a ThreadGroup

        int attachedHere = 0; // know if detaching at the end is necessary
        jint res = jvm->GetEnv((void**)&myNewEnv, JNI_VERSION_1_6); // checks if current env needs attaching or it is already attached
        if (JNI_OK != res) {
            qDebug() << "[JCRASH] GetEnv env not attached yet, attaching now..";
            res = jvm->AttachCurrentThread(reinterpret_cast<JNIEnv **>(&myNewEnv), &jvmArgs);
            if (JNI_OK != res) {
                qDebug() << "[JCRASH] Failed to AttachCurrentThread, ErrorCode = " << res;
                return;
            } else {
                attachedHere = 1;
            }
        }

        QAndroidJniObject string = QAndroidJniObject::fromString(a);
        jboolean jBackToScene = (jboolean) backToScene;
        jclass hashMapClass = myNewEnv->FindClass("java/util/HashMap");
        jmethodID mapClassConstructor =  myNewEnv->GetMethodID(hashMapClass, "<init>", "()V");
        jobject hashmap = myNewEnv->NewObject(hashMapClass, mapClassConstructor);
        jmethodID mapClassPut = myNewEnv->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
        QMap<QString, QString>::iterator i;
        for (i = args.begin(); i != args.end(); ++i) {
            QAndroidJniObject jKey = QAndroidJniObject::fromString(i.key());
            QAndroidJniObject jValue = QAndroidJniObject::fromString(i.value());
            myNewEnv->CallObjectMethod(hashmap, mapClassPut, jKey.object<jstring>(), jValue.object<jstring>());
        }
        __interfaceActivity.callMethod<void>("openAndroidActivity", "(Ljava/lang/String;ZLjava/util/HashMap;)V", string.object<jstring>(), jBackToScene, hashmap);
        if (attachedHere) {
            jvm->DetachCurrentThread();
        }
    });

    QObject::connect(&AndroidHelper::instance(), &AndroidHelper::hapticFeedbackRequested, [](int duration) {
        jint iDuration = (jint) duration;
        __interfaceActivity.callMethod<void>("performHapticFeedback", "(I)V", iDuration);
    });
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnDestroy(JNIEnv* env, jobject obj) {
    QObject::disconnect(&AndroidHelper::instance(), &AndroidHelper::androidActivityRequested,
                        nullptr, nullptr);

}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeGotoUrl(JNIEnv* env, jobject obj, jstring url) {
    QAndroidJniObject jniUrl("java/lang/String", "(Ljava/lang/String;)V", url);
    DependencyManager::get<AddressManager>()->loadSettings(jniUrl.toString());
    AndroidHelper::instance().muteMic();
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeGoToUser(JNIEnv* env, jobject obj, jstring username) {
    QAndroidJniObject jniUsername("java/lang/String", "(Ljava/lang/String;)V", username);
    DependencyManager::get<AddressManager>()->goToUser(jniUsername.toString(), false);
    AndroidHelper::instance().muteMic();
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnPause(JNIEnv* env, jobject obj) {
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnResume(JNIEnv* env, jobject obj) {
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnExitVr(JNIEnv* env, jobject obj) {
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

JNIEXPORT jstring JNICALL Java_io_highfidelity_hifiinterface_fragment_HomeFragment_nativeGetLastLocation(JNIEnv *env, jobject instance) {
    Setting::Handle<QUrl> currentAddressHandle(QStringList() << "AddressManager" << "address", QString());
    QUrl lastLocation = currentAddressHandle.get();
    return env->NewStringUTF(lastLocation.toString().toLatin1().data());
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_HifiUtils_updateHifiSetting(JNIEnv *env, jobject instance,
                                                               jstring group_, jstring key_,
                                                               jboolean value_) {
    const char *c_group = env->GetStringUTFChars(group_, 0);
    const char *c_key = env->GetStringUTFChars(key_, 0);

    const QString group = QString::fromUtf8(c_group);
    const QString key = QString::fromUtf8(c_key);

    env->ReleaseStringUTFChars(group_, c_group);
    env->ReleaseStringUTFChars(key_, c_key);

    bool value = value_;

    Setting::Handle<bool> setting { QStringList() << group << key , !value };
    setting.set(value);
}

JNIEXPORT jboolean JNICALL
Java_io_highfidelity_hifiinterface_HifiUtils_getHifiSettingBoolean(JNIEnv *env,
                                                                   jobject instance,
                                                                   jstring group_,
                                                                   jstring key_,
                                                                   jboolean defaultValue) {
    const char *c_group = env->GetStringUTFChars(group_, 0);
    const char *c_key = env->GetStringUTFChars(key_, 0);

    const QString group = QString::fromUtf8(c_group);
    const QString key = QString::fromUtf8(c_key);

    env->ReleaseStringUTFChars(group_, c_group);
    env->ReleaseStringUTFChars(key_, c_key);

    Setting::Handle<bool> setting { QStringList() << group << key , defaultValue};
    return setting.get();
}

JNIEXPORT jboolean JNICALL
Java_io_highfidelity_hifiinterface_HifiUtils_isUserLoggedIn(JNIEnv *env, jobject instance) {
    return DependencyManager::get<AccountManager>()->isLoggedIn();
}

JNIEXPORT jboolean JNICALL
Java_io_highfidelity_hifiinterface_HifiUtils_isKeepingLoggedIn(JNIEnv *env, jobject instance) {
    Setting::Handle<bool> setting(AUTO_LOGOUT_SETTING_NAME, true);
    return !setting.get();
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_fragment_LoginFragment_cancelLogin(JNIEnv *env, jobject instance) {

    auto accountManager = DependencyManager::get<AccountManager>();

    QObject::disconnect(accountManager.data(), &AccountManager::loginComplete, nullptr, nullptr);
    QObject::disconnect(accountManager.data(), &AccountManager::loginFailed, nullptr, nullptr);

}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_fragment_SignupFragment_cancelLogin(JNIEnv *env,
                                                                             jobject instance) {

    Java_io_highfidelity_hifiinterface_fragment_LoginFragment_cancelLogin(env, instance);
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_fragment_LoginFragment_login(JNIEnv *env, jobject instance,
                                                            jstring username_, jstring password_,
                                                            jboolean keepLoggedIn) {
    const char *c_username = env->GetStringUTFChars(username_, 0);
    const char *c_password = env->GetStringUTFChars(password_, 0);
    QString username = QString(c_username);
    QString password = QString(c_password);
    env->ReleaseStringUTFChars(username_, c_username);
    env->ReleaseStringUTFChars(password_, c_password);

    auto accountManager = DependencyManager::get<AccountManager>();

    __loginCompletedListener = QAndroidJniObject(instance);

    QObject::connect(accountManager.data(), &AccountManager::loginComplete, [username, keepLoggedIn](const QUrl& authURL) {
        jboolean jSuccess = (jboolean) true;
        if (__loginCompletedListener.isValid()) {
            __loginCompletedListener.callMethod<void>("handleLoginCompleted", "(Z)V", jSuccess);
        }
        Setting::Handle<QVariant>(AUTO_LOGOUT_SETTING_NAME).set(!keepLoggedIn);
        QString usernameToSave = keepLoggedIn ? username : "";
        Setting::Handle<QVariant>(WALLET_USERNAME_SETTING_NAME).set(usernameToSave);
    });

    QObject::connect(accountManager.data(), &AccountManager::loginFailed, []() {
        jboolean jSuccess = (jboolean) false;
        if (__loginCompletedListener.isValid()) {
            __loginCompletedListener.callMethod<void>("handleLoginCompleted", "(Z)V", jSuccess);
        }
    });

    QMetaObject::invokeMethod(accountManager.data(), "requestAccessToken",
                              Q_ARG(const QString&, username), Q_ARG(const QString&, password));
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_fragment_LoginFragment_retrieveAccessToken(JNIEnv *env,
                                                                                  jobject instance,
                                                                                  jstring authCode_,
                                                                                  jstring clientId_,
                                                                                  jstring clientSecret_,
                                                                                  jstring redirectUri_) {
    const char *c_authCode = env->GetStringUTFChars(authCode_, 0);
    const char *c_clientId = env->GetStringUTFChars(clientId_, 0);
    const char *c_clientSecret = env->GetStringUTFChars(clientSecret_, 0);
    const char *c_redirectUri = env->GetStringUTFChars(redirectUri_, 0);

    QString authCode = QString(c_authCode);
    QString clientId = QString(c_clientId);
    QString clientSecret = QString(c_clientSecret);
    QString redirectUri = QString(c_redirectUri);

    env->ReleaseStringUTFChars(authCode_, c_authCode);
    env->ReleaseStringUTFChars(clientId_, c_clientId);
    env->ReleaseStringUTFChars(clientSecret_, c_clientSecret);
    env->ReleaseStringUTFChars(redirectUri_, c_redirectUri);

    auto accountManager = DependencyManager::get<AccountManager>();

    __loginCompletedListener = QAndroidJniObject(instance); // TODO: use a different listener?

    QObject::connect(accountManager.data(), &AccountManager::loginComplete, [](const QUrl& authURL) {
        jboolean jSuccess = (jboolean) true;
        if (__loginCompletedListener.isValid()) {
            __loginCompletedListener.callMethod<void>("handleLoginCompleted", "(Z)V", jSuccess);
        }
    });

    QObject::connect(accountManager.data(), &AccountManager::loginFailed, []() {
        jboolean jSuccess = (jboolean) false;
        if (__loginCompletedListener.isValid()) {
            __loginCompletedListener.callMethod<void>("handleLoginCompleted", "(Z)V", jSuccess);
        }
    });

    QMetaObject::invokeMethod(accountManager.data(), "requestAccessTokenWithAuthCode",
                              Q_ARG(const QString&, authCode), Q_ARG(const QString&, clientId),
                              Q_ARG(const QString&, clientSecret), Q_ARG(const QString&, redirectUri));

}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_fragment_SignupFragment_login(JNIEnv *env,
                                                                       jobject instance,
                                                                       jstring username_,
                                                                       jstring password_,
                                                                       jboolean keepLoggedIn) {
    Java_io_highfidelity_hifiinterface_fragment_LoginFragment_login(env, instance, username_, password_, keepLoggedIn);
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeInitAfterAppLoaded(JNIEnv* env, jobject obj) {
    AndroidHelper::instance().moveToThread(qApp->thread());
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_fragment_SignupFragment_signup(JNIEnv *env, jobject instance,
                                                                       jstring email_, jstring username_,
                                                                       jstring password_) {

    const char *c_email = env->GetStringUTFChars(email_, 0);
    const char *c_username = env->GetStringUTFChars(username_, 0);
    const char *c_password = env->GetStringUTFChars(password_, 0);
    QString email = QString(c_email);
    QString username = QString(c_username);
    QString password = QString(c_password);
    env->ReleaseStringUTFChars(email_, c_email);
    env->ReleaseStringUTFChars(username_, c_username);
    env->ReleaseStringUTFChars(password_, c_password);

    __signupCompletedListener = QAndroidJniObject(instance);

    // disconnect any previous callback
    QObject::disconnect(&AndroidHelper::instance(), &AndroidHelper::handleSignupCompleted, nullptr, nullptr);
    QObject::disconnect(&AndroidHelper::instance(), &AndroidHelper::handleSignupFailed, nullptr, nullptr);

    QObject::connect(&AndroidHelper::instance(), &AndroidHelper::handleSignupCompleted, []() {
        jboolean jSuccess = (jboolean) true;
        if (__signupCompletedListener.isValid()) {
            __signupCompletedListener.callMethod<void>("handleSignupCompleted", "()V", jSuccess);
        }
    });

    QObject::connect(&AndroidHelper::instance(), &AndroidHelper::handleSignupFailed, [](QString errorString) {
        if (__signupCompletedListener.isValid()) {
            QAndroidJniObject string = QAndroidJniObject::fromString(errorString);
            __signupCompletedListener.callMethod<void>("handleSignupFailed", "(Ljava/lang/String;)V", string.object<jstring>());
        }
    });

    AndroidHelper::instance().signup(email, username, password);
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_fragment_SignupFragment_cancelSignup(JNIEnv *env, jobject instance) {
    QObject::disconnect(&AndroidHelper::instance(), &AndroidHelper::handleSignupCompleted, nullptr, nullptr);
    QObject::disconnect(&AndroidHelper::instance(), &AndroidHelper::handleSignupFailed, nullptr, nullptr);

    __signupCompletedListener = nullptr;
}

JNIEXPORT jstring JNICALL
Java_io_highfidelity_hifiinterface_fragment_FriendsFragment_nativeGetAccessToken(JNIEnv *env, jobject instance) {
    auto accountManager = DependencyManager::get<AccountManager>();
    return env->NewStringUTF(accountManager->getAccountInfo().getAccessToken().token.toLatin1().data());
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

JNIEXPORT jstring JNICALL
Java_io_highfidelity_hifiinterface_fragment_ProfileFragment_getDisplayName(JNIEnv *env,
                                                                           jobject instance) {

    QString displayName = AndroidHelper::instance().getDisplayName();
    return env->NewStringUTF(displayName.toLatin1().data());
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_fragment_ProfileFragment_setDisplayName(JNIEnv *env,
                                                                           jobject instance,
                                                                           jstring name_) {
    const char *c_name = env->GetStringUTFChars(name_, 0);
    const QString name = QString::fromUtf8(c_name);
    env->ReleaseStringUTFChars(name_, c_name);
    AndroidHelper::instance().setDisplayName(name);
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_fragment_ProfileFragment_setAvatarUrl(JNIEnv *env,
                                                                         jobject instance,
                                                                         jstring url_) {
    const char *url = env->GetStringUTFChars(url_, 0);
    QString avatarUrl = QString::fromUtf8(url);
    AndroidHelper::instance().setMyAvatarUrl(avatarUrl);
    env->ReleaseStringUTFChars(url_, url);
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_MainActivity_logout(JNIEnv *env, jobject instance) {
    DependencyManager::get<AccountManager>()->logout();
}

JNIEXPORT jstring JNICALL
Java_io_highfidelity_hifiinterface_MainActivity_getUsername(JNIEnv *env,
                                                                     jobject instance) {
    QString username = DependencyManager::get<AccountManager>()->getAccountInfo().getUsername();
    return env->NewStringUTF(username.toLatin1().data());
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_MainActivity_setUsernameChangedListener(JNIEnv *env,
                                                                           jobject instance,
                                                                           jobject usernameChangedListener) {
    __usernameChangedListener = QAndroidJniObject(usernameChangedListener);

    if (!__usernameChangedListener.isValid()) {
        return;
    }

    auto accountManager = DependencyManager::get<AccountManager>();

    QObject::connect(accountManager.data(), &AccountManager::usernameChanged, [](const QString& username) {
        QAndroidJniObject string = QAndroidJniObject::fromString(username);
        if (__usernameChangedListener.isValid()) {
            __usernameChangedListener.callMethod<void>("handleUsernameChanged", "(Ljava/lang/String;)V", string.object<jstring>());
        }
    });

}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeBeforeEnterBackground(JNIEnv *env, jobject obj) {
    AndroidHelper::instance().notifyBeforeEnterBackground();
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeEnterBackground(JNIEnv *env, jobject obj) {
    AndroidHelper::instance().notifyEnterBackground();
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeEnterForeground(JNIEnv *env, jobject obj) {
    AndroidHelper::instance().notifyEnterForeground();
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_WebViewActivity_nativeProcessURL(JNIEnv* env, jobject obj, jstring url_str) {
    const char *nativeString = env->GetStringUTFChars(url_str, 0);
    AndroidHelper::instance().processURL(QString::fromUtf8(nativeString));
}

JNIEXPORT void JNICALL
Java_io_highfidelity_hifiinterface_receiver_HeadsetStateReceiver_notifyHeadsetOn(JNIEnv *env,
                                                                                 jobject instance,
                                                                                 jboolean pluggedIn) {
    AndroidHelper::instance().notifyHeadsetOn(pluggedIn);
}

}
