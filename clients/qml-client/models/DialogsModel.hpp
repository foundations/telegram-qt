#ifndef TELEGRAMQT_DIALOGS_MODEL_HPP
#define TELEGRAMQT_DIALOGS_MODEL_HPP

#include <QAbstractTableModel>

namespace Telegram {

namespace Client {

class DeclarativeClient;
class DialogList;

class PeerEnums : public QObject
{
    Q_OBJECT
public:
    enum Type {
        Invalid,
        Contact,
        Room,
    };
    Q_ENUM(Type)
};

struct Peer
{
    Q_GADGET
    Q_PROPERTY(int type READ typeInt WRITE setTypeInt)
    Q_PROPERTY(QString id MEMBER id)
public:
    using Type = PeerEnums::Type;

    Peer() = default;
    Peer(const Peer &p) = default;

    Peer(const QString &id, Type t) : type(t), id(id)
    {
    }

    Type type = Type::Invalid;
    QString id;

    Q_INVOKABLE bool isValid() const { return (type != Type::Invalid) && id.isEmpty(); }
    int typeInt() const { return static_cast<int>(type); }
    void setTypeInt(int t) { type = static_cast<Type>(t); }

    bool operator==(const Peer &p) const
    {
        return (p.type == type) && (p.id == id);
    }

    static Peer fromContactId(const QString &id)
    {
        return Peer(id, Type::Contact);
    }

    static Peer fromRoomId(const QString &id)
    {
        return Peer(id, Type::Room);
    }
};

class DialogsModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_PROPERTY(Telegram::Client::DeclarativeClient *client READ client WRITE setClient NOTIFY clientChanged)
public:
    struct DialogInfo {
        Peer peer;
        QString typeIcon;
        QString name;
        int unreadCount = 0;
    };

    enum class Column {
        PeerType,
        PeerId,
        PeerName,
        Picture, // Photo (in terms of Telegram)
        MuteUntil,
        MuteUntilDate,
        Count,
        Invalid
    };

    enum class Role {
        Peer,
        PeerTypeIcon,
        DisplayName,
        Picture, // Photo (in terms of Telegram)
        MuteUntil,
        MuteUntilDate,
        UnreadMessageCount,
        Count,
        Invalid
    };

    DeclarativeClient *client() const;

    explicit DialogsModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;

    bool hasPeer(const Peer peer) const;
    QString getName(const Peer peer) const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant getData(int index, Role role) const;

public slots:
    void setClient(DeclarativeClient *client);

    void populate();
//    void setDialogs(const QVector<Telegram::Peer> &dialogs);
//    void syncDialogs(const QVector<Telegram::Peer> &added, const QVector<Telegram::Peer> &removed);

//protected slots:
//    void onPeerPictureChanged(const Telegram::Peer peer);

//protected:
//    static Role columnToRole(Column column, int qtRole);
//    CPeerModel *modelForPeer(const Telegram::Peer peer) const;

Q_SIGNALS:
    void clientChanged();

private slots:
    void onListReady();

private:
    static Role intToRole(int value);
    static Column intToColumn(int value);
    static Role indexToRole(const QModelIndex &index, int role = Qt::DisplayRole);
    QVector<DialogInfo> m_dialogs;
    DeclarativeClient *m_client = nullptr;
    DialogList *m_list = nullptr;

};

} // Client namespace

} // Telegram namespace

#endif // TELEGRAMQT_DIALOGS_MODEL_HPP
