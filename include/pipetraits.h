/** @file
 *  @brief Загаловочный файл свойств диспетчера очередей
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.07.2017
 *  @version 1.0 (alpha)
 */
#ifndef PIPETRAITS_H
#define PIPETRAITS_H
#include "common.h"
#include "pipeprocessor_cfg.h"
#include "error.h"
#include "syncro.h"
#include "mng_thread.h"
#include "node.h"
#include "nodeImpl.h"

/// @namespace MultiPipeProcessor
namespace MultiPipeProcessor
{
/// @enum TTraitID
/// Список идентификаторов свойств
enum TTraitID {first = 1, second, third, fourth};

/// @struct TAbstractPipeTraits
/// @brief Структура общих свойств диспетчера очередей
struct TAbstractPipeTraits
{
    TAbstractPipeTraits() = delete;
    /// Максимальное количество потребителей
    static const TSize c_MaxConsumers = MAXCONSUMERS;
    using TContainer = void;
    using TNode = void;
    using TNodeContainer = void;
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
    /// Тип объекта блокировки на операции вставки/удаление узла
    #ifdef MPP_NO_MUTEX
    typedef TOptimSpinLock TExLockObjType;
    #else
    typedef std::mutex TExLockObjType;
    #endif
    /// Тип блокировки на операции вставки/удаление узла
    using TLordEX = std::unique_lock<TExLockObjType>;
};

/// @struct TPipeTrait
/// @brief Абстрактная структура свойств диспетчера очередей
template <typename _TValue, TTraitID id>
struct TPipeTrait : public TAbstractPipeTraits { };

/// @struct TPipeTrait<_TValue,TTraitID::first>
/// @brief Структура свойств диспетчера очередей для контейнера std::list
template <typename _TValue>
struct TPipeTrait<_TValue, TTraitID::first> : public TAbstractPipeTraits
{
    using TContainer = typename std::list<_TValue>;
    using TNode = TConsumerNode<_TValue,TLinkedNode<TContainer> >;
    using TNodeContainer = TLinkedNodeContainer<TNode>;
};

/// @struct TPipeTrait<_TValue,TTraitID::second>
/// @brief Структура свойств диспетчера очередей для контейнера boost::circular_buffer
template <typename _TValue>
struct TPipeTrait<_TValue,TTraitID::second> : public TAbstractPipeTraits
{
    using TContainer = typename boost::circular_buffer<_TValue>;
    using TNode = TConsumerNode<_TValue, TLinkedNode<TContainer> >;
    using TNodeContainer = TLinkedNodeContainer<TNode>;
};

/// @struct TPipeTrait<_TValue,TTraitID::third>
/// @brief Структура свойств диспетчера очередей для контейнера boost::lockfree
template <typename _TValue>
struct TPipeTrait<_TValue,TTraitID::third> : public TAbstractPipeTraits
{
    using TContainer = typename boost::lockfree::queue<_TValue,
    boost::lockfree::fixed_sized<true> >;
    using TNode = TConsumerNode<_TValue, TLinkedNode<TContainer> >;
    using TNodeContainer = TLinkedNodeContainer<TNode>;
};

/// @struct TPipeTrait<_TValue,TTraitID::forth>
/// @brief Структура свойств диспетчера очередей для контейнера boost::lockfree
template <typename _TValue>
struct TPipeTrait<_TValue,TTraitID::fourth> : public TAbstractPipeTraits
{
    using TContainer = typename boost::lockfree::queue<_TValue,
    boost::lockfree::fixed_sized<true> >;    
    using TNode = TConsumerNode<_TValue, TIntrusiveNode<TContainer> >;
    using TNodeContainer = TIntrusNodeContainer<TNode>;
};
}

#endif // PIPETRAITS_H
