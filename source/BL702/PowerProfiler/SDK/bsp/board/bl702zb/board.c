#include "bflb_uart.h"
#include "bflb_gpio.h"
#include "bflb_clock.h"
#include "bflb_rtc.h"
#include "bflb_flash.h"
#ifdef CONFIG_TLSF
#include "bflb_tlsf.h"
#else
#include "bflb_mmheap.h"
#endif
#include "board.h"
#include "bl702_glb.h"
#include "bl702_sflash.h"

extern uint32_t __HeapBase;
extern uint32_t __HeapLimit;

#ifndef CONFIG_TLSF
struct heap_info mmheap_root;

static struct heap_region system_mmheap[] = {
    { NULL, 0 },
    { NULL, 0 }, /* Terminates the array. */
};
#endif

static struct bflb_device_s *uart0;

#if defined(CONFIG_BFLOG)
static struct bflb_device_s *rtc;
#endif

static void system_clock_init(void)
{
    GLB_Set_System_CLK(GLB_DLL_XTAL_32M, GLB_SYS_CLK_DLL144M);
    GLB_Set_MTimer_CLK(1, GLB_MTIMER_CLK_BCLK, 71);
}

static void peripheral_clock_init(void)
{
    PERIPHERAL_CLOCK_ADC_DAC_ENABLE();
    PERIPHERAL_CLOCK_SEC_ENABLE();
    PERIPHERAL_CLOCK_DMA0_ENABLE();
    PERIPHERAL_CLOCK_UART0_ENABLE();
    PERIPHERAL_CLOCK_UART1_ENABLE();
    PERIPHERAL_CLOCK_SPI0_ENABLE();
    PERIPHERAL_CLOCK_I2C0_ENABLE();
    PERIPHERAL_CLOCK_PWM0_ENABLE();
    PERIPHERAL_CLOCK_TIMER0_1_WDG_ENABLE();
    PERIPHERAL_CLOCK_IR_ENABLE();
    PERIPHERAL_CLOCK_I2S_ENABLE();
    PERIPHERAL_CLOCK_USB_ENABLE();

    GLB_Set_UART_CLK(ENABLE, HBN_UART_CLK_96M, 0);
    GLB_Set_SPI_CLK(ENABLE, 0);
    GLB_Set_I2C_CLK(ENABLE, 0);
    GLB_Set_IR_CLK(ENABLE, GLB_IR_CLK_SRC_XCLK, 15);

    GLB_Set_ADC_CLK(ENABLE, GLB_ADC_CLK_XCLK, 0);
    GLB_Set_DAC_CLK(ENABLE, GLB_DAC_CLK_XCLK, 0x3E);

    GLB_Set_USB_CLK(ENABLE);
}


void bl_show_flashinfo(void)
{
    SPI_Flash_Cfg_Type flashCfg;
    uint8_t *pFlashCfg = NULL;
    uint32_t flashCfgLen = 0;
    uint32_t flashJedecId = 0;

    flashJedecId = bflb_flash_get_jedec_id();
    bflb_flash_get_cfg(&pFlashCfg, &flashCfgLen);
    arch_memcpy((void *)&flashCfg, pFlashCfg, flashCfgLen);
    printf("=========== flash cfg ==============\r\n");
    printf("jedec id   0x%06X\r\n", flashJedecId);
    printf("mid            0x%02X\r\n", flashCfg.mid);
    printf("iomode         0x%02X\r\n", flashCfg.ioMode);
    printf("clk delay      0x%02X\r\n", flashCfg.clkDelay);
    printf("clk invert     0x%02X\r\n", flashCfg.clkInvert);
    printf("read reg cmd0  0x%02X\r\n", flashCfg.readRegCmd[0]);
    printf("read reg cmd1  0x%02X\r\n", flashCfg.readRegCmd[1]);
    printf("write reg cmd0 0x%02X\r\n", flashCfg.writeRegCmd[0]);
    printf("write reg cmd1 0x%02X\r\n", flashCfg.writeRegCmd[1]);
    printf("qe write len   0x%02X\r\n", flashCfg.qeWriteRegLen);
    printf("cread support  0x%02X\r\n", flashCfg.cReadSupport);
    printf("cread code     0x%02X\r\n", flashCfg.cReadMode);
    printf("burst wrap cmd 0x%02X\r\n", flashCfg.burstWrapCmd);
    printf("=====================================\r\n");
}

