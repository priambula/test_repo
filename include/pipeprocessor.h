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
#include "pipeprocessor_cfg.h"
#include "error.h"
#include "syncro.h"

/// @namespace MultiPipeProcessor
namespace MultiPipeProcessor
{
#if defined(__GNUC__) || defined(__MINGW32__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
/// @class IConsumer
/// @brief Эских класса потребителя
template <typename TValue>
class IConsumer
{
    public:
        virtual void Consume(TValue & value)
        {
            // Do something
        }
        void setKey(void * key)
        {
            m_key = key;
        }
        virtual ~IConsumer() {}
    protected:
        void * m_key;
};
#if defined(__GNUC__) || defined(__MINGW32__)
#pragma GCC diagnostic pop
#endif
/// @struct TThreadArgs
/// @brief Структура входных и выходных аргументов потока и объекта
/// диспетчера
struct TThreadArgs
{
    /// @enum TLockNodeCodes
    /// Список кодов возврата при завершении работы объекта и потока
    enum TTExitCode
    {
        th_exit_succ = 0,
        th_exit_force = 1,
        th_exit_error = -1
    };
    /// @enum TLockNodeCodes
    /// Список кодов событий из потока или метода объекта
    /// в цикл ожадания главного потока.
    enum TTEvent
    {
        th_event_loop = 0,
        th_event_break = -1
    };
    // Reserved for future
    void*  inp_arg;
    // Exit code
    std::atomic<TTExitCode> out_arg;
    // Event
    std::atomic<TTEvent> event;
};

template <typename TValue>
using _TContainer3 = boost::lockfree::queue<TValue,
    boost::lockfree::fixed_sized<true> >;

/// @struct TNode
/// @brief Интерфейсный класс
template <typename TValue>
struct TNode
{
    IConsumer<TValue>* consumer; // data node lock required
};


/// @class TMultiPipeProcessor
/// @brief Потокозащищённый класс диспетчера очередей
template <typename TValue,
          class TContainer,
          const TSize c_MaxConsumers = MAXCONSUMERS>
class TMultiPipeProcessor
{
protected:
    using TList = std::list<TValue>;
    using bthread = boost::thread;
    #ifdef MPP_NO_MUTEX
    using TLordEX = TCustomLock<TOptimSpinLock>;
    #else
    using TLordEX = std::unique_lock<std::mutex>;
    #endif
    using TLordNS = std::unique_lock<std::mutex>;
    typedef TOptimRWLock<TLockCodesPlace> TRWLock;
public:
    /// @typedef TKey
    /// @brief Интерфейсный тип для ключа очереди
    typedef TNode<TValue>* TKey;
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
    virtual ~TMultiPipeProcessor();
    /// Subscribe consumer nethod
    void Subscribe(IConsumer<TValue>* consumer);
    /// Unsubscribe consumer method
    bool UnSubscribe(TKey key);
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
    bool popValue(TKey key);
    /// Thread procedure method
    void process(TThreadArgs & args);
    /// Exclude key from set method
    bool excludeNode(TKey key);
    /// Include key to set method
    bool includeNode(TKey key);
    /// Init node method
    void initNode(TKey key);
    /// Send exit event method
    void send_exit_event(TThreadArgs & args,TThreadArgs::TTExitCode code);
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
    #ifdef MPP_NO_MUTEX
    TOptimSpinLock m_mtx_exclusive;
    #else
    std::mutex m_mtx_exclusive;
    #endif
    std::mutex m_mtx_debug;
    /// Флаг работы основного цикла потока
    std::atomic<bool> m_isRunning;
    /// Указатель на головной узел
    TKey m_Head;
    /// Указатель на текущий узел, с которым работает поток
    TKey m_currNode;
    /// Потребитель ожидающий удаления доступ ???
    std::atomic<TKey> m_expextedRemoveNode;
    /// Указатель на объект потока
    bthread * m_pthread;
    /// Указатель на структуру аргументов потока и объекта диспетчера
    TThreadArgs * m_args;
    /// Реестр производителей
    TProducerRecord * m_prodlist;
    /// Множество ключей узлов
    std::set<TKey> m_nodes;
}; // End of TMultiPipeProcessor
}  // End of MultiPipeProcessor

#endif // PIPEPROCESSOR_H

