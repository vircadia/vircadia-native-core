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

    JNIEXPORT void JNICALL
    Java_io_highfidelity_questInterface_QuestActivity_questNativeOnCreate(JNIEnv *env, jobject obj) {
        initOculusPlatform(env, obj);


        if(qApp) {
            QThread *thr = qApp->thread();
            AndroidHelper::instance().moveToThread(thr);
        }
        else{
            __android_log_print(ANDROID_LOG_ERROR,"QQQ_", "APP is not valid");
        }
    }

    JNIEXPORT void JNICALL
    Java_io_highfidelity_questInterface_QuestActivity_questNativeOnDestroy(JNIEnv *env, jobject obj) {
    }

    JNIEXPORT void JNICALL
    Java_io_highfidelity_questInterface_QuestActivity_questNativeOnPause(JNIEnv *env, jobject obj) {
        AndroidHelper::instance().notifyEnterBackground();
    }

    JNIEXPORT void JNICALL
    Java_io_highfidelity_questInterface_QuestActivity_questNativeOnResume(JNIEnv *env, jobject obj) {
        AndroidHelper::instance().notifyEnterForeground();
    }

    JNIEXPORT void JNICALL
    Java_io_highfidelity_questInterface_receiver_HeadsetStateReceiver_notifyHeadsetOn(JNIEnv *env,
                                                                                      jobject instance,
                                                                                      jboolean pluggedIn) {
        AndroidHelper::instance().notifyHeadsetOn(pluggedIn);
    }

}
