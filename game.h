#ifndef __GAME_H__
#define __GAME_H__

#include <cassert>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <bitset>
#include <cstdio>
#include <queue>

#ifdef _WIN32
#include <Windows.h>
#include <conio.h>
#undef max
#undef min
#else
#include <curses.h>
#endif


#ifdef _WIN32
void InitScr() {
    return;
}

void PutXY(short x, short y, short fg, short bg, const std::string& s) {
    DWORD dummy;
    COORD c{ x, y };

    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    FillConsoleOutputAttribute(h, bg * 16 + fg, s.size(), c, &dummy);
    WriteConsoleOutputCharacterA(h, s.data(), s.size(), c, &dummy);
}

void GotoXY(short x, short y) {
    COORD c{ x, y };
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorPosition(h, c);
}

int ReadKey() {
    return _getch();
}
#define ERR 0

#define HIGHLIGHT_COLOR 1

#else
void InitScr() {
    initscr();
    noecho();
    cbreak();
    start_color();
}

int ColorToCursesColor(short color) {
    switch (color) {
    case 0: return COLOR_BLACK;
    case 1: return COLOR_BLUE;
    case 2: return COLOR_GREEN;
    case 3: return COLOR_CYAN;
    case 4: return COLOR_RED;
    case 5: return COLOR_MAGENTA;
    case 6: return COLOR_YELLOW;
    case 7: return COLOR_WHITE;
    case 8: return COLOR_WHITE;
    case 9: return COLOR_BLUE;
    case 10: return COLOR_GREEN;
    case 11: return COLOR_CYAN;
    case 12: return COLOR_RED;
    case 13: return COLOR_MAGENTA;
    case 14: return COLOR_YELLOW;
    case 15: return COLOR_WHITE;
    default: return COLOR_BLACK;
    }
}

void PutXY(short x, short y, short fg, short bg, const std::string& s) {
    fg = ColorToCursesColor(fg);
    bg = ColorToCursesColor(bg);

    static std::unordered_map<int, int> idByKey;
    int key = fg + (bg << 4);
    int id;

    auto pos = idByKey.find(key);
    if(pos == idByKey.end()) {
        id = idByKey.size() + 1;
        idByKey.insert({key, id});
        init_pair(id, fg, bg);
    } else {
        id = pos->second;
    }

    attron(COLOR_PAIR(id));
    mvaddstr(y, x, s.data());
    attroff(COLOR_PAIR(id));
}

void GotoXY(short x, short y) {
    move(y, x);
}

int ReadKey() {
    return getch();
}

#define HIGHLIGHT_COLOR 2

#endif

using TDungeon = std::vector<std::string>;
using TCell = std::pair<size_t, size_t>;


const char PLAYER = '@';
const char WALL = '#';
const char SLEEPING_MONSTER = 'M';
const char MONSTER = '!';
const char GOLD = '$';
const char EMPTY = '.';
const char EXIT = '<';

const size_t WAKE_UP_DIST = 5;
const size_t MAX_HP = 100;
const size_t HP_LOST = 20;


TCell Up(const TCell& cell) {
    return TCell(cell.first - 1, cell.second);
}

TCell Down(const TCell& cell) {
    return TCell(cell.first + 1, cell.second);
}

TCell Left(const TCell& cell) {
    return TCell(cell.first, cell.second - 1);
}

TCell Right(const TCell& cell) {
    return TCell(cell.first, cell.second + 1);
}

TDungeon ReadDungeon(const std::string& name) {
    std::ifstream input(name);

    TDungeon dungeon;

    std::string line;
    while (getline(input, line)) {
        if(!line.empty() && line.back() == '\r')
            line.pop_back();
        dungeon.push_back(line);
    }

    for (size_t i = 0; i < dungeon.size(); ++i)
        if(dungeon[i].size() != dungeon[0].size())
            throw std::runtime_error("Invalid dungeon format.");

    return dungeon;
}

template <>
struct std::hash<TCell> {
    size_t operator()(const TCell& c) const {
        return hash<size_t>()(c.first) * 6221 + hash<size_t>()(c.second);
    }
};

template <>
struct std::hash<std::pair<TCell, TCell> > {
    size_t operator()(const pair<TCell, TCell>& c) const {
        return hash<TCell>()(c.first) * 7741 + hash<TCell>()(c.second);
    }
};


