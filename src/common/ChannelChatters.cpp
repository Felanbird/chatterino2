#include "ChannelChatters.hpp"

#include "common/Channel.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"

#include <QColor>

namespace chatterino {

ChannelChatters::ChannelChatters(Channel &channel)
    : channel_(channel)
    , chatterColors_(ChannelChatters::maxChatterColorCount)
{
}

SharedAccessGuard<const ChatterSet> ChannelChatters::accessChatters() const
{
    return this->chatters_.accessConst();
}

void ChannelChatters::addRecentChatter(const QString &user)
{
    auto chatters = this->chatters_.access();
    chatters->addRecentChatter(user);
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
            joinedUsers->sort();

            MessageBuilder builder;
            TwitchMessageBuilder::listOfUsersSystemMessage(
                "Users joined:", *joinedUsers, &this->channel_, &builder);
            builder->flags.set(MessageFlag::Collapsed);
            this->channel_.addMessage(builder.release());

            joinedUsers->clear();
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
            partedUsers->sort();

            MessageBuilder builder;
            TwitchMessageBuilder::listOfUsersSystemMessage(
                "Users parted:", *partedUsers, &this->channel_, &builder);
            builder->flags.set(MessageFlag::Collapsed);
            this->channel_.addMessage(builder.release());

            partedUsers->clear();
            this->partedUsersMergeQueued_ = false;
        });
    }
}

void ChannelChatters::updateOnlineChatters(
    const std::unordered_set<QString> &usernames)
{
    auto chatters = this->chatters_.access();
    chatters->updateOnlineChatters(usernames);
}

size_t ChannelChatters::colorsSize() const
{
    auto size = this->chatterColors_.access()->size();
    return size;
}

const QColor ChannelChatters::getUserColor(const QString &user)
{
    const auto chatterColors = this->chatterColors_.access();

    auto lowerUser = user.toLower();

    if (!chatterColors->exists(lowerUser))
    {
        // Returns an invalid color so we can decide not to override `textColor`
        return QColor();
    }

    return QColor::fromRgb(chatterColors->get(lowerUser));
}

void ChannelChatters::setUserColor(const QString &user, const QColor &color)
{
    const auto chatterColors = this->chatterColors_.access();
    chatterColors->put(user.toLower(), color.rgb());
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
        {"aeaer", "Ae/Aer"},       {"any", "Any"},
        {"eem", "E/Em"},           {"faefaer", "Fae/Faer"},
        {"hehim", "He/Him"},       {"heshe", "He/She"},
        {"hethem", "He/They"},     {"itits", "It/Its"},
        {"other", "Other"},        {"perper", "Per/Per"},
        {"sheher", "She/Her"},     {"shethem", "She/They"},
        {"theythem", "They/Them"}, {"vever", "Ve/Ver"},
        {"xexem", "Xe/Xem"},       {"ziehir", "Zie/Hir"}};

    return pronounTable[search->second];
}

void ChannelChatters::setUserPronouns(const QString &user,
                                      const QString &pronouns)
{
    const auto chatterPronouns = this->chatterPronouns_.access();
    chatterPronouns->insert_or_assign(user.toLower(), pronouns);
}

}  // namespace chatterino
