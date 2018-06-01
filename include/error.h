/** @file
 *  @brief         Загаловочный файл модуля вывода сообщений  об ошибки
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.07.2017
 *  @version 1.0 (alpha)
 */
#ifndef ERROR_H
#define ERROR_H
#include "common.h"

namespace errspace
{
extern const char * error_log_filename;
extern FILE * logFile;
extern bool doWriteToFile;
extern bool doShowtoDisplay;

void start_file_log();
void show_errmsg(const char * msg);
void stop_file_log();
void show_errAddons(const unsigned char code);
}

#endif // ERROR_H
