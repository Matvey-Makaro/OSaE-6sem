#include "words.h"

#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cout << "INCORRECT NUMBER OF ARGUMENTS!\n";
        exit(1);
    }

    std::list<std::string> forbidden_words = get_forbidden_words(argv[1]);

    std::string curr_line;
    std::getline(std::cin, curr_line);

    while (std::cin)
    {
        std::cout << replace_forbidden_words(curr_line, forbidden_words) << "\n";

        std::getline(std::cin, curr_line);
    }
    
    return 0;
}
