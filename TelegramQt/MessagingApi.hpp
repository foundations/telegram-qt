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

#ifndef TELEGRAMQT_CLIENT_MESSAGING_API_HPP
#define TELEGRAMQT_CLIENT_MESSAGING_API_HPP

#include "telegramqt_global.h"
#include "TelegramNamespace.hpp"

namespace Telegram {

class PendingOperation;

namespace Client {

class DialogList;
class MessagesOperation;

class MessagingApiPrivate;

class MessagingApi : public QObject
{
    Q_OBJECT
public:
    explicit MessagingApi(QObject *parent = nullptr);

    PendingOperation *syncDialogs();
    DialogList *getDialogList();
    MessagesOperation *getHistory(const Telegram::Peer peer, quint32 limit);

    //    quint64 sendMessage(const Telegram::Peer &peer, const QString &message); // Message id is a random number
    //    quint64 forwardMessage(const Telegram::Peer &peer, quint32 messageId);
    //    /* Typing status is valid for 6 seconds. It is recommended to repeat typing status with localTypingRecommendedRepeatInterval() interval. */
    //    void setTyping(const Telegram::Peer &peer, TelegramNamespace::MessageAction action);
    //    void setMessageRead(const Telegram::Peer &peer, quint32 messageId);

Q_SIGNALS:
    void messageReceived(const Telegram::Peer peer, quint32 messageId);

protected:
    friend class MessagingApiPrivate;
    MessagingApiPrivate *d;
};

} // Client namespace

} // Telegram namespace

#endif // TELEGRAMQT_CLIENT_MESSAGING_API_HPP
