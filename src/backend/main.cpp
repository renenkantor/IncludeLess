#include <iostream>
#include <fstream>
#include <string_view>
#include <string>
#include <vector>
#include <regex>
#include <filesystem>
#include "IncludeEntity.h"

using std::string;
namespace fs = std::filesystem;

/* ========================================================================= */
static std::string get_trim(
/* ------------------------------------------------------------------------- */
    const std::string& str)
{
    std::string trimmed(str);
    std::string::iterator end_pos = std::remove(trimmed.begin(), trimmed.end(), ' ');
    trimmed.erase(end_pos, trimmed.end());
    return trimmed;
}

/* ========================================================================= */
static void mark_all_duplicated_includes_except_one(
/* ------------------------------------------------------------------------- */
    std::vector<IncludeEntity> &includes)
{
    for (auto &outer_inc : includes)
    {
        if (outer_inc.m_is_useless) 
        {
            continue;
        }
        const auto &trimed_outer = get_trim(outer_inc.m_str);
        bool first_appear = false;
        for (auto &inner_inc : includes) 
        {
            const auto &trimed_inner = get_trim(inner_inc.m_str);
            if (trimed_inner != trimed_outer)
            {
                continue;
            }
            if (first_appear == false) 
            {
                first_appear = true;
            }
            else
            {
                inner_inc.m_is_useless = true;
            }
        }
    }
}


/* ========================================================================= */
static void write_includes_to_file(
/* ------------------------------------------------------------------------- */
    const std::vector<IncludeEntity> &includes, 
    const string                     &all_path,
    const string                     &useless_path,
    const string                     &useless_range_path)
{
    std::ofstream all_file;
    std::ofstream useless_file;
    std::ofstream useless_range_file;

    all_file.open(all_path);
    useless_file.open(useless_path);
    useless_range_file.open(useless_range_path);
    if (!all_file.is_open() || !useless_file.is_open() || !useless_range_file.is_open())
    {
        std::cerr << "Error in open output file\n";
        return;
    }
    bool first_line = true;
    for (const auto &inc : includes)
    {
        if (inc.m_is_useless) 
        {
            if(!first_line)
            {
                useless_range_file << "\n";
            }
            first_line = false;
            useless_file << inc.m_str << "\n";
            useless_range_file << inc.line << " " << inc.m_match.position() << " " << inc.m_match.position() + inc.m_match.length();
        }
        all_file << inc.m_str << "\n";
    }
    all_file.close();
    useless_file.close();
    useless_range_file.close();
}

/* ========================================================================= */
static string read_source_file(
/* ------------------------------------------------------------------------- */
    const fs::path &path)
{
    constexpr auto read_size = std::size_t(4096);
    std::ifstream stream(path.c_str());
    stream.exceptions(std::ios_base::badbit);
    string source_content;
    string buf(read_size, '\0');
    while (stream.read(&buf[0], read_size))
    {
        source_content.append(buf, 0, stream.gcount());
    }
    source_content.append(buf, 0, stream.gcount());
    return source_content;
}

/* ========================================================================= */
static std::vector<IncludeEntity> get_all_includes_from_source(
/* ------------------------------------------------------------------------- */
    const string &source)
{
    string changing_src(source);
    std::vector<IncludeEntity> includes;
    const std::regex include_regex(" *# *include *(\"|<)[^\n\">]+(\"|>) *(//.*)?", std::regex_constants::ECMAScript);
    std::vector<std::string> lines;
    size_t pos = 0;
    while (pos < changing_src.size()) 
    {
        size_t newline_pos = changing_src.find('\n', pos);
        if (newline_pos == std::string::npos) 
        {
            newline_pos = changing_src.size();
        }
        lines.push_back(changing_src.substr(pos, newline_pos - pos));
        pos = newline_pos + 1;
    }

    for (size_t i_line = 0; i_line < lines.size(); i_line++) 
    {
        std::smatch match;
        if (std::regex_search(lines[i_line], match, include_regex)) 
        {
            includes.emplace_back(false, match, i_line);
            changing_src = match.suffix().str();
        }
    }
    return includes;
}

