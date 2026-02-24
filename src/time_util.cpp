#include "time_util.h"

void TimeUtil::initTime(const char* tz) {
    configTzTime(tz, "pool.ntp.org");  
}

String TimeUtil::formatTime(const tm& ti, bool hour24) {
  char buf[16];                                                                 // INIT DISPLAY CHAR BUFFER
  if (!hour24) strftime(buf, sizeof(buf), " %I:%M:%S  %p ", &ti);               // WRITE TEXT TO DISPLAY TO CHAR BUFFER FORMATTED FOR 12HR
  else strftime(buf, sizeof(buf), "   %H:%M:%S   ", &ti);                       // WRITE TEXT TO DISPLAY TO CHAR BUFFER FORMATTER FOR 24HR
  return String(buf);                                                           // RETURN THE TEXT TO DISPLAY AS A STRING
}