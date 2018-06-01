/** @file
 *  @brief         Модуля вывода сообщений об ошибки
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.07.2017
 *  @version 1.0 (alpha)
 */
#include "error.h"

namespace errspace
{
const char * error_log_filename = TARGET".log";
FILE * logFile = nullptr;
bool doWriteToFile = false;
bool doShowtoDisplay = true;

void show_errmsg(const char * msg) noexcept
{
    const char * prompt = "Error occured in: ";
    const char * rt = "\n";
    if (doShowtoDisplay)
    {
        fputs(prompt,stderr);
        fputs(msg,stderr);
        fputs(rt,stderr);
    }
    if ((doWriteToFile) && (logFile != nullptr))
    {
        fputs(prompt,logFile);
        fputs(msg,logFile);
        fputs(rt,logFile);
        fflush(logFile);
    }
}

void show_errAddons(const unsigned char code)  noexcept
{
    const char c_codes[] = {'0','1','2','3','4'};
    const char * prompt = "with code: ";
    const char * rt = "\n";
    char symbol[2] = {c_codes[0],0};
    symbol[0] = c_codes[code % 5];
    if (doShowtoDisplay)
    {
        fputs(prompt,stderr);
        fputs(symbol,stderr);
        fputs(rt,stderr);
    }
    if ((doWriteToFile) && (logFile != nullptr))
    {
        fputs(prompt,logFile);
        fputs(symbol,logFile);
        fputs(rt,logFile);
        fflush(logFile);
    }
}


void start_file_log()
{
    logFile = fopen(error_log_filename,"w");
    if (logFile != nullptr)
    {
        doWriteToFile = true;
    }
}

void stop_file_log()
{
    fclose(logFile);
    doWriteToFile = false;
    logFile = nullptr;
}

}
// Do Nothing
