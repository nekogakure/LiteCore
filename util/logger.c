#ifndef LOGGER_H
#define LOGGER_H

#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define debug(fmt, ...) ((void)0)
#endif

#endif /* LOGGER_H */