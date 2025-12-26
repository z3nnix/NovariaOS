#ifndef PAUSE_H
#define PAUSE_H

#define cpu_relax() asm volatile("pause" ::: "memory")

#endif // PAUSE_H