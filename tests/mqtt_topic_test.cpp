#include <gtest/gtest.h>

#include <hwlib/communication/mqtt_topic.hpp>

namespace
{

using hwlib::communication::MatchTopic;

// Every case the original carried as a static_assert, most of them from the
// examples in MQTT 3.1.1 section 4.7. Kept as they were, so the rewrite is held to
// the behaviour the two copies in the field already had.
static_assert(MatchTopic({}, {}));
static_assert(!MatchTopic({}, "a"));
static_assert(!MatchTopic("a", {}));
static_assert(MatchTopic("/", "/"));
static_assert(MatchTopic("/aaa", "/aaa"));
static_assert(!MatchTopic("/aaa", "/bbb"));
static_assert(MatchTopic("/aaa/bbb", "/aaa/bbb"));
static_assert(!MatchTopic("/aaa/bbb", "/aaa/ccc"));
static_assert(MatchTopic("/aaa/+/ccc", "/aaa/bbb/ccc"));
static_assert(!MatchTopic("/aaa/+/bbb", "/aaa/bbb/ccc"));
static_assert(MatchTopic("/aaa/bbb/#", "/aaa/bbb/ccc"));
static_assert(!MatchTopic("/aaa/ddd/#", "/aaa/bbb/ccc"));
static_assert(MatchTopic("/aaa/bbb/#", "/aaa/bbb/ccc/ddd/eee"));
static_assert(MatchTopic("/aaa/+/+/ddd/#", "/aaa/bbb/ccc/ddd/eee/fff"));
static_assert(MatchTopic("/aaa/#", "/aaa"));
static_assert(MatchTopic("/aaa/#", "/aaa/"));
static_assert(MatchTopic("/aaa/+", "/aaa/"));
static_assert(MatchTopic("sport/tennis/player1/#", "sport/tennis/player1"));
static_assert(MatchTopic("sport/tennis/player1/#", "sport/tennis/player1/ranking"));
static_assert(MatchTopic("sport/tennis/player1/#", "sport/tennis/player1/score/wimbledon"));
static_assert(MatchTopic("#", "sport/tennis/player1/score/wimbledon"));
static_assert(!MatchTopic("#", "$sport/tennis/player1/score/wimbledon"));
static_assert(MatchTopic("sport/tennis/+", "sport/tennis/player1"));
static_assert(MatchTopic("sport/tennis/+", "sport/tennis/player2"));
static_assert(!MatchTopic("sport/tennis/+", "sport/tennis/player1/ranking"));
static_assert(MatchTopic("sport/+", "sport/"));
static_assert(!MatchTopic("sport/+", "sport"));
static_assert(MatchTopic("+/+", "/finance"));
static_assert(MatchTopic("/+", "/finance"));
static_assert(!MatchTopic("+", "/finance"));
static_assert(!MatchTopic("+/monitor/Clients", "$SYS/monitor/Clients"));
static_assert(MatchTopic("$SYS/monitor/+", "$SYS/monitor/Clients"));

TEST(MqttTopicTest, DoesNotTreatHashAsAStringPrefix)
{
    // The original matched both of these: it compared "sensors" against the first
    // seven characters of the topic, without looking for the level boundary.
    EXPECT_FALSE(MatchTopic("sensors/#", "sensorsX/temp"));
    EXPECT_FALSE(MatchTopic("a/#", "ab"));
    // What it should match still does.
    EXPECT_TRUE(MatchTopic("sensors/#", "sensors/temp"));
    EXPECT_TRUE(MatchTopic("sensors/#", "sensors"));
}

TEST(MqttTopicTest, MatchesTheParentLevelAfterAPlus)
{
    // "+" takes "a", and "#" covers the parent level, so the topic matches. The
    // original missed it.
    EXPECT_TRUE(MatchTopic("+/#", "a"));
    EXPECT_TRUE(MatchTopic("+/#", "a/b/c"));
    EXPECT_TRUE(MatchTopic("home/+/#", "home/kitchen"));
}

TEST(MqttTopicTest, CountsAPlusAsExactlyOneLevel)
{
    EXPECT_TRUE(MatchTopic("home/+/temp", "home/kitchen/temp"));
    EXPECT_FALSE(MatchTopic("home/+/temp", "home/kitchen/sink/temp"));
    EXPECT_FALSE(MatchTopic("home/+/temp", "home/temp"));
    EXPECT_TRUE(MatchTopic("home/+/temp", "home//temp"));
}

TEST(MqttTopicTest, KeepsDollarTopicsFromFirstLevelWildcards)
{
    EXPECT_FALSE(MatchTopic("#", "$SYS/broker/uptime"));
    EXPECT_FALSE(MatchTopic("+/broker/uptime", "$SYS/broker/uptime"));
    EXPECT_TRUE(MatchTopic("$SYS/#", "$SYS/broker/uptime"));
    // Only the first level is special.
    EXPECT_TRUE(MatchTopic("a/+", "a/$b"));
}

TEST(MqttTopicTest, MatchesNothingWithAHashThatIsNotLast)
{
    EXPECT_FALSE(MatchTopic("a/#/b", "a/x/b"));
    EXPECT_FALSE(MatchTopic("#/b", "a/b"));
}

TEST(MqttTopicTest, TreatsAWildcardInsideALevelAsAnOrdinaryCharacter)
{
    // Not a wildcard unless it is the whole level.
    EXPECT_FALSE(MatchTopic("a+", "ab"));
    EXPECT_TRUE(MatchTopic("a+", "a+"));
    EXPECT_FALSE(MatchTopic("a#", "ab"));
}

TEST(MqttTopicTest, IsCaseSensitive)
{
    EXPECT_FALSE(MatchTopic("ACCOUNTS", "Accounts"));
}

} // namespace
