/** @mainpage
                 Программа для демонстраии работы
               многоканнального диспетчера очередей.
*/
/** @file
 *  @brief        Главный Модуль
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.10.2017
 *  @version 1.0 (alpha)
*/
#include "windows.h"
#include "pipeprocessor.h"
#include "error.h"
#include "testbench.h"

#if defined(__GNUC__) || defined(__MINGW32__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
int main(int argc, char *argv[])
{    
    using boost::chrono::milliseconds;
    using namespace MultiPipeProcessor;
    using TMulti_t = TMultiPipeProcessor_t;
    const char * funcname = __FUNCTION__;
    try
    {
        boost::this_thread::sleep_for(milliseconds(1000));
        TThreadArgs args;
        TESTBENCH::TB_TThreadArgs tb_args;
        TThreadArgs::TTEvent isRunning = TThreadArgs::th_event_loop;
        TMultiPipeProcessor<TMulti_t,TESTBENCH::TcurrContainer> * mpp = nullptr;
        args.event = TThreadArgs::th_event_loop;
        args.out_arg = TThreadArgs::th_exit_error;

        TESTBENCH::InitTestBench(mpp,tb_args,args);
        if (TESTBENCH::tb_pthreads[0] != nullptr)
        {
            TESTBENCH::tb_pthreads[0]->join();
            TESTBENCH::tb_pthreads[1]->join();
            TESTBENCH::tb_pthreads[2]->join();
        }
        TESTBENCH::FinitTestBench();
        while (isRunning)
        {
            isRunning = atomic_load(&args.event);
        }
        std::cout << "Program successfully terminated" << std::endl;
        system("pause");
    }
    catch (...)
    {
        errspace::show_errmsg(funcname);
    }
    return EXIT_SUCCESS;
}
#if defined(__GNUC__) || defined(__MINGW32__)
#pragma GCC diagnostic pop
#endif
