import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import Qt.labs.platform 1.0
import TelegramQt 1.0 as Telegram
import Client 1.0 as Telegram
import TelegramQtTheme 1.0

Frame {
    id: messageView
    width: 800
    height: 600
    property alias peer: messagesModel.peer
    leftPadding: 0
    rightPadding: 0

    Telegram.MessagesModel {
        client: telegramClient
        id: messagesModel
    }

    Connections {
        target: messageSendStubProxy
        onMessageSent: {
            var text = message
            var dateTime = new Date()
            var timeText = Qt.formatTime(dateTime, "h:mm AP")
            timeText = timeText.slice(0, -3)
            if (dateTime.getHours() >= 12) {
                timeText += " PM"
            } else {
                timeText += " AM"
            }

//            messagesModel.append({
//                                    type: Telegram.MessageModel.MessageTypeText,
//                                    sender: "You",
//                                    senderPeer: Telegram.Namespace.peerFromUserId(1),
//                                    message: text,
//                                    time: timeText
//                                })
        }
    }

    Component {
        id: messageDelegate
        MessageDelegate {
        }
    }

    Component {
        id: newDayDelegate
        ServiceMessageDelegate {
            text: Qt.formatDate(model.timestamp, Qt.DefaultLocaleLongDate)
        }
    }
    Component {
        id: serviceActionDelegate
        ServiceMessageDelegate {
            text: mkLinkToPeer(model.actor) + " added " + mkLinkToPeer(model.users)
            plainText: model.actor + " added " + model.users
            textFormat: Text.StyledText
            linkColor: textColor
            function mkLinkToPeer(peer) {
                return "<a href=\"" + peer + "\">" + peer + "</a>"
            }
            onLinkActivated: {
                console.log("Link activated: " + link)
            }
        }
    }

    MessageListView {
        id: listView
        anchors.fill: parent
        model: messagesModel
        delegate: Item {
            property var itemModel: model
            width: loader.width
            height: loader.height
            Loader {
                id: loader
                width: listView.width
                property var model: parent.itemModel // inject 'model' to the loaded item's context
                sourceComponent: {
                    if (model.eventType == Telegram.Event.Type.NewDay) {
                        return newDayDelegate
                    } else if (model.eventType == Telegram.Event.Type.Message) {
                        return messageDelegate
                    } else if (model.eventType == Telegram.Event.Type.ServiceAction) {
                        return serviceActionDelegate
                    }
                    return messageDelegate
                }
            }
        }
    }
}
