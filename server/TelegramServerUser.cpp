#include "TelegramServerUser.hpp"

#include "CTelegramStream.hpp"
#include "CTelegramStreamExtraOperators.hpp"
#include "ServerRpcLayer.hpp"
#include "Session.hpp"
#include "TelegramServerClient.hpp"
#include "Utils.hpp"

#include <QDateTime>
#include <QLoggingCategory>

namespace Telegram {

namespace Server {

User::User(QObject *parent) :
    QObject(parent)
{
}

void User::setPhoneNumber(const QString &phoneNumber)
{
    m_phoneNumber = phoneNumber;
    m_id = qHash(m_phoneNumber);
}

void User::setFirstName(const QString &firstName)
{
    m_firstName = firstName;
}

void User::setLastName(const QString &lastName)
{
    m_lastName = lastName;
}

bool User::isOnline() const
{
    return true;
}

void User::setDcId(quint32 id)
{
    m_dcId = id;
}

Session *User::getSession(quint64 authId) const
{
    for (Session *s : m_sessions) {
        if (s->authId == authId) {
            return s;
        }
    }
    return nullptr;
}

QVector<Session *> User::activeSessions() const
{
    QVector<Session *> result;
    for (Session *s : m_sessions) {
        if (s->isActive()) {
            result.append(s);
        }
    }
    return result;
}

bool User::hasActiveSession() const
{
    for (Session *s : m_sessions) {
        if (s->isActive()) {
            return true;
        }
    }
    return false;
}

void User::addSession(Session *session)
{
    m_sessions.append(session);
    session->setUser(this);
    emit sessionAdded(session);
}

void User::setPlainPassword(const QString &password)
{
    if (password.isEmpty()) {
        m_passwordSalt.clear();
        m_passwordHash.clear();
        return;
    }
    QByteArray pwdSalt(8, Qt::Uninitialized);
    Utils::randomBytes(&pwdSalt);
    const QByteArray pwdData = pwdSalt + password.toUtf8() + pwdSalt;
    const QByteArray pwdHash = Utils::sha256(pwdData);
    setPassword(pwdSalt, pwdHash);
}

void User::setPassword(const QByteArray &salt, const QByteArray &hash)
{
    qDebug() << Q_FUNC_INFO << "salt:" << salt.toHex();
    qDebug() << Q_FUNC_INFO << "hash:" << hash.toHex();

    m_passwordSalt = salt;
    m_passwordHash = hash;
}

TLPeer User::toPeer() const
{
    TLPeer p;
    p.tlType = TLValue::PeerUser;
    p.userId = id();
    return p;
}

Telegram::Peer User::tlMessageToPeer(const TLMessage &message)
{
    switch (message.toId.tlType) {
    case TLValue::PeerUser:
        if (message.fromId != id()) {
            return Telegram::Peer::fromUserId(message.fromId);
        } else {
            return Telegram::Peer::fromUserId(message.toId.userId);
        }
    case TLValue::PeerChat:
        return Telegram::Peer::fromChatId(message.toId.chatId);
    case TLValue::PeerChannel:
        return Telegram::Peer::fromChannelId(message.toId.channelId);
    default:
        return Telegram::Peer();
    }
}

void User::addMessage(const TLMessage &message)
{
    m_messages.append(message);

    const Telegram::Peer messagePeer = tlMessageToPeer(message);
    UserDialog *dialog = ensureDialog(messagePeer);
    dialog->lastMessageId = message.id;
}

quint32 User::postMessage(const TLMessage &message, Session *excludeSession)
{
    switch (message.toId.tlType) {
    case TLValue::PeerUser:
    case TLValue::PeerChat:
        break;
    case TLValue::PeerChannel:
        return 0;
    default:
        qWarning() << Q_FUNC_INFO << "Unexpected peer type" << message.toId.tlType;
        return 0;
    };

    addMessage(message);

    TLUpdate newMessageUpdate;
    newMessageUpdate.tlType = TLValue::UpdateNewMessage;
    newMessageUpdate.message = m_messages.last();
    newMessageUpdate.pts = m_pts;
    newMessageUpdate.ptsCount = 1;

    for (Session *s : activeSessions()) {
        if (s == excludeSession) {
            continue;
        }
        s->rpcLayer()->sendUpdate(newMessageUpdate);
    }
    return m_pts;
}

const TLMessage *User::getMessage(quint32 messageId) const
{
    if (!messageId || m_messages.isEmpty()) {
        return nullptr;
    }
    if (m_messages.count() < messageId) {
        return nullptr;
    }
    return &m_messages.at(messageId - 1);
}

//bool User::getMessage(TLMessage *output, quint32 messageId) const
//{
//    const TLMessage *m = getMessage(messageId);
//    if (m) {
//        *output = *m;
//        return true;
//    }
//    return false;
//}

//quint32 User::addMessage(RemoteUser *sender, const QString &text)
//{
//    ++m_pts;
//    TLMessage newMessage;
//    newMessage.tlType = TLValue::Message;
//    newMessage.id = m_pts;
//    newMessage.message = text;
//    newMessage.fromId = sender->id();
//    newMessage.toId = toPeer();
//    newMessage.date = static_cast<quint32>(QDateTime::currentSecsSinceEpoch());
//    m_messages.append(newMessage);

//    const auto sessions = activeSessions();

//    TLUpdates updates;
//    updates.tlType = TLValue::UpdateShortMessage;
//    updates.message = newMessage.message;
//    updates.fromId = newMessage.fromId;
//    updates.date = newMessage.date;
//    updates.pts = m_pts;
//    updates.ptsCount = m_pts;

//    CTelegramStream stream(CTelegramStream::WriteOnly);
//    stream << updates;
//    const QByteArray payload = stream.getData();

//    for (const Session *session : sessions) {
//        session->getConnection()->rpcLayer()->sendRpcMessage(payload);
//    }

//    return m_pts;
//}

quint32 User::addPts()
{
    return ++m_pts;
}

UserDialog *User::ensureDialog(const Telegram::Peer &peer)
{
    for (int i = 0; i < m_dialogs.count(); ++i) {
        if (m_dialogs.at(i)->peer == peer) {
            return m_dialogs[i];
        }
    }
    m_dialogs.append(new UserDialog());
    m_dialogs.last()->peer = peer;
    return m_dialogs.last();
}

} // Server

} // Telegram
