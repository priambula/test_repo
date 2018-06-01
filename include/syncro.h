/** @file
 *  @brief Загаловочный файл модуля средств синхронизации
 *  @author https://gist.github.com/sguzman/9594227
 *  @date   created Unknown
 *  @date   modified 15.07.2017 Petin Yuriy recycle@List.ru
 *  @version 1.0 (alpha)
 */
#ifndef SYNCRO_H
#define SYNCRO_H
#include "common.h"
#include "pipeprocessor_cfg.h"

namespace MultiPipeProcessor
{
using bthread = boost::thread;
typedef unsigned long TThreadIdType;
typedef bthread::native_handle_type native_handle_type;
typedef unsigned char TLockCodesPlace;

/// @enum TLockNodeCodes
/// Список идексов возможных "владельццев" узла
/// Содержит урезанную версия идентификатора потока
enum TLockNodeCodes : TLockCodesPlace
{
    lock_free,
    lock_unsubscribe,
    lock_push,
    lock_process,
    lock_first,
    lock_last = MAXPRODUCERS
};

///
TLockNodeCodes & operator++(TLockNodeCodes & r);
///
TThreadIdType getThreadId();
///
native_handle_type getThreadIdN(boost::thread * pth);

//  refactoring of unque_lock mutex.h
template<typename _Mutex>
class TCustomLock
{
public:
  typedef _Mutex mutex_type;
    explicit TCustomLock(mutex_type& __m)
    : m_device(&__m), m_owns(false)
    { }
    ~TCustomLock()
    {
        if (m_owns)
        {
            unlock();
        }
    }
    bool lock()
    {
        bool result = false;
        if (!m_device)
        {
            throw "";
        }
        else
        {
            if (m_owns)
            {
                throw "";
            }
            else
            {
                result = m_device->lock();
                if (result)
                {
                    m_owns = true;
                }
            }
       }
       return result;
    }
    void unlock()
    {
        if (!m_owns)
        {
            throw "";
        }
        else
        {
            if (m_device)
            {
                m_device->unlock();
                m_owns = false;
            }
        }
    }
private:
    mutex_type*	m_device;
    bool		m_owns; // XXX use atomic_bool
};

struct _TOptimSpinLock
{   
    inline static void initlock(std::atomic_flag &_locked)
    {
        _locked.clear(std::memory_order_release);
    }
    inline static bool lock(std::atomic_flag & _locked)
    {        
        using boost::chrono::milliseconds;
        typedef boost::chrono::high_resolution_clock clock;
        typedef boost::chrono::time_point<clock> time_point;
        using boost::chrono::duration_cast;
        bool result = true;
        int64_t duration = 0;
        time_point start;
        time_point end;
        start = clock::now();
        do
        {
            result = _locked.test_and_set(std::memory_order_acquire);
            if (result)
            {
                end = clock::now();
                boost::this_thread::yield();
                duration = duration_cast<milliseconds>(
                            end - start).count();
            }
        }
        while ((result) && (duration < TIMEOUT));
        return !result;
    }
    inline static void unlock(std::atomic_flag & _locked)
    {
        _locked.clear(std::memory_order_release);
    }
};

class TOptimSpinLock : public _TOptimSpinLock
{
public:
    TOptimSpinLock()
    {
        _TOptimSpinLock::initlock(m_flag);
    }
    bool lock()
    {
        return _TOptimSpinLock::lock(m_flag);
    }
    void unlock()
    {
        _TOptimSpinLock::unlock(m_flag);
    }
private:
 std::atomic_flag  m_flag;
};

template <typename TSizeType>
class TOptimRWLock
{
public:
    static void initlock(std::atomic<TSizeType> * _counter,
                  std::atomic<bool> * _wlock)
    {
        std::atomic_store_explicit(_counter,
                                   (TSizeType)0,
                                   std::memory_order_seq_cst);
        std::atomic_store_explicit(_wlock,
                                   false,
                                   std::memory_order_seq_cst);
    }
    static bool readlock(std::atomic<TSizeType> * _counter,
               std::atomic<bool> * _wlock)
    {
        using boost::chrono::duration_cast;
        using boost::chrono::milliseconds;
        typedef boost::chrono::high_resolution_clock clock;
        typedef boost::chrono::time_point<clock> time_point;
        bool result = true;
        bool val;
        long long duration;
        time_point start;
        time_point end;
        start = clock::now();
        do
        {
            do
            {
                val = std::atomic_load_explicit(
                            _wlock,
                            std::memory_order_seq_cst);
                if (val)
                {
                    result = false;
                    end = clock::now();
                    boost::this_thread::yield();
                    duration = duration_cast<milliseconds>(
                                end - start).count();
                }
                else
                {
                    result = true;
                }
            } while ((!result) && (duration < TIMEOUT));
            if (!result)
            {
                break;
            }
            std::atomic_fetch_add_explicit(
                                  _counter,
                                  (TSizeType)1,
                                   std::memory_order_seq_cst);
            val = std::atomic_load_explicit(
                                     _wlock,
                                     std::memory_order_seq_cst);
            if (val)
            {
                std::atomic_fetch_sub_explicit(
                                      _counter,
                                      (TSizeType)1,
                                       std::memory_order_seq_cst);
                end = clock::now();
                boost::this_thread::yield();
                duration = duration_cast<milliseconds>(end - start).count();
                result = false;
            }
            else
            {
                result = true;
            }
        } while ((!result) && (duration < TIMEOUT));
        return result;
    }
    static void readunlock(std::atomic<TSizeType> * _counter,
               std::atomic<bool> * _wlock)
    {
        std::atomic_fetch_sub_explicit(
                              _counter,
                              (TSizeType)1,
                              std::memory_order_seq_cst);
    }

