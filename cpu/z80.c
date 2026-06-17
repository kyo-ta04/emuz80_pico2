// license: BSD 3-Clause License
// copyright-holders: Norihiro Kumagai
//
// gpio config for z80
//
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/resets.h"
#include "hardware/regs/addressmap.h"
#include "hardware/clocks.h"
#include "tusb.h"
#include "pico/stdio_usb.h"

#include "conf/z80_rp2350b.h"

#include "emuz80.h"
#include "z80_rp2350a.pio.h"

//
// configure switches
//
#define USE_DMA

//
// Narrow IO Register write
//
// By ORing bit14 of the write_addr register alias, 
// in DMA write operation with size DMA_SIZE_16 or DMA_SIZE_8,
// only the corresponding width (bytes) of the target register is written.
//
#if !defined(REG_ALIAS_NARROW_ZERO_BITS)
#define REG_ALIAS_NARROW_ZERO_BITS (_u(0x1)<<_u(14))
#endif
#define hw_narrow_zero_alias(p) ((void *)((unsigned long int)(p)|REG_ALIAS_NARROW_ZERO_BITS))



extern volatile int rx_rdy, tx_rdy, rx_data, tx_data, txbuf_full;
//
// No `sm_config_set_in_pin_count()` is provided for RP2040
// So, here we have a stub function
//
#if defined(RP2040)
#define sm_config_set_in_pin_count(c, num)
#endif

//
// sys_clock_khz = 400000; // 400MHz Core 1.3V
//
// float clk_divider = 7;      // 7 ... about 28.57MHz NG
// float clk_divider = 8;      // 8 ... about 25MHz
// float clk_divider = 8.3;    // 8.3 ... about 24MHz 
// float clk_divider = 9;      // 9 ... about 22.22MHz 
// float clk_divider = 10;     // 10 ... about 20MHz
// float clk_divider = 11;     // 11 ... about 18.18MHz
float clk_divider = 8.3;    // 13 ... about 15.38MHz



//
// sys_clock_khz = 250000; // 250MHz
//
// float clk_divider = 5;      // 5 ... about 25MHz NG
// float clk_divider = 6;      // 6 ... about 20.83MHz NG
// float clk_divider = 7;      // 7 ... about 17.86MHz
// float clk_divider = 8;      // 8 ... about 15.62MHz
// float clk_divider = 15;      // 15 ... about 8.3MHz
// float clk_divider = 31;      // 31 ... about 4MHz
// float clk_divider = 125;      // 125 ... about 1MHz
// float clk_divider = 250;      // 250 ... about 0.5MHz
// float clk_divider = 500;      // 500 ... about 0.25MHz
// float clk_divider = 1250;      // 1250 ... about 0.1MHz
// float clk_divider = 62500;      // 62500 ... about 2KHz
                            // 30000 for debugging
                            // 9 ... 13.89MHz seems to OK
                            // 8 ... 15.62MHz no good.
void emuz80_gpio_init()
{
    // GPIO Out
    gpio_out_init(WAIT_Pin, true);
    gpio_out_init(RESET_Pin, false);
#if defined(BUSRQ_Pin)
    gpio_out_init(BUSRQ_Pin, true);
#endif
#if defined(INT_Pin)
    gpio_out_init(INT_Pin, false); // INT Pin has an inverter, so negate signal is needed
#endif
#if defined(TEST_Pin)
    gpio_out_init(TEST_Pin, false);
#endif
    // GPIO In
    // MREQ, IORQ, RD, RFSH, M1 are covered by PIO
    //
    gpio_init_mask(ADDR_MASK); // A0-A15 input
    // gpio_init(BUSAK_Pin);
    // gpio_init(MREQ_Pin);
    gpio_init(IORQ_Pin);
    // gpio_init(RFSH_Pin);
    gpio_init(RD_Pin);
    gpio_init(WR_Pin);

    // data bus

    for (int i = 0; i < 8; ++i)
        pio_gpio_init(pio0, D0_Pin + i);
    for (int i = 0; i < 16; ++i)
        pio_gpio_init(pio0, A0_Pin + i);
}

