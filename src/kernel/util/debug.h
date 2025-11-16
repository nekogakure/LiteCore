#ifndef _DEBUG_H
#define _DEBUG_H

#define INFO 0
#define WARN 1
#define ERR 2
#define ALL 3

void set_log_level(int level);
void debug(const char *msg, int level);

#endif /* _DEBUG_H */