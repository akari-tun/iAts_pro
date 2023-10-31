#include "target/target.h"

#if defined(USE_RX5808)

#include <hal/spi.h>
#include <hal/err.h>
#include <hal/log.h>

#include "rx5808.h"
#include "channels.h"

//static void updateRssiLimits();
//static void writeSerialData();

#define RX5808_SPI_CMD_LEN 25

const hal_spi_device_handle_t spi_handle;

    //
    // Sends SPI command to receiver module to change frequency.
    //
    // Format is LSB first, with the following bits in order:
    //     4 bits - address
    //     1 bit  - read/write enable
    //    20 bits - data
    //
    // Address for frequency select (Synth Register B) is 0x1
    // Expected data is (LSB):
    //     7 bits - A counter divider ratio
    //      1 bit - seperator
    //    12 bits - N counter divder ratio
    //
    // Forumla for calculating N and A is:/
    //    F_lo = 2 * (N * 32 + A) * (F_osc / R)
    //    where:
    //        F_osc = 8 Mhz
    //        R = 8
    //
    // Refer to RTC6715 datasheet for further details.
    //
void rx5808_set_channel(rx5808_t *rx5808, uint8_t channel)
{
    //setSynthRegisterB(getSynthRegisterB(channel));

    uint32_t c = getSynthRegisterB(channel);

    c = c << 5;
    c = c | (1 << 4);
    c = c | RX5808_SPI_ADDRESS_SYNTH_A;

    HAL_ERR_ASSERT_OK(hal_spi_device_transmit_bits(&spi_handle, 0, 0, &c, RX5808_SPI_CMD_LEN, 0, 0));
    rx5808->active_channel = channel;
}

void rx5808_update_rssi(rx5808_t *rx5808) {

}

void rx5808_init(rx5808_t *rx5808) {
    rx5808->rssi_adc_config.channel = BATTERY_ADC_CHAENNL;
    rx5808->rssi_adc_config.atten = BATTERY_ADC_ATTEN;
    rx5808->rssi_adc_config.unit = BATTERY_ADC_UNIT;
    rx5808->rssi_adc_config.bit_width = BATTERY_ADC_WIDTH;

    HAL_ERR_ASSERT_OK(hal_adc_init(&rx5808->rssi_adc_config));

    rx5808->spi_bus = RX5808_SPI_BUS;
    HAL_ERR_ASSERT_OK(hal_spi_bus_init(rx5808->spi_bus, RX5808_SPI_MOSI_GPIO, 0, RX5808_SPI_CLK_GPIO));

    const hal_spi_device_config_t dev_cfg = {
        .command_bits = 0,
        .address_bits = 0,
        .clock_speed_hz = 10*1000*1000,
        .spi_mode = 1,
        .cs = RX5808_SPI_CS_GPIO
    };

    HAL_ERR_ASSERT_OK(hal_spi_bus_add_device(rx5808->spi_bus, &dev_cfg, &spi_handle));
}

void rx5808_update(rx5808_t *rx5808) {
    //updateRssi(rx5808);
}
#endif