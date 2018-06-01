/*! @file
 *  @brief      Модуль диспетчера очередей для варианта
 *              неблокирующий очереди boost::lockfree
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.10.2017
 *  @version 1.0 (alpha)
 */
#include "pipeprocessor.h"
#include "node.h"
using namespace MultiPipeProcessor;

typedef _TContainer3<TMultiPipeProcessor_t> _TCurrContainer;
using TLN = TLinkedNode<_TCurrContainer>;


template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
TMultiPipeProcessor(TThreadArgs & args) : m_args(&args)
{
    typedef void (TMultiPipeProcessor<TValue, TContainer,c_MaxConsumers>::
                  *Tfunc_ptr)( TThreadArgs & );
    const char * funcname = __PRETTY_FUNCTION__;
    try
    {
        m_pthread = nullptr;
        m_prodlist = new TProducerRecord[TLockNodeCodes::lock_last];
        initProdList(m_prodlist);
        atomic_store(&m_isRunning, true);
        m_Head = nullptr;
        m_currNode = nullptr;
        m_expextedRemoveNode.store(nullptr);
        Tfunc_ptr func_ptr = &TMultiPipeProcessor::process;
        auto resbi = boost::bind(func_ptr,
                                 this,
                                 boost::ref(args));
        m_pthread = new(std::nothrow) bthread(resbi);
    }
    catch (...)
    {
        errspace::show_errmsg(funcname);
        send_exit_event(args,TThreadArgs::th_exit_error);
    }
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
TMultiPipeProcessor::~TMultiPipeProcessor()
{
    const char * funcname = __PRETTY_FUNCTION__;
    bool result = false;
    TLN * plnode = nullptr;
    try
    {
        atomic_store(&m_isRunning, false);
        if (m_pthread != nullptr)
        {
            result = m_pthread->try_join_for(
                        boost::chrono::milliseconds(TIMEOUT)
                        );
            if (!result)
            {
                m_pthread->interrupt();
                result = m_pthread->try_join_for(
                            boost::chrono::milliseconds(TIMEOUT)
                            );
            }
            if (!result)
            {
        // forced termination
        #if defined(_WIN32) || defined(_WIN64)
            TerminateThread(m_pthread->native_handle(), 0);
        #elif  defined(__unix)
            pthread_cancel(m_pthread->native_handle());
        #else
            #error Unknown platform
        #endif
            }
            delete m_pthread;
        }
        for(TKey key: m_nodes)
        {
            plnode = (TLN *)(key);
            // Do not need since POD data type is used
            //plnode->m_list->clear();
            delete plnode;
        }
        delete [] m_prodlist;
    }
    catch (...)
    {
        errspace::show_errmsg(funcname);
        send_exit_event(*m_args,TThreadArgs::th_exit_error);
    }
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
void TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
Subscribe(IConsumer<TValue>* consumer)
{
    const char * funcname = __PRETTY_FUNCTION__;
    TKey key = nullptr;
    TLN * plnode = nullptr;
    try
    {
        TLordNS lock{ m_mtx_nodeset };
        if (consumer != nullptr)
        {
            plnode = new(std::nothrow) TLN;
            if (plnode != nullptr)
            {
                plnode->consumer = consumer;
                plnode->m_list = new(std::nothrow) TContainer(MAXCAPACITY);
                if (plnode->m_list != nullptr)
                {
                    key = (TKey)(plnode);
                    initNode(key);
                    consumer->setKey(key);
                    m_nodes.insert(key);
                    #ifdef  MPP_NO_IDLEMODE
                    // An order of locks doesn`t matter since
                    // key->isLinked = false
                    TLordEX lock_ex(m_mtx_exclusive);
                    if (lock_ex.lock())
                    {
                        // if another producer linked
                        // due waiting of mutex
                        if (!plnode->isLinked)
                        {
                            includeNode(key);
                        }
                    }
                    else // Exclusive lock is frozen
                    {
                        throw "";
                    }
                    #endif
                }
                else
                {
                    errspace::show_errmsg(funcname);
                    send_exit_event(*m_args,TThreadArgs::th_exit_error);
                }
            }
            else
            {
                errspace::show_errmsg(funcname);
                send_exit_event(*m_args,TThreadArgs::th_exit_error);
            }
        }
        else
        {
            errspace::show_errmsg(funcname);
            send_exit_event(*m_args,TThreadArgs::th_exit_error);
        }
    }
    catch (...)
    {
        errspace::show_errmsg(funcname);
        send_exit_event(*m_args,TThreadArgs::th_exit_error);
    }
}

template <typename TValue,
          typename TContainer,
          const TSize c_MaxConsumers>
bool TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
UnSubscribe(TKey key)
{
    using boost::chrono::milliseconds;
    typedef boost::chrono::high_resolution_clock clock;
    typedef boost::chrono::time_point<clock> time_point;
    using boost::chrono::duration_cast;
    int64_t duration = 0;
    time_point start;
    time_point end;
    typename std::set<TKey>::iterator iter;
    const char * funcname = __PRETTY_FUNCTION__;
    bool result = false;
    TLN * plnode = nullptr;
    TLN * currNode = nullptr;
    TLockNodeCodes lockedProducer;
    bool isLinked;
    try
    {
        TLordNS lock{ m_mtx_nodeset };
        if (key != nullptr)
        {
            #ifndef DISABLE_SET_CHECK
            iter = m_nodes.find(key);
            if (iter != m_nodes.end())
            {
                result = true;
            }
            #else
                result = true;
            #endif
            if (result)
            {
                m_expextedRemoveNode.store(key);
                // Producer must register a key before
                // check of m_expextedRemoveNode
                lockedProducer = findKey(key);
                start = clock::now();
                while ((lockedProducer != TLockNodeCodes::lock_last) &&
                       (duration < TIMEOUT) )
                {
                    // to wait untill all producers, using this key,
                    // finish a work with it
                    lockedProducer = findKey(key);
                    if (lockedProducer != TLockNodeCodes::lock_last)
                    {
                        end = clock::now();
                        boost::this_thread::yield();
                        duration = duration_cast<milliseconds>(
                                    end - start).count();
                    }
                }
                if (lockedProducer != TLockNodeCodes::lock_last)
                {
                    // Found zombie producer
                    throw "";
                }
                plnode = (TLN *)(key);
                TLordEX lock_ex(m_mtx_exclusive);
                result = lock_ex.lock();
                if (result)
                {
                    isLinked = plnode->isLinked.load();
                    if (isLinked)
                    {
                        if (m_currNode == key)
                        {
                            currNode = (TLN *)(m_currNode);
                            currNode = plnode->next;
                            m_currNode = (TKey)(currNode);
                        }
                        // if key != nullptr then success
                        result = excludeNode(key);
                    }
                    else // not isLinked
                    {
                        if (m_currNode == key)
                        {
                            currNode = (TLN *)(m_currNode);
                            currNode = plnode->next;
                            m_currNode = (TKey)(currNode);
                        }
                        result = true;
                    }
                }
                else // Exclusive lock is frozen
                {
                    throw "";
                }
                // m_mtx_nodeset = locked
                // m_mtx_exclusive = locked for linked state
                // In order to avoid ABA issue;
                if (result)
                {
                    // Do not need since POD data type is used
                    //plnode->m_list->clear();
                    delete plnode->m_list;
                    m_nodes.erase(key);
                    delete key;
                }
                else
                {
                    // Unexpected error
                    throw "";
                }
                m_expextedRemoveNode.store(nullptr);
            }
            else // Unfortune try to catch a node lock
            {    // Unfortunate try to find a key
                errspace::show_errmsg(funcname);
            }
        }
        else // Invalid key result = false
        {
            // Do Nothing
        }
    }
    catch (...)
    {
        result = false;
        errspace::show_errmsg(funcname);
        send_exit_event(*m_args,TThreadArgs::th_exit_error);
    }
    return result;
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
inline TLockNodeCodes TMultiPipeProcessor<TValue,
                                          TContainer,
                                          c_MaxConsumers>::
RegisterProducer(TThreadIdType _id)
{
    TLockNodeCodes index = TLockNodeCodes::lock_first;
    index = findId(TLockNodeCodes::lock_free);
    if (index < TLockNodeCodes::lock_last)
    {
        if (!insertProducer(_id,nullptr,index))
        {
            index = TLockNodeCodes::lock_last;
        }
    }
    return static_cast<TLockNodeCodes>(index);
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
inline bool TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
UnRegisterProducer(TLockNodeCodes code)
{
    bool result = false;
    if ((code < TLockNodeCodes::lock_last) &&
            (code >= TLockNodeCodes::lock_first))
    {
        if (insertProducer(0,nullptr,code))
        {
            result = true;
        }
    }
    return result;
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
bool TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
PushValueMT(TKey key,TValue & value,TLockNodeCodes code)
{
    typename std::set<TKey>::iterator iter;
    const char * funcname = __PRETTY_FUNCTION__;
    bool result = false;
    TLN * plnode = nullptr;
    TKey expectedRemove = nullptr;
    bool isLinked = false;
    try
    {
        expectedRemove = m_expextedRemoveNode.load();
        if (expectedRemove != key)
        {
            if ((code < TLockNodeCodes::lock_last) &&
                    (code >= TLockNodeCodes::lock_first))
            {
                result = registerKey(code,key);
            }
            else
            {
                result = false;
                // A Key can't been registered by
                // unknown error
                throw "";
            }
            try // Double try-block allows to continue an execution after
                // exception is occured, if an error handling is supported
                // registerKey cant't been wrapped by unique_lock
            {
                expectedRemove = m_expextedRemoveNode.load();
                if ((key != nullptr) && (result) && (expectedRemove != key))
                {
                    #ifndef DISABLE_SET_CHECK
                    std::lock_guard<std::mutex> lock{ m_mtx_nodeset };
                    iter = m_nodes.find(key);
                    if (iter != m_nodes.end())
                    {
                        result = lock_node(key,lock_push);
                    }
                    else // Key not found
                    {
                        // "Key not found"
                        throw "";
                    }
                    m_mtx_nodeset.unlock();
                    #endif
                    plnode = (TLN *)(key);
                    if (plnode->m_list->bounded_push(value))
                    {
                        #ifndef  MPP_NO_IDLEMODE
                        isLinked = plnode->isLinked.load();
                        if (!isLinked)
                        {
                            // An order of locks doesn`t matter since
                            // key->isLinked = false
                            TLordEX lock_ex(m_mtx_exclusive);
                            result = lock_ex.lock();
                            if (result)
                            {
                                // if another producer linked
                                // due waiting of mutex
                                isLinked = plnode->isLinked.load();
                                if (!plnode->isLinked)
                                {
                                    result = includeNode(key);
                                }
                            }
                            else // Exclusive lock is frozen
                            {
                                throw "";
                            }
                        }
                        #endif
                    }
                    else // Limit of queue is reached
                    {
                        result = false;
                        throw "";
                    }
                }
                else
                {   // Node expects removing
                    result = false;
                    throw "";
                }                
            }
            catch (...)
            {
                unregisterKey(code);
                throw;
            }
            unregisterKey(code);
        }
    }
    catch (...)
    {
        result = false;
        errspace::show_errmsg(funcname);
        send_exit_event(*m_args,TThreadArgs::th_exit_error);
    }
    return result;
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
bool TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
popValue(TKey key)
{
    bool result = false;
    TValue value;
    TLN * plnode = nullptr;
    if (key != nullptr)
    {
        plnode = (TLN *)(key);
        if (!plnode->m_list->empty())
        {
            if (plnode->m_list->pop(value))
            {
                plnode->consumer->Consume(value);
            }
        }
        else
        {
            // Exclusive lock activity ourside function body
            #ifndef  MPP_NO_IDLEMODE
            result = excludeNode(key);
            #endif
        }
    }
    return result;
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
void TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
process(TThreadArgs & args)
{
    using namespace boost::this_thread;
    using boost::posix_time::milliseconds;
    const char * funcname = __PRETTY_FUNCTION__;
    const char * c_welcome = "=== Multipipe queue thread is ran ===";
    bool result = false;
    bool isRunning = false;
    bool idleState = false;
    TLN * currNode = nullptr;
    try // External
    {
        std::cout << std::endl << c_welcome << std::endl;
        isRunning = atomic_load(&m_isRunning);
        TLordEX lock_ex(m_mtx_exclusive);
        while (isRunning)
        {
            idleState = false;            
            isRunning = atomic_load(&m_isRunning);
            result = lock_ex.lock();
            if (result)
            {
                currNode = (TLN *)(m_currNode);
                // At the end or at the start of linked list
                if (currNode == nullptr)
                {
                    currNode = (TLN *)(m_Head);
                    m_currNode = (TKey)(currNode);
                    if ( currNode != nullptr )
                    {
                        result = true;
                    }
                    else
                    {
                        result = false;
                        idleState = true;
                    }
                }
                else
                {
                    result = true;
                }
                if (result)
                {
                    popValue(currNode);
                    currNode = currNode->next;
                    m_currNode = (TKey)(currNode);
                    lock_ex.unlock();
                    boost::this_thread::yield();
                }
                else
                {
                    if (idleState)
                    {
                        lock_ex.unlock();
                        #ifndef MPP_NO_IDLEMODE
                        sleep(milliseconds(THREAD_IDLE_SLEEP));
                        interruption_point();
                        #endif
                    }
                    else
                    {
                        // Immpossible error
                        throw "";
                    }
                }
            }
            else
            {
                throw "";
            }
            isRunning = atomic_load(&m_isRunning);
        } // End of while
        send_exit_event(args,TThreadArgs::th_exit_succ);
    } // End of external try block
    catch(const boost::thread_interrupted &)
    {
        errspace::show_errmsg(funcname);
        errspace::show_errAddons(TThreadArgs::th_exit_force);
        send_exit_event(args,TThreadArgs::th_exit_error);
    }
    catch(...)
    {
        errspace::show_errmsg(funcname);
        errspace::show_errAddons(TThreadArgs::th_exit_error);
        send_exit_event(args,TThreadArgs::th_exit_error);
    }
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
void TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
exec()
{
    if (m_pthread != nullptr)
    {
        m_pthread->join();
    }
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
void inline TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
initNode(TMultiPipeProcessor::TKey key)
{
    TLN * plnode = nullptr;
    if (key != nullptr)
    {
        plnode = (TLN *)(key);
        plnode->isLinked.store(false);
        plnode->next = nullptr;
        plnode->prev = nullptr;
    }
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
bool inline TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
excludeNode(TMultiPipeProcessor::TKey key)
{
    bool result = false;
    TLN * plnode = nullptr;
    if (key != nullptr)
    {
        plnode = (TLN *)(key);
        plnode->isLinked.store(false);
        if (plnode->next != nullptr)
        {
                plnode->next->prev = plnode->prev;
        }
        if (plnode->prev != nullptr)
        {
                plnode->prev->next = plnode->next;
        }
        else
        {
            m_Head = plnode->next;
        }
        // Don`t assign key->next = nullptr in order to
        // Let go on removing without a return to Head node
        plnode->prev = nullptr;
        result = true;
    }
    else
    {
        result = false;
    }
    return result;
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
bool TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
includeNode(TMultiPipeProcessor::TKey key)
{
    bool result = false;
    TLN * plnode = nullptr;
    TLN * phead = nullptr;
    if (key != nullptr)
    {
        plnode = (TLN *)(key);
        phead = (TLN *)(m_Head);
        plnode->prev = nullptr;
        plnode->isLinked.store(true);
        if (phead != nullptr)
        {
                plnode->next = phead;
                phead->prev = plnode;
                phead = plnode;
        }
        else
        {
            plnode->next = nullptr;
            phead = plnode;
        }
        m_Head = (TKey)(phead);
        result = true;
    }
    else
    {
        result = false;
    }
    return result;
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
inline void TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
send_exit_event(TThreadArgs & args,TThreadArgs::TTExitCode code)
{
    TThreadArgs::TTEvent new_event = TThreadArgs::th_event_break;
    atomic_store(&args.out_arg, code);
    atomic_store(&args.event, new_event);
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
inline void TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
initProdList(TProducerRecord * _list)
{
    TLockCodesPlace i1 = 0;
    for (i1 = 0; i1 < TLockNodeCodes::lock_last; i1++)
    {
        TRWLock::initlock(&_list[i1].counter,
                          &_list[i1].islocked);
        _list[i1].id = (TThreadIdType)0;
        _list[i1].key = nullptr;
    };
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
inline bool TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
registerKey(TLockNodeCodes code,TKey key)
{
    bool result = false;
    if (code < TLockNodeCodes::lock_last)
    {

        if (TRWLock::writelock(&m_prodlist[code].counter,
                           &m_prodlist[code].islocked))
        {
            m_prodlist[code].key = key;
            TRWLock::writeunlock(&m_prodlist[code].counter,
                                 &m_prodlist[code].islocked);
            result = true;
        }
    }
    return result;
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
inline bool TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
unregisterKey(TLockNodeCodes code)
{
    bool result = false;
    if (code < TLockNodeCodes::lock_last)
    {
        if (TRWLock::writelock(&m_prodlist[code].counter,
                           &m_prodlist[code].islocked))
        {
            m_prodlist[code].key = nullptr;
            result = true;
            TRWLock::writeunlock(&m_prodlist[code].counter,
                                 &m_prodlist[code].islocked);
        }
    }
    return result;
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
inline TLockNodeCodes TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
findKey(TKey key)
{
    const char * errmsg = "Zombie thread is detected by findKey:";
    std::stringstream strcode;
    TLockCodesPlace index = TLockNodeCodes::lock_first;
    do
    {
        if (TRWLock::readlock(&m_prodlist[index].counter,
                           &m_prodlist[index].islocked))
        {
            if (m_prodlist[index].key == key)
            {
                TRWLock::readunlock(&m_prodlist[index].counter,
                                     &m_prodlist[index].islocked);
                break;
            }
            TRWLock::readunlock(&m_prodlist[index].counter,
                                 &m_prodlist[index].islocked);
        } // to do else ...
        else
        {
            errspace::show_errmsg(errmsg);
            strcode << index;
            errspace::show_errmsg(strcode.str().c_str());

        }
        ++index;
    } while(index < TLockNodeCodes::lock_last);
    return static_cast<TLockNodeCodes>(index);
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
inline TLockNodeCodes TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
findId(TThreadIdType id)
{
    TLockCodesPlace index = TLockNodeCodes::lock_first;
    do
    {
        if (TRWLock::readlock(&m_prodlist[index].counter,
                           &m_prodlist[index].islocked))
        {
            if (m_prodlist[index].id == id)
            {
                TRWLock::readunlock(&m_prodlist[index].counter,
                                     &m_prodlist[index].islocked);
                break;
            }
            TRWLock::readunlock(&m_prodlist[index].counter,
                                 &m_prodlist[index].islocked);
        }
        ++index;
    } while(index < TLockNodeCodes::lock_last);
    return static_cast<TLockNodeCodes>(index);
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
inline bool TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
compareKey(TKey key,TLockNodeCodes code)
{
    TLockCodesPlace index = code;
    bool result = false;
    if (TRWLock::readlock(&m_prodlist[index].counter,
                       &m_prodlist[index].islocked))
    {
        if (m_prodlist[index].key == key)
        {
            result = true;
        }
        TRWLock::readunlock(&m_prodlist[index].counter,
                             &m_prodlist[index].islocked);
    }
    return result;
}

template <typename TValue,typename TContainer,const TSize c_MaxConsumers>
inline bool TMultiPipeProcessor<TValue, TContainer, c_MaxConsumers>::
insertProducer(TThreadIdType _id,TKey key,TLockNodeCodes code)
{
    bool result = false;
    if (code < TLockNodeCodes::lock_last)
    {
        if (TRWLock::writelock(&m_prodlist[code].counter,
                             &m_prodlist[code].islocked))
        {
            m_prodlist[code].key = key;
            m_prodlist[code].id = _id;
            TRWLock::writeunlock(&m_prodlist[code].counter,
                                 &m_prodlist[code].islocked);
            result = true;
        }
    }
    return result;
}

template class TMultiPipeProcessor<TMultiPipeProcessor_t,
                                   _TCurrContainer>;

