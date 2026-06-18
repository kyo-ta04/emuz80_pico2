// license: BSD 3-Clause License
// copyright-holders: Norihiro Kumagai
//
// gpio config for z80 
//
// Pin Definitions
// This section should be located
// before #include "blink.pio.h"
//
// New Pin Assigns, avoid using Pin23 (which we cannot use on WeAct RP2350B CoreBoard)
#if !defined(__Z80_RP2350B_H)
#if defined(RP2350B_CoreBoard)
#if 1
// AE-RP2040 ピンコンパチ
#define ADDR_MASK 0xffff
#define A0_Pin 0
#define D0_Pin 16
#define IORQ_Pin 24
#define RD_Pin 25
#define WR_Pin 26
#define WAIT_Pin 27
#define RESET_Pin 28
#define CLK_Pin  29
//#define TEST_Pin 15
#if defined(TEST_Pin)
#define ADDR_MASK 0x7fff
#endif
#else
#define ADDR_MASK 0xffff
#define A0_Pin 0
#define D0_Pin 24
#define RD_Pin 20
#define WR_Pin 21
#define IORQ_Pin 22
#define WAIT_Pin 16
//#define M1_Pin   XX
#define CLK_Pin  40
#define INT_Pin  41
#define RESET_Pin 42
#define BUSRQ_Pin 43
#define TEST_Pin 45
#endif
#endif
#if defined(RP2350_Zero)
#define ADDR_MASK 0xffff
#define A0_Pin 0
#define D0_Pin 16
#define IORQ_Pin 24
#define RD_Pin 25
#define WR_Pin 26
#define WAIT_Pin 27
#define RESET_Pin 28
#define CLK_Pin  29
//#define TEST_Pin 15
#if defined(TEST_Pin)
#define ADDR_MASK 0x7fff
#endif
#endif
#if defined(AE_RP2040)
#define ADDR_MASK 0xffff
#define D0_Pin 16
#define RD_Pin 25
#define WR_Pin 26
#define IORQ_Pin 24
#define WAIT_Pin 27
#define CLK_Pin  29
#define RESET_Pin 28
//#define TEST_Pin 15
#if defined(TEST_Pin)
#define ADDR_MASK 0x7fff
#endif
#endif
#endif //__Z80_RP2350B_H
