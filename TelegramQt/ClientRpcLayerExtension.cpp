/*
   Copyright (C) 2018 Alexandr Akulich <akulichalexander@gmail.com>

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

#include "ClientRpcLayerExtension_p.hpp"
#include "Debug_p.hpp"
#include "PendingRpcOperation.hpp"
#include "MTProto/Stream.hpp"
#include "Utils.hpp"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(c_clientRpcLayerExtensionCategory, "telegram.client.rpclayer.ext", QtWarningMsg)

namespace Telegram {

namespace Client {

template bool BaseRpcLayerExtension::processReply(PendingRpcOperation *operation, TLBool *output);

BaseRpcLayerExtension::BaseRpcLayerExtension(QObject *parent) :
    QObject(parent)
{
}

void BaseRpcLayerExtension::prepareReplyStream(MTProto::Stream *stream,
                                               PendingRpcOperation *operation)
{
    // TODO: Implement static isValid(TLValue::Value) method for TLTypes and
    // add a generated check that TLType is valid type for the RPC request.
    // Probably it would be better to hide this method from subclasses and
    // replace it with a processReply() reimpl with type-specific code
    // (check for TLType::isValid() and call this method)

    QByteArray data = operation->replyData();

    if (data.size() > 4) {
        if (TLValue::firstFromArray(data) == TLValue::GzipPacked) {
            MTProto::Stream packedStream(data);
            TLValue gzipValue;
            packedStream >> gzipValue;
            packedStream >> data;
            data = Utils::unpackGZip(data);
        }
    }
#ifdef DUMP_CLIENT_RPC_PACKETS
    qCDebug(c_clientRpcLayerExtensionCategory).noquote() << "BaseRpcLayerExtension: RPC Reply bytes:"
                                                         << data.size() << data.toHex();
#endif
    stream->setData(data);
}

void BaseRpcLayerExtension::setRpcProcessingMethod(RpcProcessingMethod sendMethod)
{
    qCDebug(c_clientRpcLayerExtensionCategory) << CALL_INFO << "update processing method";
    m_processingMethod = sendMethod;
}

void BaseRpcLayerExtension::processRpcCall(PendingRpcOperation *operation)
{
    const bool hasMethod(m_processingMethod);
    qCDebug(c_clientRpcLayerExtensionCategory) << CALL_INFO
                                               << "process (" << hasMethod << ")" << operation
                                               << TLValue::firstFromArray(operation->requestData());
    if (m_processingMethod) {
        m_processingMethod(operation);
    }
}

} // Client namespace

} // Telegram namespace
