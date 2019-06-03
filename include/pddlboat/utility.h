/* Author: Zachary Kingston */

#ifndef PDDLBOAT_UTILITY_
#define PDDLBOAT_UTILITY_

#include <iostream>
#include <optional>
#include <cstdlib>  // for std::getenv
#include <memory>   // for std::shared_ptr
#include <array>    // for std::array

#include <boost/filesystem.hpp>  // for filesystem paths
#include <boost/format.hpp>      // for format

namespace pddlboat
{
    namespace log
    {
        std::string formatRecurse(boost::format &f);

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

    namespace io
    {
        boost::filesystem::path expandHome(const boost::filesystem::path &in);
        boost::filesystem::path expandSymlinks(const boost::filesystem::path &in);
        boost::filesystem::path expandPath(const boost::filesystem::path &in);
        bool isPrefix(const std::string &lhs, const std::string &rhs);
        bool isSuffix(const std::string &lhs, const std::string &rhs);
        bool isExtension(const std::string &path_string, const std::string &extension);
        std::string resolvePath(const std::string &path);
        std::string loadFileToString(const std::string &path);
        std::optional<std::string> runCommand(const std::string &cmd);
        void createFile(std::ofstream &out, const std::string &file);
        void deleteFile(const std::string &file);
        std::pair<bool, std::vector<std::string>> listDirectory(const std::string &directory);
        std::string generateUUID();
    }  // namespace io
}  // namespace pddlboat

#endif
