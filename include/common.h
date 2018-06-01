/** @file
 *  @brief         Загаловочный файл общих констант и типов данных
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.07.2017
 *  @version 1.0 (alpha)
 */
#ifndef COMMON_H
#define COMMON_H
#include <iostream>
#include <cstdlib>
#include "fstream"
#include "list"
#include "algorithm"
#include "set"
#include "mutex"
#include "atomic"
#include "chrono"
#include "vector"
#include "memory.h"
#include <condition_variable>
#include <thread>
#if defined(__GNUC__) || defined(__MINGW32__)
/// Disable boost warnings
#pragma GCC system_header
#endif

#include "boost/thread.hpp"
#include "boost/random.hpp"
#include "boost/random/random_device.hpp"
#include "boost/range/algorithm.hpp"
#include "boost/range/algorithm/random_shuffle.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/circular_buffer.hpp"
#include "boost/lockfree/queue.hpp"
#include "boost/intrusive/list.hpp"

#if defined(_WIN32) || defined(_WIN64)
    #include "windows.h"
#elif  defined(__unix)
    #include "pthread.h"
#else
    #error Unknown platform
#endif

/// @namespace MultiPipeProcessor
namespace MultiPipeProcessor
{
#if defined(_W64) || defined(_WIN64) || defined(__x86_64__)
    // for 64 bit system
/// @typedef TSize
/// @brief Тип количества очередей
/// выбираемый в зависимости от размера указателя для данной платформы
    typedef long long TSize;
#else
    // for 32 bit system
/// @typedef TSize
/// @brief Тип количества очередей
/// выбираемый в зависимости от размера указателя для данной платформы
    typedef unsigned TSize;
#endif
}
#endif // COMMON_H
