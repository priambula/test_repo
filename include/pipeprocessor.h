/** @file
 *  @brief Загаловочный файл модуля диспетчера очередей
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.07.2017
 *  @version 1.0 (alpha)
 */
#ifndef PIPEPROCESSOR_H
#define PIPEPROCESSOR_H
#include "common.h"
#include "pipetraits.h"
#include "error.h"
#include "syncro.h"
#include "mng_thread.h"

/// @namespace MultiPipeProcessor
namespace MultiPipeProcessor
{


/// @class TMultiPipeProcessor
/// @brief Потокозащищённый класс диспетчера очередей
template <typename TValue,const TTraitID _ID>
class TMultiPipeProcessor
{
protected:
    using TCurrTrait = TPipeTrait<TValue,_ID>;
    using TContainer = typename TCurrTrait::TContainer;
    using TNode = typename TCurrTrait::TNode;
    using TNodeContainer = typename TCurrTrait::TNodeContainer;
    using bthread = typename boost::thread;
    using TThreadArgs = typename TCurrTrait::TThreadArgs;
    using TTExitCode = typename TCurrTrait::TThreadArgs::TTExitCode;
    /// Тип блокировки на операции вставки/удаление узла
    using TLordEX = typename TCurrTrait::TLordEX;
    /// Тип объекта блокировки на операции вставки/удаление узла
    using TExLockObjType = typename TCurrTrait::TExLockObjType;
    using TLordNS = typename std::unique_lock<std::mutex>;
    using TRWLock = TOptimRWLock<TLockCodesPlace>;    
public:
    typedef TNode* TKey;
    struct TProducerRecord
    {
        TThreadIdType id;
        TKey key;
        std::atomic<bool> islocked;
        std::atomic<TLockCodesPlace> counter;
    };
public:
    std::atomic<bool> wlock;
    /// Conversion constructor
    TMultiPipeProcessor(TThreadArgs & args);
    /// Destructor
    ~TMultiPipeProcessor();
    void finit();
    /// Subscribe consumer method
    void Subscribe(IConsumer<TValue>* consumer);
    /// Unsubscribe consumer method
    bool UnSubscribe(TKey key);
    /// Push value to queue method
    bool PushValue(TKey key,TValue & value);
    /// Push value to queue reentrancy method
    bool PushValueMT(TKey key,TValue & value,TLockNodeCodes code);
    /// Get restricted thread id for producer and registration
    TLockNodeCodes RegisterProducer(TThreadIdType _id);
    /// Remove registration record
    bool UnRegisterProducer(TLockNodeCodes code);
    /// Test debug method
    void exec();
protected:
    /// Deleted default constructor
    TMultiPipeProcessor() = delete;
    /// Deleted copy constructor
    TMultiPipeProcessor(TMultiPipeProcessor const &) = delete;
    /// Deleted move constructor
    TMultiPipeProcessor(TMultiPipeProcessor const &&) = delete;
    /// Deleted copy-assign operator
    TMultiPipeProcessor& operator=(TMultiPipeProcessor const&) = delete;
    /// Deleted move-assign operator
    TMultiPipeProcessor& operator=(TMultiPipeProcessor &&) = delete;
    /// Pop from queue to consumer method
    bool PopValue(TKey key);    
    /// Thread procedure method
    void Process(TThreadArgs & args);
    /// Lock particular node method
    bool lock_node(TKey key,const TLockNodeCodes code);
    /// Unlock particular node method
    void unlock_node(TKey key);
    /// Safe unlock node method
    bool unlock_node_safe(TKey key,TLockNodeCodes code);
    /// Send exit event method
    void send_exit_event(TThreadArgs & args,TTExitCode code);
    /// Initiallization of producer`s list
    void initProdList(TProducerRecord * _list);
    /// Registration of key
    bool registerKey(TLockNodeCodes code,TKey key);
    /// Remove registration of key
    bool unregisterKey(TLockNodeCodes code);
    /// Compare key and m_prodlist[code].key
    bool compareKey(TKey key,TLockNodeCodes code);
    /// Insert data in register of producers
    bool insertProducer(TThreadIdType _id,TKey key,TLockNodeCodes code);
    /// find record, which matches key
    TLockNodeCodes findKey(TKey key);
    /// find record, which matches id
    TLockNodeCodes findId(TThreadIdType id);
    /// Мьютекс для блокировки доступа к m_nodes
    std::mutex m_mtx_nodeset;
    /// Мьютекс на операции вставки/удаление    
    TExLockObjType m_mtx_exclusive;
    std::mutex m_mtx_debug;
    /// Контейнер узлов
    TNodeContainer m_nodecont;
    /// Потребитель ожидающий удаления доступ ???
    std::atomic<TKey> m_expextedRemoveNode;
    /// Указатель на объект потока
    bthread * m_pthread;
    ManagedThread * m_mngthread;
    /// Указатель на структуру аргументов потока и объекта диспетчера
    TThreadArgs * m_args;
    /// Реестр производителей
    TProducerRecord * m_prodlist;
    /// Множество ключей узлов
    std::set<TKey> m_nodes;
    /// Указатель на объект исключения из потока
    std::exception_ptr m_exptr;
}; // End of TMultiPipeProcessor

}  // End of MultiPipeProcessor

#endif // PIPEPROCESSOR_H

