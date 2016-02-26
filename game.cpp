#include "game.h"

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cout << "USAGE: game <dungeon.txt>" << std::endl;
        return 1;
    }

    try {
        game(argv[1]);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what();
        return 1;
    }
}
