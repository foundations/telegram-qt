#ifndef TELEGRAM_QT_DEVICE_VIEW_HPP
#define TELEGRAM_QT_DEVICE_VIEW_HPP

#ifndef TELEGRAM_QT_PRIVATE_DEVICE_API
#error "This file is a very private API. Use IODevicePtr wrapper instead"
#endif

#include <QIODevice>

namespace Telegram {

namespace Utils {

class IODeviceView : public QIODevice
{
public:
    IODeviceView(QIODevice *source, qint64 limit);
    ~IODeviceView() override;
    bool atEnd() const override;

    qint64 bytesAvailable() const override;
    qint64 bytesToWrite() const override;

protected:
    qint64 readData(char *data, qint64 maxlen) final;
    qint64 writeData(const char *data, qint64 len) final;

    QIODevice *m_source = nullptr;
    qint64 m_limit = 0;
};

} // Utils

} // Telegram

#endif // TELEGRAM_QT_DEVICE_VIEW_HPP
