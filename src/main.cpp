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
#include "iostream"
#if defined(__GNUC__) || defined(__MINGW32__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
int main(int argc, char *argv[])
{
    const char * funcname = __FUNCTION__;
    using boost::chrono::milliseconds;
    using namespace MultiPipeProcessor;
    using TMulti_t = TPipeValueType;
    using TThreadArgs = TPipeTrait<TMulti_t,TESTBENCH::currTypeID>::TThreadArgs;    
    const char * c_exitsstr = "Program successfully terminated";
    const char * c_exitfstr = "Program abnormally terminated";
    const char * c_pausestr = "pause";
    const unsigned c_qtdelay = 1000;
    TThreadArgs args;
    TESTBENCH::TB_TThreadArgs tb_args;
    TMultiPipeProcessor<TMulti_t,TESTBENCH::currTypeID> * mpp = nullptr;
    try
    {
        for (int numRuns = 0; numRuns < 1; numRuns++)
        {
            mpp = nullptr;
            // Delay for launch under qt creator
            boost::this_thread::sleep_for(milliseconds(c_qtdelay));
            // Set loop state for Multipipe queue
            TThreadArgs::TTEvent isRunning = TThreadArgs::th_event_loop;
            args.event = TThreadArgs::th_event_loop;
            args.out_arg = TThreadArgs::th_exit_error;
            //E1<int>* e1 = new E1<int>(9);
            // Initialize testbench object, which examines
            // Multipipe queue
            // and create Multipipe queue object
            TESTBENCH::InitTestBench(mpp,tb_args,args);
            // Wait untill all threads will be terminated
            for (auto i1 = 0; i1 < N_PRODUCERS; i1++)
            {
                if (TESTBENCH::tb_pthreads[i1] != nullptr)
                {
                    TESTBENCH::tb_pthreads[i1]->join();
                }
            }
            TESTBENCH::FinitTestBench();
            while (isRunning)
            {
                isRunning = atomic_load(&args.event);
            }
            if (atomic_load(&args.out_arg) == TThreadArgs::th_exit_succ)
            {
                std::cout << c_exitsstr << std::endl;
            }
            else
            {
                std::cout << c_exitfstr << std::endl;
            }
            system(c_pausestr);
        }
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
