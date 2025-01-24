/**
 * @class Action
 * @brief Operates on the world state.
 *
 * @date  July 2014
 * @copyright (c) 2014 Prylis Inc.. All rights reserved.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <WorldState.h>

// To support Google Test for private members
#ifndef TEST_FRIENDS
#define TEST_FRIENDS
#endif

namespace goap
{
    class Action
    {
    private:
        std::string name_ = ""; // The human-readable action name
        int cost_ = 0;          // The numeric cost of this action

        // Preconditions are things that must be satisfied before this
        // action can be taken. Only preconditions that "matter" are here.
        std::unordered_map<std::string, bool> preconditions_{};

        // Effects are things that happen when this action takes place.
        std::unordered_map<std::string, bool> effects_{};

    public:
        Action(std::unordered_map<std::string, bool> preconditions, std::unordered_map<std::string, bool> effects, int cost = 0)
        {
            cost_ = cost;
            preconditions_ = preconditions;
            effects_ = effects;
        }

        /**
         Is this action eligible to operate on the given worldstate?
         @param ws the worldstate in question
         @return true if this worldstate meets the preconditions
         */
        bool operableOn(const goap::WorldState& ws) const
        {
            for (const auto& precond : preconditions_)
            {
                try
                {
                    if (ws.vars_.at(precond.first) != precond.second)
                    {
                        return false;
                    }
                }
                catch (const std::out_of_range&)
                {
                    return false;
                }
            }
            return true;
        }

        /**
         Act on the given worldstate. Will not check for "eligiblity" and will happily
         act on whatever worldstate you provide it.
         @param the worldstate to act on
         @return a copy worldstate, with effects applied
         */
        WorldState actOn(const WorldState& ws) const
        {
            goap::WorldState tmp(ws);
            for (const auto& effect : effects_)
            {
                tmp.setVariable(effect.first, effect.second);
            }
            return tmp;
        }


        /**
         Set the given precondition variable and value.
         @param key the name of the precondition
         @param value the value the precondition must hold
         */
        void setPrecondition(const std::string& key, const bool value)
        {
            preconditions_[key] = value;
        }

        /**
         Set the given effect of this action, in terms of variable and new value.
         @param key the name of the effect
         @param value the value that will result
         */
        void setEffect(const std::string& key, const bool value)
        {
            effects_[key] = value;
        }

        int cost() const { return cost_; }

        std::string name() const { return name_; }

        TEST_FRIENDS;
    };
}
