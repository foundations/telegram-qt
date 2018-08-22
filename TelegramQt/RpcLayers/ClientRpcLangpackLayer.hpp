/*
   Copyright (C) 2018 

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

#ifndef TELEGRAM_CLIENT_RPC_LANGPACK_LAYER_HPP
#define TELEGRAM_CLIENT_RPC_LANGPACK_LAYER_HPP

#include "ClientRpcLayerExtension.hpp"
#include "TLTypes.hpp"

namespace Telegram {

class PendingRpcOperation;

namespace Client {

class LangpackRpcLayer : public BaseRpcLayerExtension
{
    Q_OBJECT
public:
    explicit LangpackRpcLayer(QObject *parent = nullptr);

    // Generated Telegram API declarations
    PendingRpcOperation *getDifference(quint32 fromVersion);
    PendingRpcOperation *getLangPack(const QString &langCode);
    PendingRpcOperation *getLanguages();
    PendingRpcOperation *getStrings(const QString &langCode, const TLVector<QString> &keys);
    // End of generated Telegram API declarations
};

} // Client namespace

} // Telegram namespace

#endif // TELEGRAM_CLIENT_RPC_LANGPACK_LAYER_HPP