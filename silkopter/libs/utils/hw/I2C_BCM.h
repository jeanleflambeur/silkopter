#pragma once

#include "II2C.h"
#include <atomic>
#include <vector>

namespace util
{
namespace hw
{

class I2C_BCM : public II2C
{
public:
    I2C_BCM();
    ~I2C_BCM();

    ts::Result<void> init(uint32_t device, uint32_t baud);

    bool read(uint8_t address, uint8_t* data, size_t size) override;
    bool write(uint8_t address, uint8_t const* data, size_t size) override;

    bool read_register(uint8_t address, uint8_t reg, uint8_t* data, size_t size) override;
    bool write_register(uint8_t address, uint8_t reg, uint8_t const* data, size_t size) override;

private:
    void close();

    uint32_t m_device = 0;
    uint32_t m_baud = 0;
    std::vector<uint8_t> m_buffer;

    mutable std::atomic_bool m_is_used = { false };
};

}
}
