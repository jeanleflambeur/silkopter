#include "BrainStdAfx.h"
#include "bus/SPI_Linux.h"
#include "def_lang/Mapper.h"
#include <linux/spi/spidev.h>

namespace silk
{
namespace bus
{

SPI_Linux::SPI_Linux(ts::IDeclaration_Scope const& scope)
{
    std::shared_ptr<const ts::IType> type = scope.find_specialized_symbol_by_path<const ts::IType>("::silk::SPI_Linux_Descriptor");
    if (!type)
    {
        QLOGE("Cannot find descriptor type");
    }
    else
    {
        m_descriptor = type->create_value();
    }
}

SPI_Linux::~SPI_Linux()
{
    close();
}

bool SPI_Linux::init(std::shared_ptr<ts::IValue> descriptor)
{
    if (!descriptor)
    {
        QLOGE("Null descriptor!");
        return false;
    }
    std::string dev;
    uint32_t speed = 0;

    auto result = ts::mapper::get(*descriptor, "dev", dev) &
                    ts::mapper::get(*descriptor, "speed", speed);
    if (result != ts::success)
    {
        QLOGE("{}", result.error().what());
        return false;
    }

    result = m_descriptor->copy_assign(*descriptor);
    if (result != ts::success)
    {
        QLOGE("{}", result.error().what());
        return false;
    }

    return open(dev, speed);
}

std::shared_ptr<const ts::IValue> SPI_Linux::get_descriptor() const
{
    return m_descriptor;
}

bool SPI_Linux::open(std::string const& dev, size_t speed)
{
    QLOG_TOPIC("spi_linux::open");

    QASSERT(m_fd < 0);
    m_fd = ::open(dev.c_str(), O_RDWR);
    if (m_fd < 0)
    {
        QLOGE("can't open {}: {}", dev, strerror(errno));
        return false;
    }
    m_speed = speed;
    return true;
}
void SPI_Linux::close()
{
    QLOG_TOPIC("spi_linux::close");

    QASSERT(m_fd >= 0);
    if (m_fd >= 0)
    {
        ::close(m_fd);
        m_fd = -1;
    }
}

void SPI_Linux::lock()
{
//    if (m_fd >= 0)
//    {
//        QASSERT(0);
//        return;
//    }
//    auto res = open();
//    QASSERT(res);
}

auto SPI_Linux::try_lock() -> bool
{
//    if (m_fd >= 0)
//    {
//        return false;
//    }
//    return open();
    return true;
}

void SPI_Linux::unlock()
{
//    close();
}

auto SPI_Linux::do_transfer(uint8_t const* tx_data, uint8_t* rx_data, size_t size, size_t speed) -> bool
{
    QASSERT(m_fd >= 0 && size > 0);
    if (m_fd < 0 || size == 0)
    {
        return false;
    }

    spi_ioc_transfer spi_transfer;
    memset(&spi_transfer, 0, sizeof(spi_ioc_transfer));

    spi_transfer.tx_buf = (unsigned long)tx_data;
    spi_transfer.rx_buf = (unsigned long)rx_data;
    spi_transfer.len = size;
    spi_transfer.speed_hz = speed ? speed : m_speed;
    spi_transfer.bits_per_word = 8;
    spi_transfer.delay_usecs = 0;

    int status = ioctl(m_fd, SPI_IOC_MESSAGE(1), &spi_transfer);
    if (status < 0)
    {
        QLOGW("transfer failed: {}", strerror(errno));
        return false;
    }

    return true;
}

auto SPI_Linux::transfer(uint8_t const* tx_data, uint8_t* rx_data, size_t size, size_t speed) -> bool
{
    QLOG_TOPIC("SPI_Linux::transfer");
    return do_transfer(tx_data, rx_data, size, speed);
}

auto SPI_Linux::transfer_register(uint8_t reg, uint8_t const* tx_data, uint8_t* rx_data, size_t size, size_t speed) -> bool
{
    QLOG_TOPIC("SPI_Linux::transfer_register");

    m_tx_buffer.resize(size + 1);
    m_tx_buffer[0] = reg;
    std::copy(tx_data, tx_data + size, m_tx_buffer.begin() + 1);

    m_rx_buffer.resize(size + 1);
    if (!do_transfer(m_tx_buffer.data(), m_rx_buffer.data(), size + 1, speed))
    {
        return false;
    }

    std::copy(m_rx_buffer.begin() + 1, m_rx_buffer.end(), rx_data);
    return true;
}

}
}
