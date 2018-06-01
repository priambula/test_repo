/** @file
 *  @brief         Модуля класса тестирования
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.07.2017
 *  @version 1.0 (alpha)
 */
#include "testbench.h"

using namespace TESTBENCH;
#if defined(__GNUC__) || defined(__MINGW32__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"
    #pragma GCC diagnostic ignored "-Wunused-variable"
    #pragma GCC diagnostic ignored "-Wreturn-type"
#endif

namespace TESTBENCH
{
/// Список значений
std::vector<unsigned short> tb_vallist;
/// Список значений
std::vector<TValue> tb_th_vallist[N_PRODUCERS];
/// Список значений
time_point tb_th_timelist[N_PRODUCERS];
time_point tb_timestamp;
unsigned long long tb_th_clklist[N_PRODUCERS];
unsigned long long tb_clkstamp;
/// Список значений
std::vector<TValue> tb_cmp_vallist;
/// Список значений ключей
std::vector<TKey> tb_keylist;
/// Список значений ключей
std::vector<TKey> tb_th_keylist[N_PRODUCERS];
/// Имя файла для импорта значения или списка значений
const char * tb_fname_imp = "import.dat";
/// Имя файла для экспорта значения или списка значений
const char * tb_fname_exp = "export";
/// Указатель на объект потока
bthread * tb_pthreads[N_PRODUCERS];
/// Список кодов производителя
unsigned char tb_prodlist[N_PRODUCERS];
/// Указатель на структуру аргументов потока
TB_TThreadArgs * tb_thargs;
/// Список указателей на объекты-потребители
std::vector<TB_IConsumer * > tb_conslist;
/// Указатель на объект диспетчер
TMultiPipeProcessor<TValue,currTypeID > * tb_pmpp;
/// Начальная отметка времени
time_point tb_start;
/// Конечная отметка времени
time_point tb_end;
/// Флаг запуска потока
std::atomic<bool> tb_isRunning;
/// Файловый поток для импорта данных
std::ifstream tb_fstm_imp;
/// Файловый поток для экспорта данных
std::ofstream tb_fstm_exp[N_CONSUMERS];
std::mutex tb_mtx_cout;
std::mutex tb_mtx_start;
std::condition_variable tb_cv_start;
bool tb_isStarted = false;

/// Процедура инициаллизации тестбенча
bool InitTestBench(TMultiPipeProcessor<TValue,currTypeID >* & apmpp,
                   TB_TThreadArgs & args,TThreadArgs & _args)
{
    using TMulti_t = TPipeValueType;
    const char * funcname = __PRETTY_FUNCTION__;
    bool result = false;
    std::string fname_exp;
    TB_IConsumer * consumer;
    TKey key;
    TValue value;
    unsigned long long duration;
    try
    {
        // clear all
        tb_pthreads[0] = nullptr;
        tb_conslist.clear();
        tb_keylist.clear();
        tb_isStarted = false;
        for (auto i1 = 0; i1 < N_PRODUCERS; i1++)
        {
                tb_th_keylist[i1].clear();
                tb_th_vallist[i1].clear();
        }
        tb_thargs = &args;
        // platform tests
        std::cout << "sizeof std::atomic<char> is ";
        std::cout << sizeof(std::atomic<char>) << std::endl;
        apmpp = new TMultiPipeProcessor<TMulti_t,
                                        currTypeID>(_args);
        tb_pmpp = apmpp;
        // Prepare
        tb_vallist.clear();
        // Создать множество чисел от 1 до size и поместить его в tb_vallist
        // И вывести на экран
        generate_ordered_set(N_VALUES);
        // Перемешать список чисел из tb_vallist в случайном порядке
        shuffleList();
        // Перемешать список чисел из tb_vallist в случайном порядке
        shuffleList();
        // Записать список чисел из tb_vallist в текстовый файл импорта
        writeListToFileIm();        
        // Чтение из файла tb_fname_imp списка чисел в tb_vallist
        readListFromFile();
        tb_keylist.clear();
        // Открыть файлы в которые будет писать потребитель,
        // создать объекты-потебители и поместить их в вектор
        // tb_conslist
        for (auto i1 = 0; i1 < N_CONSUMERS; i1++)
        {
            fname_exp = tb_fname_exp;
            fname_exp = fname_exp + std::to_string(i1) + ".dat";
            tb_fstm_exp[i1].open(fname_exp,
                            std::ios_base::out | std::ios_base::trunc);
            if (tb_fstm_exp[i1].is_open())
            {
                consumer = new TB_IConsumer(&tb_fstm_exp[i1]);
                tb_conslist.push_back(consumer);
                std::cout << fname_exp << " was created" << std::endl;
            }
        }
        // Засеччь время
        // Подписать все обекты в диспетчере.
        // Измерить время подписывания и показать его
        time_point tb_subscribe_start,
                   tb_subscribe_end;
        tb_subscribe_start = clock::now();
        for_each(tb_conslist.begin(),
                 tb_conslist.end(),[&](TB_IConsumer * consumer)
        {
            tb_pmpp->Subscribe(consumer);
        });
        tb_subscribe_end = clock::now();
        duration = duration_cast<microseconds>(tb_subscribe_end - tb_subscribe_start).count();
        std::cout << "time latancy for Subscribe is:" << std::to_string(duration);
        std::cout << std::endl;
        // Сформировать вектор ключей
        for_each(tb_conslist.begin(),
                 tb_conslist.end(),[&](TB_IConsumer * consumer)
        {
            key = (TKey)consumer->getKey();
            tb_keylist.push_back(key);
        });
        for (auto i1 = 0; i1 < N_PRODUCERS; i1++)
        {
            tb_th_timelist[i1] = clock::now();
        }
        tb_timestamp = clock::now();
        tb_clkstamp = getRDTDC();

        for (auto i1 = 0; i1 < N_PRODUCERS; i1++)
        {
            for (auto j1 = 0; j1 < N_CONSUMERS; j1++)
            {
                tb_th_keylist[i1].push_back(tb_keylist[j1]);
            }
            for_each(tb_vallist.begin(),
                     tb_vallist.end(),[&](unsigned short v)
            {
                value.value = v;
                tb_th_vallist[i1].push_back(value);
            });
        }

        createThreads(args);
        result = true;
    }
    catch (...)
    {
        FinitTestBench();
        errspace::show_errmsg(funcname);
        for (TLockCodesPlace i1 = 0; i1 < 255; i1++)
        {
            tb_send_exit_event(*tb_thargs,
                               TB_TThreadArgs::th_exit_error,
                               i1);
        }
        throw;
    }
    return result;
}

/// Процедура финициаллизации тестбенча
void FinitTestBench()
{
    const char * funcname = __PRETTY_FUNCTION__;
    try
    {
        time_point tp_start = clock::now();
        time_point tp_end = tb_timestamp;
        unsigned long long tc_start = getRDTDC();
        unsigned long long tc_end = tb_clkstamp;
        unsigned long long duration;
        deleteThreads();
        delete tb_pmpp;
        for_each(tb_conslist.begin(),tb_conslist.end(),
                 [&](TB_IConsumer * consumer)
        {
            if (consumer->i_end > tp_end)
            {
                tp_end = consumer->i_end;
            }
            if (consumer->i_cend > tc_end)
            {
                tc_end = consumer->i_cend;
            }
            delete consumer;
        });
        for (auto i1 = 0; i1 < N_PRODUCERS; i1++)
        {
            if (tb_th_timelist[i1] != tb_timestamp)
            {
                if (tb_th_timelist[i1] < tp_start)
                {
                    tp_start = tb_th_timelist[i1];
                }
            }
            if (tb_th_clklist[i1] != tb_clkstamp)
            {
                if (tb_th_clklist[i1] < tc_start)
                {
                    tc_start = tb_th_clklist[i1];
                }
            }
        }
        // mesaure in microseconds
        duration = duration_cast<microseconds>(tp_end - tp_start).count();
        // mesaure in cpu clocks
        //duration = tc_end - tc_start;
        std::cout << "time latancy is:" << std::to_string(duration);
        std::cout << std::endl;
        for (auto i1 = 0; i1 < N_CONSUMERS; i1++)
        {
            tb_fstm_exp[i1].close();
        }
        for (TLockCodesPlace i1 = 0; i1 < 255; i1++)
        {
            tb_send_exit_event(*tb_thargs,
                               TB_TThreadArgs::th_exit_error,
                               i1);
        }
    }
    catch (...)
    {
        errspace::show_errmsg(funcname);
        for (auto i1 = 0; i1 < 255; i1++)
        {
            tb_send_exit_event(*tb_thargs,
                               TB_TThreadArgs::th_exit_error,
                               i1);
        }
    }
}

/// Создание потоков
bool createThreads(TB_TThreadArgs & args)
{
    typedef void (*TB_Tfunc_ptr)( TB_TThreadArgs &,
                                  int _currThreadIndex );
    atomic_store(&tb_isRunning, true);
    TB_Tfunc_ptr func_ptr = &threadLoop;

    for (int i1 = N_PRODUCERS - 1; i1 >= 0; i1--)
    {
        auto resbi = boost::bind(func_ptr,boost::ref(args),i1);
        tb_pthreads[i1] = new(std::nothrow) bthread(resbi);
    }
    //Sleep(2000);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(2000));
    tb_isStarted = true;
    tb_cv_start.notify_all();
}

/// Удаление Потоков
void deleteThreads()
{
    const char * funcname = __PRETTY_FUNCTION__;
    bool result = false;
    atomic_store(&tb_isRunning, false);
    for (int i1 = 0; i1 < N_PRODUCERS; i1++)
    {
        if (tb_pthreads[i1] != nullptr)
        {
            result = tb_pthreads[i1]->try_join_for(
                        boost::chrono::milliseconds(TIMEOUT)
                        );
            if (!result)
            {
                tb_pthreads[i1]->interrupt();
                result = tb_pthreads[i1]->try_join_for(
                            boost::chrono::milliseconds(TIMEOUT)
                            );
            }
            if (!result)
            {
        // forced termination
        #if defined(_WIN32) || defined(_WIN64)
            TerminateThread(tb_pthreads[i1]->native_handle(), 0);
        #elif  defined(__unix)
            pthread_cancel(tb_pthreads[i1]->native_handle());
        #else
            #error Unknown platform
        #endif
            }
            delete tb_pthreads[i1];
            tb_pthreads[i1] = nullptr;
        }
    }
}

/// Процедура потока
void threadLoop(TB_TThreadArgs & args,int _currThreadIndex)
{
    const char * funcname = __PRETTY_FUNCTION__;
    TKey key;
    TLockNodeCodes code = TLockNodeCodes::lock_last;
    TThreadIdType id = 0;
    int n_fails = 0;
    unsigned char currThreadIndex = _currThreadIndex;
    try
    {
        id = getThreadId();
        code = tb_pmpp->RegisterProducer(id);
        tb_mtx_cout.lock();
        //std::cout << "tb_thread";
        //std::cout << std::to_string(code);
        //std::cout << " started" << std::endl;
        tb_mtx_cout.unlock();
        if (code == TLockNodeCodes::lock_last)
        {
            std::cout << "unreg key";
        }
        boost::this_thread::sleep_for(boost::chrono::milliseconds(20));
        std::unique_lock<std::mutex> lock(tb_mtx_start);        
        while (!tb_isStarted)
        {
            tb_cv_start.wait(lock);
            lock.unlock();
            //boost::this_thread::yield();
            //boost::this_thread::sleep_for(boost::chrono::nanoseconds(60));
        }        
        tb_th_timelist[currThreadIndex] = clock::now();
        tb_th_clklist[currThreadIndex] = getRDTDC();
        for_each(tb_th_keylist[currThreadIndex].begin(),
                 tb_th_keylist[currThreadIndex].end(),[&](TKey ky)
        {
            for_each(tb_th_vallist[currThreadIndex].begin(),
                     tb_th_vallist[currThreadIndex].end(),[&](TValue value)
            {
               if (!tb_pmpp->PushValueMT(ky,value,code))
               {
                   n_fails++;
               }
               boost::this_thread::yield();
            });
        });
        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        tb_mtx_cout.lock();
        std::cout << "fails:" << std::to_string(n_fails) << std::endl;
        std::cout << "Sleep 2 seconds..." << std::endl;
        tb_mtx_cout.unlock();
        //startProfiling();
        Sleep(3000);
        //stopProfiling();
        tb_send_exit_event(*tb_thargs,
                           TB_TThreadArgs::th_exit_succ,
                           currThreadIndex);
    }
    catch (...)
    {
        errspace::show_errmsg(funcname);
        tb_send_exit_event(*tb_thargs,
                           TB_TThreadArgs::th_exit_error,
                           currThreadIndex);
    }
}

/// Запись числа в текстовый файл
void writeNumToFile(TValue value)
{
    // Do Nothing
}

/// Чтение из файла числа
void readNumFromFile(TValue value)
{
    // Do Nothing
}

/// Запись списка чисел в текстовый файл экспорта
void writeListToFileEx()
{
    const char * c_coldelim = " ";
    const char c_ledzero = '0';
    const unsigned char c_nd4 = 4;
    const unsigned char c_nd6 = 6;
    unsigned short val;
    /// Файловый поток для импорта данных
    std::ofstream fstm_imp;
    fstm_imp.open(tb_fname_exp,
                    std::ios_base::out | std::ios_base::trunc);
    if (fstm_imp.is_open())
    {
        for_each(tb_vallist.begin(),tb_vallist.end(),[&](unsigned short & val)
        {
            fstm_imp << std::setw(c_nd6) << std::setfill(c_ledzero);
            fstm_imp << val << c_coldelim;
        });
        fstm_imp.close();
    }
}

/// Записать список чисел из tb_vallist в текстовый файл импорта
void writeListToFileIm()
{
    const char * c_coldelim = " ";
    const char c_ledzero = '0';
    const unsigned char c_nd4 = 4;
    const unsigned char c_nd5 = 5;
    unsigned short val;
    /// Файловый поток для импорта данных
    std::ofstream fstm_imp;
    fstm_imp.open(tb_fname_imp,
                    std::ios_base::out | std::ios_base::trunc);
    if (fstm_imp.is_open())
    {
        for_each(tb_vallist.begin(),tb_vallist.end(),[&](unsigned short & val)
        {
            fstm_imp << std::setw(c_nd5) << std::setfill(c_ledzero);
            fstm_imp << val << c_coldelim;
        });
        fstm_imp.close();
    }
}

/// Чтение из файла списка чисел
void readListFromFile()
{
    const char * funcname = __PRETTY_FUNCTION__;
    const char * c_startstageheader = "Start parsing...";
    const char * c_stopstageheader = "=END=";
    const char   c_columndelimiter1 = ' ';
    const unsigned char c_hexbase = 16;
    const unsigned char c_decbase = 10;
    std::stringstream * istrstm = NULL;
    std::string lexema = "";
    unsigned short tmpsint;
    tb_fstm_imp.open(tb_fname_imp);
    if (tb_fstm_imp.is_open())
    {        
        istrstm = new std::stringstream();
        *istrstm << tb_fstm_imp.rdbuf();
        tb_fstm_imp.close();
        std::cout << c_startstageheader << std::endl;
    }
    tb_vallist.clear();
    if (istrstm != nullptr)
    {        
        while (getline((*istrstm),lexema,c_columndelimiter1))
        {
            if (lexema.empty())
            {
               continue;
            }
            if ((is_decnumber(lexema)))
            {
                tmpsint = (unsigned short)strtol(lexema.c_str(),
                                               NULL,
                                               c_decbase);
                tb_vallist.push_back(tmpsint);
            }
        }
        std::copy(tb_vallist.begin(), tb_vallist.end(),
                  std::ostream_iterator<unsigned short>(std::cout, " "));
        std::cout << std::endl << c_stopstageheader << std::endl;
        delete istrstm;
    }
    else
    {
        errspace::show_errmsg(funcname);
    }
}

/// Перемешать список чисел из tb_vallist в случайном порядке
void shuffleList()
{    
    boost::random_device rd;
    typedef boost::mt19937 RNGType;
    RNGType rng(rd);
    boost::uniform_int<> one_to_size( 1, tb_vallist.size());
    boost::variate_generator< RNGType, boost::uniform_int<> >
                dice(rng, one_to_size);
    boost::range::random_shuffle(tb_vallist, dice);
    std::copy(tb_vallist.begin(), tb_vallist.end(),
              std::ostream_iterator<unsigned short>(std::cout, " "));
    std::cout << std::endl;
}

/// Сортировка списка случайный значений
void sortList()
{
    std::sort(tb_vallist.begin(),tb_vallist.end());
    std::copy(tb_vallist.begin(), tb_vallist.end(),
              std::ostream_iterator<unsigned short>(std::cout, " "));
    std::cout << std::endl;
}

/// Старт измерению времени
void startProfiling()
{
    tb_start = clock::now();
}

/// Остановка измерение времени и получение значения
void stopProfiling()
{
    const char * c_dura1 = "\nDuration is: ";
    const char * c_dura2 = " microseconds";
    unsigned long long duration;
    tb_end = clock::now();
    duration = duration_cast<microseconds>(tb_end - tb_start).count();
    std::cout << c_dura1 << duration << c_dura2 << std::endl;
}

/// Send exit event method
void tb_send_exit_event(TB_TThreadArgs & args,
                        TB_TThreadArgs::TTExitCode code,
                        TLockCodesPlace thcode)
{
    TB_TThreadArgs::TTEvent new_event = TB_TThreadArgs::th_event_break;
    atomic_store(&args.out_arg[thcode], code);
    atomic_store(&args.event[thcode], new_event);
}

// Создать множество чисел от 1 до size и поместить его в tb_vallist
// И вывести на экран
void generate_ordered_set(TSize size)
{
    tb_vallist.clear();
    tb_vallist.resize(size);
    std::iota(tb_vallist.begin(),tb_vallist.end(),1);
    std::copy(tb_vallist.begin(), tb_vallist.end(),
              std::ostream_iterator<unsigned short>(std::cout, " "));
    std::cout << std::endl;
}

bool is_hexnumber(const std::string &str)
{
    const char * hextable = "0123645789abcdefABCDEF";
    const int c_valuedigits = 4;
    bool result;
    result = (str.size() == c_valuedigits) &&
             (str.find_first_not_of(hextable) == std::string::npos);
    return result;
}

bool is_decnumber(const std::string &str)
{
    const char * hextable = "0123645789";
    const int c_valuedigits = 5;
    bool result;
    result = (str.size() == c_valuedigits) &&
             (str.find_first_not_of(hextable) == std::string::npos);
    return result;
}

}
#if defined(__GNUC__) || defined(__MINGW32__)
#pragma GCC diagnostic pop
#endif
