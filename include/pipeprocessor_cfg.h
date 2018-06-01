/** @file
 *  @brief Загаловочный файл настроек модуля диспетчера очередей
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.07.2017
 *  @version 1.0 (alpha)
 */
#ifndef PIPEPROCESSOR_CFG_H
#define PIPEPROCESSOR_CFG_H

/// Максимальное количество производителей
#define MAXPRODUCERS 240
/// Максимальное количество потребителей
#define MAXCONSUMERS 1000
/// Максимальный размер очереди
#define MAXCAPACITY  3000
/// Таймаут ожидания при блокировки узла и удаления потока
#define TIMEOUT 250
/// Время сна потока при пусом списке узлов
#define THREAD_IDLE_SLEEP 10
/// Disable valid key check.
#define DISABLE_SET_CHECK

#define MPP_NO_IDLEMODE
#define MPP_NO_MUTEX
/// @namespace MultiPipeProcessor
namespace MultiPipeProcessor
{
      //typedef unsigned short TMultiPipeProcessor_t;
      struct TPipeValueType
      {
            volatile unsigned c_aa = 0x55AA55AA;
            volatile unsigned short value;
      };
}

#endif // PIPEPROCESSOR_CFG_H