class TPreCalc {
public:
    TPreCalc(const TDungeon& dungeon)
        : DungeonMask(dungeon) {
        Walls.assign(dungeon.size(), std::vector<bool>(dungeon[0].size(), false));
        for (size_t i = 0; i < dungeon.size(); ++i) {
            for (size_t j = 0; j < dungeon[i].size(); ++j) {
                if (dungeon[i][j] == EXIT) {
                    Exit = TCell(i, j);
                }

                if (dungeon[i][j] == WALL) {
                    Walls[i][j] = true;
                } else {
                    Cells.push_back(TCell(i, j));
                    DungeonMask[i][j] = EMPTY;
                }
            }
        }

        DungeonMask[Exit.first][Exit.second] = EXIT;

        Distances.reserve(Cells.size() * Cells.size());
        for (size_t i = 0; i < Cells.size(); ++i) {
            Distances[std::make_pair(Cells[i], Cells[i])] = 0;

            if (!IsWall(Up(Cells[i]))) {
                Distances[std::make_pair(Cells[i], Up(Cells[i]))] = 1;
            }
            if (!IsWall(Down(Cells[i]))) {
                Distances[std::make_pair(Cells[i], Down(Cells[i]))] = 1;
            }
            if (!IsWall(Left(Cells[i]))) {
                Distances[std::make_pair(Cells[i], Left(Cells[i]))] = 1;
            }
            if (!IsWall(Right(Cells[i]))) {
                Distances[std::make_pair(Cells[i], Right(Cells[i]))] = 1;
            }
        }

        std::cout << "Empty cells: " << Cells.size() << std::endl;
        for (size_t k = 0; k < Cells.size(); ++k) {
            std::cout << "Preparing: " << k * 100 / Cells.size() << "%" << "\r" << std::flush;
            for (size_t i = 0; i < Cells.size(); ++i) {
                auto ik = make_pair(Cells[i], Cells[k]);
                for (size_t j = i + 1; j < Cells.size(); ++j) {
                    auto ikIt = Distances.find(ik);
                    auto kjIt = Distances.find(make_pair(Cells[k], Cells[j]));
                    if (ikIt != Distances.end() && kjIt != Distances.end()) {
                        auto it = Distances.find(make_pair(Cells[i], Cells[j]));
                        if (it == Distances.end() || it->second > ikIt->second + kjIt->second) {
                            Distances[make_pair(Cells[i], Cells[j])] = ikIt->second + kjIt->second;
                            Distances[make_pair(Cells[j], Cells[i])] = ikIt->second + kjIt->second;
                        }
                    }
                }
            }
        }
    }

    size_t GetDistance(const TCell& start, const TCell& finish) const {
        auto it = Distances.find(make_pair(start, finish));
        assert(it != Distances.end());
        return it->second;
    }

    size_t GetDistanceSafe(const TCell& start, const TCell& finish) const {
        auto it = Distances.find(make_pair(start, finish));
        if (it == Distances.end())
            return std::numeric_limits<size_t>::max();
        return it->second;
    }

    size_t GetExitDistance(const TCell& cell) const {
        return GetDistance(cell, Exit);
    }

    bool IsWall(const TCell& cell) const {
        return Walls[cell.first][cell.second];
    }

    TCell GetNextCellOnPath(const TCell& start, const TCell& finish) const {
        size_t dist = GetDistance(start, finish);

        TCell cell = Up(start);
        if (!IsWall(cell) && dist == GetDistance(cell, finish) + 1) {
            return cell;
        }
        cell = Right(start);
        if (!IsWall(cell) && dist == GetDistance(cell, finish) + 1) {
            return cell;
        }
        cell = Down(start);
        if (!IsWall(cell) && dist == GetDistance(cell, finish) + 1) {
            return cell;
        }
        cell = Left(start);
        if (!IsWall(cell) && dist == GetDistance(cell, finish) + 1) {
            return cell;
        }
        assert(false);
        return start;
    }

    TCell GetExitCell() const {
        return Exit;
    }

    const TDungeon& GetDungeonMask() const {
        return DungeonMask;
    }

private:
    TDungeon DungeonMask;
    TCell Exit;
    std::vector<std::vector<bool> > Walls;
    std::vector<TCell> Cells;
    std::unordered_map<std::pair<TCell, TCell>, size_t> Distances;
};

