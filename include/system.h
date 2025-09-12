/**
 * @file system.h
 * @brief Core system functions declarations
 * @details
 * Contains declarations for core system initialization functions
 * including GDT, IDT, memory management, and task scheduler.
 * @author nekogakure
 */

#ifndef _SYSTEM_H
#define _SYSTEM_H

/**
 * @brief Initialize Global Descriptor Table
 * @details Sets up the GDT for memory segmentation
 */
void init_gdt(void);

/**
 * @brief Initialize Interrupt Descriptor Table
 * @details Sets up the IDT for handling interrupts
 */
void init_idt(void);

/**
 * @brief Initialize memory management
 * @details Sets up paging and memory allocation systems
 */
void init_memory(void);

/**
 * @brief Initialize task scheduler
 * @details Sets up the multitasking system
 */
void init_scheduler(void);

/**
 * @brief Start the task scheduler
 * @details Begins the multitasking process
 */
void start_scheduler(void);

#endif /* _SYSTEM_H */
