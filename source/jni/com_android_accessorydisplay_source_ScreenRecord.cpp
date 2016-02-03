#define LOG_TAG "screenrecord_jni"
//#define LOG_NDEBUG 0

#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"

#include <utils/Log.h>

#include "include/libscreenrecord.h"

extern int screen_record_main(int argc, char * const argv[]);

#define ERROR_LOG()     ALOGE("Error at [%s] line[%d]", __FUNCTION__, __LINE__)

#if (LOG_NDEBUG == 0)
#define CheckErr(ret) do {         \
    if (ret < 0) {            \
        ALOGE("Error at [%s] line[%d]", __FUNCTION__, __LINE__);  \
    }   \
} while(0)
#else
#define CheckErr(ret)
#endif

struct fields_t {
    jmethodID   notify_update;
};
static struct fields_t fields;

namespace android {

// ref-counted object for callbacks
class JNIContext
{
public:
    explicit JNIContext(JNIEnv* env, jobject thiz, jobject weak_thiz);
    ~JNIContext();
    void setNativeContext(void* nativeContext) { mNativeContext = nativeContext; }
    void *getNativeContext() { return mNativeContext; }
    jclass getClass() { return mClass; }
    jobject getObject() { return mObject; }
private:
    JNIContext();
    jclass      mClass;     // Reference to Native class
    jobject     mObject;    // Weak ref to Native Java object to call on
    jobject     mWeakObject;    // Weak ref to Native Java object to call on
    void        *mNativeContext; // native context handle from native_startup()
};

JNIContext::JNIContext(JNIEnv* env, jobject thiz, jobject weak_thiz)
{
    mNativeContext = NULL;
    // Hold onto the Native class for use in calling the method
    // that posts events to the application thread.
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        ALOGE("Can't find com/android/accessorydisplay/source/ScreenRecord");
        jniThrowException(env, "java/lang/Exception", NULL);
        return;
    }
    mClass = (jclass)env->NewGlobalRef(clazz);

    mObject  = env->NewGlobalRef(thiz);

    // We use a weak reference so the Native object can be garbage collected.
    // The reference is only used as a proxy for callbacks.
    if (weak_thiz) {
        mWeakObject = env->NewGlobalRef(weak_thiz);
    } else {
        mWeakObject = NULL;
    }
}

JNIContext::~JNIContext()
{
    // remove global references
    JNIEnv *env = AndroidRuntime::getJNIEnv();
    if (mWeakObject) env->DeleteGlobalRef(mWeakObject);
    env->DeleteGlobalRef(mObject);
    env->DeleteGlobalRef(mClass);
}

/*
 * Makes the current thread visible to the VM.
 *
 * The JNIEnv pointer returned is only valid for the current thread, and
 * thus must be tucked into thread-local storage.
 */
static int javaAttachThread(const char* threadName, JNIEnv** pEnv)
{
    JavaVMAttachArgs args;
    JavaVM* vm;
    jint result;

    vm = AndroidRuntime::getJavaVM();
    assert(vm != NULL);

    args.version = JNI_VERSION_1_4;
    args.name = (char*) threadName;
    args.group = NULL;

    result = vm->AttachCurrentThread(pEnv, (void*) &args);
    if (result != JNI_OK)
        ALOGI("NOTE: attach of thread '%s' failed\n", threadName);

    return result;
}

/*
 * Detach the current thread from the set visible to the VM.
 */
static int javaDetachThread(void)
{
    JavaVM* vm;
    jint result;

    vm = AndroidRuntime::getJavaVM();
    assert(vm != NULL);

    result = vm->DetachCurrentThread();
    if (result != JNI_OK)
        ALOGE("ERROR: thread detach failed\n");
    return result;
}

static int native_callback(void *para, int update_flag, int arg1, int arg2, void *data)
{
    JNIContext *c = reinterpret_cast<JNIContext *>(para);
    JNIEnv *env;
    ALOGV("jni ctx:%p, update_flag:0x%08x, arg1:0x%08x, arg2:0x%08x %p", c, update_flag, arg1, arg2, data);

    env = AndroidRuntime::getJNIEnv();
    if (!env) {
        javaAttachThread(NULL, &env);
        env = AndroidRuntime::getJNIEnv();
    }
    int ret = 0;
    if (env) {
        ret = env->CallIntMethod(c->getObject(), fields.notify_update, update_flag, arg1, arg2);
        if (env->ExceptionCheck()) {
            ALOGW("An exception occurred while notifying an event.");
            env->ExceptionClear();
        }
    } else {
        ALOGE("No JNI Env");
    }
    //javaDetachThread();
    return ret;
}

