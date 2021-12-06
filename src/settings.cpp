#include <settings.hpp>
#include <dao.hpp>
#include <common.hpp>

namespace hypha {

Settings::Settings(dao& dao, 
                          const checksum256& hash, 
                          const checksum256& rootHash)
  : Document(dao.get_self(), hash),
    m_dao(&dao),
    m_rootHash(rootHash)
    //m_dirty(false)
  {}

  void Settings::setSetting(const Content& setting)
  {
    auto oldHash = getHash();
    auto updateDateContent = Content(UPDATED_DATE, eosio::current_time_point());

    ContentWrapper cw = getContentWrapper();
    ContentGroup *settings = cw.getGroupOrFail("settings");

    ContentWrapper::insertOrReplace(*settings, setting);
    ContentWrapper::insertOrReplace(*settings, updateDateContent);

    static_cast<Document&>(*this) = m_dao->getGraph()
                                         .updateDocument(m_dao->get_self(), 
                                                         oldHash, 
                                                         getContentGroups());

    //m_dirty = true;
  }

  void Settings::remSetting(const string& key)
  {
    auto oldHash = getHash();
    auto& contentGroups = getContentGroups();
    auto& settingsGroup = contentGroups[SETTINGS_IDX];

    auto isKey = [&key](auto &c) {
        return c.label == key;
    };

    //First let's check if key exists
    auto contentItr = std::find_if(settingsGroup.begin(), settingsGroup.end(), isKey);
    if (contentItr != settingsGroup.end())
    {
        settingsGroup.erase(contentItr);
        auto updateDateContent = Content(UPDATED_DATE, eosio::current_time_point());
        ContentWrapper::insertOrReplace(settingsGroup, updateDateContent);
        static_cast<Document&>(*this) = m_dao->getGraph()
                                             .updateDocument(m_dao->get_self(), 
                                                             oldHash, 
                                                             std::move(contentGroups));
    }
    //Should we assert if setting doesn't exits ?
    EOS_CHECK(false, "The specified setting does not exist: " + key);
  }

  }