class TState {
public:
    TState(const TPreCalc* precalc, const TDungeon& dungeon)
        : Precalc(precalc)
        , Position(precalc->GetExitCell())
        , Gold(0)
        , HP(MAX_HP)
    {
        for (size_t i = 0; i < dungeon.size(); ++i) {
            for (size_t j = 0; j < dungeon[i].size(); ++j) {
                TCell cell(i, j);
                if (dungeon[i][j] == SLEEPING_MONSTER || dungeon[i][j] == MONSTER) {
                    size_t dist = Precalc->GetDistance(Position, cell);
                    if (dist <= WAKE_UP_DIST) {
                        ChasingMonsters[cell] = 1;
                    } else {
                        Monsters[cell] = 1;
                    }
                }
            }
        }
        for (size_t i = 0; i < dungeon.size(); ++i) {
            for (size_t j = 0; j < dungeon[i].size(); ++j) {
                TCell cell(i, j);
                if (dungeon[i][j] == GOLD) {
                    GoldPositions.insert(cell);
                }
            }
        }
    }

    bool IsWinState() const {
        return Precalc->GetExitDistance(Position) == 0 && GoldPositions.empty();
    }

    std::vector<TState> GetNextStates() const {
        std::vector<TState> res;

        TState nextState = *this;
        if (TryMove(Precalc, Up(Position), nextState)) {
            nextState.Path += "U";
            res.push_back(nextState);
        }
        nextState = *this;
        if (TryMove(Precalc, Right(Position), nextState)) {
            nextState.Path += "R";
            res.push_back(nextState);
        }
        nextState = *this;
        if (TryMove(Precalc, Down(Position), nextState)) {
            nextState.Path += "D";
            res.push_back(nextState);
        }
        nextState = *this;
        if (TryMove(Precalc, Left(Position), nextState)) {
            nextState.Path += "L";
            res.push_back(nextState);
        }
        std::random_shuffle(res.begin(), res.end());
        return res;
    }

    void Print() const {
        TDungeon dungeon = Precalc->GetDungeonMask();

        dungeon[Position.first][Position.second] = PLAYER;

        for (auto it : GoldPositions) {
            dungeon[it.first][it.second] = GOLD;
        }

        for (auto it : Monsters) {
            dungeon[it.first.first][it.first.second] = SLEEPING_MONSTER;
        }

        for (auto it : ChasingMonsters) {
            char c = '0' + it.second;
            if (it.second >= 10) {
                c = 'a' + it.second - 10;
            }
            dungeon[it.first.first][it.first.second] = c;
        }

        for (size_t y = 0; y < dungeon.size(); ++y) {
            for (size_t x = 0; x < dungeon[y].size(); x++) {
                bool lit = Precalc->GetDistanceSafe(Position, { y, x }) <= WAKE_UP_DIST;

                char c = dungeon[y][x];
                short fg;
                if (c == SLEEPING_MONSTER) {
                    fg = 0x4;
                } else if (c == PLAYER) {
                    fg = 0xf;
                } else if (c == GOLD) {
                    fg = 0xe;
                } else if (c == WALL) {
                    fg = 0x8;
                } else if (c == EMPTY) {
                    fg = 0x7;
                } else if (c == EXIT) {
                    fg = 0x8;
                } else {
                    fg = 0xc;
                }

                PutXY(x, y, fg, lit ? HIGHLIGHT_COLOR : 0, std::string(1, dungeon[y][x]));
            }
        }

        std::stringstream s0, s1;
        s0 << "HP: " << HP << "   ";
        s1 << "Gold: " << Gold << "   ";
        PutXY(0, dungeon.size(),     0xf, 0, s0.str());
        PutXY(0, dungeon.size() + 1, 0xf, 0, s1.str());

        PutXY(40, dungeon.size(),    0xf, 0, "Move with WASD ");
    }

    const std::string& GetPath() const {
        return Path;
    }

    static bool TryMove(const TPreCalc* precalc, const TCell& pos, TState& next, bool* dead = nullptr) {
        bool inFight = false;
        { // move player
            next.Position = pos;
            if (precalc->IsWall(next.Position)) {
                if (dead) {
                    *dead = false;
                }
                return false;
            }
        }
        { // check monster attack
            auto monsterIt = next.ChasingMonsters.find(next.Position);
            if (monsterIt != next.ChasingMonsters.end()) {
                next.HP -= HP_LOST * monsterIt->second;
                inFight = true;
                if (next.HP <= 0) {
                    if (dead) {
                        *dead = true;
                    }
                    return false;
                }
                next.ChasingMonsters.erase(monsterIt);
            }
        }
        { // check gold
            auto pos = next.GoldPositions.find(next.Position);
            if(pos != next.GoldPositions.end()) {
                ++next.Gold;
                next.GoldPositions.erase(pos);
            }
        }
        { // move chasing monsters
            std::unordered_map<TCell, size_t> chasingMonsters;
            for (auto it : next.ChasingMonsters) {
                TCell nextCell = precalc->GetNextCellOnPath(it.first, next.Position);
                if (nextCell == next.Position) {
                    next.HP -= HP_LOST * it.second;
                    inFight = true;
                    if (next.HP <= 0) {
                        if (dead) {
                            *dead = true;
                        }
                        return false;
                    }
                } else {
                    chasingMonsters[nextCell] += it.second;
                }
            }
            next.ChasingMonsters.swap(chasingMonsters);
        }
        { // wake up monsters
            std::vector<TCell> toDelete;
            for (auto it : next.Monsters) {
                if (precalc->GetDistance(it.first, next.Position) <= WAKE_UP_DIST) {
                    next.ChasingMonsters[it.first] += it.second;
                    toDelete.push_back(it.first);
                }
            }
            for (const TCell& c : toDelete) {
                next.Monsters.erase(c);
            }
        }
        if (!inFight) { // heal player
            next.HP = std::min(next.HP + 1, (int)MAX_HP);
        }

        if (dead) {
            *dead = next.HP <= 0;
        }
        return next.HP > 0;
    }

