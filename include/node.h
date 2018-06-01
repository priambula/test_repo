/** @file
 *  @brief Загаловочный файл модуля интерфейса классов узла
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.07.2017
 *  @version 1.0 (alpha)
 */
#ifndef NODE_H
#define NODE_H
#include "common.h"
#include "syncro.h"
/// @namespace MultiPipeProcessor
namespace MultiPipeProcessor
{
#if defined(__GNUC__) || defined(__MINGW32__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
/// @class IConsumer
/// @brief Эскиз класса потребителя
template <typename TValue>
class IConsumer
{
    public:
        virtual void Consume(TValue & value)
        {
            // Do something
        }
        void setKey(void * key) noexcept
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

/// @struct TNode
/// @brief Интерфейсный класс
template <typename TValue, typename TNode>
class TConsumerNode : public TNode
{
public:
    TConsumerNode()
    {
        consumer = nullptr;
    }
    virtual ~TConsumerNode() {}
    IConsumer<TValue>* getConsumer() const noexcept
    {
        return consumer;
    }
    void setConsumer(IConsumer<TValue>* ac) noexcept
    {
        consumer = ac;
    }
protected:
    IConsumer<TValue>* consumer; // data node lock required    
};

}

#endif // NODE_H
