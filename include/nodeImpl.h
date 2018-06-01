/** @file
 *  @brief Загаловочный файл модуля реализации классов узла
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.07.2017
 *  @version 1.0 (alpha)
 */
#ifndef NODEIMPL_H
#define NODEIMPL_H
#include "common.h"
#include "error.h"

namespace MultiPipeProcessor
{

/// @struct TLinkedNode
/// @brief Класс узла самодельного связаного списка
template <typename TContainer>
struct TLinkedNode
{
    TLinkedNode()
    {
        const char * funcname = __PRETTY_FUNCTION__;
        try
        {
            m_list = new(&copool) TContainer(MAXCAPACITY);
            next = nullptr;
            prev = nullptr;
            isLinked = false;
            // Unused operation for lockfree containers
            //lockindex.store(TLockNodeCodes::lock_free);
        }
        catch(...)
        {
            errspace::show_errmsg(funcname);
            throw;
        }
    }
    virtual ~TLinkedNode()
    {
        // Unused operation for POD data type
        //m_list->clear();
        delete m_list;
    }
    // In order to avoid empty node processing
    // linked list gear is used
    // Node with usefull data is included in list
    // Empty node is excluded from list
    bool isLinked; // Exclisive lock required
    TLinkedNode*  next;     // Exclusive lock required
    TLinkedNode*  prev;     // Exclusive lock required
    TContainer * m_list; // Data node lock is required
    // Unused variable for lockfree containers
    // Data node lock
    // using atomic char instead mutex object
    // for memory optimization
    //std::atomic<TLockNodeCodes> lockindex;
    char copool[sizeof TContainer(MAXCAPACITY)];
};

/// @struct TIntrusiveNode
/// @brief Класс узла интрузивного списка
template <typename TContainer>
class TIntrusiveNode: public boost::intrusive::list_base_hook<>
{
public:
    TIntrusiveNode()
    {
        const char * funcname = __PRETTY_FUNCTION__;
        try
        {
            m_list = new(&copool) TContainer(MAXCAPACITY);
            um_list.reset(m_list);
        }
        catch(...)
        {
            errspace::show_errmsg(funcname);
            throw;
        }
    }
    virtual ~TIntrusiveNode()
    {
        // Do not need since POD data type is used
        //m_list->clear();
        //delete m_list;
        um_list.release();
        m_list = nullptr;
    }
    TContainer * m_list; // Data node lock is not required
protected:
    char copool[sizeof TContainer(MAXCAPACITY)];
    std::unique_ptr<TContainer> um_list;
};


/// @struct TLinkedNodeContainer
/// @brief Контейней класс для узлов самодельного связаного списка
template <typename TNode>
struct TLinkedNodeContainer
{
    TLinkedNodeContainer()
    {
        Head = nullptr;
        currNode = nullptr;
    }
    virtual ~TLinkedNodeContainer() {}
    /// Указатель на головной узел
    TNode* Head;
    /// Указатель на текущий узел, с которым работает поток
    TNode* currNode;
    /// Exclude key from set method
    bool excludeNode(TNode* key) noexcept
    {
        bool result = false;
        TNode* plnode = nullptr;
        if (key != nullptr)
        {
            plnode = key;
            plnode->isLinked = false;
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
                Head = (TNode *)plnode->next;
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
    /// Include key to set method
    bool includeNode(TNode* key) noexcept
    {
        bool result = false;
        TNode* plnode = nullptr;
        TNode* phead = nullptr;
        if (key != nullptr)
        {
            plnode = key;
            phead = Head;
            plnode->prev = nullptr;
            plnode->isLinked = true;
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
            Head = phead;
            result = true;
        }
        else
        {
            result = false;
        }
        return result;
    }

};

/// @struct TIntrusNodeContainer
/// @brief Контейней класс для узлов интрузивного списка
template <typename TNode>
struct TIntrusNodeContainer : boost::intrusive::list<TNode>
{
    TIntrusNodeContainer()
    {
        nodeIt = this->begin();
    }
    virtual ~TIntrusNodeContainer() {}
    typename TIntrusNodeContainer<TNode>::iterator nodeIt;
    /// Init node method
    void initNode(TNode* key)     {    }

    bool excludeNode(TNode* key)
    {
        bool result = false;
        TNode* pinode = nullptr;
        typename TIntrusNodeContainer<TNode>::iterator intrus_it;
        if (key != nullptr)
        {
            pinode = key;
            intrus_it = this->iterator_to(*pinode);
            this->erase(intrus_it);
            result = true;
        }
        else
        {
            result = false;
        }
        return result;
    }
    /// Include key to set method
    bool includeNode(TNode* key)
    {
        bool result = false;
        TNode* pinode;
        if (key != nullptr)
        {
            pinode = key;
            this->push_back(*pinode);
            result = true;
        }
        else
        {
            result = false;
        }
        return result;
    }
};

}

#endif // NODEIMPL_H
