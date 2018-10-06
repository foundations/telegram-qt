#include "TelegramServer.hpp"

#include <QLoggingCategory>
#include <QTcpServer>
#include <QTcpSocket>

#include "TelegramServerUser.hpp"
#include "TelegramServerClient.hpp"
#include "RemoteServerConnection.hpp"
#include "Session.hpp"

#include "CServerTcpTransport.hpp"

// Generated RPC Operation Factory includes
#include "AccountOperationFactory.hpp"
#include "AuthOperationFactory.hpp"
#include "BotsOperationFactory.hpp"
#include "ChannelsOperationFactory.hpp"
#include "ContactsOperationFactory.hpp"
#include "HelpOperationFactory.hpp"
#include "LangpackOperationFactory.hpp"
#include "MessagesOperationFactory.hpp"
#include "PaymentsOperationFactory.hpp"
#include "PhoneOperationFactory.hpp"
#include "PhotosOperationFactory.hpp"
#include "StickersOperationFactory.hpp"
#include "UpdatesOperationFactory.hpp"
#include "UploadOperationFactory.hpp"
#include "UsersOperationFactory.hpp"
// End of generated RPC Operation Factory includes

Q_LOGGING_CATEGORY(loggingCategoryServer, "telegram.server.main", QtWarningMsg)
Q_LOGGING_CATEGORY(loggingCategoryServerApi, "telegram.server.api", QtWarningMsg)

