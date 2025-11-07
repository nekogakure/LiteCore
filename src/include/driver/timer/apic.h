#ifndef _DRIVER_TIMER_APIC_H
#define _DRIVER_TIMER_APIC_H

#include <config.h>
#include <stdint.h>

/* APIC レジスタのベースアドレス (デフォルト) */
#define APIC_BASE_DEFAULT 0xFEE00000

/* APIC レジスタオフセット */
#define APIC_ID            0x020   /* Local APIC ID */
#define APIC_VERSION       0x030   /* Local APIC Version */
#define APIC_TPR           0x080   /* Task Priority Register */
#define APIC_EOI           0x0B0   /* End Of Interrupt */
#define APIC_LDR           0x0D0   /* Logical Destination Register */
#define APIC_DFR           0x0E0   /* Destination Format Register */
#define APIC_SPURIOUS      0x0F0   /* Spurious Interrupt Vector Register */
#define APIC_ESR           0x280   /* Error Status Register */
#define APIC_TIMER_LVT     0x320   /* LVT Timer Register */
#define APIC_TIMER_INIT    0x380   /* Timer Initial Count */
#define APIC_TIMER_CURRENT 0x390   /* Timer Current Count */
#define APIC_TIMER_DIV     0x3E0   /* Timer Divide Configuration */

/* APIC Timer モード */
#define APIC_TIMER_MODE_ONESHOT  0x00000000
#define APIC_TIMER_MODE_PERIODIC 0x00020000
#define APIC_TIMER_MODE_TSCDEADLINE 0x00040000

/* APIC Timer 分周比 */
#define APIC_TIMER_DIV_1   0x0B
#define APIC_TIMER_DIV_2   0x00
#define APIC_TIMER_DIV_4   0x01
#define APIC_TIMER_DIV_8   0x02
#define APIC_TIMER_DIV_16  0x03
#define APIC_TIMER_DIV_32  0x08
#define APIC_TIMER_DIV_64  0x09
#define APIC_TIMER_DIV_128 0x0A

int apic_timer_init(void);
int apic_timer_available(void);
void apic_timer_tick(uint32_t irq, void *context);
uint64_t apic_get_uptime_us(void);
uint64_t apic_get_uptime_ms(void);
uint32_t apic_timer_get_frequency(void);
void apic_timer_delay_us(uint32_t us);
void apic_timer_delay_ms(uint32_t ms);

#endif /* _DRIVER_TIMER_APIC_H */