    static bool writelock(std::atomic<TSizeType> * _counter,
               std::atomic<bool> * _wlock)
    {
        using boost::chrono::duration_cast;
        using boost::chrono::milliseconds;
        typedef boost::chrono::high_resolution_clock clock;
        typedef boost::chrono::time_point<clock> time_point;
        bool result = true;
        bool code = false;
        long long duration = 0;
        time_point start;
        time_point end;
        start = clock::now();
        TSizeType val;
        do
        {
            code = false;
            result = atomic_compare_exchange_weak_explicit(
                                   _wlock,
                                   &code,
                                   true,
                                   std::memory_order_seq_cst,
                                   std::memory_order_seq_cst);
            if (!result)
            {
                boost::this_thread::yield();
                end = clock::now();
                duration = duration_cast<milliseconds>(
                            end - start).count();
            }
        } while ((!result) && (duration < TIMEOUT));
        if (result)
        {
            start = clock::now();
            do
            {
                val = std::atomic_load_explicit(_counter,
                                                std::memory_order_seq_cst);
                if (val == 0)
                {
                    result = true;
                }
                else
                {
                    result = false;
                    boost::this_thread::yield();
                    end = clock::now();
                    duration = duration_cast<milliseconds>(
                                end - start).count();
                }
            } while ((!result) && (duration < TIMEOUT));
            if (!result)
            {
                std::atomic_store(_wlock,false);
            }
        }
        return result;
    }
    static void writeunlock(std::atomic<TSizeType> * _counter,
               std::atomic<bool> * _wlock)
    {
        std::atomic_store_explicit(_wlock,
                                   false,
                                   std::memory_order_seq_cst);
    }
};

#if defined(_W64) && defined(__MINGW32__)
extern "C" unsigned __int64 __rdtsc();
#endif
#if defined(_W64) && defined(_MSC_VER)
extern "C" unsigned __int64 __rdtsc();
#endif

inline unsigned __int64 getRDTDC()
{
#if defined(_W64) && defined(__MINGW32__)
    // This code may be usefull for some platform
    //#pragma intrinsic(__rdtsc)
    unsigned __int64 i;
        i = __rdtsc();
    return i;
#endif
#if defined(_W64) && defined(_MSC_VER)
    // This code may be usefull for some platform
    //#pragma intrinsic(__rdtsc)
    unsigned __int64 i;
        i = __rdtsc();
    return i;
#endif
#if (defined(__unix) || defined(__linux)) && defined(__GNUC__)
    #if defined(__x86_64__)
        unsigned int lo, hi;
        asm volatile ( "rdtsc\n" : "=a" (lo), "=d" (hi) );
        return ((unsigned long long)hi << 32) | lo;
    #else
        #if defined(__i386__)
            unsigned long long int x;
            __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
            return x;
        #endif
    #endif
#endif
}

}

#endif // SYNCRO_H
