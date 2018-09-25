/*
   Copyright (C) 2018 Alexander Akulich <akulichalexander@gmail.com>

   This file is a part of TelegramQt library.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

 */

#include "DataStorage_p.hpp"
#include "TLTypesDebug.hpp"
#include "Debug.hpp"

#include "TelegramNamespace_p.hpp"

#include <QLoggingCategory>

namespace Telegram {

namespace Client {

/*!
    \class Telegram::Client::DataStorage
    \brief The DataStorage class provides a basic interface for session
           data management
    \inmodule TelegramQt
    \ingroup Client

    \sa AccountStorage
*/

DataStorage::DataStorage(QObject *parent) :
    DataStorage(new DataStoragePrivate(), parent)
{
    Q_D(DataStorage);
    d->m_api = new DataInternalApi(this);
}

DataInternalApi *DataStorage::internalApi()
{
    Q_D(DataStorage);
    return d->m_api;
}

DcConfiguration DataStorage::serverConfiguration() const
{
    Q_D(const DataStorage);
    return d->m_serverConfig;
}

void DataStorage::setServerConfiguration(const DcConfiguration &configuration)
{
    Q_D(DataStorage);
    d->m_serverConfig = configuration;
}

QVector<Peer> DataStorage::dialogs() const
{
    Q_D(const DataStorage);
    const auto dialogs = d->m_api->m_dialogs;
    QVector<Peer> result;
    result.reserve(dialogs.count);
    for (const TLDialog &dialog : dialogs.dialogs) {
        result.append(DataInternalApi::toPublicPeer(dialog.peer));
    }
    return result;
}

bool DataStorage::getDialogInfo(DialogInfo *info, const Peer &peer) const
{
    Q_D(const DataStorage);
    const auto &dialogs = d->m_api->m_dialogs;
    for (const TLDialog &dialog : dialogs.dialogs) {
        Telegram::Peer thisDialogPeer = DataInternalApi::toPublicPeer(dialog.peer);
        if (thisDialogPeer == peer) {
            TLDialog *infoData = Telegram::DialogInfo::Private::get(info);
            *infoData = dialog;
            return true;
        }
    }
    qDebug() << Q_FUNC_INFO << "Unknown dialog" << peer.toString();
    return false;
}

bool DataStorage::getUserInfo(UserInfo *info, quint32 userId) const
{
    Q_D(const DataStorage);
    const auto &users = d->m_api->m_users;
    if (!users.contains(userId)) {
        qDebug() << Q_FUNC_INFO << "Unknown user" << userId;
        return false;
    }

    const TLUser *user = users.value(userId);
    TLUser *infoData = Telegram::UserInfo::Private::get(info);
    *infoData = *user;
    return true;
}

bool DataStorage::getChatInfo(ChatInfo *info, quint32 chatId) const
{
    Q_D(const DataStorage);
    const auto &chats = d->m_api->m_chats;
    if (!chats.contains(chatId)) {
        qDebug() << Q_FUNC_INFO << "Unknown user" << chatId;
        return false;
    }

    const TLChat *chat = chats.value(chatId);
    TLChat *infoData = Telegram::ChatInfo::Private::get(info);
    *infoData = *chat;
    return true;
}

bool DataStorage::getMessage(Message *message, const Peer &peer, quint32 messageId)
{
    Q_D(const DataStorage);
    const TLMessage *m = nullptr;
    if (peer.type == Peer::Channel) {
        quint64 key = DataInternalApi::channelMessageToKey(peer.id, messageId);
        m = d->m_api->m_channelMessages.value(key);
    } else {
        m = d->m_api->m_clientMessages.value(messageId);
    }
    if (!m) {
        qDebug() << Q_FUNC_INFO << "Unknown message" << peer << message;
        return false;
    }
    message->text = m->message;
    message->flags = TelegramNamespace::MessageFlagNone;
    if (m->out()) {
        message->flags |= TelegramNamespace::MessageFlagOut;
    }
    if (m->flags && TLMessage::FwdFrom) {
        message->flags |= TelegramNamespace::MessageFlagForwarded;
        if (m->fwdFrom.flags & TLMessageFwdHeader::FromId) {
            //message->setForwardFromPeer((m->fwdFrom))
        }
    }
    return true;
}

DataStorage::DataStorage(DataStoragePrivate *d, QObject *parent)
    : QObject(parent),
      d_ptr(d)
{
}

InMemoryDataStorage::InMemoryDataStorage(QObject *parent) :
    DataStorage(parent)
{
}

DataInternalApi::DataInternalApi(QObject *parent) :
    QObject(parent)
{
}

DataInternalApi::~DataInternalApi()
{
}

const TLUser *DataInternalApi::getSelfUser() const
{
    if (!m_selfUserId) {
        return nullptr;
    }
    return m_users.value(m_selfUserId);
}

void DataInternalApi::processData(const TLMessage &message)
{
    TLMessage *m = nullptr;
    if (message.toId.tlType == TLValue::PeerChannel) {
        const quint64 key = channelMessageToKey(message.toId.channelId, message.id);
        if (!m_channelMessages.contains(key)) {
            m_channelMessages.insert(key, new TLMessage());
        }
        m = m_channelMessages.value(key);
    } else {
        const quint32 key = message.id;
        if (!m_clientMessages.contains(key)) {
            m_clientMessages.insert(key, new TLMessage());
        }
        m = m_clientMessages.value(key);
    }
    *m = message;
}

void DataInternalApi::processData(const TLChat &chat)
{
    if (!m_chats.contains(chat.id)) {
        TLChat *newChatInstance = new TLChat(chat);
        m_chats.insert(chat.id, newChatInstance);
    } else {
        *m_chats[chat.id] = chat;
    }
}

void DataInternalApi::processData(const TLUser &user)
{
    TLUser *existsUser = m_users.value(user.id);
    if (existsUser) {
        *existsUser = user;
    } else {
        m_users.insert(user.id, new TLUser(user));
    }
    if (user.self()) {
        if (m_selfUserId && (m_selfUserId != user.id)) {
            qWarning() << "Got self user with different id.";
        }
        m_selfUserId = user.id;
        //emit selfUserAvailable(user.id);
    }
    //int indexOfRequest = m_askedUserIds.indexOf(user.id);
    //if (indexOfRequest >= 0) {
    //    m_askedUserIds.remove(indexOfRequest);
    //}
    //if (!existsUser) {
    //    emit peerAdded(toPublicPeer(user));
    //    emit userInfoReceived(user.id);
    //}
}

void DataInternalApi::processData(const TLAuthAuthorization &authorization)
{
    processData(authorization.user);
}

void DataInternalApi::processData(const TLMessagesDialogs &dialogs)
{
    //qDebug() << Q_FUNC_INFO << dialogs;
    m_dialogs = dialogs;
    for (const TLUser &user : dialogs.users) {
        processData(user);
    }
    for (const TLChat &chat : dialogs.chats) {
        processData(chat);
    }
    for (const TLMessage &message : dialogs.messages) {
        processData(message);
    }
}

Peer DataInternalApi::toPublicPeer(const TLPeer &peer)
{
    switch (peer.tlType) {
    case TLValue::PeerChat:
        return Telegram::Peer(peer.chatId, Telegram::Peer::Chat);
    case TLValue::PeerChannel:
        return Telegram::Peer(peer.channelId, Telegram::Peer::Channel);
    case TLValue::PeerUser:
        return Telegram::Peer(peer.userId, Telegram::Peer::User);
    default:
        return Telegram::Peer();
    }
}

TLInputUser DataInternalApi::toInputUser(quint32 userId) const
{
    TLInputUser inputUser;
    if (userId == selfUserId()) {
        inputUser.tlType = TLValue::InputUserSelf;
        return inputUser;
    }
    const TLUser *user = m_users.value(userId);
    if (user) {
        if (user->tlType == TLValue::User) {
            inputUser.tlType = TLValue::InputUser;
            inputUser.userId = user->id;
            inputUser.accessHash = user->accessHash;
        } else {
            qWarning() << Q_FUNC_INFO << "Unknown user type: " << QString::number(user->tlType, 16);
        }
    } else {
        qWarning() << Q_FUNC_INFO << "Unknown user.";
    }
    return inputUser;
}

quint64 DataInternalApi::channelMessageToKey(quint32 channelId, quint32 messageId)
{
    quint64 key = channelId;
    return (key << 32) + messageId;
}

//QVector<Peer> InMemoryDataStorage::dialogs() const
//{
//    return {};
//}

} // Client namespace

} // Telegram namespace
