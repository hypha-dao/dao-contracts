#pragma once

#include <optional>

#include <eosio/eosio.hpp>

#include <document_graph/document.hpp>

namespace hypha {

class dao;

using eosio::name;
using eosio::time_point;

class TimeShare : public Document
{
public:
  /**
  * Inserts a new TimeShare in the Document Graph
  */
  TimeShare(name contract, name creator, int64_t timeShare, time_point startDate, uint64_t assignment);

  /**
  * Loads a TimeShare with the given id
  */ 
  TimeShare(name contract, uint64_t id);

  std::optional<TimeShare> getNext();

  time_point getStartDate();
private:

  ContentGroups constructContentGroups(int64_t timeShare, time_point startDate, uint64_t assignment);
};

}