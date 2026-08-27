#ifndef FRAMILTON_PLATFORM_H
#define FRAMILTON_PLATFORM_H

/* Minimal Android and libc ABI declarations. Android supplies these symbols at
 * runtime. The freestanding build keeps the project usable on Fedora without a
 * full Gradle/NDK installation, because apparently a chess board should not
 * require half a data centre. */
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef unsigned long size_t;
typedef signed long ssize_t;
typedef int pid_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned long pthread_t;

typedef struct ANativeWindow ANativeWindow;
typedef struct AInputQueue AInputQueue;
typedef struct AInputEvent AInputEvent;
typedef struct ALooper ALooper;
typedef struct AAssetManager AAssetManager;
typedef int jint;
typedef unsigned char jboolean;
typedef void *jobject;
typedef jobject jclass;
typedef struct _jmethodID *jmethodID;
typedef void JavaVM;

struct JNINativeInterface_;
typedef const struct JNINativeInterface_ *JNIEnv;
struct JNINativeInterface_ {
    void *reserved0; void *reserved1; void *reserved2; void *reserved3;
    void *GetVersion; void *DefineClass; void *FindClass; void *FromReflectedMethod;
    void *FromReflectedField; void *ToReflectedMethod; void *GetSuperclass;
    void *IsAssignableFrom; void *ToReflectedField; void *Throw; void *ThrowNew;
    void *ExceptionOccurred; void *ExceptionDescribe; void *ExceptionClear;
    void *FatalError; void *PushLocalFrame; void *PopLocalFrame; void *NewGlobalRef;
    void *DeleteGlobalRef;
    void (*DeleteLocalRef)(JNIEnv *env, jobject obj);
    void *IsSameObject; void *NewLocalRef; void *EnsureLocalCapacity; void *AllocObject;
    void *NewObject; void *NewObjectV; void *NewObjectA;
    jclass (*GetObjectClass)(JNIEnv *env, jobject obj);
    void *IsInstanceOf;
    jmethodID (*GetMethodID)(JNIEnv *env, jclass clazz, const char *name, const char *sig);
    jobject (*CallObjectMethod)(JNIEnv *env, jobject obj, jmethodID methodID, ...);
    void *CallObjectMethodV; void *CallObjectMethodA;
    jboolean (*CallBooleanMethod)(JNIEnv *env, jobject obj, jmethodID methodID, ...);
    void *CallBooleanMethodV; void *CallBooleanMethodA;
};

typedef struct ARect { int32_t left, top, right, bottom; } ARect;
typedef struct ANativeWindow_Buffer {
    int32_t width;
    int32_t height;
    int32_t stride;
    int32_t format;
    void *bits;
    uint32_t reserved[6];
} ANativeWindow_Buffer;

typedef struct ANativeActivity ANativeActivity;
typedef struct ANativeActivityCallbacks {
    void (*onStart)(ANativeActivity *);
    void (*onResume)(ANativeActivity *);
    void *(*onSaveInstanceState)(ANativeActivity *, size_t *);
    void (*onPause)(ANativeActivity *);
    void (*onStop)(ANativeActivity *);
    void (*onDestroy)(ANativeActivity *);
    void (*onWindowFocusChanged)(ANativeActivity *, int);
    void (*onNativeWindowCreated)(ANativeActivity *, ANativeWindow *);
    void (*onNativeWindowResized)(ANativeActivity *, ANativeWindow *);
    void (*onNativeWindowRedrawNeeded)(ANativeActivity *, ANativeWindow *);
    void (*onNativeWindowDestroyed)(ANativeActivity *, ANativeWindow *);
    void (*onInputQueueCreated)(ANativeActivity *, AInputQueue *);
    void (*onInputQueueDestroyed)(ANativeActivity *, AInputQueue *);
    void (*onContentRectChanged)(ANativeActivity *, const ARect *);
    void (*onConfigurationChanged)(ANativeActivity *);
    void (*onLowMemory)(ANativeActivity *);
} ANativeActivityCallbacks;

