/*! @file
 *  @brief      Модуль диспетчера очередей для варианта
 *              неблокирующий очереди boost::lockfree
 *              минимальный timeoverhead
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.10.2017
 *  @version 1.0 (alpha)
 */
#include "pipeprocessor.h"
using namespace MultiPipeProcessor;

template <typename TValue, TTraitID _ID>
TMultiPipeProcessor<TValue,_ID>::
TMultiPipeProcessor(TThreadArgs & args) : m_args(&args)
{
    const char * funcname = __PRETTY_FUNCTION__;
    try
    {
        m_mngthread = nullptr;
        m_prodlist = nullptr;
        std::unique_ptr<TProducerRecord[]> um_prodlist( new TProducerRecord[TLockNodeCodes::lock_last]);
        initProdList(um_prodlist.get());
        m_expextedRemoveNode.store(nullptr);
        std::unique_ptr<ManagedThread> um_mngthread(
                    new ManagedThread(&TMultiPipeProcessor::Process,
                                      this,
                                      &m_exptr,
                                      boost::ref(args)));
        m_prodlist = um_prodlist.release();
        m_mngthread = um_mngthread.release();        
    }
    catch (...)
    {
        finit();
        errspace::show_errmsg(funcname);
        throw;
    }
}

template <typename TValue, TTraitID _ID>
void TMultiPipeProcessor<TValue,_ID>::
finit()
{
    bool result = false;
    TKey plnode = nullptr;
    delete m_mngthread;
    m_mngthread = nullptr;
    for(TKey key: m_nodes)
    {
        plnode = key;
        delete plnode;
    }
    delete [] m_prodlist;
    m_prodlist = nullptr;
}


template <typename TValue, TTraitID _ID>
TMultiPipeProcessor<TValue,_ID>::
TMultiPipeProcessor::~TMultiPipeProcessor()
{
    const char * funcname = __PRETTY_FUNCTION__;
    try
    {
        finit();
    }
    catch (...)
    {
        errspace::show_errmsg(funcname);
        send_exit_event(*m_args,TThreadArgs::th_exit_error);
    }
}


