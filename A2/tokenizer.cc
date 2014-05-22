#include <cstring>
#include <sstream>
#include <iostream>
#include <cstring>
#include <vector>
#include <cstddef>

std::vector<std::string> tokenize(std::string input) {
    std::stringstream ss;
    ss << input;
    std::string temp;

    std::vector<std::string> vs;
    while (getline(ss, temp, ' ')) {
        if (temp != "") {
            vs.push_back(temp);
        }
    }

    return vs;
}
