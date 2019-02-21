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

#include <OVR_Platform.h>
#include <OVR_Functions_Voip.h>

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

void getClassName(JNIEnv *env, jobject obj){
    jclass cls = env->GetObjectClass(obj);
    jmethodID mid = env->GetMethodID(cls,"getClass", "()Ljava/lang/Class;");
    jobject clsObj = env->CallObjectMethod(obj, mid);

    cls= env->GetObjectClass(clsObj);

    mid= env->GetMethodID(cls, "getName", "()Ljava/lang/String;");

    jstring strObj = (jstring) env->CallObjectMethod(clsObj, mid);

    const char* str = env->GetStringUTFChars(strObj, NULL);

    __android_log_print(ANDROID_LOG_ERROR,__FUNCTION__, "Native Class call: %s",str);

    env->ReleaseStringUTFChars(strObj, str);
}


extern "C" {
    JNIEXPORT void JNICALL
    Java_io_highfidelity_oculus_OculusMobileActivity_nativeInitOculusPlatform(JNIEnv *env, jobject obj){
        initOculusPlatform(env, obj);
    }
QAndroidJniObject __interfaceActivity;

    JNIEXPORT void JNICALL
    Java_io_highfidelity_oculus_OculusMobileActivity_questNativeOnCreate(JNIEnv *env, jobject obj) {
        __android_log_print(ANDROID_LOG_INFO, "QQQ", __FUNCTION__);
        initOculusPlatform(env, obj);
        getClassName(env, obj);

        __interfaceActivity = QAndroidJniObject(obj);

        QObject::connect(&AndroidHelper::instance(), &AndroidHelper::qtAppLoadComplete, []() {
            __interfaceActivity.callMethod<void>("onAppLoadedComplete", "()V");

            QObject::disconnect(&AndroidHelper::instance(), &AndroidHelper::qtAppLoadComplete,
                                nullptr,
                                nullptr);
        });
    }



JNIEXPORT void Java_io_highfidelity_oculus_OculusMobileActivity_questOnAppAfterLoad(JNIEnv* env, jobject obj) {
    AndroidHelper::instance().moveToThread(qApp->thread());
}

    JNIEXPORT void JNICALL
    Java_io_highfidelity_oculus_OculusMobileActivity_questNativeOnPause(JNIEnv *env, jobject obj) {
        AndroidHelper::instance().notifyEnterBackground();
    }

    JNIEXPORT void JNICALL
    Java_io_highfidelity_oculus_OculusMobileActivity_questNativeOnResume(JNIEnv *env, jobject obj) {
        AndroidHelper::instance().notifyEnterForeground();
    }

    JNIEXPORT void JNICALL
    Java_io_highfidelity_questInterface_receiver_HeadsetStateReceiver_notifyHeadsetOn(JNIEnv *env,
                                                                                      jobject instance,
                                                                                      jboolean pluggedIn) {
        AndroidHelper::instance().notifyHeadsetOn(pluggedIn);
    }

}