void emuz80_pio_init() {
    //
    // PIO StateMachine(SM) initialzation
    //

    // PIO0: pin assign
	// RD,WR,MREQ,IORQ,WAIT
	pio_gpio_init(pio0, RD_Pin);
	pio_gpio_init(pio0, WR_Pin);
    // PIO1: pin assign
	//pio_gpio_init(pio1, MREQ_Pin);
	pio_gpio_init(pio1, IORQ_Pin);
	pio_gpio_init(pio1, WAIT_Pin); 


    uint offset1;

#if defined(RP2350B)
    // pio_set_gpio_base should be invoked before pio_add_program
    pio_set_gpio_base(pio1, 16);
#endif //defined(RP2350B)
    pio_sm_config c;
    // PIO0:SM0 ... ram_read
    //   IN: A0-A15(0), IN_COUNT: 16
    //   OUT/MOV: D0-Pin(24), OUT_COUNT: 8(D0-D7)
    //   JMP_PIN: RD_Pin(20)
    //   SET_BASE: 31 (debug pin)
    printf("---start---\n");
	offset1 = pio_add_program(pio0, &ram_read_addr_program);
    printf("ram_read_addr: %d\n", offset1);
    pio_sm_set_consecutive_pindirs(pio0, 0, A0_Pin, 16, false);
    //pio_sm_set_consecutive_pindirs(pio0, 0, RD_Pin, 1, false);
    //pio_sm_set_consecutive_pindirs(pio0, 0, IORQ_Pin, 1, false);
    c = ram_read_addr_program_get_default_config(offset1);
    sm_config_set_in_pins(&c, A0_Pin);
    sm_config_set_in_pin_count(&c, 16);
    sm_config_set_in_shift(&c, false, false, 32);    // 16bit autopush
    sm_config_set_out_pins(&c, D0_Pin, 8);
    sm_config_set_out_shift(&c, false, false, 32);    // 8bit autopull
    sm_config_set_jmp_pin(&c, IORQ_Pin);
    sm_config_set_clkdiv(&c, 1);         // 1 ... full speed 
    pio_sm_init(pio0, 0, offset1, &c);

    // PIO0:SM1 ... ram_write_addr
    //   IN: A0-A15(0), IN_COUNT: 16, autopush
    //   OUT/MOV: D0-Pin(24), OUT_COUNT: 8
    //   JMP_PIN: WR_Pin(21)
	offset1 = pio_add_program(pio0, &ram_write_addr_program);
    printf("ram_write_addr: %d\n", offset1);
    pio_sm_set_consecutive_pindirs(pio0, 1, A0_Pin, 16, false);
    pio_sm_set_consecutive_pindirs(pio0, 1, WR_Pin, 1, false);
    pio_sm_set_consecutive_pindirs(pio0, 1, IORQ_Pin, 1, false);
    c = ram_write_addr_program_get_default_config(offset1);
    sm_config_set_in_pins(&c, A0_Pin);
    sm_config_set_in_pin_count(&c, 16);
    sm_config_set_in_shift(&c, false, false, 32); //16);    // 16bit autopush
    sm_config_set_out_pins(&c, D0_Pin, 8);
    sm_config_set_out_pin_count(&c, 8);
    sm_config_set_out_shift(&c, false, false, 8);    // 8bit no-autopush
    sm_config_set_jmp_pin(&c, IORQ_Pin);
    sm_config_set_clkdiv(&c, 1);         // 1 ... full speed 
    pio_sm_init(pio0, 1, offset1, &c);

    // PIO0:SM2: ram_write_data
    //   IN:  D0_Pin(24), count: 8, autopush
    offset1 = pio_add_program(pio0, &ram_write_data_program);
    printf("ram_write_data: %d\n", offset1);
    pio_sm_set_consecutive_pindirs(pio0, 2, D0_Pin, 8, false);  // data as input
    pio_sm_set_consecutive_pindirs(pio0, 2, WR_Pin, 1, false);
    pio_sm_set_consecutive_pindirs(pio0, 2, IORQ_Pin, 1, false);
    c = ram_write_data_program_get_default_config(offset1);
    sm_config_set_in_pins(&c, D0_Pin);
    sm_config_set_in_pin_count(&c, 8);
    sm_config_set_in_shift(&c, false, false, 8);   // no-autopush
    sm_config_set_jmp_pin(&c, IORQ_Pin);
    sm_config_set_clkdiv(&c, 1);         // 1 ... full speed 
    pio_sm_init(pio0, 2, offset1, &c);
 
    // PIO1:SM2 ... two/one phase clock generator(program clockgen)
	// 	 SET: BASE: 40(CLK_Pin, inverted), 41(INT_Pin, inverted)
    offset1 = pio_add_program(pio1, &clockgen_program);
    printf("clockgen: %d\n", offset1);
    int phase = 1;
    pio_gpio_init(pio1, CLK_Pin);
    if (phase == 2)
        pio_gpio_init(pio1, CLK_Pin + 1);
    pio_sm_set_consecutive_pindirs(pio1, 2, CLK_Pin, phase, true);
    c = clockgen_program_get_default_config(offset1);
    // set_set_pin_base should have been adjusted by pio->gpiobase
    // so far not so in set_set_pin_base();
    sm_config_set_set_pins(&c, CLK_Pin, phase);
    // two-phase: (4 instruction loop)
    //  16.0 ... 2.33MHz (420ns/cycle)
    //   9.42 ... 4.0MHz  (250ns/cycle)
    // single clock: (2 instruction loop)
    //  50.0 ... 1.5MHz (660-670ns) 
    //  30.0 ... 2.5MHz (400ns) ... no wait, 
    //  18.7 ... 4.0-4.17MHz (250-260ns) .... 0/1 wait in M1, 1 wait in WR, 0/1 wait in RD
    //  10.0 ... 7.1-7.6MHz (130-140ns) ... seems to work
    //   5.0 ... 14-16MHz (60-70ns) ... does not works
    sm_config_set_clkdiv(&c, clk_divider); // 12.0 ... 6.25MHz max
    pio_sm_init(pio1, 2, offset1, &c);

    uint32_t sysclk = clock_get_hz(clk_sys);

    printf("CLK: %dMHz, div: %0.1f, Z80: %.2fMHz\n", sysclk/1000000, clk_divider, (double)((sysclk/2)/clk_divider)/1000000.0);

    // PIO0:SM3 ... data_out
    //   OUT/MOV: D0-Pin(24), OUT_COUNT: 8(D0-D7)
    //   SET_BASE: WAIT
	offset1 = pio_add_program(pio0, &data_out_program);
    printf("data_out: %d\n", offset1);
    //pio_sm_set_consecutive_pindirs(pio0, 3, A0_Pin, 16, false);
    pio_sm_set_consecutive_pindirs(pio0, 3, RD_Pin, 1, false);
    c = data_out_program_get_default_config(offset1);
    //sm_config_set_in_pins(&c, A0_Pin);
    //sm_config_set_in_pin_count(&c, 16);
    //sm_config_set_in_shift(&c, false, false, 32);    // 16bit autopush
    sm_config_set_out_pins(&c, D0_Pin, 8);
    sm_config_set_out_shift(&c, true, false, 32);    // 8bit autopull
    //sm_config_set_jmp_pin(&c, RD_Pin);
    sm_config_set_clkdiv(&c, 1);         // 1 ... full speed 
    pio_sm_init(pio0, 3, offset1, &c);


    // PIO1: SM3 ... IO cycle WAIT handler
    //   SET: BASE: 19(WAIT_Pin)
    //   wait: 18(IORQ_Pin)
    offset1 = pio_add_program(pio1, &iorq_wait_program);
    //iorq_wait_program_init(pio1, 3, offset1, WAIT_Pin, D0_Pin);
	//   IN: IORQ_Pin(24), count 1
	//	 SET: WAIT_Pin(27), count: 1
    //pio_sm_set_consecutive_pindirs(pio, sm, iorq_pin, 1, false);
    pio_sm_set_consecutive_pindirs(pio1, 3, WAIT_Pin, 1, true);
    c = iorq_wait_program_get_default_config(offset1);
    sm_config_set_in_pins(&c, D0_Pin);
    sm_config_set_in_pin_count(&c, 8);
    sm_config_set_in_shift(&c, false, false, 8);
    sm_config_set_out_pins(&c, D0_Pin, 8);
    sm_config_set_out_shift(&c, false, false, 8);
    sm_config_set_set_pins(&c, WAIT_Pin, 1);
    sm_config_set_set_pin_count(&c, 1);
    sm_config_set_clkdiv(&c, 1);         // 1 ... full speed 
    pio_sm_init(pio1, 3, offset1, &c);

    printf("iorq_wait = %d\n", offset1);


    // input override
    // These should be below pio_gpio_init
#if defined(RP2040)
    for (int i = WAIT_Pin; i < 30 ; i++) {
        printf ("inover: %d\n", i);
        gpio_set_input_enabled(i, false);
        gpio_set_inover(i, GPIO_OVERRIDE_LOW);
        gpio_set_slew_rate(i, GPIO_SLEW_RATE_FAST);
    }
#endif
}

