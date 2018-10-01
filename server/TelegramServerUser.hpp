#ifndef TELEGRAMSERVERUSER_HPP
#define TELEGRAMSERVERUSER_HPP

#include <QObject>
#include <QVector>

#include "TLTypes.hpp"
#include "TelegramNamespace.hpp"

namespace Telegram {

namespace Server {

class Session;
class User;
class RemoteClientConnection;

class RemoteUser;

struct UserDialog
{
    Telegram::Peer peer;
    quint32 lastMessageId;
    QString draftText;
};

class MessageRecipient
{
public:
    virtual ~MessageRecipient() = default;
    virtual void addMessage(const TLMessage &message) = 0;
    virtual quint32 postMessage(const TLMessage &message) = 0;

    virtual TLPeer toPeer() const = 0;
};

class RemoteGroup : public MessageRecipient
{
    // Cross-server addMessage to group
};

class RemoteChannel : public MessageRecipient
{
    // Cross-server addMessage to channel
};

class LocalGroup : public RemoteGroup
{
    // Local-server addMessage to group (no save to group)
    //     Foreach (user : member)
    //         user->addMessage()
};

class LocalChannel : public RemoteChannel
{
    // Local-server addMessage to channel:
    //     channel->addMessage()
    //     Foreach (user : member)
    //         user->updateChannelPts()
};

class RemoteUser : public MessageRecipient
 {
public:
    virtual ~RemoteUser() = default;
    virtual quint32 id() const = 0;
    virtual QString phoneNumber() const = 0;
    virtual QString firstName() const = 0;
    virtual QString lastName() const = 0;
    virtual bool isOnline() const = 0;
    virtual quint32 dcId() const = 0;

    virtual TLPeer toPeer() const = 0;
};

class User : public QObject, public RemoteUser
{
    Q_OBJECT
public:
    explicit User(QObject *parent = nullptr);

    quint32 id() const { return m_id; }
    QString phoneNumber() const { return m_phoneNumber; }
    void setPhoneNumber(const QString &phoneNumber);

    QString firstName() const { return m_firstName; }
    void setFirstName(const QString &firstName);

    QString lastName() const { return m_lastName; }
    void setLastName(const QString &lastName);

    bool isOnline() const;

    quint32 dcId() const { return m_dcId; }
    void setDcId(quint32 id);

    Session *getSession(quint64 authId) const;
    QVector<Session*> sessions() const { return m_sessions; }
    QVector<Session*> activeSessions() const;
    bool hasActiveSession() const;
    void addSession(Session *session);

    bool hasPassword() const { return !m_passwordSalt.isEmpty() && !m_passwordHash.isEmpty(); }
    QByteArray passwordSalt() const { return m_passwordSalt; }
    QByteArray passwordHash() const { return m_passwordHash; }

    void setPlainPassword(const QString &password);
    void setPassword(const QByteArray &salt, const QByteArray &hash);

    QString passwordHint() const { return QString(); }

    TLPeer toPeer() const override;

    Telegram::Peer tlMessageToPeer(const TLMessage &message);

    void addMessage(const TLMessage &message) override;
    quint32 postMessage(const TLMessage &message) override { return postMessage(message, nullptr); }
    quint32 postMessage(const TLMessage &message, Session *excludeSession);

    const TLMessage *getMessage(quint32 messageId) const;
    //bool getMessage(TLMessage *output, quint32 messageId) const;

    quint32 addPts();

    quint32 pts() const { return m_pts; }

    const QVector<UserDialog *> dialogs() const { return m_dialogs; }

signals:
    void sessionAdded(Session *newSession);
    void sessionDestroyed(Session *destroyedSession);

protected:
    UserDialog *ensureDialog(const Telegram::Peer &peer);

    quint32 m_id = 0;
    QString m_phoneNumber;
    QString m_firstName;
    QString m_lastName;
    QString m_userName;
    QByteArray m_passwordSalt;
    QByteArray m_passwordHash;
    QVector<Session*> m_sessions;
    quint32 m_dcId = 0;

    quint32 m_pts = 0;
    QVector<TLMessage> m_messages;
    QVector<UserDialog *> m_dialogs;
    QVector<quint32> m_contactList;
};

} // Server

} // Telegram

#endif // TELEGRAMSERVERUSER_HPP
