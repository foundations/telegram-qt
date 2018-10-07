#include "MessagingApi.hpp"
#include "MessagingApi_p.hpp"

#include "DialogList.hpp"

#include "ClientBackend.hpp"
#include "ClientRpcMessagesLayer.hpp"

#include "Operations/ClientMessagesOperation.hpp"

namespace Telegram {

class PendingOperation;

namespace Client {

MessagingApiPrivate::MessagingApiPrivate(MessagingApi *parent) :
    QObject(parent),
    m_parent(parent)
{
}

MessagingApiPrivate *MessagingApiPrivate::get(MessagingApi *parent)
{
    return parent->d;
}

void MessagingApiPrivate::setBackend(Backend *backend)
{
    m_backend = backend;
}

MessagingApi::MessagingApi(QObject *parent) :
    QObject(parent),
    d(new MessagingApiPrivate(this))
{
}

PendingOperation *MessagingApi::syncDialogs()
{
    return MessagesOperation::getDialogs(this);
}

DialogList *MessagingApi::getDialogList()
{
    TG_D(MessagingApi);

    if (!d->m_dialogList) {
        d->m_dialogList = new DialogList(this);
    }
    return d->m_dialogList;
}

MessagesOperation *MessagingApi::getHistory(const Telegram::Peer peer, quint32 limit)
{
    return MessagesOperation::getHistory(this, peer, limit);
}

DataStorage *MessagingApiPrivate::dataStorage()
{
    return m_backend->dataStorage();
}

MessagesRpcLayer *MessagingApiPrivate::messagesLayer()
{
    return m_backend->messagesLayer();
}

} // Client namespace

} // Telegram namespace
