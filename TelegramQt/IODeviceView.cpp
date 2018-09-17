#include "IODevicePtr.hpp"

#define TELEGRAM_QT_PRIVATE_DEVICE_API
#include "IODeviceView_p.hpp"

namespace Telegram {

namespace Utils {

IODeviceView::IODeviceView(QIODevice *source, qint64 limit) :
    m_source(source),
    m_limit(limit)
{
    open(source->openMode());
}

IODeviceView::~IODeviceView()
{
    if (Q_UNLIKELY(m_limit)) {
        m_source->skip(m_limit);
    }
}

bool IODeviceView::atEnd() const
{
    return m_limit == 0;
}

qint64 IODeviceView::bytesAvailable() const
{
    return m_limit;
}

qint64 IODeviceView::bytesToWrite() const
{
    return m_limit;
}

qint64 IODeviceView::readData(char *data, qint64 maxlen)
{
    if (Q_LIKELY(maxlen > m_limit)) {
        maxlen = m_limit;
    }
    const qint64 bytesRead = m_source->read(data, maxlen);
    m_limit -= bytesRead;
    return bytesRead;
}

qint64 IODeviceView::writeData(const char *data, qint64 len)
{
    if (Q_LIKELY(len > m_limit)) {
        len = m_limit;
    }
    const qint64 bytesWritten = m_source->write(data, len);
    m_limit -= bytesWritten;
    return bytesWritten;
}

IODevicePtr::IODevicePtr(IODeviceView *p) :
    d(p)
{
}

IODevicePtr::~IODevicePtr()
{
    delete d; // delete automatically checks d for nullptr
}

} // Utils

} // Telegram
