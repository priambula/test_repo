/** @file
 *  @brief    Заголовочный файл модуля класса тестирования
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.07.2017
 *  @version 1.0 (alpha)
 */
#ifndef TESTBENCH_H
#define TESTBENCH_H
#include "common.h"
#include "pipeprocessor.h"
#include "syncro.h"
// Для тестбенча имхо позволительна небрежность кода.
#if defined(__GNUC__) || defined(__MINGW32__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"
    #pragma GCC diagnostic ignored "-Wunused-variable"
#endif
namespace TESTBENCH
{
#define N_PRODUCERS 30
#define N_CONSUMERS 3
#define N_VALUES 10
#define CONSUMER_TRIGGER_VALUE N_PRODUCERS*N_VALUES
using namespace MultiPipeProcessor;
/// Тип для значения
typedef TMultiPipeProcessor_t TValue;
using TcurrContainer = _TContainer3< TValue>;
class TB_IConsumer;
using bthread = boost::thread;
using boost::chrono::duration_cast;
//using boost::chrono::milliseconds;
using boost::chrono::microseconds;
typedef boost::chrono::high_resolution_clock clock;
typedef boost::chrono::time_point<clock> time_point;
using TKey = typename TMultiPipeProcessor<TValue,TcurrContainer >::TKey;
/// Тип для структуры аргументов потока
struct TB_TThreadArgs
{
    /// @enum TLockNodeCodes
    /// Список кодов возврата при завершении работы объекта и потока
    enum TTExitCode
    {
        th_exit_succ = 0,
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
    void*  inp_arg[255];
    // Exit code
    std::atomic<TTExitCode> out_arg[255];
    // Event
    std::atomic<TTEvent> event[255];
};
/// Список значений
extern std::vector<TValue> tb_th_vallist[N_PRODUCERS];
/// Список значений
extern std::vector<unsigned short> tb_vallist;
/// Список значений
extern std::vector<TKey> tb_th_keylist[N_PRODUCERS];
/// Список значений
extern std::vector<TKey> tb_keylist;
/// Имя файла для импорта значения или списка значений
extern const char * tb_fname_imp;
/// Имя файла для экспорта значения или списка значений
extern const char * tb_fname_exp;
/// Указатель на объект потока
extern bthread * tb_pthreads[N_PRODUCERS];
/// Список кодов производителя
extern unsigned char tb_prodlist[N_PRODUCERS];
/// Указатель на структуру аргументов потока
extern TB_TThreadArgs * tb_thargs;
/// Список указателей на объекты-потребители
extern std::vector<TB_IConsumer * > tb_conslist;
/// Указатель на объект диспетчер
extern TMultiPipeProcessor<TValue,TcurrContainer > * tb_pmpp;
/// Начальная отметка времени
extern time_point tb_start;
/// Конечная отметка времени
extern time_point tb_end;
/// Флаг запуска потока
extern std::atomic<bool> tb_isRunning;
/// Файловый поток для импорта данных
extern std::ifstream tb_fstm_imp;
/// Файловый поток для экспорта данных
extern std::ofstream tb_fstm_exp[N_CONSUMERS];
/// Процедура инициаллизации тестбенча
bool InitTestBench(TMultiPipeProcessor<TValue,TcurrContainer > * &apmpp,
                   TB_TThreadArgs & args,TThreadArgs &_args);
/// Процедура финициаллизации тестбенча
void FinitTestBench();
/// Создание потоков
bool createThreads(TB_TThreadArgs & args);
/// Удаление Потоков
void deleteThreads();
/// Процедура потока
void threadLoop(TB_TThreadArgs &args, int _currThreadIndex);
/// Запись числа в текстовый файл
void writeNumToFile(TValue value);
/// Чтение из файла числа
void readNumFromFile(TValue value);
/// Запись списка чисел в текстовый файл экспорта
void writeListToFileEx();
/// Запись списка чисел в текстовый файл импорта
void writeListToFileIm();
/// Чтение из файла списка числа
void readListFromFile();
/// Перемешивание списка случайный значений
void shuffleList();
/// Сортировка списка случайный значений
void sortList();
/// Старт измерению времени
void startProfiling();
/// Остановка измерение времени и получение значения
void stopProfiling();
/// Send exit event method
void tb_send_exit_event(TB_TThreadArgs & args,
                        TB_TThreadArgs::TTExitCode code,
                        TLockCodesPlace thcode);
/// Создать список упорядоченных значений
void generate_ordered_set(TSize size);
bool is_hexnumber(const std::string &str);
bool is_decnumber(const std::string &str);

struct TB_IConsumer : public IConsumer<TValue>
{
    TB_IConsumer(std::ofstream * fstm) : i_fstm(fstm)
    {
        i_cc = 0;
        i_end = clock::now();
        i_cend = getRDTDC();
    }
    ~TB_IConsumer()
    {
        std::cout << "consumed:" << std::to_string(i_cc);
        std::cout << " values" << std::endl;
    }
    void Consume(TValue & value)
    {
        const char * c_coldelim = " ";
        const char c_ledzero = '0';
        const unsigned char c_nd4 = 4;
        const unsigned char c_nd5 = 5;
        //*i_fstm << std::setw(c_nd5) << std::setfill(c_ledzero) << value << c_coldelim;
        if (i_cc < CONSUMER_TRIGGER_VALUE)
        {
            i_cc++;
            if (i_cc == CONSUMER_TRIGGER_VALUE)
            {
                i_end = clock::now();
                i_cend = getRDTDC();
            }
        }        
    }
    void * getKey()
    {
        return m_key;
    }
    std::ofstream * i_fstm;
    time_point i_end;
    unsigned long long i_cend;
    unsigned i_cc;
};
}
#if defined(__GNUC__) || defined(__MINGW32__)
#pragma GCC diagnostic pop
#endif
#endif // TESTBENCH_H
