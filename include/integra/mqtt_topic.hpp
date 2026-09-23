#pragma once

#include <string_view>

namespace integra
{

/// Whether an MQTT topic filter matches a topic name, per MQTT 3.1.1 section 4.7
/// (and 5.0 section 4.7, which did not change it).
///
/// The filter is walked level by level, `/` to `/`, because that is what the
/// wildcards are defined over:
///
/// * `+` matches exactly one level, including an empty one;
/// * `#` matches the level it stands on, every level below, and the parent level
///   too — `sport/#` matches `sport`;
/// * neither matches a topic that starts with `$` from the first level, so `#` does
///   not deliver `$SYS/...` to a client that did not ask for it by name.
///
/// A wildcard only counts as one when it is the whole level. `#` that is not the
/// last level makes the filter invalid, and an invalid filter matches nothing.
///
/// The original compared a `#` filter as a string prefix, so `sensors/#` matched
/// `sensorsX/temp`; and it could not match `#` against the parent level after a `+`,
/// so `+/#` missed `a`. Walking levels removes both.
[[nodiscard]] constexpr bool MatchTopic(std::string_view filter, std::string_view topic) noexcept
{
    if (!topic.empty() && topic.front() == '$' && !filter.empty() && (filter.front() == '+' || filter.front() == '#'))
    {
        return false;
    }

    while (true)
    {
        const auto filterSlash = filter.find('/');
        const auto filterLevel = filter.substr(0, filterSlash);
        if (filterLevel == "#")
        {
            return filterSlash == std::string_view::npos;
        }

        const auto topicSlash = topic.find('/');
        if (filterLevel != "+" && filterLevel != topic.substr(0, topicSlash))
        {
            return false;
        }

        const bool filterHasMore = filterSlash != std::string_view::npos;
        const bool topicHasMore  = topicSlash != std::string_view::npos;
        if (!filterHasMore)
        {
            return !topicHasMore;
        }
        if (!topicHasMore)
        {
            // The topic ended a level early. Only a closing `#` still matches,
            // because it covers its parent level.
            return filter.substr(filterSlash + 1U) == "#";
        }

        filter.remove_prefix(filterSlash + 1U);
        topic.remove_prefix(topicSlash + 1U);
    }
}

} // namespace integra
