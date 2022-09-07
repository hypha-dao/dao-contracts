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
  {}

  Settings::Settings(dao& dao, 
                     uint64_t rootID)
  : Document(dao.get_self(), dao.get_self(), ContentGroups{
    ContentGroup{
        Content(CONTENT_GROUP_LABEL, SETTINGS),
        Content(ROOT_NODE, util::to_str(rootID))
    },
    ContentGroup{
        Content(CONTENT_GROUP_LABEL, SYSTEM),
        Content(TYPE, common::SETTINGS_EDGE),
        Content(NODE_LABEL, "Settings")
    }
  })
  {
    Edge::getOrNew(dao.get_self(), dao.get_self(), rootID, getID(), common::SETTINGS_EDGE);
  }

  void Settings::setSetting(const std::string& group, const Content& setting)
  {
    TRACE_FUNCTION()

    EOS_CHECK(group != SYSTEM, "system group is not editable")

    ContentWrapper cw = getContentWrapper();

    ContentGroup* settings = cw.getGroup(group).second;

    //Check if the group exits, otherwise create it
    if (settings == nullptr) {
      auto& cg = getContentGroups();
      cg.push_back({Content { CONTENT_GROUP_LABEL, group }});
      settings = &cg.back();
    }

    ContentWrapper::insertOrReplace(*settings, setting);

    update();
  }

  void Settings::setSetting(const Content& setting)
  {
    setSetting(SETTINGS, setting);
  }

  void Settings::setSettings(const std::string& group, const std::map<std::string, Content::FlexValue>& kvs)
  {
    TRACE_FUNCTION()

    EOS_CHECK(group != SYSTEM, "system group is not editable")

    ContentWrapper cw = getContentWrapper();

    ContentGroup* settings = cw.getGroup(group).second;

    //Check if the group exits, otherwise create it
    if (settings == nullptr) {
      auto& cg = getContentGroups();
      cg.push_back({Content { CONTENT_GROUP_LABEL, group }});
      settings = &cg.back();
    }

    for (auto& kv : kvs) {
      ContentWrapper::insertOrReplace(*settings, Content{kv.first, kv.second});
    }

    update();
  }

  void Settings::addSetting(const std::string& group, const Content& setting)
  {
    TRACE_FUNCTION()
    EOS_CHECK(group != SYSTEM, "system group is not editable")
    
    ContentWrapper cw = getContentWrapper();
    ContentGroup* settings = cw.getGroup(group).second;

    //Check if the group exits, otherwise create it
    if (settings == nullptr) {
      auto& cg = getContentGroups();
      cg.push_back({Content { CONTENT_GROUP_LABEL, group }});
      settings = &cg.back();
    }

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