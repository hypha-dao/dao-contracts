#include <settings.hpp>

#include <algorithm>

#include <dao.hpp>
#include <common.hpp>
#include <logger/logger.hpp>

namespace hypha {
    Settings::Settings(dao& dao,
                       uint64_t id,
                       uint64_t rootID)
        : Document(dao.get_self(), id),
        m_dao(&dao),
        m_rootID(rootID)
        //m_dirty(false)
    {
    }

    void Settings::setSetting(const std::string& group, const Content& setting)
    {
        TRACE_FUNCTION()

        EOS_CHECK(group != SYSTEM, "system group is not editable")

        auto updateDateContent = Content(UPDATED_DATE, eosio::current_time_point());

        ContentWrapper cw = getContentWrapper();

        ContentGroup *settings = cw.getGroup(group).second;

        //Check if the group exits, otherwise create it
        if (settings == nullptr)
        {
            auto& cg = getContentGroups();
            cg.push_back({ Content { CONTENT_GROUP_LABEL, group } });
            settings = &cg.back();
        }

        ContentWrapper::insertOrReplace(*settings, setting);
        ContentWrapper::insertOrReplace(
            group != SETTINGS ? *cw.getGroupOrFail(SETTINGS) : *settings,
            updateDateContent
            );

        update();
    }

    void Settings::setSetting(const Content& setting)
    {
        setSetting(SETTINGS, setting);
    }

    void Settings::addSetting(const std::string& group, const Content& setting)
    {
        TRACE_FUNCTION()
        EOS_CHECK(group != SYSTEM, "system group is not editable")

        auto updateDateContent = Content(UPDATED_DATE, eosio::current_time_point());

        ContentWrapper cw       = getContentWrapper();
        ContentGroup * settings = cw.getGroup(group).second;

        //Check if the group exits, otherwise create it
        if (settings == nullptr)
        {
            auto& cg = getContentGroups();
            cg.push_back({ Content { CONTENT_GROUP_LABEL, group } });
            settings = &cg.back();
        }

        ContentWrapper::insertOrReplace(
            group != SETTINGS ? *cw.getGroupOrFail(SETTINGS) : *settings,
            updateDateContent
            );
        settings->push_back(setting);

        update();
    }

    void Settings::remSetting(const std::string& group, const std::string& key)
    {
        TRACE_FUNCTION()

        EOS_CHECK(group != SYSTEM, "system group is not editable")

        auto oldID = getID();

        auto cw = getContentWrapper();

        auto [groupIdx, contentGroup] = cw.getGroup(group);

        cw.removeContent(groupIdx, key);

        auto updateDateContent = Content(UPDATED_DATE, eosio::current_time_point());

        ContentWrapper::insertOrReplace(
            group != SETTINGS ? *cw.getGroupOrFail(SETTINGS) : *contentGroup,
            updateDateContent
            );

        update();
    }

    void Settings::remSetting(const std::string& key)
    {
        remSetting(SETTINGS, key);
    }

    void Settings::remKVSetting(const std::string& group, const Content& setting)
    {
        TRACE_FUNCTION()

        EOS_CHECK(group != SYSTEM, "system group is not editable")

        auto oldID = getID();

        auto cw = getContentWrapper();

        auto [groupIdx, contentGroup] = cw.getGroup(group);

        auto itemIt = std::find(contentGroup->begin(), contentGroup->end(), setting);

        EOS_CHECK(
            itemIt != contentGroup->end(),
            util::to_str("Couldn't find setting: ", setting, " in group: ", group)
            );

        contentGroup->erase(itemIt);

        auto updateDateContent = Content(UPDATED_DATE, eosio::current_time_point());

        ContentWrapper::insertOrReplace(
            group != SETTINGS ? *cw.getGroupOrFail(SETTINGS) : *contentGroup,
            updateDateContent
            );

        update();
    }
}
