#include "DialogList.hpp"
#include "ClientBackend.hpp"
#include "DataStorage.hpp"

#include "Operations/ClientMessagesOperation.hpp"

namespace Telegram {

namespace Client {

DialogList::DialogList(Backend *backend) :
    QObject(backend),
    m_backend(backend)
{
}

bool DialogList::isReady() const
{
    return m_readyOperation && m_readyOperation->isSucceeded();
}

PendingOperation *DialogList::becomeReady()
{
    if (!m_readyOperation) {
        m_readyOperation = MessagesOperation::getDialogs(m_backend);
        connect(m_readyOperation, &PendingOperation::finished, this, &DialogList::onFinished);
        m_readyOperation->startLater();
    }
    return m_readyOperation;
}

void DialogList::onFinished()
{
    if (m_readyOperation->isFailed()) {
        return;
    }
    m_peers = m_backend->dataStorage()->dialogs();
}

} // Client namespace

} // Telegram namespace
