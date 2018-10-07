#ifndef TELEGRAMQT_CLIENT_MESSAGES_OPERATION
#define TELEGRAMQT_CLIENT_MESSAGES_OPERATION

#include "PendingOperation.hpp"
#include "TelegramNamespace.hpp"

#include "RpcLayers/ClientRpcMessagesLayer.hpp"

namespace Telegram {

class PendingRpcOperation;

namespace Client {

class DataStorage;
class MessagingApi;
class MessagesRpcLayer;

class MessagesOperation : public PendingOperation
{
    Q_OBJECT
public:
    explicit MessagesOperation(QObject *parent = nullptr);
    void setBackend(MessagingApi *backend);

    static MessagesOperation *getDialogs(MessagingApi *backend);
    static MessagesOperation *getHistory(MessagingApi *backend, const Telegram::Peer peer, quint32 limit);

    QVector<quint32> messages() const { return m_messages; }

    using RunMethod = void(MessagesOperation::*)();

    void setRunMethod(RunMethod method);

public slots:
    void start() override;

    void getDialogs();
    void getMessageHistory(const Telegram::Peer peer, quint32 limit);

protected:
    MessagesRpcLayer *messagesLayer() const;
    DataStorage *dataStorage();

    MessagingApi *m_backend = nullptr;
    RunMethod m_runMethod = nullptr;

protected:
    // Implementation:
    void onGetDialogsFinished(MessagesRpcLayer::PendingMessagesDialogs *operation);
    void onGetHistoryFinished(MessagesRpcLayer::PendingMessagesMessages *operation);

    QVector<quint32> m_messages;
};

} // Client

} // Telegram

#endif // TELEGRAMQT_CLIENT_MESSAGES_OPERATION
