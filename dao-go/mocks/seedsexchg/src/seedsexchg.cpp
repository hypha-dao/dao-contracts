#include <seedsexchg.hpp>

void seedsexchg::updateconfig(asset seeds_per_usd, asset tlos_per_usd, asset citizen_limit,
                    asset resident_limit, asset visitor_limit)
{
   configtables config_s(get_self(), get_self().value);
   configtable c = config_s.get_or_create(get_self());
   c.seeds_per_usd = seeds_per_usd;
   c.tlos_per_usd = tlos_per_usd;
   c.citizen_limit = citizen_limit;
   c.resident_limit = resident_limit;
   c.visitor_limit = visitor_limit;
   config_s.set(c, get_self());
}

ACTION seedsexchg::inshistory(uint64_t id, asset seeds_usd, time_point date)
{
   price_history_tables ph_t (get_self(), get_self().value);
   ph_t.emplace (get_self(), [&](auto &p) {
      p.id = id;
      p.seeds_usd = seeds_usd;
      p.date = date;
   });
}
