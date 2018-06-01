/** @file
 *  @brief         Загаловочный файл менеджмента потока
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.07.2017
 *  @version 1.0 (alpha)
 */
#ifndef THREAD_H
#define THREAD_H
#include "common.h"
#include "error.h"

/// Таймаут ожидания при удаления потока
#define THREAD_TIMEOUT 250

class cancellation_token
{
public:
  cancellation_token(bool _initvalue = false) : _cancelled(_initvalue) {}
  explicit operator bool() const { return !_cancelled; }
  void clear() { _cancelled = false; }
  void cancel()  { _cancelled = true; }
private:
  std::atomic<bool> _cancelled;
};

/// @class thread_errors
/// @brief Класс исключений для менедмента потока
class thread_errors: public std::exception
{
public:
    enum Tcodes {error_creation, error_launch, error_procedure};
    thread_errors(Tcodes _codes) noexcept : m_codes(_codes) {}
    Tcodes getCode() const noexcept { return m_codes; }
private:
    Tcodes m_codes;
};

/// @class ManagedThread
/// @brief Класс-обёртка для менедмента потока
class ManagedThread
{
   using bthread = boost::thread;
public:
   /// Конструктор класса
   template< class Function, class Class, typename... Args>
   explicit ManagedThread(Function&& f,
                          Class obj,
                          std::exception_ptr * _eptr,
                          Args&&... _args);
    /// Deleted methodes
    ManagedThread() = delete;
    ManagedThread(const ManagedThread&) = delete;
    ManagedThread(const ManagedThread &&) = delete;
    ManagedThread& operator=(const ManagedThread&) = delete;
    ManagedThread& operator=(const ManagedThread&&) = delete;
   /// Деструктор класса
   virtual ~ManagedThread();
   /// Получить статус поток запущен/незапущен
   bool isActive() const noexcept { return atomic_load(&m_active_flag); }
   /// Полуить флаг разрешения работы потока
   bool isRunning() const noexcept { return atomic_load(&m_isRunning); }
   /// Проверить на наличие исключения
   bool isException() const noexcept
   {
       // The standard has no information about thread-safety of std::exception_ptr
       return ((!isActive())? (m_expointer != nullptr)?true : false: false);
   }
   /// Получить указатель исключения
   std::exception_ptr getExceptionPtr() const noexcept
   {
       // The standard has no information about thread-safety of std::exception_ptr
       return ((!isActive())?m_expointer:nullptr);
   }
   /// Получить платформозависимый идентификатор потока
   boost::thread::native_handle_type GetThreadID() const
   {
       return m_pthread->native_handle();
   }
protected:
   /// Указатель на объект потока
   bthread * m_pthread;
   /// Флаг разрешения работы основного цикла потока
   std::atomic<bool> m_isRunning;
   /// Флаг состояния потока
   std::atomic<bool> m_active_flag;
   /// Указатель на исключениев потоке
   /// [ Note: An implementation might use a reference-counted smart pointer
   /// as exception_ptr. —end     note ]
   /// 18.8.5 The default constructor of exception_ptr produces the null value of the type.
   /// For purposes of determining the presence of a data race, operations on exception_ptr
   /// objects shall access and modify only the exception_ptr objects themselves
   /// and not the exceptions they refer to.
   std::exception_ptr m_expointer;
   /// Указатель на указатель на исключение в потоке
   /// Актуальное значение может быть получено
   /// извне после уничтожения объекта ManagedThread
   std::exception_ptr * m_pexpointer;
   /// Разрешить работу основной петли(цикла) потока
   void StartLoop() noexcept { atomic_store(&m_isRunning, true); }
   /// Запретить работу основной петли(цикла) потока
   void StopLoop() noexcept { atomic_store(&m_isRunning, false); }
   /// Изменить флаг состояния активности потока
   void SetThreadeState(bool _state) noexcept { atomic_store(&m_active_flag, _state); }
   /// Установить указатель исключения
   void setExceptionPtr(std::exception_ptr& _eptr) noexcept
   {
       m_expointer = _eptr;
   }
   /// Функция-обертка для потока
   template< class Function, class Class, typename... Args>
   void threadFunction(Function&& f,Class obj, Args&&... _args) noexcept;
};

/// Конструктор класса
template< class Function, class Class, typename... Args>
ManagedThread::ManagedThread(Function&& f,
                             Class obj,
                             std::exception_ptr * _eptr,
                             Args&&... _args) : m_pexpointer(_eptr)
{
    const char * funcname = __PRETTY_FUNCTION__;
    try
    {
        typedef void (ManagedThread::
                      *Tfunc_ptr)( Function&&,Class,Args&&... );
         m_pthread = nullptr;
         m_expointer == nullptr;
         SetThreadeState(false);
         StopLoop();
         Tfunc_ptr func_ptr = &ManagedThread::threadFunction<Function&&,Class,Args&&...>;

         m_pthread = new bthread(func_ptr,this,
                     boost::forward<Function>(f),
                     boost::forward<Class>(obj),
                     boost::forward<Args>(_args)...);
         m_pthread->detach();
         StartLoop();
    }
    catch(...)
    {
        errspace::show_errmsg(funcname);
        throw (thread_errors(thread_errors::error_creation));
    }
}

/// Функция-обертка для потока
template< class Function, class Class, typename... Args>
   void ManagedThread::threadFunction(Function&& f,Class obj, Args&&... _args) noexcept
{
   const char * funcname = __PRETTY_FUNCTION__;
   std::exception_ptr e_ptr;
   bool bisRunning = false;
   try
   {
       SetThreadeState(true);
       // Thread function return before detaching causes crash in case of forced destroying.
       while (bisRunning)
       {
           bisRunning = isRunning();// atomic_load(&m_isRunning);
           boost::this_thread::yield();
       }
       (obj->*f)(boost::forward<Args>(_args)...);
       SetThreadeState(false);
   }
   catch(const thread_errors& e)
   {
        errspace::show_errmsg(funcname);
        e_ptr = std::current_exception();
        setExceptionPtr(e_ptr);
        SetThreadeState(false);
   }
   catch(...)
   {
        errspace::show_errmsg(funcname);
        e_ptr = std::current_exception();
        setExceptionPtr(e_ptr);
        SetThreadeState(false);
   }
}


#endif // THREAD_H
