#include "ChannelChatters.hpp"

#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"

namespace chatterino {

ChannelChatters::ChannelChatters(Channel &channel)
    : channel_(channel)
{
}

AccessGuard<const UsernameSet> ChannelChatters::accessChatters() const
{
    return this->chatters_.accessConst();
}

void ChannelChatters::addRecentChatter(const QString &user)
{
    this->chatters_.access()->insert(user);
}

void ChannelChatters::addJoinedUser(const QString &user)
{
    auto joinedUsers = this->joinedUsers_.access();
    joinedUsers->append(user);

    qDebug() << "joined user: " << user;

    if (!this->joinedUsersMergeQueued_)
    {
        this->joinedUsersMergeQueued_ = true;

        QTimer::singleShot(500, &this->lifetimeGuard_, [this] {
            auto joinedUsers = this->joinedUsers_.access();

            MessageBuilder builder(systemMessage,
                                   "Users joined: " + joinedUsers->join(", "));
            builder->flags.set(MessageFlag::Collapsed);
            joinedUsers->clear();
            this->channel_.addMessage(builder.release());
            this->joinedUsersMergeQueued_ = false;
        });
    }
}

void ChannelChatters::addPartedUser(const QString &user)
{
    auto partedUsers = this->partedUsers_.access();
    partedUsers->append(user);

    if (!this->partedUsersMergeQueued_)
    {
        this->partedUsersMergeQueued_ = true;

        QTimer::singleShot(500, &this->lifetimeGuard_, [this] {
            auto partedUsers = this->partedUsers_.access();

            MessageBuilder builder(systemMessage,
                                   "Users parted: " + partedUsers->join(", "));
            builder->flags.set(MessageFlag::Collapsed);
            this->channel_.addMessage(builder.release());
            partedUsers->clear();

            this->partedUsersMergeQueued_ = false;
        });
    }
}

void ChannelChatters::setChatters(UsernameSet &&set)
{
    this->chatters_.access()->merge(std::move(set));
}

const QColor ChannelChatters::getUserColor(const QString &user)
{
    const auto chatterColors = this->chatterColors_.accessConst();

    const auto search = chatterColors->find(user.toLower());
    if (search == chatterColors->end())
    {
        // Returns an invalid color so we can decide not to override `textColor`
        return QColor();
    }

    return search->second;
}

void ChannelChatters::setUserColor(const QString &user, const QColor &color)
{
    const auto chatterColors = this->chatterColors_.access();
    chatterColors->insert_or_assign(user.toLower(), color);
}

const QString ChannelChatters::getUserPronouns(const QString &user)
{
    const auto chatterPronouns = this->chatterPronouns_.accessConst();

    const auto search = chatterPronouns->find(user.toLower());
    if (search == chatterPronouns->end())
    {
        // Returns empty string; no pronouns set
        return QString();
    }

    const static QHash<QString, QString> pronounTable = {
        {"hehim", "He/Him"},       {"sheher", "She/Her"},
        {"hethem", "He/They"},     {"shethem", "She/They"},
        {"theythem", "They/Them"}, {"any", "Any"},
        {"other", "Other"},        {"itits", "It/Its"},
        {"aeaer", "Ae/Aer"},       {"eem", "E/Em"},
        {"faefaer", "Fae/Faer"},   {"perper", "Per/Per"},
        {"vever", "Ve/Ver"},       {"xexem", "Xe/Xem"},
        {"ziehir", "Zie/Hir"}};

    return pronounTable[search->second];
}

void ChannelChatters::setUserPronouns(const QString &user,
                                      const QString &pronouns)
{
    const auto chatterPronouns = this->chatterPronouns_.access();
    chatterPronouns->insert_or_assign(user.toLower(), pronouns);
}

}  // namespace chatterino
