#define LOG_TAG "screenrecord_jni"
//#define LOG_NDEBUG 0

#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"

#include <utils/Log.h>

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
    void setNaviContext(void* naviContext) { mNaviContext = naviContext; }
    void *getNaviContext() { return mNaviContext; }
    jclass getClass() { return mClass; }
    jobject getObject() { return mObject; }
private:
    JNIContext();
    jclass      mClass;     // Reference to Native class
    jobject     mObject;    // Weak ref to Native Java object to call on
    jobject     mWeakObject;    // Weak ref to Native Java object to call on
    void        *mNaviContext; // eledog native context handle from native_startup()
};

JNIContext::JNIContext(JNIEnv* env, jobject thiz, jobject weak_thiz)
{
    mNaviContext = NULL;
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

static void native_callback(void *para, int update_flag, int arg1, int arg2)
{
    JNIContext *c = reinterpret_cast<JNIContext *>(para);
    JNIEnv *env;
    ALOGV("jni ctx:%p, update_flag:0x%08x, arg1:0x%08x, arg2:0x%08x", c, update_flag, arg1, arg2);

    env = AndroidRuntime::getJNIEnv();
    if (!env) {
        javaAttachThread(NULL, &env);
        env = AndroidRuntime::getJNIEnv();
    }
    if (env) {
        env->CallVoidMethod(c->getObject(), fields.notify_update, update_flag, arg1, arg2);
        if (env->ExceptionCheck()) {
            ALOGW("An exception occurred while notifying an event.");
            env->ExceptionClear();
        }
    } else {
        ALOGE("No JNI Env");
    }
    //javaDetachThread();
}

static jlong com_android_accessorydisplay_source_ScreenRecord_native_startup(JNIEnv * env, jobject jobj, jstring args)
{
    void *naviContext;
    JNIContext *c = new JNIContext(env, jobj, NULL);
    const char *args_cstr= env->GetStringUTFChars(args, NULL);

    // /data/local/tmp/screenrecord --size 800x480 --output-format jpg --verbose /sdcard/aa.jpg
    int argc = 0;
    const char * argv[10];
    argv[0] = "/data/local/tmp/screenrecord";
    argv[1] = "--size";
    argv[2] = "800x480";
    argv[3] = "--output-format";
    argv[4] = "mp4";
    argv[5] = "--verbose";
    argv[6] = "/sdcard/aa.mp4";
    argc = 7;


    screen_record_main(argc, (char* const*)argv);

    env->ReleaseStringUTFChars(args, args_cstr);

    return reinterpret_cast<jlong>(c);
}

static jint com_android_accessorydisplay_source_ScreenRecord_native_shutdown(JNIEnv * env, jobject jobj, jlong ctx)
{
    int ret = -1;
    JNIContext *c = reinterpret_cast<JNIContext *>(ctx);
    if (c) {
        delete c;
    }
    CheckErr(ret);
    return ret;
}

static void com_android_accessorydisplay_source_ScreenRecord_native_init(JNIEnv *env)
{
    jclass clazz;

    clazz = env->FindClass("com/android/accessorydisplay/source/ScreenRecord");
    if (clazz == NULL) {
        ERROR_LOG();
        return;
    }

    fields.notify_update = env->GetMethodID(clazz, "notify_update",
                                               "(III)V");
    if (fields.notify_update == NULL) {
        ERROR_LOG();
        return;
    }
}

static JNINativeMethod sMethods[] = {
    {"native_init",              "()V",                             (void*)com_android_accessorydisplay_source_ScreenRecord_native_init},
    {"native_startup",           "(Ljava/lang/String;)J",           (void*)com_android_accessorydisplay_source_ScreenRecord_native_startup},
    {"native_shutdown",          "(J)I",                            (void*)com_android_accessorydisplay_source_ScreenRecord_native_shutdown},

};

#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

int register_com_android_accessorydisplay_source_ScreenRecord(JNIEnv* env)
{
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

