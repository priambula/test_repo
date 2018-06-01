/** @file
 *  @brief Модуль средств синхронизации
 *  @author https://gist.github.com/sguzman/9594227
 *          https://stackoverflow.com/questions/4548395/
 *          how-to-retrieve-the-thread-id-from-a-boostthread
 *  @date   created Unknown
 *  @date   modified 15.07.2017 Petin Yuriy recycle@List.ru
 *  @version 1.0 (alpha)
 */
#include "syncro.h"

using namespace MultiPipeProcessor;

TThreadIdType MultiPipeProcessor::getThreadId()
{
    const char * sscanf_parameter = "%lx";
    TThreadIdType threadNumber = 0;
    std::string threadId;
    boost::thread::id this_id;
    this_id = boost::this_thread::get_id();
    threadId = boost::lexical_cast<std::string>(this_id);
    sscanf(threadId.c_str(), sscanf_parameter, &threadNumber);
    return threadNumber;
}

native_handle_type MultiPipeProcessor::getThreadIdN(bthread * pth)
{
    native_handle_type id = 0;
    id = pth->native_handle();
    return id;
}


TLockNodeCodes & MultiPipeProcessor::operator++(TLockNodeCodes & r)
{
   r = static_cast<TLockNodeCodes>(static_cast<unsigned char>(r + 1));
   return r;
}


