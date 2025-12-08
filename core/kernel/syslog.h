#ifndef SYSLOG_H
#define SYSLOG_H

void syslog_init(void);
void syslog_write(const char* message);
void syslog_print(const char* message, int color);

#endif // SYSLOG_H