void bl_show_log(void)
{
    printf("\r\nFW Build:%s,%s\r\n", __TIME__, __DATE__);
}

extern void bflb_uart_set_console(struct bflb_device_s *dev);

static void console_init()
{
    struct bflb_device_s *gpio;

    gpio = bflb_device_get_by_name("gpio");
    bflb_gpio_uart_init(gpio, GPIO_PIN_14, GPIO_UART_FUNC_UART0_TX);
    bflb_gpio_uart_init(gpio, GPIO_PIN_15, GPIO_UART_FUNC_UART0_RX);

    struct bflb_uart_config_s cfg;
    cfg.baudrate = 2000000;
    cfg.data_bits = UART_DATA_BITS_8;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.parity = UART_PARITY_NONE;
    cfg.flow_ctrl = 0;
    cfg.tx_fifo_threshold = 15;
    cfg.rx_fifo_threshold = 15;

    uart0 = bflb_device_get_by_name("uart0");

    bflb_uart_init(uart0, &cfg);
    bflb_uart_set_console(uart0);
}

void board_init(void)
{
    uintptr_t flag;

    flag = bflb_irq_save();

    bflb_flash_init();

    system_clock_init();
    peripheral_clock_init();
    bflb_irq_initialize();

    bflb_irq_restore(flag);

#ifdef CONFIG_TLSF
    bflb_mmheap_init((void *)&__HeapBase, ((size_t)&__HeapLimit - (size_t)&__HeapBase));
#else
    system_mmheap[0].addr = (uint8_t *)&__HeapBase;
    system_mmheap[0].mem_size = ((size_t)&__HeapLimit - (size_t)&__HeapBase);

    if (system_mmheap[0].mem_size > 0) {
        bflb_mmheap_init(&mmheap_root, system_mmheap);
    }
#endif

    console_init();

    bl_show_log();
    bl_show_flashinfo();

    printf("dynamic memory init success,heap size = %d Kbyte \r\n", ((size_t)&__HeapLimit - (size_t)&__HeapBase) / 1024);

    printf("cgen1:%08x\r\n", getreg32(BFLB_GLB_CGEN1_BASE));
#if defined(CONFIG_BFLOG)
    rtc = bflb_device_get_by_name("rtc");
#endif
}

void board_uartx_gpio_init()
{
    struct bflb_device_s *gpio;

    gpio = bflb_device_get_by_name("gpio");
    bflb_gpio_uart_init(gpio, GPIO_PIN_18, GPIO_UART_FUNC_UART1_TX);
    bflb_gpio_uart_init(gpio, GPIO_PIN_19, GPIO_UART_FUNC_UART1_RX);
}

