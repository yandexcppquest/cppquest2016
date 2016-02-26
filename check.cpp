#include "game.h"

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cout << "USAGE: check <dungeon.txt>" << std::endl;
        return 1;
    }

    try {
        check(argv[1], false);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what();
        return 1;
    }
}

