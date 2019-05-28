/* Author: Zachary Kingston */

#ifndef PDDLBOAT_LOG_
#define PDDLBOAT_LOG_

#include <boost/format.hpp>

namespace pddlboat
{
    namespace log
    {
        std::string formatRecurse(boost::format &f)
        {
            return f.str();
        }

        template <class T, class... Args>
        std::string formatRecurse(boost::format &f, T &&t, Args &&... args)
        {
            return formatRecurse(f % std::forward<T>(t), std::forward<Args>(args)...);
        }

        template <typename... Args>
        std::string format(const std::string &fmt, Args &&... args)
        {
            boost::format f(fmt);
            return formatRecurse(f, std::forward<Args>(args)...);
        }
    }  // namespace log
}  // namespace pddlboat

#endif