template <typename TValue, TTraitID _ID>
void TMultiPipeProcessor<TValue,_ID>::
Subscribe(IConsumer<TValue>* consumer)
{
    const char * funcname = __PRETTY_FUNCTION__;
    TKey key = nullptr;
    TKey plnode = nullptr;
    try
    {
        TLordNS lock{ m_mtx_nodeset };
        if (consumer != nullptr)
        {
            plnode = new(std::nothrow) TNode;
            throw "";
            if (plnode != nullptr)
            {
                plnode->setConsumer(consumer);
                key = plnode;
                consumer->setKey(key);
                m_nodes.insert(key);
                #ifdef  MPP_NO_IDLEMODE
                if (!plnode->isLinked)
                {
                    // An order of locks doesn`t matter since
                    // key->isLinked = false
                    TLordEX lock(m_mtx_exclusive);
                    // if another producer linked
                    // due waiting of mutex
                    //if (!plnode.get()->isLinked)
                    if (!plnode->isLinked)
                    {
                        m_nodecont.includeNode(key);
                    }
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
    catch (...)
    {
        if (plnode != nullptr) delete plnode;
        errspace::show_errmsg(funcname);
        send_exit_event(*m_args,TThreadArgs::th_exit_error);
        throw;
    }
}


template <typename TValue, TTraitID _ID>
bool TMultiPipeProcessor<TValue,_ID>::
UnSubscribe(TKey key)
{
    typename std::set<TKey>::iterator iter;
    const char * funcname = __PRETTY_FUNCTION__;
    bool result = true;
    bool lock_result = false;
    TKey plnode = nullptr;
    TKey currNode = nullptr;
    TLockNodeCodes lockedProducer;
    try
    {
        TLordNS lock{ m_mtx_nodeset };
        if (key != nullptr)
        {
            #ifndef DISABLE_SET_CHECK
            iter = m_nodes.find(key);
            if (iter != m_nodes.end())
            {
                lock_result = lock_node(key,lock_unsubscribe);
            }
            #else
            lock_result = lock_node(key,lock_unsubscribe);
            #endif
            if (lock_result)
            {
                m_expextedRemoveNode.store(key);
                lockedProducer = findKey(key);
                while (lockedProducer != TLockNodeCodes::lock_last)
                {
                    // ждём пока уйдёт
                    // когда дождёмся то проверим снова
                    // и так до тех пор пока не убедимся что данный
                    // ключ никем не занят
                    lockedProducer = findKey(key);
                }
                plnode = key;
                if (plnode->isLinked)
                {
                    unlock_node(key);
                    TLordEX lock2(m_mtx_exclusive);
                    lock_result = lock_node(key,lock_unsubscribe);
                    if (lock_result)
                    {
                        if (m_nodecont.currNode == key)
                        {
                            currNode = m_nodecont.currNode;
                            currNode = (TKey)plnode->next;
                            m_nodecont.currNode = currNode;
                        }
                        // success if key != nullptr
                        result = m_nodecont.excludeNode(key);
                    }
                }
                else // not isLinked
                {
                    if (m_nodecont.currNode == key)
                    {
                        currNode = m_nodecont.currNode;
                        currNode = (TKey)plnode->next;
                        m_nodecont.currNode = currNode;
                    }
                }
                // m_mtx_nodeset = locked
                // m_mtx_exclusive = locked for linked state
                // In order to avoid ABA issue;
                if (result)
                {
                    // Do not need since POD data type is used
                    //plnode->m_list->clear();
                    //delete plnode->m_list;
                    m_nodes.erase(key);
                    delete key;
                }
                else
                {
                    if (lock_result)
                    {
                        unlock_node(key);
                    }
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


template <typename TValue, TTraitID _ID>
inline TLockNodeCodes TMultiPipeProcessor<TValue,_ID>::
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


template <typename TValue, TTraitID _ID>
inline bool TMultiPipeProcessor<TValue,_ID>::
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


template <typename TValue, TTraitID _ID>
bool TMultiPipeProcessor<TValue,_ID>::
PushValue(TMultiPipeProcessor::TKey key, TValue & value)
{
    typename std::set<TKey>::iterator iter;
    const char * funcname = __PRETTY_FUNCTION__;
    bool result = false;
    TKey plnode = nullptr;
    try
    {
        std::lock_guard<std::mutex> lock{ m_mtx_nodeset };
        if (key != nullptr)
        {
            #ifndef DISABLE_SET_CHECK
            iter = m_nodes.find(key);
            if (iter != m_nodes.end())
            {
                result = lock_node(key,lock_push);
            }
            else // Key not found
            {
                std::cout << "Key not found" << std::endl;
            }
            #else
            result = true;//lock_node(key,lock_push);
            #endif
            if (result)
            {
                plnode = key;
                if (plnode->m_list->bounded_push(value))
                {
                    if (!plnode->isLinked)
                    {
                        // An order of locks doesn`t matter since
                        // key->isLinked = false
                        TLordEX lock(m_mtx_exclusive);
                        result = m_nodecont.includeNode(key);
                    }
                }
                //unlock_node(key);
            }
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


template <typename TValue, TTraitID _ID>
bool TMultiPipeProcessor<TValue,_ID>::
PushValueMT(TKey key,TValue & value,TLockNodeCodes code)
{
    typename std::set<TKey>::iterator iter;
    const char * funcname = __PRETTY_FUNCTION__;
    bool result = false;
    TKey plnode = nullptr;
    TKey expectedRemove = nullptr;
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
                std::cout << " Invalid code producer";
            }
            try
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
                        std::cout << "Key not found" << std::endl;
                    }
                    m_mtx_nodeset.unlock();
                    #endif
                    plnode = key;

                    if (plnode->m_list->bounded_push(value))
                    {
                        #ifndef  MPP_NO_IDLEMODE
                        if (!plnode->isLinked)
                        {
                            // An order of locks doesn`t matter since
                            // key->isLinked = false
                            TLordEX lock(m_mtx_exclusive);
                            // if another producer linked
                            // due waiting of mutex
                            if (!plnode->isLinked)
                            {
                                result = includeNode(key);
                            }
                        }
                        #endif
                    }
                    else
                    {
                        result = false;
                        std::cout << "bo";
                    }
                }                                
                else
                {
                    result = false;
                    std::cout << "key not found \n";
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


template <typename TValue, TTraitID _ID>
bool TMultiPipeProcessor<TValue,_ID>::
PopValue(TKey key)
{
    bool result = false;
    TValue value;
    TKey plnode = nullptr;
    if (key != nullptr)
    {
        plnode = key;
        if (!plnode->m_list->empty())
        {
            if (plnode->m_list->pop(value))
            {
                plnode->getConsumer()->Consume(value);
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


template <typename TValue, TTraitID _ID>
void TMultiPipeProcessor<TValue,_ID>::
Process(TThreadArgs & args)
{
    using namespace boost::this_thread;
    using boost::posix_time::milliseconds;
    const char * funcname = __PRETTY_FUNCTION__;
    bool lock_result = false;
    bool isRunning = false;
    bool idleState = false;
    TKey currNode = nullptr;
    try // External
    {
        std::cout << std::endl << "Queue`s thread is ran" << std::endl;        
        isRunning = m_mngthread->isRunning();
        while (isRunning)
        {
            lock_result = false;
            idleState = false;
            interruption_point();
            isRunning = m_mngthread->isRunning();
            m_mtx_exclusive.lock();
            try // Internal
            {
                currNode = m_nodecont.currNode;
                // At the end or at the start of linked list
                if (currNode == nullptr)
                {
                    currNode = m_nodecont.Head;
                    m_nodecont.currNode = currNode;
                    if ( currNode != nullptr )
                    {
                        lock_result = true;
                    }
                    else
                    {
                        lock_result = false;
                        idleState = true;
                    }
                }
                else
                {
                    lock_result = true;
                }
                if (lock_result)
                {
                    PopValue(currNode);
                    unlock_node(currNode);
                    currNode = (TKey)currNode->next;
                    m_nodecont.currNode = currNode;
                }
            } // Enf of internal try block
            catch(const boost::thread_interrupted &)
            {
                m_mtx_exclusive.unlock();
                throw;
            }
            catch (...)
            {
                m_mtx_exclusive.unlock();
                throw;
            }
            if (lock_result)
            {
                m_mtx_exclusive.unlock();
                boost::this_thread::yield();
            }
            else
            {
                if (idleState)
                {
                    m_mtx_exclusive.unlock();
                    #ifndef MPP_NO_IDLEMODE
                    sleep(milliseconds(THREAD_IDLE_SLEEP));
                    #endif
                }
                else
                {
                    m_mtx_exclusive.unlock();
                    throw "";
                }
            }
            isRunning = m_mngthread->isRunning();
        } // End of while
        send_exit_event(args,TThreadArgs::th_exit_succ);
    } // End of external try block
    catch(const boost::thread_interrupted &)
    {
        errspace::show_errmsg(funcname);
        errspace::show_errAddons(TThreadArgs::th_exit_force);
        send_exit_event(args,TThreadArgs::th_exit_error);
        throw;
    }
    catch(...)
    {
        errspace::show_errmsg(funcname);
        errspace::show_errAddons(TThreadArgs::th_exit_error);
        send_exit_event(args,TThreadArgs::th_exit_error);
        throw;
    }
}


template <typename TValue, TTraitID _ID>
void TMultiPipeProcessor<TValue,_ID>::
exec()
{
    if (m_pthread != nullptr)
    {
        //m_pthread->join();
    }
}

template <typename TValue, TTraitID _ID>
inline bool TMultiPipeProcessor<TValue,_ID>::
lock_node(TKey key,TLockNodeCodes code)
{
    return true;
}


template <typename TValue, TTraitID _ID>
inline bool TMultiPipeProcessor<TValue,_ID>::
unlock_node_safe(TKey key,TLockNodeCodes code)
{
    return true;
}


template <typename TValue, TTraitID _ID>
inline void TMultiPipeProcessor<TValue,_ID>::
unlock_node(TKey key)
{
// Do Nothing
}


template <typename TValue, TTraitID _ID>
inline void TMultiPipeProcessor<TValue,_ID>::
send_exit_event(TThreadArgs & args,TTExitCode code)
{
    typename TThreadArgs::TTEvent new_event = TThreadArgs::th_event_break;
    atomic_store(&args.out_arg, code);
    atomic_store(&args.event, new_event);
}


template <typename TValue, TTraitID _ID>
inline void TMultiPipeProcessor<TValue,_ID>::
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


template <typename TValue, TTraitID _ID>
inline bool TMultiPipeProcessor<TValue,_ID>::
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


template <typename TValue, TTraitID _ID>
inline bool TMultiPipeProcessor<TValue,_ID>::
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


template <typename TValue, TTraitID _ID>
inline TLockNodeCodes TMultiPipeProcessor<TValue,_ID>::
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


template <typename TValue, TTraitID _ID>
inline TLockNodeCodes TMultiPipeProcessor<TValue,_ID>::
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


template <typename TValue, TTraitID _ID>
inline bool TMultiPipeProcessor<TValue,_ID>::
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


template <typename TValue, TTraitID _ID>
inline bool TMultiPipeProcessor<TValue,_ID>::
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

template class TMultiPipeProcessor<TPipeValueType,TTraitID::third>;

