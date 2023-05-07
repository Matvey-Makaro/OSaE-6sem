#include "words.h"

#include <regex>
#include <fstream>
#include <iostream>

std::list<std::string> get_forbidden_words(const std::string& dict_file)
{
    std::list<std::string> words;
    std::ifstream fin(dict_file);

    std::string curr_word;
    fin >> curr_word;
    while (fin) {
        words.push_back(curr_word);
        fin >> curr_word;
    }

    fin.close();
    return words;
}

std::string replace_forbidden_words(const std::string& src, const std::list<std::string>& forbidden_words)
{
    std::string regex;
    for (const auto& word : forbidden_words)
    {
        regex += word + "|";
    }
    regex.pop_back();
//    std::cout << "Regex:" <<  regex << std::endl;
    return std::regex_replace(src, std::regex(regex, std::regex_constants::icase),"[*]");
}