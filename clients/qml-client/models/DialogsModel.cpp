#include "DialogsModel.hpp"
#include "../../imports/TelegramQtQml/DeclarativeClient.hpp"
#include "../../TelegramQt/Client.hpp"
#include "../../TelegramQt/DataStorage.hpp"
#include "../../TelegramQt/MessagingApi.hpp"
#include "../../TelegramQt/DialogList.hpp"
#include "../../TelegramQt/PendingOperation.hpp"

#include <QDateTime>

#include <QDebug>

namespace Telegram {

namespace Client {

static const int UserRoleOffset = Qt::UserRole + 1;

DeclarativeClient *DialogsModel::client() const
{
    return m_client;
}

DialogsModel::DialogsModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

QHash<int, QByteArray> DialogsModel::roleNames() const
{
    static const QHash<int, QByteArray> extraRoles {
        { UserRoleOffset + static_cast<int>(Role::Peer), "peer" },
        { UserRoleOffset + static_cast<int>(Role::DisplayName), "displayName" },
        { UserRoleOffset + static_cast<int>(Role::UnreadMessageCount), "unreadMessageCount" },
        { UserRoleOffset + static_cast<int>(Role::LastMessage), "lastMessage" },
        { UserRoleOffset + static_cast<int>(Role::FormattedLastMessage), "formattedLastMessage" },
    };

    return extraRoles;
}

bool DialogsModel::hasPeer(const Peer peer) const
{
    return true;
}

QString DialogsModel::getName(const Peer peer) const
{
    return QString();
}

int DialogsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(Column::Count);
}

int DialogsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_dialogs.count();
}

QVariant DialogsModel::data(const QModelIndex &index, int role) const
{
    const Role r = indexToRole(index, role);
    return getData(index.row(), r);
}

QVariant DialogsModel::getData(int index, DialogsModel::Role role) const
{
    if (index < 0 || index >= m_dialogs.count()) {
        return QVariant();
    }
    const DialogInfo dialog = m_dialogs.at(index);

    switch (role) {
    case Role::Peer:
        return QVariant::fromValue(dialog.peer);
    case Role::PeerTypeIcon:
        return QVariant::fromValue(dialog.typeIcon);
    case Role::DisplayName:
        return dialog.name;
    case Role::UnreadMessageCount:
        return dialog.unreadCount;
    case Role::FormattedLastMessage:
        return dialog.formattedLastMessage;
    case Role::LastMessage:
        return QVariantMap({{"type", "text"},
                            {"text", dialog.lastChatMessage.text},
                            {"flags", static_cast<int>(dialog.lastChatMessage.flags / 2)},
                           });
        return dialog.formattedLastMessage;
    case Role::Picture:
    case Role::MuteUntil:
    case Role::MuteUntilDate:
        // invalid roles
    case Role::Count:
    case Role::Invalid:
        return QVariant();
    }
    Q_UNREACHABLE();
    return QVariant();
}

void DialogsModel::setClient(DeclarativeClient *target)
{
    m_client = target;
    emit clientChanged();
}

void DialogsModel::populate()
{
    m_list = m_client->messagingApi()->getDialogList();
    connect(m_list->becomeReady(), &Telegram::PendingOperation::finished, this, &DialogsModel::onListReady);
    if (m_list->isReady()) {
        onListReady();
    }
}

QString getPeerAlias(const Telegram::Peer &peer, const Telegram::Client::Client *client)
{
    if (peer.type == Telegram::Peer::Type::User) {
        Telegram::UserInfo info;
        if (client->dataStorage()->getUserInfo(&info, peer.id)) {
            QString name;
            if (!info.firstName().isEmpty()) {
                name = info.firstName();
            }
            if (!info.lastName().isEmpty()) {
                if (!name.isEmpty()) {
                    name += QLatin1Char(' ') + info.lastName();
                } else {
                    name = info.lastName();
                }
            }

            if (!name.simplified().isEmpty()) {
                return name;
            }

            if (!info.userName().isEmpty()) {
                return info.userName();
            }
        }
    } else {
        Telegram::ChatInfo info;
        if (client->dataStorage()->getChatInfo(&info, peer.id)) {
            return info.title();
        }
    }
    return peer.toString();
}

void DialogsModel::onListReady()
{
    qWarning() << Q_FUNC_INFO;
    connect(m_list, &DialogList::peerAdded, this, &DialogsModel::onNewPeer);
    beginResetModel();
    m_dialogs.clear();
    const QVector<Telegram::Peer> peers = m_list->getPeers();
    for (const Telegram::Peer &peer : peers) {
        addPeer(peer);
    }
    endResetModel();
}

void DialogsModel::onNewPeer(const Peer &peer)
{
    beginInsertRows(QModelIndex(), m_dialogs.count(), m_dialogs.count());
    addPeer(peer);
    endInsertRows();
}

void DialogsModel::addPeer(const Peer &peer)
{
    DialogInfo d;
    d.name = getPeerAlias(peer, m_client->backend());
    d.peer = peer;

    Telegram::DialogInfo apiInfo;
    m_client->backend()->dataStorage()->getDialogInfo(&apiInfo, peer);

    d.unreadCount = apiInfo.unreadCount();

    quint32 messageId = apiInfo.lastMessageId();
    Message message;
    m_client->backend()->dataStorage()->getMessage(&message, peer, messageId);
    //message.text = "long long long long text long long long long text";
    //message.flags = TelegramNamespace::MessageFlagOut;
    d.lastChatMessage = message;
    qWarning().noquote() << "message text:" << message.text;

    m_dialogs << d;
}

DialogsModel::Role DialogsModel::intToRole(int value)
{
    if (value < 0 || value > static_cast<int>(Role::Count)) {
        return Role::Invalid;
    }
    return static_cast<Role>(value);
}

DialogsModel::Column DialogsModel::intToColumn(int value)
{
    if (value < 0 || value > static_cast<int>(Column::Count)) {
        return Column::Invalid;
    }
    return static_cast<Column>(value);
}

DialogsModel::Role DialogsModel::indexToRole(const QModelIndex &index, int role)
{
    if (role >= UserRoleOffset) {
        return intToRole(role - UserRoleOffset);
    }
    //const Column section = intToColumn(index.column());
    return Role::Invalid;
}

} // Client namespace

} // Telegram namespace
