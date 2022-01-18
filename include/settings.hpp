#pragma once

#include <document_graph/document.hpp>
#include <logger/logger.hpp>
#include <eosio/eosio.hpp>

namespace hypha {

class dao;

using eosio::checksum256;

/**
 * @brief 
 * Stores a setting document
 */
class Settings : public Document
{
public:
    Settings(dao& dao, 
            uint64_t id, 
            uint64_t rootID);
    void setSetting(const Content& setting);
    void remSetting(const string& key);
    //const Content* getSetting(const string& key);

    inline uint64_t getRootID() 
    {
        return m_rootID;
    }

    template<class T>
    std::optional<T> getSettingOpt(const string& key)
    {
        TRACE_FUNCTION()
        auto [idx, content] = getContentWrapper().get(SETTINGS_IDX, key);
        if (auto p = std::get_if<T>(&content->value))
        {
            return *p;
        }

        return {};
    }

    template<class T>
    const T& getOrFail(const string& key)
    {
        TRACE_FUNCTION()
        auto [_, content] = getContentWrapper().getOrFail(SETTINGS_IDX, key, "setting " + key + " does not exist");

        if (auto p = std::get_if<T>(&content->value)) {
            return *p;
        }
        else {
            EOS_CHECK(
                false, 
                util::to_str("Setting value is not of expected type: ", key)
            )
            
            //Just to suprim warnings but it will never get called
            return std::get<T>(content->value);
        }
    }

    template <class T>
    T getSettingOrDefault(const string &setting, const T &def = T{})
    {
        if (auto content = getSettingOpt<T>(setting))
        {
            return *content;
        }

        return def;
    }

    //Default index for settings group
    static constexpr int64_t SETTINGS_IDX = 0;
private:
    //bool m_dirty;
    checksum256 m_rootHash;
    uint64_t m_rootID;
    dao* m_dao;
};

}