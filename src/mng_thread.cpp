/** @file
 *  @brief         Модуль менеджмента потока
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.07.2017
 *  @version 1.0 (alpha)
 */
#include "mng_thread.h"

/// Деструктор класса
ManagedThread::~ManagedThread()
{
    const char * funcname = __PRETTY_FUNCTION__;
    bool result = false;
    try
    {
        StopLoop();
        if (m_pthread != nullptr)
        {
            result = m_pthread->try_join_for(
                        boost::chrono::milliseconds(THREAD_TIMEOUT)
                        );
            if (!result)
            {
                m_pthread->interrupt();
                result = m_pthread->try_join_for(
                            boost::chrono::milliseconds(THREAD_TIMEOUT)
                            );
            }
            if (!result)
            {
        // forced termination
        #if defined(_WIN32) || defined(_WIN64)
            TerminateThread(GetThreadID(), 0);
        #elif  defined(__unix)
            pthread_cancel(GetThreadID());
        #else
            #error Unknown platform
        #endif
            }
        }
        delete m_pthread; // noexcept by C++11 standard
        m_pthread = nullptr;        
        if (m_pexpointer != nullptr)
        {
            *m_pexpointer = m_expointer;
        }
    }
    catch (...)
    {
        errspace::show_errmsg(funcname);
        // warning !!!
        // delete with noexcept by C++11 standard
        // if thread is joining while destroing, then
        // delete operator can provoke segfault
        delete m_pthread;
        m_pthread = nullptr;
        if (m_pexpointer != nullptr)
        {
            *m_pexpointer = m_expointer;
        }
    }
}
