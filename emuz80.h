// license:MIT 3-clauses
// copyright-holders:Norihiro Kumagai

#if !defined(__EMUZ80_H)

// emuz80 configure header
#include "pico/stdlib.h"

// external functions
extern void emuz80_gpio_init(void);
extern void emuz80_pio_init(void);
extern void emuz80_dma_init(void);
extern __attribute__((noinline)) void __time_critical_func(emuz80_core1_entry)(void);
extern void emuz80_unreset(void);
extern void gpio_out_init(uint gpio, bool value);

extern uint8_t __aligned(65536) mem[];



// a magic number for core1/core2 queue
#define FLAG_VALUE 123

#if defined(TEST_Pin)
static int toggle_value = 1;
#if defined(RP2350B)
#define TOGGLE() do {    gpio_xor_mask64(((uint64_t)1)<<TEST_Pin); } while(0)
#define TOGGLE1() do {    gpio_xor_mask64(((uint64_t)1)<<TEST_Pin); sleep_us(1); } while(0)
//#define TOGGLE() do {    (*(volatile uint32_t *)&(sio_hw->gpio_hi_togl)) = 1; } while(0)
//#define TOGGLE() do { gpio_put(TEST_Pin, (toggle_value ^= 1));    } while(0)
#endif
#if defined(RP2350A)
#define TOGGLE() do {    gpio_xor_mask(((uint32_t)1)<<TEST_Pin); } while(0)
#define TOGGLE1() do {    gpio_xor_mask(((uint32_t)1)<<TEST_Pin); sleep_us(1); } while(0)
#endif
#if defined(RP2040)
#define TOGGLE() do {    gpio_xor_mask(((uint32_t)1)<<TEST_Pin); } while(0)
#define TOGGLE1() do {    gpio_xor_mask(((uint32_t)1)<<TEST_Pin); sleep_us(1); } while(0)
#endif
#else   //defined(TEST_Pin)
#define TOGGLE()
#define TOGGLE1()
#endif //defined(TEST_Pin)

#endif //__EMUZ80_H
