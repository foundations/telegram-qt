#ifndef TELEGRAM_QT_DEVICE_PTR_HPP
#define TELEGRAM_QT_DEVICE_PTR_HPP

#include "telegramqt_global.h"

QT_FORWARD_DECLARE_CLASS(QIODevice)

namespace Telegram {

namespace Utils {

class IODeviceView;

class TELEGRAMQT_EXPORT IODevicePtr
{
public:
    IODevicePtr(IODeviceView *p);
    ~IODevicePtr();

    QIODevice *getDevice() { return d; }
    operator QIODevice *() { return d; }

protected:
    QIODevice *d = nullptr;
    Q_DISABLE_COPY(IODevicePtr)
};

} // Utils

} // Telegram

#endif // TELEGRAM_QT_DEVICE_PTR_HPP
