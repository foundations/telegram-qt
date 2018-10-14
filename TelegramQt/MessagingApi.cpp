#include "MessagingApi.hpp"
#include "MessagingApi_p.hpp"

#include "ApiUtils.hpp"
#include "ClientBackend.hpp"
#include "ClientRpcMessagesLayer.hpp"
#include "DataStorage.hpp"
#include "DataStorage_p.hpp"
#include "DialogList.hpp"
#include "Utils.hpp"

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

void MessagingApiPrivate::onMessageReceived(const TLMessage &message)
{
    emit m_parent->messageReceived(Telegram::Utils::toPublicPeer(message.toId), message.id);
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

void MessagingApi::setDraftMessage(const Peer peer, const QString &text)
{

}

quint64 MessagingApi::sendMessage(const Peer peer, const QString &message)
{
    //return MessagesOperation::getHistory(this, peer, limit);

    DataInternalApi *dataApi = d->dataStorage()->internalApi();

    int flags = 1 << 7; // clearDraft
    TLInputPeer inputPeer = dataApi->toInputPeer(peer);
    int replyToMessageId = 0;
    quint64 randomId = Utils::randomBytes<quint64>();
    d->messagesLayer()->sendMessage(flags, inputPeer, replyToMessageId, message, randomId, TLReplyMarkup(), {});
    return randomId;
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
