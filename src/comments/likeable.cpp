#include <comments/likeable.hpp>
#include <comments/reaction.hpp>
#include <dao.hpp>

namespace hypha
{
    Likeable::~Likeable()
    {
    }

    const void Likeable::like(name user, name type)
    {
        TRACE_FUNCTION()
        Reaction::getReaction(this->getDao(), *this, type).react(user);
    }

    const void Likeable::unlike(name user)
    {
        TRACE_FUNCTION()
        Reaction::getReactionByUser(this->getDao(), *this, user).unreact(user);
    }
}
