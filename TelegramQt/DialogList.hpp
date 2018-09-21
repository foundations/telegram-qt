#ifndef TELEGRAMQT_DIALOG_LIST_HPP
#define TELEGRAMQT_DIALOG_LIST_HPP

#include "ReadyObject.hpp"
#include "TelegramNamespace.hpp"
#include <QVector>

namespace Telegram {

namespace Client {

class Backend;

class TELEGRAMQT_EXPORT DialogList : public QObject, public ReadyObject
{
    Q_OBJECT
public:
    explicit DialogList(Backend *backend);

    QVector<Telegram::Peer> getPeers() const { return m_peers; }

    bool isReady() const override;
    PendingOperation *becomeReady() override;

protected:
    void onFinished();
    PendingOperation *m_readyOperation = nullptr;
    QVector<Telegram::Peer> m_peers;
    Backend *m_backend;
};

} // Client namespace

} // Telegram namespace

#endif // TELEGRAMQT_DIALOG_LIST_HPP