/* ========================================================================= */
static bool makefile_exists_in_dir(
/* ------------------------------------------------------------------------- */
    const fs::path &dir_path)
{
    return fs::exists("GNUmakefile") ||
           fs::exists("makefile")    ||
           fs::exists("Makefile");
}

/* ========================================================================= */
static fs::path create_copy_of_dir(
/* ------------------------------------------------------------------------- */
    const fs::path &dir_path)
{
    fs::path copy_path = dir_path;
    copy_path.concat("_copy");
    fs::remove_all(copy_path);
    fs::copy(dir_path, copy_path, fs::copy_options::recursive);
    return copy_path;
}

/* ========================================================================= */
static void remove_line_from_file(
/* ------------------------------------------------------------------------- */
    const fs::path    &file_path, 
    const std::string &include_to_remove) 
{
    const auto &trimmed_inc = get_trim(include_to_remove);
    std::ifstream in_file(file_path);
    std::ofstream out_file("tmp.txt");
    if (in_file.is_open())
    {
        std::string line;
        while (std::getline(in_file, line))
        {
            const auto &trimmed_line = get_trim(line);
            if (trimmed_line != trimmed_inc) 
            {
                out_file << line << "\n";
            }
        }
    }
    in_file.close();
    out_file.close();
    fs::remove(file_path);
    fs::copy("tmp.txt", file_path, std::filesystem::copy_options::overwrite_existing);
    fs::remove("tmp.txt");
}

/* ========================================================================= */
static bool is_useless_include(
/* ------------------------------------------------------------------------- */
    const IncludeEntity &include,
    const fs::path      &file_path)
{
    fs::path tmp_path(file_path);
    tmp_path.concat("_tmp.txt");
    fs::remove(tmp_path);
    fs::copy(file_path, tmp_path, std::filesystem::copy_options::overwrite_existing);
    remove_line_from_file(file_path, include.m_str);
    int exe_res = std::system("make -s"); // -s prevents make output to cout
    if (exe_res != 0)
    {
        fs::remove(file_path);
        fs::copy(tmp_path, file_path);
        fs::remove(tmp_path);
        return false;
    }
    fs::remove(tmp_path);
    return true;
}

/* ========================================================================= */
static void mark_useless_includes(
/* ------------------------------------------------------------------------- */
    std::vector<IncludeEntity> &includes,
    fs::path                    dir_path)
{
    dir_path.append("main.cpp");
    std::vector<string> useless_includes;
    mark_all_duplicated_includes_except_one(includes);
    for (auto &include : includes)
    {
        if (include.m_is_useless)
        {
            continue;
        }
        if (is_useless_include(include, dir_path)) 
        {
            include.m_is_useless = true;
        }
    }
}

/* ========================================================================= */
int main(
/* ------------------------------------------------------------------------- */
    int    argv,
    char **argc)
{
    if (argv <= 1)
    {
        std::cerr << "Missing args!\n";
        return -1;
    }
    const fs::path source_file_path(argc[1]);
    const auto source_content = read_source_file(source_file_path);
    auto includes = get_all_includes_from_source(source_content);
    const fs::path dir_path(source_file_path.parent_path());
    fs::current_path(dir_path);
    if (!makefile_exists_in_dir(dir_path))
    {
        std::cerr << "No makefile found!\n";
        return -1;
    }
    const auto copy_path = create_copy_of_dir(dir_path);
    fs::current_path(copy_path);
    mark_useless_includes(includes, copy_path);
    std::cout << "\nYou can remove the following includes:\n";
    for (const auto &include : includes) 
    {
        if (include.m_is_useless)
        {
            std::cout << include.m_str << "\n";
        }
    }
    fs::current_path(dir_path); // so we can delete copy dir and return to old state.
    fs::remove_all(copy_path);
    write_includes_to_file(includes, "all_includes.txt", "useless_includes.txt", "ranges.txt");
    return 0;
}