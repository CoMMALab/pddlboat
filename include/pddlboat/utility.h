/* Author: Zachary Kingston */

#ifndef PDDLBOAT_UTILITY_
#define PDDLBOAT_UTILITY_

#include <iostream>
#include <cstdlib>  // for std::getenv
#include <memory>   // for std::shared_ptr
#include <array>    // for std::array

#include <boost/filesystem.hpp>  // for filesystem paths
#include <boost/format.hpp>      // for format

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

    namespace io
    {
        boost::filesystem::path expandHome(const boost::filesystem::path &in)
        {
            const char *home = std::getenv("HOME");
            if (home == nullptr)
            {
                std::cerr << "HOME Environment variable is not set! Cannot resolve ~ in path." << std::endl;
                return in;
            }

            boost::filesystem::path out;
            for (const auto &p : in)
                out /= (p.string() == "~") ? home : p;

            return out;
        }

        boost::filesystem::path expandSymlinks(const boost::filesystem::path &in)
        {
            boost::filesystem::path out;
            for (const auto &p : in)
            {
                auto tmp = out / p;
                if (boost::filesystem::is_symlink(tmp))
                    out = boost::filesystem::read_symlink(tmp);
                else
                    out /= p;
            }

            return out;
        }

        boost::filesystem::path expandPath(const boost::filesystem::path &in)
        {
            boost::filesystem::path out = in;
            out = expandHome(out);
            out = expandSymlinks(out);

            return boost::filesystem::absolute(out);
        }

        // is lhs a prefix of rhs?
        bool isPrefix(const std::string &lhs, const std::string &rhs)
        {
            return std::equal(lhs.begin(), lhs.begin() + std::min(lhs.size(), rhs.size()), rhs.begin());
        }

        // is lhs a suffix? of rhs?
        bool isSuffix(const std::string &lhs, const std::string &rhs)
        {
            return std::equal(lhs.rbegin(), lhs.rbegin() + std::min(lhs.size(), rhs.size()), rhs.rbegin());
        }

        bool isExtension(const std::string &path_string, const std::string &extension)
        {
            boost::filesystem::path path(path_string);
            const std::string last = boost::filesystem::extension(path);
            return isSuffix(extension, last);
        }

        const std::string resolvePath(const std::string &path)
        {
            boost::filesystem::path file(path);

            if (!boost::filesystem::exists(file))
            {
                std::cerr << log::format("File `%1%` does not exist.", path) << std::endl;
                return "";
            }

            return boost::filesystem::canonical(boost::filesystem::absolute(file)).string();
        }

        const std::string loadFileToString(const std::string &path)
        {
            const std::string full_path = resolvePath(path);
            if (full_path.empty())
                return "";

            std::ifstream ifs(full_path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

            std::ifstream::pos_type size = ifs.tellg();
            ifs.seekg(0, std::ios::beg);

            std::vector<char> bytes(size);
            ifs.read(bytes.data(), size);

            return std::string(bytes.data(), size);
        }

        const std::string runCommand(const std::string &cmd)
        {
            std::array<char, 128> buffer;
            std::string result;
            std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
            if (!pipe)
            {
                std::cerr << log::format("Failed to run command `%1%`!", cmd) << std::endl;
                return "";
            }

            while (!feof(pipe.get()))
            {
                if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
                    result += buffer.data();
            }

            return result;
        }

        void createFile(std::ofstream &out, const std::string &file)
        {
            boost::filesystem::path path(file);
            path = expandHome(path);
            path = expandSymlinks(path);

            const auto parent = path.parent_path().string();

            if (!parent.empty())
                boost::filesystem::create_directories(parent);

            out.open(path.string(), std::ofstream::out | std::ofstream::trunc);
        }

        const std::pair<bool, std::vector<std::string>> listDirectory(const std::string &directory)
        {
            std::vector<std::string> contents;

            const std::string full_path = resolvePath(directory);
            if (full_path.empty())
                return std::make_pair(false, contents);

            boost::filesystem::path path(full_path);
            if (!boost::filesystem::is_directory(path))
                return std::make_pair(false, contents);

            for (auto it = boost::filesystem::directory_iterator(path);
                 it != boost::filesystem::directory_iterator(); ++it)
                contents.emplace_back(expandPath(it->path()).string());

            return std::make_pair(true, contents);
        }
    }  // namespace io
}  // namespace pddlboat

#endif
