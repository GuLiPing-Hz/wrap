/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2013-2017 Chukong Technologies Inc.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/
#include "JniHelper.h"

#ifndef COCOS_PROJECT

jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGD("JNI_OnLoad");
    Wrap::JniHelper::setJavaVM(vm);

    return JNI_VERSION_1_4;
}

// void JNICALL
// JNI_OnUnload(JavaVM *vm, void *reserved){
// }

jclass _getClassID(JNIEnv *env, const char *className) {
    if (nullptr == className || env == nullptr) {
        return nullptr;
    }

    jstring _jstrClassName = env->NewStringUTF(className);

    jclass _clazz = (jclass) env->CallObjectMethod(Wrap::JniHelper::classloader,
		Wrap::JniHelper::loadclassMethod_methodID, _jstrClassName);

    if (nullptr == _clazz) {
        LOGE("Classloader failed to find class of %s", className);
        env->ExceptionClear();
    }

    env->DeleteLocalRef(_jstrClassName);

    return _clazz;
}

void _detachCurrentThread(void *a) {
    LOGD("_detachCurrentThread");
    Wrap::JniHelper::getJavaVM()->DetachCurrentThread();
}

namespace Wrap {

    MAPINTJNIENV JniHelper::m_mapEnv;
    JavaVM *JniHelper::_psJavaVM = nullptr;
    jmethodID JniHelper::loadclassMethod_methodID = nullptr;
    jobject JniHelper::classloader = nullptr;
    std::function<void()> JniHelper::classloaderCallback = nullptr;

    jobject JniHelper::_context = nullptr;
    std::unordered_map<JNIEnv *, std::vector<jobject>> JniHelper::localRefs;

    std::string JniHelper::getStringUTFCharsJNI(JNIEnv *env, jstring srcjStr, bool *ret) {
        std::string utf8Str;
        if (srcjStr != nullptr) {
            //("%s : env = %x\n", __FUNCTION__, env);
            const char *str = env->GetStringUTFChars(srcjStr, 0);
            //LOGD("%s : str = %s\n", __FUNCTION__, str);
            if (ret)
                *ret = true;
            std::string temp = str;
            utf8Str = std::move(temp);
            env->ReleaseStringUTFChars(srcjStr, str);
        } else {
            if (ret)
                *ret = false;
            utf8Str = "";
        }
        return utf8Str;
    }

    jstring JniHelper::newStringUTFJNI(JNIEnv *env, const std::string &utf8Str, bool *ret) {
        jstring jstr = env->NewStringUTF(utf8Str.c_str());
        if (ret)
            *ret = true;
        return jstr;
    }

    JavaVM *JniHelper::getJavaVM() {
        //pthread_t thisthread = pthread_self();
        //LOGD("JniHelper::getJavaVM(), pthread_self() = %ld", thisthread);
        return _psJavaVM;
    }

    void JniHelper::setJavaVM(JavaVM *javaVM) {
        //pthread_t thisthread = pthread_self();
        //LOGD("JniHelper::setJavaVM(%p), pthread_self() = %ld", javaVM, thisthread);
        _psJavaVM = javaVM;

        //pthread_key_create(&g_key, _detachCurrentThread);
    }

    //在native代码中，如果线程需要使用jnienv环境变量，需要先附加到当前线程，注：jnienv只在本线程使用
    bool JniHelper::attachCurThread(unsigned int threadid) {
        LOGD("%s threadid=%d", __FUNCTION__, threadid);
        JNIEnv *env = NULL;
#ifdef _WIN32
        void** pEnv = (void**)&env;
        if (JNI_OK != _psJavaVM->AttachCurrentThread(pEnv, NULL))
#else
        if (JNI_OK != _psJavaVM->AttachCurrentThread(&env, NULL))
#endif
            return false;

        LOGD("%s threadid=%d", __FUNCTION__, threadid);
        m_mapEnv[threadid] = env;
        return true;
    }

    //线程退出时务必分离
    bool JniHelper::detachCurThread(unsigned int threadid) {
        m_mapEnv.erase(threadid);//移除
        return (JNI_OK == _psJavaVM->DetachCurrentThread());
    }

    //获取线程的java环境变量
    bool JniHelper::getJniByThreadID(unsigned int threadid, JNIEnv **penv) {
        MAPINTJNIENV::iterator it = m_mapEnv.find(threadid);
        if (it == m_mapEnv.end())
            return false;
        *penv = it->second;
        return true;
    }

    JNIEnv *JniHelper::getEnv() {
        if (m_mapEnv.empty())
            return nullptr;
        return m_mapEnv.begin()->second;
    }

    jobject JniHelper::getAndroidContext() {
        return _context;
    }

