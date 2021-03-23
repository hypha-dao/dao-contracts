#pragma once

#include <optional>

#include <eosio/eosio.hpp>

#include <document_graph/document.hpp>

namespace hypha {

class dao;

using eosio::name;
using eosio::time_point;
using eosio::checksum256; 

class TimeShare : public Document
{
public:
  /**
  * Inserts a new TimeShare in the Document Graph
  */
  TimeShare(name contract, name creator, int64_t timeShare, time_point startDate, checksum256 assignment);

  /**
  * Loads a TimeShare with the given hash
  */ 
  TimeShare(name contract, checksum256 hash);

  std::optional<TimeShare> getNext(name contract);
private:

  ContentGroups constructContentGroups(int64_t timeShare, time_point startDate, checksum256 assignment);
};

}