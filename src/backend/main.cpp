#include <iostream>
#include <fstream>
#include <string_view>
#include <string>
#include <vector>
#include <regex>
#include <direct.h>
#include <filesystem>

using std::string;
namespace fs = std::filesystem;

/* ========================================================================= */
static void write_includes_to_file(
/* ------------------------------------------------------------------------- */
    const std::vector<string> &includes)
{
    std::ofstream includes_file;
    includes_file.open("all_includes.txt");
    if (!includes_file.is_open())
    {
        std::cerr << "Error in open output file\n";
        return;
    }
    for (const auto &inc : includes)
    {
        includes_file << inc << "\n";
    }
    includes_file.close();
}

/* ========================================================================= */
static string read_source_file(
/* ------------------------------------------------------------------------- */
    const string &path)
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
static std::vector<string> get_all_includes_from_source(
/* ------------------------------------------------------------------------- */
    const string &source)
{
    std::vector<string> includes;
    const std::regex include_regex("#include *(\"|<)[^\n]*(\"|>)");
    std::sregex_iterator regex_it(source.begin(), source.end(), include_regex);
    const std::sregex_iterator end_it;
    while (regex_it != end_it)
    {
        includes.emplace_back(regex_it->str());
        regex_it++;
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
    std::ifstream in_file(file_path);
    std::ofstream out_file("tmp.txt");
    if (in_file.is_open())
    {
        std::string line;
        while (std::getline(in_file, line))
        {
            if (line != include_to_remove) 
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
    const string   &include,
    const fs::path &file_path)
{
    fs::path tmp_path(file_path);
    tmp_path.concat("_tmp.txt");
    fs::remove(tmp_path);
    fs::copy(file_path, tmp_path, std::filesystem::copy_options::overwrite_existing);
    remove_line_from_file(file_path, include);
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
static std::vector<string> get_useless_includes(
/* ------------------------------------------------------------------------- */
    const std::vector<string> &includes,
    fs::path                   dir_path)
{
    dir_path.append("main.cpp");
    std::vector<string> useless_includes;
    for (const auto &include : includes)
    {
        if (is_useless_include(include, dir_path)) 
        {
            useless_includes.push_back(include);
        }
    }
    return useless_includes;
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
    const string source_file_path(argc[1]);
    const auto source_content = read_source_file(source_file_path);
    const auto includes       = get_all_includes_from_source(source_content);
    const fs::path dir_path("C:\\IncludeLess\\src\\tests\\simple");
    fs::current_path(dir_path);
    if (!makefile_exists_in_dir(dir_path))
    {
        std::cerr << "No makefile found!\n";
        return -1;
    }
    write_includes_to_file(includes);
    const auto copy_path = create_copy_of_dir(dir_path);
    fs::current_path(copy_path);
    const auto useless_includes = get_useless_includes(includes, copy_path);
    std::cout << "\nYou can remove the following includes:\n";
    for (const auto &useless_include : useless_includes) 
    {
        std::cout << useless_include << "\n";
    }
    fs::current_path(dir_path); // so we can delete copy dir and return to old state.
    fs::remove_all(copy_path);
    return 0;
}