    bool JniHelper::setClassLoaderFrom(JNIEnv *env, jobject contextInstance) {
        JniMethodInfo _getclassloaderMethod;
        if (!JniHelper::getMethodInfo_DefaultClassLoader(env, _getclassloaderMethod,
                                                         "android/content/Context",
                                                         "getClassLoader",
                                                         "()Ljava/lang/ClassLoader;")) {
            return false;
        }

        jobject _c = env->CallObjectMethod(contextInstance, _getclassloaderMethod.methodID);

        if (nullptr == _c) {
            return false;
        }

        JniMethodInfo _m;
        if (!JniHelper::getMethodInfo_DefaultClassLoader(env, _m,
                                                         "java/lang/ClassLoader",
                                                         "loadClass",
                                                         "(Ljava/lang/String;)Ljava/lang/Class;")) {
            return false;
        }

        JniHelper::classloader = env->NewGlobalRef(_c);
        JniHelper::loadclassMethod_methodID = _m.methodID;
        JniHelper::_context = env->NewGlobalRef(contextInstance);
        if (JniHelper::classloaderCallback != nullptr) {
            JniHelper::classloaderCallback();
        }

        return true;
    }

    bool JniHelper::getStaticMethodInfo(JniMethodInfo &methodinfo, unsigned int threadId,
                                        const char *className,
                                        const char *methodName,
                                        const char *paramCode) {
        if ((nullptr == className) ||
            (nullptr == methodName) ||
            (nullptr == paramCode)) {
            return false;
        }

        JNIEnv *env;
        bool ret = getJniByThreadID(threadId, &env);
        //JNIEnv *env = JniHelper::getEnv(threadId);
        if (!ret) {
            LOGE("Failed to get JNIEnv");
            return false;
        }

        jclass classID = _getClassID(env, className);
        if (!classID) {
            LOGE("Failed to find class %s", className);
            env->ExceptionClear();
            return false;
        }

        //LOGD("%s %s %s", __FUNCTION__, methodName, paramCode);
        jmethodID methodID = env->GetStaticMethodID(classID, methodName, paramCode);
        if (!methodID) {
            LOGE("Failed to find static method id of %s", methodName);
            env->ExceptionClear();
            return false;
        }

        methodinfo.classID = classID;
        methodinfo.env = env;
        methodinfo.methodID = methodID;
        return true;
    }

    bool JniHelper::getMethodInfo_DefaultClassLoader(JNIEnv *env, JniMethodInfo &methodinfo,
                                                     const char *className,
                                                     const char *methodName,
                                                     const char *paramCode) {
        if ((nullptr == className) ||
            (nullptr == methodName) ||
            (nullptr == paramCode)) {
            return false;
        }

//        JNIEnv *env = JniHelper::getEnv();
//        if (!env) {
//            return false;
//        }

        jclass classID = env->FindClass(className);
        if (!classID) {
            LOGE("Failed to find class %s", className);
            env->ExceptionClear();
            return false;
        }

        jmethodID methodID = env->GetMethodID(classID, methodName, paramCode);
        if (!methodID) {
            LOGE("Failed to find method id of %s", methodName);
            env->ExceptionClear();
            return false;
        }

        methodinfo.classID = classID;
        methodinfo.env = env;
        methodinfo.methodID = methodID;

        return true;
    }

    bool JniHelper::getMethodInfo(JniMethodInfo &methodinfo,
                                  const char *className,
                                  const char *methodName,
                                  const char *paramCode) {
        if ((nullptr == className) ||
            (nullptr == methodName) ||
            (nullptr == paramCode)) {
            return false;
        }

        JNIEnv *env = JniHelper::getEnv();
        if (!env) {
            return false;
        }

        jclass classID = _getClassID(env, className);
        if (!classID) {
            LOGE("Failed to find class %s", className);
            env->ExceptionClear();
            return false;
        }

        jmethodID methodID = env->GetMethodID(classID, methodName, paramCode);
        if (!methodID) {
            LOGE("Failed to find method id of %s", methodName);
            env->ExceptionClear();
            return false;
        }

        methodinfo.classID = classID;
        methodinfo.env = env;
        methodinfo.methodID = methodID;

        return true;
    }

    std::string JniHelper::jstring2string(jstring jstr) {
        //LOGD("%s 不带env", __FUNCTION__);
        if (jstr == nullptr) {
            return "";
        }

        return jstring2string(JniHelper::getEnv(), jstr);
    }

    std::string JniHelper::jstring2string(JNIEnv *env, jstring jstr) {
        if (!env)
            return "";

        std::string strValue = getStringUTFCharsJNI(env, jstr);

        return strValue;
    }

    jstring JniHelper::convert(JniMethodInfo &t, const char *x) {
        //LOGD("%s : x = %s\n", __FUNCTION__, x);
        jstring ret = newStringUTFJNI(t.env, x ? x : "");
        localRefs[t.env].push_back(ret);
        return ret;
    }

    jstring JniHelper::convert(JniMethodInfo &t, const std::string &x) {
        return convert(t, x.c_str());
    }

    void JniHelper::deleteLocalRefs(JNIEnv *env) {
        if (!env) {
            return;
        }

        for (const auto &ref : localRefs[env]) {
            env->DeleteLocalRef(ref);
        }
        localRefs[env].clear();
    }

    void JniHelper::reportError(const std::string &className, const std::string &methodName,
                                const std::string &signature) {
        LOGE("Failed to find static java method. Class name: %s, method name: %s, signature: %s ",
             className.c_str(), methodName.c_str(), signature.c_str());
    }

} //namespace cocos2d

#endif// COCOS_PROJECT
