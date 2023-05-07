#ifndef LAB4_WORDS_H
#define LAB4_WORDS_H

#include <list>
#include <string>

std::list<std::string> get_forbidden_words(const std::string& dict_file);
std::string replace_forbidden_words(const std::string& src, const std::list<std::string>& forbidden_words);

#endif //LAB4_WORDS_H
