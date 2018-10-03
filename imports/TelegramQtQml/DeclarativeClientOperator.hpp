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

#ifndef TELEGRAM_DECLARATIVE_OPERATOR_HPP
#define TELEGRAM_DECLARATIVE_OPERATOR_HPP

#include "telegramqt_qml_global.h"

#include <QObject>

namespace Telegram {

namespace Client {

class Client;
class DeclarativeClient;
class DeclarativeSettings;

class TELEGRAMQT_QML_EXPORT DeclarativeClientOperator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Telegram::Client::DeclarativeClient *client READ client WRITE setClient NOTIFY clientChanged)
public:
    explicit DeclarativeClientOperator(QObject *parent = nullptr);

    DeclarativeClient *client() const { return m_client; }

public slots:
    void setClient(DeclarativeClient *client);

Q_SIGNALS:
    void clientChanged();

protected:
    void syncSettings();
    Client *backend() const;

    DeclarativeClient *m_client = nullptr;
};

} // Client

} // Telegram

#endif // TELEGRAM_DECLARATIVE_OPERATOR_HPP
