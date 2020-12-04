#include <document_graph/content_wrapper.hpp>
#include <document_graph/util.hpp>
#include <proposals/proposal.hpp>
#include <proposals/proposal_factory.hpp>

#include <dao.hpp>
#include <common.hpp>

#include <util.hpp>
#include <member.hpp>
#include <period.hpp>

namespace hypha
{
   void dao::propose(const name &proposer,
                     const name &proposal_type,
                     ContentGroups &content_groups)
   {
      eosio::check(!is_paused(), "Contract is paused for maintenance. Please try again later.");

      eosio::name self = get_self();
      std::unique_ptr<Proposal> proposal = std::unique_ptr<Proposal>(ProposalFactory::Factory(*this, proposal_type));
      proposal->propose(proposer, content_groups);
      // delete proposal;
   }

   void dao::closedocprop(const checksum256 &proposal_hash)
   {
      eosio::check(!is_paused(), "Contract is paused for maintenance. Please try again later.");

      Document docprop(get_self(), proposal_hash);
      name proposal_type = docprop.getContentWrapper().getOrFail(common::SYSTEM, common::TYPE, "proposal type is required")->getAs<eosio::name>();

      Proposal *proposal = ProposalFactory::Factory(*this, proposal_type);
      proposal->close(docprop);
      // delete proposal;
   }

   void dao::apply(const eosio::name &applicant, const std::string &content)
   {
      require_auth(applicant);
      Member member(get_self(), applicant, applicant);
      member.apply(getRoot(get_self()), content);
   }

   void dao::enroll(const eosio::name &enroller, const eosio::name &applicant, const std::string &content)
   {
      require_auth(enroller);
      Member member(get_self(), enroller, applicant);
      member.enroll(enroller, content);
   }

   bool dao::is_paused() { return false; }

   Document dao::getSettingsDocument()
   {
      auto root = getRoot(get_self());

      auto edges = m_documentGraph.getEdgesFromOrFail(root, common::SETTINGS_EDGE);

      eosio::check(edges.size() == 1, "There should only exists only 1 settings edge from root node");

      eosio::print("getSettingsDocument::retrieving settings document: " + readableHash(edges[0].to_node) + "\n");
      return Document(get_self(), edges[0].to_node);
   }

   void dao::setsetting(const string &key, const Content::FlexValue &value)
   {
      require_auth(get_self());
      set_setting(key, value);
   }

   void dao::set_setting(const string &key, const Content::FlexValue &value)
   {
      auto document = getSettingsDocument();

      auto oldHash = document.getHash();

      auto settingContent = Content(key, value);

      auto updateDateContent = Content(common::UPDATED_DATE, eosio::current_time_point());

      //Might want to return by & instead of const &
      auto contentGroups = document.getContentGroups();

      auto &settingsGroup = contentGroups[0];

      ContentWrapper::insertOrReplace(settingsGroup, settingContent);
      ContentWrapper::insertOrReplace(settingsGroup, updateDateContent);

      //We could to change ContentWrapper to store a regulare reference instead of a constant
      //Maybe also a ref to Document so we can rehashContents on data changes

      m_documentGraph.updateDocument(get_self(), oldHash, std::move(contentGroups));
   }

   void dao::remsetting(const string &key)
   {
      require_auth(get_self());
      remove_setting(key);
   }

   void dao::remove_setting(const string &key)
   {
      auto document = getSettingsDocument();

      auto oldHash = document.getHash();

      auto contentGroups = document.getContentGroups();

      auto &settingsGroup = contentGroups[0];

      auto isKey = [&key](auto &c) {
         return c.label == key;
      };

      //First let's check if key exists
      auto contentItr = std::find_if(settingsGroup.begin(), settingsGroup.end(), isKey);

      if (contentItr != settingsGroup.end())
      {
         settingsGroup.erase(contentItr);

         auto updateDateContent = Content(common::UPDATED_DATE, eosio::current_time_point());

         ContentWrapper::insertOrReplace(settingsGroup, updateDateContent);

         m_documentGraph.updateDocument(get_self(), oldHash, std::move(contentGroups));
      }
      //Should we assert if setting doesn't exits ?
      eosio::check(false, "The specified setting doesn't exits: " + key);
   }

   void dao::addperiod(const eosio::time_point &start_time, const eosio::time_point &end_time, const string &label)
   {
      require_auth(get_self());
      Period period(*this, start_time, end_time, label);
      period.emplace();
   }

   void dao::createroot(const std::string &notes)
   {
      require_auth(get_self());

      Document rootDoc(get_self(), get_self(), Content(common::ROOT_NODE, get_self()));

      //Create the settings document as well and add an edge to it
      ContentGroups settingCgs{{Content(CONTENT_GROUP_LABEL, common::SETTINGS),
                                Content(common::ROOT_NODE, readableHash(rootDoc.getHash()))}};

      Document settingsDoc(get_self(), get_self(), std::move(settingCgs));

      Edge::write(get_self(), get_self(), rootDoc.getHash(), settingsDoc.getHash(), common::SETTINGS_EDGE);
   }

} // namespace hypha