void board_i2c0_gpio_init()
{
    struct bflb_device_s *gpio;

    gpio = bflb_device_get_by_name("gpio");
    /* I2C0_SDA */
    bflb_gpio_init(gpio, GPIO_PIN_7, GPIO_FUNC_I2C0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    /* I2C0_SCL */
    bflb_gpio_init(gpio, GPIO_PIN_8, GPIO_FUNC_I2C0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
}

void board_spi0_gpio_init()
{
    struct bflb_device_s *gpio;

    gpio = bflb_device_get_by_name("gpio");
    /* SPI0_SCLK */
    bflb_gpio_init(gpio, GPIO_PIN_27, GPIO_FUNC_SPI0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);
    /* SPI0_MOSI/MISO  */
    bflb_gpio_init(gpio, GPIO_PIN_0, GPIO_FUNC_SPI0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);
    /* SPI0_MISO/MOSI */
    bflb_gpio_init(gpio, GPIO_PIN_1, GPIO_FUNC_SPI0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);
    /* SPI0_SS */
    //bflb_gpio_init(gpio, GPIO_PIN_2, GPIO_FUNC_SPI0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
}

void board_pwm_gpio_init()
{
    struct bflb_device_s *gpio;

    gpio = bflb_device_get_by_name("gpio");
    bflb_gpio_init(gpio, GPIO_PIN_0, GPIO_FUNC_PWM0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_1, GPIO_FUNC_PWM0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_2, GPIO_FUNC_PWM0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_3, GPIO_FUNC_PWM0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_4, GPIO_FUNC_PWM0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
}

void board_adc_gpio_init()
{
    struct bflb_device_s *gpio;

    gpio = bflb_device_get_by_name("gpio");
    bflb_gpio_init(gpio, GPIO_PIN_9, GPIO_ANALOG | GPIO_SMT_EN | GPIO_DRV_0); // ADC_CHANNEL_7
    bflb_gpio_init(gpio, GPIO_PIN_17, GPIO_ANALOG | GPIO_SMT_EN | GPIO_DRV_0); // ADC_CHANNEL_2
    // bflb_gpio_init(gpio, GPIO_PIN_14, GPIO_ANALOG | GPIO_SMT_EN | GPIO_DRV_0); // ADC_CHANNEL_5
    // bflb_gpio_init(gpio, GPIO_PIN_15, GPIO_ANALOG | GPIO_SMT_EN | GPIO_DRV_0); // ADC_CHANNEL_1
    // bflb_gpio_init(gpio, GPIO_PIN_8, GPIO_ANALOG | GPIO_SMT_EN | GPIO_DRV_0); // ADC_CHANNEL_0 (USB_DM)
    // bflb_gpio_init(gpio, GPIO_PIN_7, GPIO_ANALOG | GPIO_SMT_EN | GPIO_DRV_0); // ADC_CHANNEL_6 (USB_DP)
}

void board_dac_gpio_init()
{
    struct bflb_device_s *gpio;

    gpio = bflb_device_get_by_name("gpio");
    /* DAC_CHA */
    bflb_gpio_init(gpio, GPIO_PIN_11, GPIO_ANALOG | GPIO_SMT_EN | GPIO_DRV_0); // none
    /* DAC_CHB */
    bflb_gpio_init(gpio, GPIO_PIN_17, GPIO_ANALOG | GPIO_SMT_EN | GPIO_DRV_0); // DAC
}

void board_emac_gpio_init()
{
    struct bflb_device_s *gpio;

    gpio = bflb_device_get_by_name("gpio");
    bflb_gpio_init(gpio, GPIO_PIN_0, GPIO_FUNC_EMAC | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_1, GPIO_FUNC_EMAC | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_2, GPIO_FUNC_EMAC | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_7, GPIO_FUNC_EMAC | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_8, GPIO_FUNC_EMAC | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_18, GPIO_FUNC_EMAC | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_19, GPIO_FUNC_EMAC | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_20, GPIO_FUNC_EMAC | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_21, GPIO_FUNC_EMAC | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_22, GPIO_FUNC_EMAC | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);

    /* enable audio clock */
    PDS_Enable_PLL_Clk(PDS_PLL_CLK_48M);
    PDS_Set_Audio_PLL_Freq(AUDIO_PLL_50000000_HZ);
}

#ifdef CONFIG_BFLOG
__attribute__((weak)) uint64_t bflog_clock(void)
{
    return bflb_mtimer_get_time_us();
}

__attribute__((weak)) uint32_t bflog_time(void)
{
    return BFLB_RTC_TIME2SEC(bflb_rtc_get_time(rtc));
}

__attribute__((weak)) char *bflog_thread(void)
{
    return "";
}
#endif
