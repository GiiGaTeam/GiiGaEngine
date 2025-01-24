/**
 * @class WorldState
 * @brief A way of describing the "world" at any point in time.
 *
 * @date  July 2014
 * @copyright (c) 2014 Prylis Inc.. All rights reserved.
 */

#pragma once

#include <ostream>
#include <string>
#include <map>

namespace goap
{
    struct WorldState
    {
        float priority_ = 0;                           // useful if this is a goal state, to distinguish from other possible goals
        std::string name_ = "";                        // the human-readable name of the state
        std::unordered_map<std::string, bool> vars_{}; // the variables that in aggregate describe a worldstate

        WorldState() = default;
        
        WorldState(const std::unordered_map<std::string, bool>& vars)
        {
            vars_ = vars;
        }

        /**
         Set a world state variable, e.g. "gunLoaded" / true
         @param var_id the unique ID of the state variable
         @param value the boolean value of the variable
         */
        void setVariable(const std::string& var_id, const bool value)
        {
            vars_[var_id] = value;
        }

        /**
         Retrieve the current value of the given variable.
         @param var_id the unique ID of the state variable
         @return the value of the variable
        */
        bool getVariable(const std::string& var_id) const
        {
            return vars_.at(var_id);
        }

        bool hasKey(const std::string& key)
        {
            return vars_.contains(key);
        }

        /**
         Useful if this state is a goal state. It asks, does state 'other'
         meet the requirements of this goal? Takes into account not only this goal's
         state variables, but which variables matter to this goal state.
         @param other the state you are testing as having met this goal state
         @return true if it meets this goal state, false otherwise
         */
        bool meetsGoal(const WorldState& goal_state) const
        {
            for (const auto& kv : goal_state.vars_)
            {
                try
                {
                    if (vars_.at(kv.first) != kv.second)
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
         Given the other state -- and what 'matters' to the other state -- how many
         of our state variables differ from the other?
         @param other the goal state to compare against
         @return the number of state-var differences between us and them
         */
        int distanceTo(const WorldState& goal_state) const
        {
            int result = 0;

            for (const auto& kv : goal_state.vars_)
            {
                auto itr = vars_.find(kv.first);
                if (itr == end(vars_) || itr->second != kv.second)
                {
                    ++result;
                }
            }

            return result;
        }

        /**
         Equality operator
         @param other the other worldstate to compare to
         @return true if they are equal, false if not
         */
        bool operator==(const WorldState& other) const
        {
            return (vars_ == other.vars_);
        }

        // A friend function of a class is defined outside that class' scope but it has the
        // right to access all private and protected members of the class. Even though the
        // prototypes for friend functions appear in the class definition, friends are not
        // member functions.
        friend std::ostream& operator<<(std::ostream& out, const WorldState& n);
    };

    inline std::ostream& operator<<(std::ostream& out, const WorldState& n)
    {
        out << "WorldState { ";
        for (const auto& kv : n.vars_)
        {
            out << kv.second << " ";
        }
        out << "}";
        return out;
    }
}