struct ANativeActivity {
    ANativeActivityCallbacks *callbacks;
    JavaVM *vm;
    JNIEnv *env;
    jobject clazz;
    const char *internalDataPath;
    const char *externalDataPath;
    int32_t sdkVersion;
    void *instance;
    AAssetManager *assetManager;
    const char *obbPath;
};

extern void ANativeActivity_setWindowFlags(ANativeActivity *, uint32_t, uint32_t);
extern void ANativeActivity_setWindowFormat(ANativeActivity *, int32_t);
extern int32_t ANativeWindow_getWidth(ANativeWindow *);
extern int32_t ANativeWindow_getHeight(ANativeWindow *);
extern int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *, int32_t, int32_t, int32_t);
extern int32_t ANativeWindow_lock(ANativeWindow *, ANativeWindow_Buffer *, ARect *);
extern int32_t ANativeWindow_unlockAndPost(ANativeWindow *);
extern ALooper *ALooper_forThread(void);
extern ALooper *ALooper_prepare(int);
extern int ALooper_addFd(ALooper *, int, int, int, int (*)(int, int, void *), void *);
extern int ALooper_removeFd(ALooper *, int);
extern void AInputQueue_attachLooper(AInputQueue *, ALooper *, int, int (*)(int, int, void *), void *);
extern void AInputQueue_detachLooper(AInputQueue *);
extern int32_t AInputQueue_getEvent(AInputQueue *, AInputEvent **);
extern int32_t AInputQueue_preDispatchEvent(AInputQueue *, AInputEvent *);
extern void AInputQueue_finishEvent(AInputQueue *, AInputEvent *, int);
extern int32_t AInputEvent_getType(const AInputEvent *);
extern int32_t AMotionEvent_getAction(const AInputEvent *);
extern float AMotionEvent_getX(const AInputEvent *, size_t);
extern float AMotionEvent_getY(const AInputEvent *, size_t);

extern int open(const char *, int, ...);
extern ssize_t read(int, void *, size_t);
extern ssize_t write(int, const void *, size_t);
extern int close(int);
extern int pipe(int [2]);
extern pid_t fork(void);
extern int dup2(int, int);
extern int execv(const char *, char *const []);
extern void _exit(int);
extern pid_t waitpid(pid_t, int *, int);
extern int kill(pid_t, int);
extern void *malloc(size_t);
extern void free(void *);
extern void *memset(void *, int, size_t);
extern void *memcpy(void *, const void *, size_t);
extern void *memmove(void *, const void *, size_t);
extern size_t strlen(const char *);
extern int strcmp(const char *, const char *);
extern int strncmp(const char *, const char *, size_t);
extern char *strstr(const char *, const char *);
extern char *strchr(const char *, int);
extern int unlink(const char *);
extern int rename(const char *, const char *);

struct pollfd { int fd; short events; short revents; };
extern int poll(struct pollfd *, unsigned long, int);
struct timespec { long tv_sec; long tv_nsec; };
extern int clock_gettime(int, struct timespec *);
extern int nanosleep(const struct timespec *, struct timespec *);
extern int pthread_create(pthread_t *, const void *, void *(*)(void *), void *);
extern int pthread_join(pthread_t, void **);
extern int pthread_detach(pthread_t);

#define WINDOW_FORMAT_RGBA_8888 1
#define AINPUT_EVENT_TYPE_MOTION 2
#define AMOTION_EVENT_ACTION_MASK 0xff
#define AMOTION_EVENT_ACTION_DOWN 0
#define AMOTION_EVENT_ACTION_UP 1
#define AMOTION_EVENT_ACTION_MOVE 2
#define AMOTION_EVENT_ACTION_CANCEL 3
#define ALOOPER_PREPARE_ALLOW_NON_CALLBACKS 1
#define POLLIN 0x0001
#define POLLHUP 0x0010
#define O_RDONLY 0
#define O_WRONLY 1
#define O_CREAT 64
#define O_TRUNC 512
#define SIGTERM 15
#define CLOCK_MONOTONIC 1

#endif
