
#include "pico.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "Adafruit_I2CDevice.h"

// Regular I2C Setup
#define I2C_PORT i2c1
#define I2C_SDA 2
#define I2C_SCL 3
#define I2C_BAUDRATE 100000

// Optional callback to service USB during long init sequences
static void (*poll_callback)(void) = NULL;

void neotrellis_set_poll_callback(void (*cb)(void)) {
    poll_callback = cb;
}

static void poll_delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        if (poll_callback) poll_callback();
        sleep_ms(1);
    }
}

///< The class which defines how we will talk to this device over I2C
Adafruit_I2CDevice::Adafruit_I2CDevice(uint8_t addr, TwoWire *theWire)
{
    _addr = addr;
    _maxBufferSize = 250;
};

uint8_t Adafruit_I2CDevice::address(void)
{
     return _addr; 
}

// Clock SCL to recover the I2C bus if a device is holding SDA low
static void i2c_bus_recovery(void) {
    gpio_init(I2C_SCL);
    gpio_init(I2C_SDA);
    gpio_set_dir(I2C_SCL, GPIO_OUT);
    gpio_set_dir(I2C_SDA, GPIO_IN);
    gpio_pull_up(I2C_SDA);
    
    for (int i = 0; i < 16; i++) {
        if (gpio_get(I2C_SDA)) break;  // SDA released, bus is free
        gpio_put(I2C_SCL, 0);
        sleep_us(5);
        gpio_put(I2C_SCL, 1);
        sleep_us(5);
    }
}

bool Adafruit_I2CDevice::begin(bool addr_detect ) 
{
    poll_delay_ms(200);
    
    // Recover I2C bus in case a device is holding SDA low
    i2c_bus_recovery();
    
    i2c_init(I2C_PORT, I2C_BAUDRATE);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    //   i2c_init(I2C_PORT, 100 * 1000);

    //  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    // gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    //   gpio_pull_up(I2C_SDA);
    // gpio_pull_up(I2C_SCL);
    return true;
};

void Adafruit_I2CDevice::end(void)
{

};

bool Adafruit_I2CDevice::detected(void)
{
    return true;
};

bool Adafruit_I2CDevice::read(uint8_t *buffer, size_t len, bool stop)
{
    i2c_read_blocking(I2C_PORT, _addr, buffer, len, !stop); // stop is a guess
    return true;
};

uint8_t bigbuf[255];
bool Adafruit_I2CDevice::write(const uint8_t *buffer, size_t len, bool stop , const uint8_t *prefix_buffer, size_t prefix_len)
{
    size_t C = 0;
    if (prefix_buffer != 0 && prefix_len > 0) for (int i = 0;i<prefix_len;i++) bigbuf[C++] = prefix_buffer[i];
    if (buffer != 0 && len > 0) for (int i = 0;i<len;i++) bigbuf[C++] = buffer[i];
    int count = i2c_write_blocking(I2C_PORT, _addr, bigbuf, C, !stop);

    return true;
};

bool Adafruit_I2CDevice::write_then_read(const uint8_t *write_buffer, size_t write_len, uint8_t *read_buffer, size_t read_len, bool stop)
{
    return true;
};

bool Adafruit_I2CDevice::setSpeed(uint32_t desiredclk)
{
    return true;
};