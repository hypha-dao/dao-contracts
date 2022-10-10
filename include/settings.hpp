#pragma once

#include <map>

#include <document_graph/document.hpp>
#include <logger/logger.hpp>
#include <eosio/eosio.hpp>

namespace hypha {

class dao;

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

    Settings(dao& dao, 
             uint64_t rootID);

    /** @brief Replaces the value of an existing setting, or creates it if it doesn't exist
    * 
    */
    void setSetting(const Content& setting);
    
    /** @brief Replaces the value of an existing setting, or creates it if it doesn't exist
    * 
    */
    void setSetting(const std::string& group, const Content& setting);

    /** @brief Replaces the values of an existing set of settings, or creates them if they don't exist
    * 
    */
    void setSettings(const std::string& group, const std::map<std::string, Content::FlexValue>& kvs);

    /** @brief Adds a new setting to the specified group, even if there is an exiting item with the same key
    * 
    */
    void addSetting(const std::string& group, const Content& setting);

    /**
     * @brief Removes the first item which label is equal to the provided key
     * 
     * @param key 
     */    
    void remSetting(const std::string& key);

    /**
     * @brief Removes the first item which label is equal to the provided key.
     * The action will fail if there is no such item.
     * 
     * @param key 
     */
    void remSetting(const std::string& group, const std::string& key);

    /**
     * @brief Removes the item which key (label) and value are exactly equal to ones in the provided setting.
     * The action will fail if there is no such item.
     * 
     * @param setting 
     */
    void remKVSetting(const std::string& group, const Content& setting);
    
    inline uint64_t getRootID() 
    {
        return m_rootID;
    }

    template<class T>
    std::optional<T> getSettingOpt(const std::string& key)
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
    const T& getOrFail(const std::string& group, const string& key)
    {
        TRACE_FUNCTION()
        auto content = getContentWrapper().getOrFail(group, key, "setting " + key + " does not exist in " + group);

        if (auto p = std::get_if<T>(&content->value)) {
            return *p;
        }
        else {
            EOS_CHECK(
                false, 
                to_str("Setting value is not of expected type: ", key)
            )
            
            //Just to suprim warnings but it will never get called
            return std::get<T>(content->value);
        }
    }

    template<class T>
    const T& getOrFail(const std::string& key)
    {
        TRACE_FUNCTION()
        return getOrFail<T>("settings", key);
    }

    template <class T>
    T getSettingOrDefault(const std::string &setting, const T &def = T{})
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
    uint64_t m_rootID;
    dao* m_dao;
};

}