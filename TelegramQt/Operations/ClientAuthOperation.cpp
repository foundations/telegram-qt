#include "ClientAuthOperation.hpp"

#include "AccountStorage.hpp"
#include "CAppInformation.hpp"
#include "ClientBackend.hpp"
#include "ClientConnection.hpp"
#include "ClientRpcLayer.hpp"
#include "ClientRpcAuthLayer.hpp"
#include "ClientRpcAccountLayer.hpp"
#include "PendingRpcOperation.hpp"
#include "Utils.hpp"

#include "RpcError.hpp"

#ifdef DEVELOPER_BUILD
#include "TLTypesDebug.hpp"
#endif

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(c_loggingClientAuthOperation, "telegram.client.auth.operation", QtDebugMsg)

namespace Telegram {

namespace Client {

// Explicit templates instantiation to suppress compilation warnings
template <>
bool BaseRpcLayerExtension::processReply(PendingRpcOperation *operation, TLAccountPassword *output);
template <>
bool BaseRpcLayerExtension::processReply(PendingRpcOperation *operation, TLAuthAuthorization *output);
template <>
bool BaseRpcLayerExtension::processReply(PendingRpcOperation *operation, TLAuthSentCode *output);

AuthOperation::AuthOperation(QObject *parent) :
    PendingOperation(parent)
{
}

AuthOperation::AuthOperation(Backend *backend)  :
    PendingOperation(backend),
    m_backend(backend)
{
}

void AuthOperation::setBackend(Backend *backend)
{
    m_backend = backend;
}

void AuthOperation::setRunMethod(AuthOperation::RunMethod method)
{
    m_runMethod = method;
}

void AuthOperation::setPhoneNumber(const QString &phoneNumber)
{
    m_phoneNumber = phoneNumber;
}

void AuthOperation::start()
{
    if (m_runMethod) {
        callMember<>(this, m_runMethod);
    }
}

void AuthOperation::abort()
{
    qCWarning(c_loggingClientAuthOperation) << Q_FUNC_INFO << "STUB";
}

PendingOperation *AuthOperation::checkAuthorization()
{
    qCDebug(c_loggingClientAuthOperation) << Q_FUNC_INFO;
    AccountStorage *storage = m_backend->accountStorage();
    if (!storage->hasMinimalDataSet()) {
        setFinishedWithError({{c_text(), QStringLiteral("No minimal account data set")}});
        return nullptr;
    }
    // Backend::connectToServer() automatically takes the data from the account storage,
    // So just make some RPC call to check if connection works.
    PendingRpcOperation *updateStatusOperation = accountLayer()->updateStatus(false);
    connect(updateStatusOperation, &PendingRpcOperation::finished,
            this, &AuthOperation::onAccountStatusUpdateFinished);
    return nullptr;
}

PendingOperation *AuthOperation::requestAuthCode()
{
    qCDebug(c_loggingClientAuthOperation) << Q_FUNC_INFO;
    const CAppInformation *appInfo = m_backend->m_appInformation;
    if (!appInfo) {
        const QString text = QStringLiteral("Unable to request auth code, "
                                            "because the application information is not set");
        return PendingOperation::failOperation(text, this);
    }

    if (phoneNumber().isEmpty()) {
        emit phoneNumberRequired();
        return nullptr;
    }

    AuthRpcLayer::PendingAuthSentCode *requestCodeOperation
            = authLayer()->sendCode(phoneNumber(), appInfo->appId(), appInfo->appHash());
    qCDebug(c_loggingClientAuthOperation) << Q_FUNC_INFO
                                          << "requestPhoneCode"
                                          << Telegram::Utils::maskPhoneNumber(phoneNumber())
                                          << "on dc" << Connection::fromOperation(requestCodeOperation)->dcOption().id;
    connect(requestCodeOperation, &PendingOperation::finished, this, [this, requestCodeOperation] {
       this->onRequestAuthCodeFinished(requestCodeOperation);
    });
    return requestCodeOperation;
}

PendingOperation *AuthOperation::submitAuthCode(const QString &code)
{
    if (m_authCodeHash.isEmpty()) {
        const QString text = QStringLiteral("Unable to submit auth code without a code hash");
        qCWarning(c_loggingClientAuthOperation) << Q_FUNC_INFO << text;
        return PendingOperation::failOperation({{QStringLiteral("text"), text}});
    }

    PendingRpcOperation *sendCodeOperation = nullptr;
    if (m_registered) {
        sendCodeOperation = authLayer()->signIn(phoneNumber(), m_authCodeHash, code);
        connect(sendCodeOperation, &PendingRpcOperation::finished, this, &AuthOperation::onSignInFinished);
    } else {
        sendCodeOperation = authLayer()->signUp(phoneNumber(), m_authCodeHash, code, m_firstName, m_lastName);
        connect(sendCodeOperation, &PendingRpcOperation::finished, this, &AuthOperation::onSignUpFinished);
    }

    return sendCodeOperation;
}

PendingOperation *AuthOperation::getPassword()
{
    PendingRpcOperation *passwordRequest = accountLayer()->getPassword();
    connect(passwordRequest, &PendingRpcOperation::finished, this, &AuthOperation::onPasswordRequestFinished);
    return passwordRequest;
}

PendingOperation *AuthOperation::submitPassword(const QString &password)
{
    if (m_passwordCurrentSalt.isEmpty()) {
        const QString text = QStringLiteral("Unable to submit auth password (password salt is missing)");
        return PendingOperation::failOperation({{QStringLiteral("text"), text}});
    }
    const QByteArray pwdData = m_passwordCurrentSalt + password.toUtf8() + m_passwordCurrentSalt;
    const QByteArray pwdHash = Utils::sha256(pwdData);
    qCDebug(c_loggingClientAuthOperation) << Q_FUNC_INFO << "slt:" << m_passwordCurrentSalt.toHex();
    qCDebug(c_loggingClientAuthOperation) << Q_FUNC_INFO << "pwd:" << pwdHash.toHex();

    PendingRpcOperation *sendPasswordOperation = authLayer()->checkPassword(pwdHash);
    connect(sendPasswordOperation, &PendingRpcOperation::finished, this, &AuthOperation::onCheckPasswordFinished);
    return sendPasswordOperation;
}

void AuthOperation::submitPhoneNumber(const QString &phoneNumber)
{
    setPhoneNumber(phoneNumber);
    if (!isFinished()) {
        start();
    }
}

bool AuthOperation::submitName(const QString &firstName, const QString &lastName)
{
    m_firstName = firstName;
    m_lastName = lastName;
    return !firstName.isEmpty() && !lastName.isEmpty();
}

void AuthOperation::requestCall()
{
    qCWarning(c_loggingClientAuthOperation) << Q_FUNC_INFO << "STUB";
}

void AuthOperation::requestSms()
{
    qCWarning(c_loggingClientAuthOperation) << Q_FUNC_INFO << "STUB";
}

void AuthOperation::recovery()
{
    qCWarning(c_loggingClientAuthOperation) << Q_FUNC_INFO << "STUB";
}

void AuthOperation::setWantedDc(quint32 dcId)
{
    m_backend->setDcForLayer(ConnectionSpec(dcId), authLayer());
    m_backend->setDcForLayer(ConnectionSpec(dcId), accountLayer());
}

void AuthOperation::setPasswordCurrentSalt(const QByteArray &salt)
{
    m_passwordCurrentSalt = salt;
}

void AuthOperation::setPasswordHint(const QString &hint)
{
    if (m_passwordHint == hint) {
        return;
    }
    m_passwordHint = hint;
    emit passwordHintChanged(hint);
}

void AuthOperation::setRegistered(bool registered)
{
    m_registered = registered;
    emit registeredChanged(registered);
}

AccountRpcLayer *AuthOperation::accountLayer() const
{
    return m_backend->accountLayer();
}

AuthRpcLayer *AuthOperation::authLayer() const
{
    return m_backend->authLayer();
}

void AuthOperation::onRequestAuthCodeFinished(AuthRpcLayer::PendingAuthSentCode *operation)
{
    if (operation->rpcError() && operation->rpcError()->type == RpcError::SeeOther) {
        setWantedDc(operation->rpcError()->argument);
        m_backend->processSeeOthers(operation);
        return;
    }

    if (!operation->isSucceeded()) {
        setDelayedFinishedWithError(operation->errorDetails());
        return;
    }
    TLAuthSentCode result;
    operation->getResult(&result);
    qCDebug(c_loggingClientAuthOperation) << Q_FUNC_INFO << result.tlType << result.phoneCodeHash;
    if (result.isValid()) {
        m_authCodeHash = result.phoneCodeHash;
        setRegistered(result.phoneRegistered());
        emit authCodeRequired();
    }
}

void AuthOperation::onSignInFinished(PendingRpcOperation *operation)
{
    if (operation->rpcError()) {
        const RpcError *error = operation->rpcError();
        if (error->reason == RpcError::SessionPasswordNeeded) {
            emit passwordRequired();
            return;
        }
    }
    if (!operation->isSucceeded()) {
        setDelayedFinishedWithError(operation->errorDetails());
        return;
    }

    TLAuthAuthorization result;
    authLayer()->processReply(operation, &result);
    onGotAuthorization(operation, result);
}

void AuthOperation::onSignUpFinished(PendingRpcOperation *operation)
{
    if (!operation->isSucceeded()) {
        setDelayedFinishedWithError(operation->errorDetails());
        return;
    }

    TLAuthAuthorization result;
    authLayer()->processReply(operation, &result);
    onGotAuthorization(operation, result);
}

void AuthOperation::onPasswordRequestFinished(PendingRpcOperation *operation)
{
    if (!operation->isSucceeded()) {
        setDelayedFinishedWithError(operation->errorDetails());
        return;
    }

    TLAccountPassword result;
    authLayer()->processReply(operation, &result);
#ifdef DEVELOPER_BUILD
    qCDebug(c_loggingClientAuthOperation) << Q_FUNC_INFO << result;
#endif
    setPasswordCurrentSalt(result.currentSalt);
    setPasswordHint(result.hint);
}

void AuthOperation::onCheckPasswordFinished(PendingRpcOperation *operation)
{
    if (operation->rpcError()) {
        const RpcError *error = operation->rpcError();
        if (error->reason == RpcError::PasswordHashInvalid) {
            emit passwordCheckFailed();
            return;
        }
    }

    if (!operation->isSucceeded()) {
        setDelayedFinishedWithError(operation->errorDetails());
        return;
    }

    TLAuthAuthorization result;
    authLayer()->processReply(operation, &result);
    onGotAuthorization(operation, result);
}

void AuthOperation::onGotAuthorization(PendingRpcOperation *operation, const TLAuthAuthorization &authorization)
{
    qCDebug(c_loggingClientAuthOperation) << authorization.user.phone
                                          << authorization.user.firstName
                                          << authorization.user.lastName;
    AccountStorage *storage = m_backend->accountStorage();
    storage->setPhoneNumber(authorization.user.phone);
    if (storage->accountIdentifier().isEmpty()) {
        storage->setAccountIdentifier(authorization.user.phone);
    }
    Connection *conn = Connection::fromOperation(operation);
    m_backend->setMainConnection(conn);
    conn->setStatus(BaseConnection::Status::Signed, BaseConnection::StatusReason::Remote);
    setFinished();
}

void AuthOperation::onAccountStatusUpdateFinished(PendingRpcOperation *operation)
{
    if (!operation->isSucceeded()) {
        setFinishedWithError(operation->errorDetails());
        return;
    }
    Connection *conn = Connection::fromOperation(operation);
    m_backend->setMainConnection(conn);
    conn->setStatus(BaseConnection::Status::Signed, BaseConnection::StatusReason::Local);
    setFinished();
}

void AuthOperation::onConnectionError(const QString &description)
{
    if (isFinished()) {
        qCDebug(c_loggingClientAuthOperation) << "Connection error on finished auth operation:" << description;
        return;
    }
    setFinishedWithError({{c_text(), description}});
}

} // Client

} // Telegram namespace
