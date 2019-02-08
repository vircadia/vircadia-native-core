#include <functional>

#include <QtCore/QBuffer>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QThread>
#include <QtCore/QStringList>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>
#include <QtCore/QObject>

#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroidExtras/QtAndroid>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <shared/Storage.h>

#include <AddressManager.h>
#include <AndroidHelper.h>
#include <udt/PacketHeaders.h>

//#include <OVR_Platform.h>
//#include <OVR_Functions_Voip.h>

void initOculusPlatform(JNIEnv* env, jobject obj) {
    static std::once_flag once;
    std::call_once(once, [&]{
        // static const char* appID = "2343652845669354";
        // if (ovr_PlatformInitializeAndroid(appID, obj, env) != ovrPlatformInitialize_Success) {
        //     __android_log_write(ANDROID_LOG_WARN, "QQQ", "Failed to init platform SDK");
        //     return;
        // }
        // ovr_Voip_SetSystemVoipSuppressed(true);
    });
}

extern "C" {
    JNIEXPORT void JNICALL
    Java_io_highfidelity_questInterface_QuestActivity_nativeInitOculusPlatform(JNIEnv *env, jobject obj){
        initOculusPlatform(env, obj);
    }

QAndroidJniObject __interfaceActivity;

    JNIEXPORT void JNICALL
    Java_io_highfidelity_questInterface_QuestActivity_nativeOnCreate(JNIEnv *env, jobject obj) {
        initOculusPlatform(env, obj);
        __interfaceActivity = QAndroidJniObject(obj);

        if(qApp) {
            QThread *thr = qApp->thread();
        }

        AndroidHelper::instance().moveToThread(thr);


         qRegisterMetaType<QAndroidJniObject>("QAndroidJniObject");

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

    }

    JNIEXPORT void JNICALL
    Java_io_highfidelity_questInterface_QuestActivity_nativeOnDestroy(JNIEnv *env, jobject obj) {
    }

    JNIEXPORT void JNICALL
    Java_io_highfidelity_questInterface_QuestActivity_nativeOnPause(JNIEnv *env, jobject obj) {
        AndroidHelper::instance().notifyEnterBackground();
    }

    JNIEXPORT void JNICALL
    Java_io_highfidelity_questInterface_QuestActivity_nativeOnResume(JNIEnv *env, jobject obj) {
        AndroidHelper::instance().notifyEnterForeground();
    }

    JNIEXPORT void JNICALL
    Java_io_highfidelity_questInterface_receiver_HeadsetStateReceiver_notifyHeadsetOn(JNIEnv *env,
                                                                                      jobject instance,
                                                                                      jboolean pluggedIn) {
        AndroidHelper::instance().notifyHeadsetOn(pluggedIn);
    }

}