void emuz80_unreset(void) {
    gpio_put(RESET_Pin, true);
}

int ch_r_base, ch_r_addr, ch_r_data, ch_w_addr, ch_w_data, ch_w_base;
volatile uint32_t addr_temp;
volatile uint32_t *dummy = (uint32_t *)0x12345678;
const uint32_t *wr_addr = (uint32_t *)&mem[0x5638];
static uint32_t *base_addr = (uint32_t *)&mem[0];

void emuz80_dma_init()
{
#if defined(USE_DMA)
    ch_r_base = dma_claim_unused_channel(true);
    ch_r_addr = dma_claim_unused_channel(true);
    ch_r_data = dma_claim_unused_channel(true);
    ch_w_base = dma_claim_unused_channel(true);
    ch_w_addr = dma_claim_unused_channel(true);
    ch_w_data = dma_claim_unused_channel(true);

    // Ch_R_Data: RAM -> PIO TX FIFO
    dma_channel_config cr_data = dma_channel_get_default_config(ch_r_data);
    channel_config_set_transfer_data_size(&cr_data, DMA_SIZE_8);
    channel_config_set_read_increment(&cr_data, false);
    channel_config_set_write_increment(&cr_data, false);
    //channel_config_set_dreq(&cr_data, pio_get_dreq(pio0, 0, true));
    channel_config_set_chain_to(&cr_data, ch_r_base);
    dma_channel_configure(ch_r_data, &cr_data, 
        &pio0_hw->txf[3],//&pio0_hw->txf[0],
        base_addr, 
        1, 
        false);
    printf("base_addr: %08lX\n", base_addr);
    printf("ch_r_read_addr: %08lX\n", dma_hw->ch[ch_r_data].al1_read_addr);
    printf("&ch_r_read_addr: %08lX\n", &dma_hw->ch[ch_r_data].al1_read_addr);

    // Ch_R_Addr: PIO RX FIFO -> READ_ADDR Register in CH_R_data (16bit ring buffer)
    dma_channel_config cr_addr = dma_channel_get_default_config(ch_r_addr);
    channel_config_set_transfer_data_size(&cr_addr, DMA_SIZE_32);
    channel_config_set_read_increment(&cr_addr, false);
    channel_config_set_write_increment(&cr_addr, false);
    channel_config_set_dreq(&cr_addr, pio_get_dreq(pio0, 0, false));
    channel_config_set_chain_to(&cr_addr, ch_r_data);
    volatile uint32_t *ch_r_read_addr_set_alias = hw_set_alias(&dma_hw->ch[ch_r_data].read_addr);
    printf("ch_r_read_addr_set_alias: %08lX\n", ch_r_read_addr_set_alias);
    dma_channel_configure(ch_r_addr, &cr_addr, 
        ch_r_read_addr_set_alias, // //ch_r_read_addr_set_alias, 
        &pio0_hw->rxf[0], 
        1, 
        false);
    //printf("ch_r_data: %d, ch_r_addr: %d\n", ch_r_data, ch_r_addr);
    printf("dma_hw->ch[ch_r_data].read_addr: %08lX\n", dma_hw->ch[ch_r_data].al1_read_addr);
#if 1
    // Ch_R_Base: &mem[0] -> ch_r_addr->read_addr
    dma_channel_config cr_base = dma_channel_get_default_config(ch_r_base);
    channel_config_set_transfer_data_size(&cr_base, DMA_SIZE_32);
    channel_config_set_read_increment(&cr_base, false);
    channel_config_set_write_increment(&cr_base, false);
    channel_config_set_chain_to(&cr_base, ch_r_addr);
    dma_channel_configure(ch_r_base, &cr_base, 
        &dma_hw->ch[ch_r_data].al1_read_addr, 
        &base_addr, 
        1, 
        false);
#endif
#if 1
    // hw_narrow_zero_aliasesは効いていないようだ。readと同じく base->addr->dataの
    // 3段構えとする。
    //uint8_t *ch_w_write_addr_set_alias = hw_narrow_zero_alias(hw_set_alias(&dma_hw->ch[ch_w_data].al1_write_addr));
    // Ch W_Data: PIO RX FIFO -> RAM
    dma_channel_config cw_data = dma_channel_get_default_config(ch_w_data);
    channel_config_set_transfer_data_size(&cw_data, DMA_SIZE_8);
    channel_config_set_read_increment(&cw_data, false);
    channel_config_set_write_increment(&cw_data, false);
    channel_config_set_dreq(&cw_data, pio_get_dreq(pio0, 2, false));
    channel_config_set_chain_to(&cw_data, ch_w_base);
    dma_channel_configure(ch_w_data, &cw_data,
        NULL,
        &pio0->rxf[2],
        1,
        false);
    
    // Ch W_Addr: PIO RX FIFO -> WRITE_ADDR register in Ch W_Data (ring buffer)
    dma_channel_config cw_addr = dma_channel_get_default_config(ch_w_addr);
    channel_config_set_transfer_data_size(&cw_addr, DMA_SIZE_32);
    channel_config_set_read_increment(&cw_addr, false);
    channel_config_set_write_increment(&cw_addr, false);
    channel_config_set_dreq(&cw_addr, pio_get_dreq(pio0, 1, false));
    channel_config_set_chain_to(&cw_addr, ch_w_data);
    volatile uint32_t *ch_w_write_addr_set_alias = hw_set_alias(&dma_hw->ch[ch_w_data].al1_write_addr);
    printf("ch_w_write_addr_set_alias: %08lX\n", ch_w_write_addr_set_alias);
    dma_channel_configure(ch_w_addr, &cw_addr,
        ch_w_write_addr_set_alias,
        &pio0_hw->rxf[1],
        1, 
        false);
    printf("hw_set_addr: %08X, write_addr: %08X\n", hw_set_alias(&dma_hw->ch[ch_w_data].al1_write_addr), &dma_hw->ch[ch_w_data].al1_write_addr);
    printf("pio0->rxf[1]: %08X\n", &pio0->rxf[1]);

#if 1
    // Ch_W_Base: &mem[0] -> ch_w_addr->read_addr
    dma_channel_config cw_base = dma_channel_get_default_config(ch_w_base);
    channel_config_set_transfer_data_size(&cw_base, DMA_SIZE_32);
    channel_config_set_read_increment(&cw_base, false);
    channel_config_set_write_increment(&cw_base, false);
    channel_config_set_chain_to(&cw_base, ch_w_addr);
    dma_channel_configure(ch_w_base, &cw_base, 
        &dma_hw->ch[ch_w_data].al1_write_addr, 
        &base_addr, 
        1, 
        false);
#endif
#endif
#endif //USE_DMA
}


