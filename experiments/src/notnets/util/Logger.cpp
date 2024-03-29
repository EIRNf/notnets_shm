
#include <notnets/util/Logger.h>

using namespace notnets::util;

Logger::Logger(){
}



//Not thread safe, do not modify array contents
Logger::Event Logger::ReadLogEvent(unsigned long array_pos)
{
    return g_events[array_pos];
}
