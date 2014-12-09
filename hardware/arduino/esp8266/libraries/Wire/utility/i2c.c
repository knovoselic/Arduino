#include <Arduino.h>
#include "i2c.h"


#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"

size_t ets_printf(const char*, ...);
#define DEBUGV ets_printf

static uint8_t s_sda_pin = 0;
static uint8_t s_scl_pin = 2;


static inline void i2c_set(int sda, int scl)
{
	digitalWrite(s_sda_pin, sda);
	digitalWrite(s_scl_pin, scl);
}

static inline void i2c_set_sda(int sda)
{
	digitalWrite(s_sda_pin, sda);
}

static inline void i2c_set_scl(int scl)
{
	digitalWrite(s_scl_pin, scl);
}

static inline uint8_t i2c_get_sda()
{
	return GPIO_INPUT_GET(GPIO_ID_PIN(s_sda_pin));
}

static inline void i2c_wait()
{
	delayMicroseconds(5);
}

void i2c_init(int sda_pin, int scl_pin)
{
	s_sda_pin = sda_pin;
	s_scl_pin = scl_pin;
	pinMode(sda_pin, OUTPUT_OPEN_DRAIN);
	pinMode(scl_pin, OUTPUT_OPEN_DRAIN);
	i2c_set(1, 1);
	i2c_wait();
}


void i2c_release()
{
	pinMode(s_sda_pin, INPUT);
	pinMode(s_scl_pin, INPUT);
}

void i2c_start()
{
	i2c_set(1, 1);
	i2c_wait();
	i2c_set_sda(0);
	i2c_wait();
	i2c_wait();
	i2c_set_scl(0);
	i2c_wait();
}

void i2c_stop()
{
	i2c_wait();
    i2c_set_scl(1);
    i2c_wait();
    i2c_set_sda(1);
    i2c_wait();
}

void i2c_set_ack(int ack)
{
    i2c_set_sda(!ack);
    i2c_wait();
    i2c_set_scl(1);
    i2c_wait();
    i2c_set_scl(0);
    i2c_wait();
    i2c_set_sda(0);
    i2c_wait();
}

int i2c_get_ack()
{
	i2c_wait();
	i2c_set_scl(1);
	i2c_wait();
	int result = i2c_get_sda();
	i2c_set_scl(0);
	i2c_wait();
	i2c_set_sda(0);
	i2c_wait();
	return result;
}

uint8_t i2c_read(void)
{
	uint8_t result = 0;
	for (int i = 7; i >= 0; --i)
	{
		i2c_wait();
		i2c_set_scl(1);
		i2c_wait();
		result <<= 1;
		result |= i2c_get_sda();
		i2c_wait();				// <- remove
		i2c_set_scl(0);
		i2c_wait();
	}
}


void i2c_write(uint8_t val)
{
	for (int i = 7; i >= 0; --i)
	{
		i2c_set_sda((val >> i) & 1);
		i2c_wait();
		i2c_set_scl(1);
		i2c_wait();
		i2c_set_scl(0);
		i2c_wait();
	}
	i2c_set_sda(1);
}

size_t i2c_master_read_from(int address, uint8_t* data, size_t size, bool sendStop)
{

	DEBUGV("\r\nrf: %02x %08x %d\r\n", address, data, size);
	i2c_start();
	i2c_write(address << 1 | 1);
	int ack = i2c_get_ack();
	uint8_t* end = data + size;
	for (;data != end; ++data )
	{
		*data = i2c_read();
		DEBUGV("r%d ", *data);
		if (data == end - 1)
			i2c_set_ack(1);
		else
			i2c_set_ack(0);
	}
	DEBUGV("s\r\n", *data);
	if (sendStop)
		i2c_stop();
	return size;
}

size_t i2c_master_write_to(int address, const uint8_t* data, size_t size, bool sendStop)
{
	i2c_start();
	i2c_write(address << 1);
	int ack = i2c_get_ack();
	const uint8_t* end = data + size;
	for (;data != end; ++data )
	{
		i2c_write(*data);
		ack = i2c_get_ack();
	}
	if (sendStop)
		i2c_stop();
	return size;
}