namespace Telegram {

namespace Server {

Server::Server(QObject *parent) :
    QObject(parent)
{
    m_rpcOperationFactories = {
        // Generated RPC Operation Factory initialization
        new AccountOperationFactory(),
        new AuthOperationFactory(),
        new BotsOperationFactory(),
        new ChannelsOperationFactory(),
        new ContactsOperationFactory(),
        new HelpOperationFactory(),
        new LangpackOperationFactory(),
        new MessagesOperationFactory(),
        new PaymentsOperationFactory(),
        new PhoneOperationFactory(),
        new PhotosOperationFactory(),
        new StickersOperationFactory(),
        new UpdatesOperationFactory(),
        new UploadOperationFactory(),
        new UsersOperationFactory(),
        // End of generated RPC Operation Factory initialization
    };
    m_serverSocket = new QTcpServer(this);
    connect(m_serverSocket, &QTcpServer::newConnection, this, &Server::onNewConnection);
}

void Server::setDcOption(const DcOption &option)
{
    m_dcOption = option;
}

void Server::setServerPrivateRsaKey(const Telegram::RsaKey &key)
{
    m_key = key;
}

bool Server::start()
{
    if (!m_serverSocket->listen(QHostAddress(m_dcOption.address), m_dcOption.port)) {
        qWarning() << "Unable to listen port" << m_dcOption.port;
        return false;
    }
    qDebug() << "Start a server" << m_dcOption.id << "on" << m_dcOption.address << ":" << m_dcOption.port << "Key:" << m_key.fingerprint;

    addServiceUser();
    return true;
}

void Server::addServiceUser()
{
    User *serviceUser = new User(this);
    serviceUser->setPhoneNumber(QStringLiteral("+42777"));
    serviceUser->setFirstName(QStringLiteral("Telegram"));
    serviceUser->setDcId(0);
    m_serviceUserId = serviceUser->id();
    insertUser(serviceUser);
}

void Server::loadData()
{
    const int number = 10;
    for (int i = 0; i < number; ++i) {
        User *newUser = new User(this);
        newUser->setPhoneNumber(QStringLiteral("%1").arg(i, 6, 10, QLatin1Char('0')));
        insertUser(newUser);
    }
}

void Server::setServerConfiguration(const DcConfiguration &config)
{
    m_dcConfiguration = config;
}

void Server::addServerConnection(RemoteServerConnection *remoteServer)
{
    m_remoteServers.insert(remoteServer);
}

quint32 Server::getDcIdForUserIdentifier(const QString &phoneNumber)
{
    if (m_phoneToUserId.contains(phoneNumber)) {
        return m_dcOption.id;
    }
    return 0;
}

void Server::setAuthorizationProvider(Authorization::Provider *provider)
{
    m_authProvider = provider;
}

void Server::onNewConnection()
{
    QTcpSocket *newConnection = m_serverSocket->nextPendingConnection();
    if (newConnection == nullptr) {
        qCDebug(loggingCategoryServer) << "expected pending connection does not exist";
        return;
    }
    qCDebug(loggingCategoryServer) << "A new incoming connection from" << newConnection->peerAddress().toString();
    TcpTransport *transport = new TcpTransport(newConnection, this);
    newConnection->setParent(transport);
    RemoteClientConnection *client = new RemoteClientConnection(this);
    connect(client, &BaseConnection::statusChanged, this, &Server::onClientConnectionStatusChanged);
    client->setServerRsaKey(m_key);
    client->setTransport(transport);
    client->setServerApi(this);
    client->setRpcFactories(m_rpcOperationFactories);

    m_activeConnections.insert(client);
}

void Server::onUserSessionAdded(Session *newSession)
{
    RemoteUser *sender = getServiceUser();
    User *recipient = newSession->user();

    QString text = QStringLiteral("Detected login from IP address %1").arg(newSession->ip);

    TLMessage m;
    m.tlType = TLValue::Message;
    m.message = text;
    m.toId = recipient->toPeer();
    m.fromId = sender->id();
    m.flags |= TLMessage::FromId;
    m.id = recipient->addPts();
    recipient->postMessage(m);
}

void Server::onClientConnectionStatusChanged()
{
    RemoteClientConnection *client = qobject_cast<RemoteClientConnection*>(sender());
    if (client->status() == RemoteClientConnection::Status::Authenticated) {
        if (!client->session()) {

            qDebug() << Q_FUNC_INFO << "A new auth key";
        }
    } else if (client->status() == RemoteClientConnection::Status::Disconnected) {
        // TODO: Initiate session cleanup after session expiration time out
    }
}

User *Server::getLocalUser(const QString &identifier) const
{
    quint32 id = m_phoneToUserId.value(identifier);
    if (!id) {
        return nullptr;
    }
    return m_users.value(id);
}

RemoteUser *Server::getRemoteUser(const QString &identifier) const
{
    for (RemoteServerConnection *remoteServer : m_remoteServers) {
        RemoteUser *u = remoteServer->getUser(identifier);
        if (u) {
            return u;
        }
    }
    return nullptr;
}

bool Server::setupTLUser(TLUser *output, quint32 requestedUserId, const User *forUser) const
{
    User *requestedUser = getUser(requestedUserId);
    if (!requestedUser) {
        return false;
    }

    output->id = requestedUserId;
    output->tlType = TLValue::User;
    output->firstName = requestedUser->firstName();
    output->lastName = requestedUser->lastName();
    // TODO: Check if the user has access to the requested user phone
    output->phone = requestedUser->phoneNumber();

    quint32 flags = 0;
    if (!output->firstName.isEmpty()) {
        flags |= TLUser::FirstName;
    }
    if (!output->lastName.isEmpty()) {
        flags |= TLUser::LastName;
    }
    if (!output->username.isEmpty()) {
        flags |= TLUser::Username;
    }
    if (!output->phone.isEmpty()) {
        flags |= TLUser::Phone;
    }
    if (requestedUserId == forUser->id()) {
        flags |= TLUser::Self;
    }
    output->flags = flags;

    return true;
}

User *Server::getUser(const QString &identifier) const
{
    return getLocalUser(identifier);
}

User *Server::getUser(quint32 userId) const
{
    return m_users.value(userId);
}

User *Server::tryAccessUser(quint32 userId, quint64 accessHash)
{
    User *u = getUser(userId);
    // TODO: Check access hash
    return u;
}

User *Server::addUser(const QString &identifier)
{
    qDebug() << Q_FUNC_INFO << identifier;
    User *user = new User(this);
    user->setPhoneNumber(identifier);
    user->setDcId(dcId());
    insertUser(user);
    return user;
}

Session *Server::createSession(quint64 authId, const QByteArray &authKey, const QString &address)
{
    Session *session = new Session();
    session->authId = authId;
    session->authKey = authKey;
    session->ip = address;
    m_authIdToSession.insert(authId, session);
    return session;
}

Session *Server::getSessionByAuthId(quint64 authKeyId) const
{
    return m_authIdToSession.value(authKeyId);
}

void Server::insertUser(User *user)
{
    qDebug() << Q_FUNC_INFO << user << user->phoneNumber() << user->id();
    m_users.insert(user->id(), user);
    m_phoneToUserId.insert(user->phoneNumber(), user->id());
    for (Session *session : user->sessions()) {
        m_authIdToSession.insert(session->authId, session);
    }

    connect(user, &User::sessionAdded, this, &Server::onUserSessionAdded);
}

RemoteUser *Server::getServiceUser()
{
    return m_users.value(m_serviceUserId);
}

PhoneStatus Server::getPhoneStatus(const QString &identifier) const
{
    PhoneStatus result;
    RemoteUser *user = getLocalOrRemoteUser(identifier);
    if (user) {
        result.online = user->isOnline();
        result.dcId = user->dcId();
    }
    return result;
}

PasswordInfo Server::getPassword(const QString &identifier)
{
    PasswordInfo result;
    User *user = getUser(identifier);
    if (user && user->hasPassword()) {
        result.currentSalt = user->passwordSalt();
        result.hint = user->passwordHint();
    }
    return result;
}

bool Server::checkPassword(const QString &identifier, const QByteArray &hash)
{
    User *user = getUser(identifier);
    if (user && user->hasPassword()) {
        return user->passwordHash() == hash;
    }
    return false;

}

bool Server::identifierIsValid(const QString &identifier)
{
    const bool result = identifier.length() > 4;
    qCDebug(loggingCategoryServerApi) << "identifierIsValid(" << identifier << "):" << result;
    return result;
}

RemoteUser *Server::getLocalOrRemoteUser(const QString &identifier) const
{
    RemoteUser *user = getLocalUser(identifier);
    if (!user) {
        user = getRemoteUser(identifier);
    }
    return user;
}

} // Server

} // Telegram