// コア1のエントリポイント
// core1_entry()はPIOの状態マシンを実行し、ROMデータを送信する
__attribute__((noinline)) void __time_critical_func(emuz80_core1_entry)(void)
{
    // main loop
    register uint32_t port;
    int32_t count = 100;
    uint16_t c = 0;
    int32_t temp;
    uint32_t status;
    uint16_t addr;
    uint8_t data;
    volatile register uint32_t a32, d32;
    int xcount = 40;


    multicore_fifo_push_blocking(FLAG_VALUE);
    uint32_t g = multicore_fifo_pop_blocking();
#if defined (USE_DMA)
    // safe DMA abort
    // 1. エラッタ RP2350-E5 対策: 先にチャンネルの有効化を解除する
    dma_hw->ch[0].al1_ctrl &= ~DMA_CH0_CTRL_TRIG_EN_BITS;
    dma_hw->ch[1].al1_ctrl &= ~DMA_CH0_CTRL_TRIG_EN_BITS;
    dma_hw->ch[2].al1_ctrl &= ~DMA_CH0_CTRL_TRIG_EN_BITS;
    dma_hw->ch[3].al1_ctrl &= ~DMA_CH0_CTRL_TRIG_EN_BITS;
    dma_hw->ch[4].al1_ctrl &= ~DMA_CH0_CTRL_TRIG_EN_BITS;
    dma_hw->ch[5].al1_ctrl &= ~DMA_CH0_CTRL_TRIG_EN_BITS;
    // （もし別チャンネルへのチェーン設定がある場合、その先のチャンネルも同様にクリアする）

    // 2. チャンネルのアボート（強制終了）を要求
    dma_channel_abort(0);
    dma_channel_abort(1);
    dma_channel_abort(2);
    dma_channel_abort(3);
    dma_channel_abort(4);
    dma_channel_abort(5);

    // 3. アボートが完全に完了するまでループで待機（RP2350の安全な作法）
    // アボート中は該当ビットが1になり、完了すると0に戻ります
    while (dma_channel_is_busy(0) ||
           dma_channel_is_busy(1) ||
           dma_channel_is_busy(2) || 
           dma_channel_is_busy(3) || 
           dma_channel_is_busy(4) || 
           dma_channel_is_busy(5)) {
            tight_loop_contents();
    }
#endif //USE_DMA
    // Z80 Input pin initialize
    emuz80_gpio_init();
    emuz80_pio_init();
    emuz80_dma_init();

    for (int i = 0; i < 0x10; ++i) {
        printf("%02X ", mem[i]);
        if (i % 8 == 7) printf("\n");
    }

    // restart pio state machnes
    pio_sm_set_enabled(pio0, 0, false);  // ram_read
    pio_sm_clear_fifos(pio0, 0);
    pio_sm_restart(pio0, 0);
    pio_sm_set_enabled(pio0, 1, false);  // ram_write_addr
    pio_sm_clear_fifos(pio0, 1);
    pio_sm_restart(pio0, 1);
    pio_sm_set_enabled(pio0, 2, false);  // ram_write_data
    pio_sm_clear_fifos(pio0, 2);
    pio_sm_restart(pio0, 2);
    pio_sm_set_enabled(pio0, 3, false);  // ram_write_data
    pio_sm_clear_fifos(pio0, 3);
    pio_sm_restart(pio0, 3);

    // start target CPU clock
    pio_sm_set_enabled(pio1, 2, true);  // clockgen
    // wait for some time, which seems to be needed
    // CPU clock: (1000 * 2 / 250) (ns) * clk_divider
    float period = ((1000 * 2 / 250) * clk_divider) * 10 / 1000;
    printf("period: %.0f\n", period);
    sleep_us((int)period);  // clk_divider / 250MHz
    // start target CPU
    emuz80_unreset();

#if defined(USE_DMA)
    dma_channel_start(ch_r_base);
    dma_channel_start(ch_w_base); //dma_channel_start(ch_w_base);
#endif
    // start pio state machines and dma's
    pio_sm_set_enabled(pio0, 0, true);  // ram_read
    pio_sm_set_enabled(pio0, 1, true);  // ram_write_addr
    pio_sm_set_enabled(pio0, 2, true);  // ram_write_data
    pio_sm_set_enabled(pio0, 3, true);  // data_out
    pio_sm_set_enabled(pio1, 3, true);  // iorq_wait
#if defined(USE_DMA)
//#define TEST_DMA
#if defined(TEST_DMA)
    // write pio test
    volatile uint8_t d2;
    xcount = 10;
    while (1) {
#define TEST_MEMRD
#if defined(TEST_MEMRD)
        while ((gpio_get_all() & ((1<<RD_Pin)|(1<<WR_Pin))) == ((1<<RD_Pin)|(1<<WR_Pin)))
            ;
        a32 = dma_hw->ch[ch_r_data].read_addr;
        if ((gpio_get_all() & (1<<RD_Pin)) == 0) {
            port = gpio_get_all();
            d2 = *(uint8_t *)a32;
            if (xcount > 0) { 
                printf("R a%08lX %08lX %08lX %02X\n", port, a32, addr_temp, d2);
                xcount--; 
            }
            while ((gpio_get_all() & (1<<RD_Pin)) == 0)
                ;
            continue;
        }
#endif
#define TEST_MEMWR
#if defined(TEST_MEMWR)
        if ((gpio_get_all() & (1<<WR_Pin)) == 0) {
            port = gpio_get_all();
            a32 = dma_hw->ch[ch_w_data].al1_write_addr;
            //sleep_us(1);
            if (xcount > 0) { 
                printf("W a%08lX %08lX %02X\n", port, a32, mem[0x5638]);
                xcount--; 
            }
            if (xcount == 1) {
                for (int i = 0; i < 0x10000; ++i) {
                    if (i >= 9 && mem[i] != 0) {
                        //printf("X %04X %02X\n", i, mem[i]);
                    }
                }
            }
            while ((gpio_get_all() & (1<<WR_Pin)) == 0)
                ;
            continue;
        }
#endif //TEST_MEMWR
    }
#endif //TEST_DMA
#endif //USE_DMA
#if 0
loop:
    while (((port = gpio_get_all()) & ((1 << RD_Pin)|(1 << WR_Pin))) == ((1 << RD_Pin)|(1 << WR_Pin)))
        ;
#if defined(USE_DMA)
    if (xcount > 0) printf("X ");
    //a32 = (uint32_t)&(mem[gpio_get_all()&ADDR_MASK]);
    a32 = (uint32_t)&mem[0] + (gpio_get_all() & ADDR_MASK);
    //dma_hw->ch[ch_r_data].al3_read_addr_trig = a32;
    sleep_us(1);
    if (xcount > 0) {
        temp = dma_hw->ch[ch_w_data].write_addr;
        printf("%08lX, %08lX\n", temp, gpio_get_all());
        xcount--;
    }
#else
    // using CPU loop
    if (xcount > 0) printf("X ");
    a32 = pio_sm_get_blocking(pio0, 0);
    data = mem[a32&ADDR_MASK];
    pio_sm_put(pio0, 0, data);
    if (xcount > 0) {
        printf("%08lX: %02lX\n", a32, data);
        xcount--;
    }
#endif
    while (((port = gpio_get_all()) & ((1 << RD_Pin)|(1 << WR_Pin))) != ((1 << RD_Pin)|(1 << WR_Pin)))
        ;
    goto loop;
#endif

#if 0 // PIO DEBUGGING, soft PIO drive code
    while (1) {
        while (((port = gpio_get_all()) & ((1 << RD_Pin)|(1 << WR_Pin))) == ((1 << RD_Pin)|(1 << WR_Pin)))
            ;
        // using CPU loop
        if (xcount > 0) printf("X ");
        a32 = pio_sm_get_blocking(pio0, 0);
        data = mem[a32&ADDR_MASK];
        pio_sm_put(pio0, 3, data);
        if (xcount > 0) {
            printf("%08lX: %02lX\n", a32, data);
            xcount--;
        }
        while (((port = gpio_get_all()) & ((1 << RD_Pin)|(1 << WR_Pin))) != ((1 << RD_Pin)|(1 << WR_Pin)))
            ;
    }
#endif
loop:
    // IO R/W cycle
    // while (((port = gpio_get_all()) & ((1<<RD_Pin)|(1<<WR_Pin))) == ((1<<RD_Pin)|(1<<WR_Pin)) )
    //    ;
    while (((port = gpio_get_all()) & (1<<IORQ_Pin)) != 0)
       ;
    while (((port = gpio_get_all()) & (1<<IORQ_Pin)) == 0) {
        if ((port & (1 << RD_Pin)) == 0 || (port & (1 << WR_Pin)) == 0) {
            break;
        }
    }
    if ((port & (1 << IORQ_Pin)) == 0) {
        if ((port & (1 << RD_Pin)) == 0) {
            // IO Read cycle
            // EMUZ80 UART emulation
            addr = port & 0xff;
            // UARTCR or DR
            if (addr == 1) {
                status = (rx_rdy | (tx_rdy & ~txbuf_full));
                // read status register
                // data = status;
                // printf("IO Read(UART SR): %02X\n", status);
                pio_sm_put(pio0, 3, status);
            } else if (addr == 0) {
                // read data register
                data = rx_data;
                rx_rdy = 0;
                pio_sm_put(pio0, 3, data);
            } else {
                // other IO read
                pio_sm_put(pio0, 3, 0);
            }
        } else if ((port & (1 << WR_Pin)) == 0) {
            // IO Write cycle
            addr = port & 0xff;
            data = ((port >> D0_Pin) & 0xff);
            if (addr == 0) {
                // printf("IO Write(UART TX): %02X\n", data);
                // sleep_ms(100);
                // UART DR
                tx_data = data;
                txbuf_full = 2;
            }
        }
        pio_sm_put(pio1, 3, 0);       // notify IO process finished to the state machine
        pio_sm_get_blocking(pio1, 3); // wait for WAIT set High
#if 0
        uint8_t c = (port & (1<<RD_Pin)) ? 'w' : 'r';
        port = gpio_get_all();
        if (xcount > 0 && c == 'w' ) { 
            printf("%c %08lX %02X\n", c, port, (data&0xff));
            xcount--;
        }
        while (((port = gpio_get_all()) & (1 << IORQ_Pin)) == 0)
            ; // wait for cycle end
            // wait for IORQ is High
    } else { // mem read/write
        sleep_us(1);
        port = gpio_get_all();
        uint8_t c = (port & (1<<RD_Pin)) ? 'W' : 'R';
        int flag_x = 0;
        xcount = 0;
        if (xcount > 0 && ((port&ADDR_MASK)&0xff00) == 0x8000 /*&& c == 'W'*/ /*&& (((port>>D0_Pin)&0xff) != mem[port&ADDR_MASK])*/) { 
            printf("%c %08lX %02X ", c, port, mem[port & ADDR_MASK]);
            for (int i = 0; i < 4; ++i)
                printf("%02X ", mem[(port & ADDR_MASK) + i]);
            if (c == 'W')
                printf(" %04lX", dma_hw->ch[ch_w_data].al1_write_addr);
            printf("\n");
            if (c == 'W') {
                flag_x = 1;
            }
            if (flag_x)
                xcount--;
        }
        while (((port = gpio_get_all()) & ((1<<RD_Pin)|(1<<WR_Pin))) != ((1<<RD_Pin)|(1<<WR_Pin)) )
            ;
#endif
    while (((port = gpio_get_all()) & (1<<IORQ_Pin)) == 0)
        ;
    }
    goto loop;
}