    const TCell& GetPosition() const {
        return Position;
    }

    bool operator==(const TState& other) const {
        return Position == other.Position
            && Gold == other.Gold
            && HP == other.HP
            && Monsters == other.Monsters
            && ChasingMonsters == other.ChasingMonsters
            && GoldPositions == other.GoldPositions;
    }

private:
    const TPreCalc* Precalc;

    TCell Position;
    size_t Gold;
    int HP;
    std::set<TCell> GoldPositions;
    std::unordered_map<TCell, size_t> Monsters;
    std::unordered_map<TCell, size_t> ChasingMonsters;
    std::string Path;
};

void PlayPath(const TDungeon& dungeon, const std::string& path, bool interactive) {
    TPreCalc precalc(dungeon);
    TState state(&precalc, dungeon);

    if (interactive) {
        state.Print();
        ReadKey();
    }

    for (char c : path) {
        bool dead = 0;

        if (c == 'U') {
            TState::TryMove(&precalc, Up(state.GetPosition()), state);
        } else if (c == 'D') {
            TState::TryMove(&precalc, Down(state.GetPosition()), state);
        } else if (c == 'L') {
            TState::TryMove(&precalc, Left(state.GetPosition()), state);
        } else if (c == 'R') {
            TState::TryMove(&precalc, Right(state.GetPosition()), state);
        }

        if (interactive) {
            state.Print();
            ReadKey();
        }

        if (dead) {
            std::cout << "YOU DIED" << std::endl;
            return;
        }
        if (state.IsWinState()) {
            std::cout << "YOU WON" << std::endl;
            return;
        }
    }

    std::cout << "YOU COULD NOT LEAVE THE DUNGEON" << std::endl;
}

void Play(const TDungeon& dungeon) {
    TPreCalc precalc(dungeon);

    TState state(&precalc, dungeon);
    state.Print();

    size_t moves = 0;
    while (true) {
        bool dead = false;

        int key = ReadKey();
        if(key == ERR)
            continue;

        if (key == 'w' || key == 'W') {
            TState next = state;
            if (TState::TryMove(&precalc, Up(state.GetPosition()), next, &dead)) {
                state = next;
                ++moves;
            }
        } else if (key == 's' || key == 'S') {
            TState next = state;
            if (TState::TryMove(&precalc, Down(state.GetPosition()), next, &dead)) {
                state = next;
                ++moves;
            }
        } else if (key == 'a' || key == 'A') {
            TState next = state;
            if (TState::TryMove(&precalc, Left(state.GetPosition()), next, &dead)) {
                state = next;
                ++moves;
            }
        } else if (key == 'd' || key == 'D') {
            TState next = state;
            if (TState::TryMove(&precalc, Right(state.GetPosition()), next, &dead)) {
                state = next;
                ++moves;
            }
        }

        state.Print();
        if (dead) {
            std::cout << "YOU DIED" << std::endl;
            return;
        }
        if (state.IsWinState()) {
            std::cout << "YOU WON" << std::endl;
            std::cout << "MOVES: " << moves << std::endl;
            return;
        }
    }
}

void game(const std::string& file) {
    InitScr();

    TDungeon dungeon = ReadDungeon(file);
    Play(dungeon);
}

void check(const std::string& file, bool interactive) {
    TDungeon dungeon = ReadDungeon(file);

    std::string solution;
    getline(std::cin, solution);

    if(interactive)
        InitScr();

    PlayPath(dungeon, solution, interactive);
}

#endif