static jlong com_android_accessorydisplay_source_ScreenRecord_native_init(JNIEnv *env, jobject jobj,
    jint gOutputFormat,           // data format for output
    jint gWantInfoScreen,    // do we want initial info screen?
    jint gWantFrameTime,     // do we want times on each frame?
    jint gVideoWidth,        // default width+height
    jint gVideoHeight,
    jint gBitRate,     // 4Mbps
    jint gTimeLimitSec,
    jstring fileName)
{
    void *nativeContext;
    JNIContext *c = new JNIContext(env, jobj, NULL);
    libscreenrecord_params_t recparams;
    memset(&recparams, 0x00, sizeof(libscreenrecord_params_t));
    
    recparams.gVerbose = true;           // chatty on stdout
    recparams.gRotate = false;            // rotate 90 degrees
    recparams.gOutputFormat = (OUT_OUT_FORMAT)gOutputFormat;           // data format for output

    recparams.gVideoWidth = 0;        // default width+height
    recparams.gVideoHeight = 0;
    recparams.gSizeSpecified = false;     // was size explicitly requested?
    if (gVideoWidth > 0 && gVideoHeight > 0) {
        recparams.gVideoWidth = gVideoWidth;        // default width+height
        recparams.gVideoHeight = gVideoHeight;
        recparams.gSizeSpecified = true;
    }

    recparams.gWantInfoScreen = gWantInfoScreen;    // do we want initial info screen?
    recparams.gWantFrameTime = gWantFrameTime;     // do we want times on each frame?
    recparams.gBitRate = gBitRate;//4000000;     // 4Mbps
    recparams.gTimeLimitSec = gTimeLimitSec; // second

    if (fileName != NULL) {
        const char *fileName_str = env->GetStringUTFChars(fileName, NULL);
        if (fileName_str) {
            strncpy(recparams.u.fileName, fileName_str, sizeof(recparams.u.fileName)-1);
            env->ReleaseStringUTFChars(fileName, fileName_str);
        }
    } else {
        strcpy(recparams.u.fileName, "/sdcard/aa.mp4");
    }

    nativeContext = libscreenrecord_init(&recparams);
    if (nativeContext) {
        libscreenrecord_registListener(nativeContext, native_callback, c);
        c->setNativeContext(nativeContext);
    }
    return reinterpret_cast<jlong>(c);
}

static jint com_android_accessorydisplay_source_ScreenRecord_native_startup(JNIEnv * env, jobject jobj, jlong ctx)
{
    JNIContext *c = reinterpret_cast<JNIContext *>(ctx);
    if (c) {
        void *nativeContext = c->getNativeContext();
        libscreenrecord_run(nativeContext);
        return 0;
    }
    return -1;
}

static jint com_android_accessorydisplay_source_ScreenRecord_native_shutdown(JNIEnv * env, jobject jobj, jlong ctx)
{
    int ret = -1;
    JNIContext *c = reinterpret_cast<JNIContext *>(ctx);
    if (c) {
        libscreenrecord_destory(c->getNativeContext());
        delete c;
        ret = 0;
    }
    CheckErr(ret);
    return ret;
}



static jint com_android_accessorydisplay_source_ScreenRecord_native_setBuf(JNIEnv * env, jobject jobj, jlong ctx, jobject bytebuf)
{
    int ret = -1;
    JNIContext *c = reinterpret_cast<JNIContext *>(ctx);
    if (c) {
        // goes here
        void* buf = (void*)env->GetDirectBufferAddress(bytebuf);
        if (buf) {
            size_t bufsize =  env->GetDirectBufferCapacity(bytebuf);
            //ALOGI("native_setPcmBuf:%p pcmbufsize:%d", buf, bufsize);
            ret = libscreenrecord_setBuf(c->getNativeContext(), buf, bufsize);
        } else {
            ERROR_LOG();
        }
    }
    CheckErr(ret);
    return ret;
}

static jint com_android_accessorydisplay_source_ScreenRecord_native_lockBuf(JNIEnv * env, jobject jobj, jlong ctx, jbyteArray data)
{
    int ret = -1;
    JNIContext *c = reinterpret_cast<JNIContext *>(ctx);
    if (c) {
        // goes here
        jbyte* data_ptr = NULL;
        jsize data_size = 0;
        if (NULL != data) {
            data_size = env->GetArrayLength(data);
            data_ptr = env->GetByteArrayElements(data, NULL);
        }
        ret = libscreenrecord_lockBuf(c->getNativeContext(), data_ptr, data_size);
        CheckErr(ret);
        if (NULL != data) {
            env->ReleaseByteArrayElements(data, data_ptr, 0);
        }
    }
    CheckErr(ret);
    return ret;
}

static JNINativeMethod sMethods[] = {
    {"native_init",              "(IIIIIIILjava/lang/String;)J",    (void*)com_android_accessorydisplay_source_ScreenRecord_native_init},
    {"native_startup",           "(J)I",                            (void*)com_android_accessorydisplay_source_ScreenRecord_native_startup},
    {"native_shutdown",          "(J)I",                            (void*)com_android_accessorydisplay_source_ScreenRecord_native_shutdown},
    {"native_setBuf",            "(JLjava/nio/ByteBuffer;)I",       (void*)com_android_accessorydisplay_source_ScreenRecord_native_setBuf},
    {"native_lockBuf",           "(J[B)I",                          (void*)com_android_accessorydisplay_source_ScreenRecord_native_lockBuf},
};

#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif


int register_com_android_accessorydisplay_source_ScreenRecord(JNIEnv* env)
{
    jclass clazz;

    clazz = env->FindClass("com/android/accessorydisplay/source/ScreenRecord");
    if (clazz == NULL) {
        ERROR_LOG();
        return -1;
    }

    fields.notify_update = env->GetMethodID(clazz, "notify_update",
                                               "(III)I");
    if (fields.notify_update == NULL) {
        ERROR_LOG();
        return -1;
    }
    return jniRegisterNativeMethods(env, "com/android/accessorydisplay/source/ScreenRecord", sMethods, NELEM(sMethods));
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }
    ALOG_ASSERT(env, "Could not retrieve the env!");
    register_com_android_accessorydisplay_source_ScreenRecord(env);
    return JNI_VERSION_1_4;
}
}

