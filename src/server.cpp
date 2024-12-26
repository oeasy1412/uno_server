#include "../thirdparty/httplib.h"
#include "cs_uno.h"

#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

bool mode = false; // 膀胱模式 :)

std::mutex g_mutex;

int max_player;      // 玩家总数（动态设置不用const）
int max_rob;         // 人机总数
int cnt_com;         // 读取com_str时的计数上限为玩家总数
std::string p1_name; // 玩家昵称
std::string p2_name; //
std::string p3_name; //
std::string p4_name; //
std::string p1;      // 返回玩家的字符串
std::string p2;      //
std::string p3;      //
std::string p4;      //
bool isStart = 0;    // 记录游戏是否开始

std::vector<std::string> warehouse;                    // 牌堆
std::vector<std::string> usedCard;                     // 弃牌堆
std::vector<std::vector<std::string>> playersHands(4); // 手牌
std::string lastCard;
int i = 0;     // 当前玩家i，0<=i<=3
int idx = 0;   // 卡牌下标
int color = 0; // 颜色
int win = -1;
int Reverse = 1;
bool isSkip = false;
bool isAddTwo = false;
bool isAdd4 = false;
int addNum = 0;
bool hadChanged = false;
int playerColor = -1;
std::string first;
std::string tmp = "";           // 寄存器
std::string com_str = "";       //
std::string turn_str = "";      //
std::vector<bool> get_com_bool; // 记录玩家是否获取人机出牌信息
std::vector<int> countWin = {0, 0, 0, 0};
bool haveUpdate = false;
// 一些映射表
std::unordered_map<char, std::string> ToString = {
    {'0', "红"},
    {'1', "绿"},
    {'2', "蓝"},
    {'3', "黄"},
    {'4', "Black"},
    {'5', "Skip"},
    {'6', "AddTwo"},
    {'7', "Reverse"},
    {'8', "BlackC"},
    {'9', "Black4"},
    {'n', "Number"},
};
std::map<int, std::string> ToPlayer = {};
std::map<std::string, bool> IsPlayer = {};

using namespace std;
// 生成牌堆
void generateCard(vector<string>& warehouse) {
    // 标准UNO 108张
    warehouse.reserve(108);
    for (int i = 0; i < 4; i++) {
        for (int j = 1; j <= 9; j++) {
            warehouse.emplace_back(to_string(i) + 'n' + to_string(j));
            warehouse.emplace_back(to_string(i) + 'n' + to_string(j));
        }
        warehouse.emplace_back(to_string(i) + 'n' + to_string(0));
    }
    for (int i = 0; i < 4; i++) {
        warehouse.emplace_back(to_string(i) + '5' + 's');
        warehouse.emplace_back(to_string(i) + '5' + 's');
        warehouse.emplace_back(to_string(i) + '6' + 'a');
        warehouse.emplace_back(to_string(i) + '6' + 'a');
        warehouse.emplace_back(to_string(i) + '7' + 'r');
        warehouse.emplace_back(to_string(i) + '7' + 'r');
        warehouse.emplace_back(to_string(4) + '8' + 'b');
        warehouse.emplace_back(to_string(4) + '9' + 'b');
    }
    // 洗牌
    mt19937 gen(time(nullptr));
    shuffle(warehouse.begin(), warehouse.end(), gen);
}

// 随机座位（未上线）
void toRandom(map<int, string>& ToPlayer,
              map<string, bool>& IsPlayer,
              const int max_player) {
    //     vector<int> r = {0, 1, 2, 3};
    //     mt19937 gen(time(nullptr));
    //     shuffle(r.begin(), r.end(), gen);
    //     for (int i = 0; i < max_player; i++) {
    //         ToPlayer.insert({r[i], "Player" + to_string(i)});
    //     }
    //     for (int i = max_player; i < 4; i++) {
    //         ToPlayer.insert({r[i], "Computer" + to_string(i - max_player)});
    //     }
    for (int i = 0; i < max_player; i++) {
        ToPlayer.insert({i, "Player" + to_string(i + 1)});
        IsPlayer.insert({"Player" + to_string(i + 1), 1});
    }
    for (int i = max_player; i < 4; i++) {
        ToPlayer.insert({i, "Computer" + to_string(i - max_player + 1)});
        IsPlayer.insert({"Computer" + to_string(i - max_player + 1), 0});
    }
}

// 判断是否为玩家
bool isPlayer(const int i) { return IsPlayer[ToPlayer[i]]; }

// 初始化手牌
void startCard(vector<string>& warehouse,
               vector<vector<string>>& playersHands) {
    const int startNum = 7;
    for (int i = 0; i < 4; i++) {
        playersHands[i].reserve(14);
        for (int j = 0; j < startNum; j++) {
            playersHands[i].emplace_back(warehouse.back());
            warehouse.pop_back();
        }
    }
}

// 获得非功能牌的第一张牌
void startGame(vector<string>& warehouse,
               string& first,
               vector<string>& usedCard) {
    while (first[1] != 'n') {
        usedCard.emplace_back(first);
        warehouse.pop_back();
        first = warehouse.back();
    }
}

// 牌堆为空时将弃牌堆更新为新牌堆
void washedCard(vector<string>& usedCard) {
    mt19937 gen(time(nullptr));
    shuffle(usedCard.begin(), usedCard.end(), gen);
}

void again(vector<string>& warehouse,
           vector<string>& usedCard,
           vector<vector<string>>& playersHands,
           string& first,
           string& lastCard) {
    // 清空上局数据
    warehouse.clear();
    usedCard.clear();
    for (int j = 0; j < 4; j++) {
        playersHands[j].clear();
    }
    for (int j = 0; j < get_com_bool.size(); j++) {
        get_com_bool[j] = false;
    }
    haveUpdate = false;
    Reverse = 1;
    isSkip = false;
    isAddTwo = false;
    isAdd4 = false;
    addNum = 0;
    hadChanged = false;

    // 生成牌堆
    generateCard(warehouse);
    // 108张空间
    usedCard.reserve(108);
    // 发牌
    startCard(warehouse, playersHands);
    // 获取第一张非功能牌
    first = warehouse.back();
    startGame(warehouse, first, usedCard);
    lastCard = first;

    if (max_player == 2) {
        com_str += '\n' + p1_name + " is Player1.\n" + p2_name +
                   " is Player2.\n" + "\nThe frist card is: \n    " +
                   ToString[first[0]] + " " + ToString[first[1]] + " " +
                   first[2] + "\n";
    } else if (max_player == 3) {
        com_str += '\n' + p1_name + " is Player1.\n" + p2_name +
                   " is Player2.\n" + p3_name + " is Player3.\n" +
                   "\nThe frist card is: \n    " + ToString[first[0]] + " " +
                   ToString[first[1]] + " " + first[2] + "\n";
    } else if (max_player == 4) {
        com_str += '\n' + p1_name + " is Player1.\n" + p2_name +
                   " is Player2.\n" + p3_name + " is Player3.\n" + p4_name +
                   " is Player4.\n" + "\nThe frist card is: \n    " +
                   ToString[first[0]] + " " + ToString[first[1]] + " " +
                   first[2] + "\n";
    } else {
        com_str += '\n' + p1_name + " is Player1.\n" +
                   "\nThe frist card is: \n    " + ToString[first[0]] + " " +
                   ToString[first[1]] + " " + first[2] + "\n";
    }
}

// 展示当前玩家i的手牌
void getShow(string& ret, vector<vector<string>>& playersHands, const int i) {
    ret.clear();
    for (int j = 0; j < playersHands[i].size(); j++) {
        ret += playersHands[i][j];
    }
}

// 到下家行动
void toNext(int& i, const int Reverse) {
    i += Reverse;
    if (i == 4) {
        i = 0;
    }
    if (i == -1) {
        i = 3;
    }
}

int getNext(int i, const int Reverse) {
    i += Reverse;
    if (i == 4) {
        return 0;
    }
    if (i == -1) {
        return 3;
    }
    return i;
}

// 换色
void changeColor(int playerColor,
                 string& lastCard,
                 vector<vector<string>>& playersHands,
                 const int i,
                 string& ret) {
    if (isPlayer(i)) {
        if (playerColor < 0 || playerColor > 3) {
            playerColor = 3;
        }
    } else {
        // 人机换色为手牌第一张的颜色
        if (playersHands[i].begin() != playersHands[i].end() &&
            playersHands[i][0][0] != '4') {
            playerColor = (int)playersHands[i][0][0] - '0';
        } else {
            playerColor = 3;
        }
    }
    // 换色
    lastCard[0] = '0' + playerColor;
    ret +=
        ToPlayer[i] + ":   change color into " + ToString[lastCard[0]] + '\n';
}

// 人机被加
void computer_Add(const char T,
                  bool& boolOfAdd,
                  string& lastCard,
                  int& addNum,
                  vector<vector<string>>& playersHands,
                  vector<string>& warehouse,
                  const int i,
                  string& ret) {
    bool have = false; // 是否有加
    int t = 0;         // 偏移计数器
    for (string card : playersHands[i]) {
        if (card[1] == T) {
            have = true;
            lastCard = card;
            ret += ToPlayer[i] + ":  play " + ToString[lastCard[0]] + " " +
                   ToString[lastCard[1]] + "   剩余：" +
                   to_string(playersHands[i].size()) + '\n';
            if (T == '6') {
                addNum += 2;
            } else {
                addNum += 4;
                changeColor(-1, lastCard, playersHands, i, ret);
            }
            ret += ToPlayer[i] + ":  play Add!Now Add is " + to_string(addNum) +
                   " !\n";
            // erase()
            auto it = playersHands[i].begin();
            it += t;
            playersHands[i].erase(it);
            if (playersHands[i].size() == 1) {
                ret += ToPlayer[i] + ":  ------UNO------!!!\n";
            }
            if (playersHands[i].empty()) {
                win = i;
                ret +=
                    ToPlayer[win] + ":  is the WINNER!!!\n\n\nGame restart!\n";
                std::cout << ToPlayer[win] + " is the WINNER!!!\n\n";
                again(warehouse, usedCard, playersHands, first, lastCard);
            }
            break;
        }
        t++; // 累加
    }
    if (!have) {
        for (; addNum > 0; addNum--) {
            if (warehouse.empty()) {
                washedCard(usedCard);
                warehouse = usedCard;
                usedCard.clear();
                ret += "Cards have been rebuilt!\n";
            }
            playersHands[i].emplace_back(warehouse.back());
            warehouse.pop_back();
            ret += ToPlayer[i] + ":  pick a card!     剩余：" +
                   to_string(playersHands[i].size()) + "\n";
        }
        boolOfAdd = false;
    }
}

// 摸牌/罚牌/洛神（
void getCard(vector<string>& warehouse,
             vector<vector<string>>& playersHands,
             const int i) {
    playersHands[i].emplace_back(warehouse.back());
    warehouse.pop_back();
}

// 出牌逻辑判断
bool playCard(int& playerChoice,
              int i,
              bool& isSkip,
              bool& isAddTwo,
              int& Reverse,
              int playerColor,
              bool& isAdd4,
              int& addNum,
              string& lastCard,
              vector<vector<string>>& playersHands,
              vector<string>& warehouse,
              vector<string>& usedCard,
              bool& hadChanged) {
    bool ret = false;
    if (playerChoice < 0 || playerChoice > (int)'A') {
        playerChoice = 0;
    }
    if ((playersHands[i][playerChoice][0] == lastCard[0] ||
         playersHands[i][playerChoice][2] == lastCard[2] ||
         playersHands[i][playerChoice][1] == '8' ||
         playersHands[i][playerChoice][1] == '9')) {
        if (playersHands[i][playerChoice][1] == '5') {
            isSkip = true;
        } else if (playersHands[i][playerChoice][1] == '6') {
            isAddTwo = true;
            addNum += 2;
        } else if (playersHands[i][playerChoice][1] == '7') {
            Reverse *= -1;
        } else if (playersHands[i][playerChoice][1] == '8') {
            hadChanged = true;
        } else if (playersHands[i][playerChoice][1] == '9') {
            hadChanged = true;
            isAdd4 = true;
            addNum += 4;
        }
        ret = true;
        lastCard = playersHands[i][playerChoice];
        usedCard.emplace_back(playersHands[i][playerChoice]);
        // erase()
        auto it = playersHands[i].begin();
        it += playerChoice;
        playersHands[i].erase(it);
        if (playersHands[i].size() == 1) {
            cout << ToPlayer[i] << ":  ------UNO------!!!\n";
        }
    }
    return ret;
}

// 人机出牌
void computerPlay(int playerChoice,
                  const int i,
                  bool& isSkip,
                  bool& isAddTwo,
                  int& Reverse,
                  int playerColor,
                  bool& isAdd4,
                  int& addNum,
                  string& lastCard,
                  vector<vector<string>>& playersHands,
                  vector<string>& warehouse,
                  vector<string>& usedCard,
                  bool& hadChanged,
                  const bool mode,
                  int& win,
                  std::string& ret) {
    if (isSkip) {
        isSkip = false;
        ret += ToPlayer[i] + ":  be Skipped!      剩余：" +
               to_string(playersHands[i].size()) + '\n';
        return;
    }
    if (isAddTwo) {
        computer_Add(
            '6', isAddTwo, lastCard, addNum, playersHands, warehouse, i, ret);
        return;
    }
    if (isAdd4) {
        computer_Add(
            '9', isAdd4, lastCard, addNum, playersHands, warehouse, i, ret);
        return;
    }
    for (int k = 0; k < playersHands[i].size(); k++) {
        if (playCard(k,
                     i,
                     isSkip,
                     isAddTwo,
                     Reverse,
                     -1,
                     isAdd4,
                     addNum,
                     lastCard,
                     playersHands,
                     warehouse,
                     usedCard,
                     hadChanged)) {
            if (lastCard[2] <= '9' && lastCard[2] >= '0') {
                ret += ToPlayer[i] + ":  play " + ToString[lastCard[0]] + " " +
                       ToString[lastCard[1]] + " " + lastCard[2] + " 剩余：" +
                       to_string(playersHands[i].size()) + '\n';
            } else {
                ret += ToPlayer[i] + ":  play " + ToString[lastCard[0]] + " " +
                       ToString[lastCard[1]] + "   剩余：" +
                       to_string(playersHands[i].size()) + '\n';
            }
            if (hadChanged) {
                changeColor(-1, lastCard, playersHands, i, ret);
                hadChanged = false;
            }
            if (playersHands[i].size() == 1) {
                ret += ToPlayer[i] + ":  ------UNO------!!!\n";
            }
            if (playersHands[i].empty()) {
                win = i;
                ret +=
                    ToPlayer[win] + ":  is the WINNER!!!\n\n\nGame restart!\n";
                std::cout << ToPlayer[win] + " is the WINNER!!!\n\n";
                again(warehouse, usedCard, playersHands, first, lastCard);
            }
            break;
        }

        if (k == playersHands[i].size() - 1) {
            if (warehouse.empty()) {
                washedCard(usedCard);
                warehouse = usedCard;
                usedCard.clear();
                ret += "Cards have been rebuilt!\n";
            }
            getCard(warehouse, playersHands, i);
            ret += ToPlayer[i] + ":  pick a card!     剩余：" +
                   to_string(playersHands[i].size()) + "\n";
            if (!mode) {
                break;
            }
        }
    }
}

// 第一次握手
void on_hello(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::cout << "Hello From Client!\n";
    res.set_content("Hello From Server!", "text/plain");
}

// 玩家登入服务端
void on_login(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!p1_name.empty() && !p2_name.empty() && !p3_name.empty() &&
        !p4_name.empty()) {
        res.set_content("-1", "text/plain");
        return;
    }
    if (!isStart) {
        if (p1_name.empty()) {
            p1_name = req.get_header_value("Name");
            // for (int i = 0; p1_name.size(); i++) {
            //     p1_[i] = p1_name[i];
            // }
            res.set_content("You are Player1 !\n欢迎 " + p1_name + " !",
                            "text/plain");
            std::cout << p1_name << " is login!\n";
            if (max_player < 2) {
                isStart = 1;
                toRandom(ToPlayer, IsPlayer, max_player);
                std::cout << "Everyone is ready!\n";
            }
        } else if (p2_name.empty()) {
            p2_name = req.get_header_value("Name");
            res.set_content("You are Player2 !\n欢迎 " + p2_name + " !",
                            "text/plain");
            std::cout << p2_name << " is login!\n";
            if (max_player < 3) {
                isStart = 1;
                toRandom(ToPlayer, IsPlayer, max_player);
                std::cout << "Everyone is ready!\n";
            }
        } else if (p3_name.empty()) {
            p3_name = req.get_header_value("Name");
            res.set_content("You are Player3 !\n欢迎 " + p3_name + " !",
                            "text/plain");
            std::cout << p3_name << " is login!\n";
            if (max_player < 4) {
                isStart = 1;
                toRandom(ToPlayer, IsPlayer, max_player);
                std::cout << "Everyone is ready!\n";
            }
        } else {
            p4_name = req.get_header_value("Name");
            res.set_content("You are Player4 !\n欢迎 " + p4_name + " !",
                            "text/plain");
            std::cout << p4_name << " is login!\n";
            isStart = 1;
            toRandom(ToPlayer, IsPlayer, max_player);
            std::cout << "Everyone is ready!\n";
        }
    }
}

// 返回游戏是否开始
void on_get_start(const httplib::Request& req, httplib::Response& res) {
    if (isStart) {
        res.set_content("1", "text/plain");
    }
}

// 返回玩家信息
void on_get_players(const httplib::Request& req, httplib::Response& res) {
    if (max_player == 2) {
        res.set_content(p1_name + "--win:" + to_string(countWin[0]) + " " +
                            p2_name + "--win:" + to_string(countWin[1]) + " " +
                            "Computer1--win:" + to_string(countWin[2]) + " " +
                            "Computer2--win:" + to_string(countWin[3]),
                        "text/plain");
    } else if (max_player == 3) {
        res.set_content(p1_name + "--win:" + to_string(countWin[0]) + " " +
                            p2_name + "--win:" + to_string(countWin[1]) + " " +
                            p3_name + "--win:" + to_string(countWin[2]) + " " +
                            "Computer1--win:" + to_string(countWin[3]),
                        "text/plain");
    } else if (max_player == 4) {
        res.set_content(p1_name + "--win:" + to_string(countWin[0]) + " " +
                            p2_name + "--win:" + to_string(countWin[1]) + " " +
                            p3_name + "--win:" + to_string(countWin[2]) + " " +
                            p4_name + "--win:" + to_string(countWin[3]),
                        "text/plain");
    } else {
        res.set_content(p1_name + "--win:" + to_string(countWin[0]) + " " +
                            "Computer1--win:" + to_string(countWin[1]) + " " +
                            "Computer2--win:" + to_string(countWin[2]) + " " +
                            "Computer3--win:" + to_string(countWin[3]),
                        "text/plain");
    }
}

// 获取第一张牌
void on_get_first(const httplib::Request& req, httplib::Response& res) {
    if (max_player == 2) {
        res.set_content('\n' + p1_name + " is Player1.\n" + p2_name +
                            " is Player2.\n" + "\nThe frist card is: \n    " +
                            ToString[first[0]] + " " + ToString[first[1]] +
                            " " + first[2] + "\n",
                        "text/plain");
    } else if (max_player == 3) {
        res.set_content('\n' + p1_name + " is Player1.\n" + p2_name +
                            " is Player2.\n" + p3_name + " is Player3.\n" +
                            "\nThe frist card is: \n    " + ToString[first[0]] +
                            " " + ToString[first[1]] + " " + first[2] + "\n",
                        "text/plain");
    } else if (max_player == 4) {
        res.set_content('\n' + p1_name + " is Player1.\n" + p2_name +
                            " is Player2.\n" + p3_name + " is Player3.\n" +
                            p4_name + " is Player4.\n" +
                            "\nThe frist card is: \n    " + ToString[first[0]] +
                            " " + ToString[first[1]] + " " + first[2] + "\n",
                        "text/plain");
    } else {
        res.set_content('\n' + p1_name + " is Player1.\n" +
                            "\nThe frist card is: \n    " + ToString[first[0]] +
                            " " + ToString[first[1]] + " " + first[2] + "\n",
                        "text/plain");
    }
}

// 显示当前玩家的手牌
void on_show_card(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(g_mutex);
    getShow(tmp, playersHands, i);
    res.set_content(tmp, "text/plain");
}

// 更新回合
void on_query_turn(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(g_mutex);
    turn_str = req.get_header_value("Player");
    if (turn_str == ToPlayer[i]) {
        res.set_content("\1", "text/plain");
    } else {
        res.set_content("\0", "text/plain");
    }
}

// 返回被加的状况
void on_query_add(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(g_mutex);
    tmp.clear();
    // 跳过
    if (isSkip) {
        tmp = "s";
    }
    // 检查是否加2
    if (isAddTwo) {
        tmp = "a";
        for (std::string card : playersHands[i]) {
            if (card[1] == '6') {
                tmp = "t";
                break;
            }
        }
    }
    // 检查是否加2
    if (isAdd4) {
        tmp = "a";
        for (std::string card : playersHands[i]) {
            if (card[1] == '9') {
                tmp = "f";
            }
        }
    }
    res.set_content(tmp, "text/plain");
}

// 加牌
void on_add(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(g_mutex);
    tmp.clear();
    if (req.get_header_value("Card").size() == 1) {
        idx = (int)req.get_header_value("Card")[0] - '1';
    } else {
        idx = (int)req.get_header_value("Card")[1] - '1' +
              10 * ((int)req.get_header_value("Card")[0] - '0');
    }
    if (idx < 0 || idx > playersHands[i].size() - 1) {
        idx = 0;
    }
    // 检查是否被禁
    if (isSkip) {
        isSkip = false;
        com_str += ToPlayer[i] + ":  is Skipped!      剩余：" +
                   std::to_string(playersHands[i].size()) + "\n";
        return;
    }
    // 检查是否加2
    if (isAddTwo) {
        if (idx >= 0 && playersHands[i][idx][1] != '6') {
            idx = -1;
        } else {
            addNum += 2;
            lastCard = playersHands[i][idx];
            // erase()
            auto it = playersHands[i].begin();
            it += idx;
            playersHands[i].erase(it);
            // 检查UNO
            if (playersHands[i].size() == 1) {
                com_str += ToPlayer[i] + ":  ------UNO------!!!\n";
            }
            if (playersHands[i].empty()) {
                win = i;
                std::cout << ToPlayer[win] + " is the WINNER!!!\n\n";
                com_str +=
                    ToPlayer[win] + ":  is the WINNER!!!\n\n\nGame restart!\n";
                again(warehouse, usedCard, playersHands, first, lastCard);
            }
        }
    } // 执行加2
    if (idx == -1) {
        for (; addNum > 0; addNum--) {
            if (warehouse.empty()) {
                washedCard(usedCard);
                warehouse = usedCard;
                usedCard.clear();
                com_str += "Cards have been rebuilt!\n";
            }
            playersHands[i].emplace_back(warehouse.back());
            warehouse.pop_back();
            com_str += ToPlayer[i] + ":  pick a card!     剩余：" +
                       std::to_string(playersHands[i].size()) + "\n";
        }
        isAddTwo = false;
    }

    // 检查是否可以加4
    if (isAdd4) {
        if (idx >= 0 && playersHands[i][idx][1] != '9') {
            idx = -1;
        } else {
            addNum += 4;
            lastCard = playersHands[i][idx];
            lastCard[0] = '4';
            // erase()
            auto it = playersHands[i].begin();
            it += idx;
            playersHands[i].erase(it);
            // 检查UNO
            if (playersHands[i].size() == 1) {
                com_str += ToPlayer[i] + ":  ------UNO------!!!\n";
            }
            if (playersHands[i].empty()) {
                win = i;
                std::cout << ToPlayer[win] + " is the WINNER!!!\n\n";
                com_str +=
                    ToPlayer[win] + ":  is the WINNER!!!\n\n\nGame restart!\n";
                again(warehouse, usedCard, playersHands, first, lastCard);
            }
        }
    } // 执行加4
    if (idx == -1) {
        for (; addNum > 0; addNum--) {
            if (warehouse.empty()) {
                washedCard(usedCard);
                warehouse = usedCard;
                usedCard.clear();
                com_str += "Cards have been rebuilt!\n";
            }
            playersHands[i].emplace_back(warehouse.back());
            warehouse.pop_back();
            com_str += ToPlayer[i] + ":  pick a card!     剩余：" +
                       std::to_string(playersHands[i].size()) + "\n";
        }
        isAdd4 = false;
    }
}

// 返回转色
void on_query_change(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (hadChanged) {
        res.set_content("w", "text/plain");
    } else {
        res.set_content("", "text/plain");
        haveUpdate = true;
        if (win == -1) {
            toNext(i, Reverse);
        } else {
            countWin[win]++;
            win = -1;
        }
    }
}

// 更新转色
void on_change_color(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (req.get_header_value("Color").size() == 1) {
        color = (int)req.get_header_value("Color")[0] - '1';
    }
    if (hadChanged) {
        changeColor(color, lastCard, playersHands, i, com_str);
        haveUpdate = true;
        if (win == -1) {
            toNext(i, Reverse);
        } else {
            countWin[win]++;
            win = -1;
        }
        hadChanged = false;
    }
}

// 玩家出牌
void on_updata(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(g_mutex);
    tmp.clear();
    bool pick = false;
    if ((int)req.get_header_value("Card")[0] == '0') {
        pick = true;
    }
    if (req.get_header_value("Card").size() == 1) {
        idx = (int)req.get_header_value("Card")[0] - '1';
    } else {
        idx = (int)req.get_header_value("Card")[1] - '1' +
              10 * ((int)req.get_header_value("Card")[0] - '0');
    }
    if (idx < 0 || idx > playersHands[i].size() - 1) {
        idx = 0;
    }
    if (!pick && playCard(idx,
                          i,
                          isSkip,
                          isAddTwo,
                          Reverse,
                          playerColor,
                          isAdd4,
                          addNum,
                          lastCard,
                          playersHands,
                          warehouse,
                          usedCard,
                          hadChanged)) {
        if (lastCard[2] <= '9' && lastCard[2] >= '0') {
            tmp += "You play :       " + ToString[lastCard[0]] + " " +
                   ToString[lastCard[1]] + " " + lastCard[2] + '\n';
        } else {
            tmp += "You play :       " + ToString[lastCard[0]] + " " +
                   ToString[lastCard[1]] + '\n';
        }
        for (int tmp_i = getNext(i, Reverse); 1;
             tmp_i = getNext(tmp_i, Reverse)) {
            if (isPlayer(tmp_i)) {
                tmp += ToPlayer[tmp_i] + ":  is thinking...\n";
                break;
            }
        }
        // 记录 player_str
        if (lastCard[2] <= '9' && lastCard[2] >= '0') {
            com_str += ToPlayer[i] + ":    play " + ToString[lastCard[0]] +
                       " " + ToString[lastCard[1]] + " " + lastCard[2] +
                       " 剩余：" + std::to_string(playersHands[i].size()) +
                       '\n';
        } else {
            com_str += ToPlayer[i] + ":    play " + ToString[lastCard[0]] +
                       " " + ToString[lastCard[1]] + "   剩余：" +
                       std::to_string(playersHands[i].size()) + '\n';
        }
        if (playersHands[i].size() == 1) {
            com_str += ToPlayer[i] + ":  ------UNO------!!!\n";
        }
        if (playersHands[i].empty()) {
            win = i;
            std::cout << ToPlayer[win] + " is the WINNER!!!\n\n";
            com_str +=
                ToPlayer[win] + ":  is the WINNER!!!\n\n\nGame restart!\n";
            again(warehouse, usedCard, playersHands, first, lastCard);
        }
    } else {
        if (warehouse.empty()) {
            washedCard(usedCard);
            warehouse = usedCard;
            usedCard.clear();
            com_str += "Cards have been rebuilt!\n";
        }
        getCard(warehouse, playersHands, i);
        tmp += ToPlayer[i] + ":    pick a card!     剩余：" +
               std::to_string(playersHands[i].size()) + "\n";
        com_str += ToPlayer[i] + ":    pick a card!     剩余：" +
                   std::to_string(playersHands[i].size()) + "\n";
        if (!mode) {
            //    return PlayerPlay();
        }
        if (isPlayer(getNext(i, Reverse))) {
            tmp += ToPlayer[getNext(i, Reverse)] + ":  is thinking...\n";
        }
    }
    if (playersHands[i].empty()) {
        win = i;
    }
    res.set_content(tmp, "text/plain");
}

// 人机出牌
void on_computer_update(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (win != -1) {
        com_str.clear();
        res.set_content(ToPlayer[win] +
                            " is the WINNER!!!\n\n\nGame restart!\n",
                        "text/plain");
        countWin[win]++;
        win = -1;
        return;
    }
    while (!isPlayer(i)) {
        computerPlay(-1,
                     i,
                     isSkip,
                     isAddTwo,
                     Reverse,
                     -1,
                     isAdd4,
                     addNum,
                     lastCard,
                     playersHands,
                     warehouse,
                     usedCard,
                     hadChanged,
                     mode,
                     win,
                     com_str);
        haveUpdate = true;
        if (win == -1) {
            toNext(i, Reverse);
        } else {
            countWin[win]++;
            win = -1;
        }
    }
    int reqIdx = (int)req.get_header_value("Bool")[6] - '1';
    if (!get_com_bool[reqIdx] && haveUpdate) {
        get_com_bool[reqIdx] = true;
        cnt_com--;
        std::string s = req.get_header_value("Bool");
        // i:{i}, {s}, {cnt_com}, see {get_com_bool[0]},{get_com_bool[1]},
        // update {haveUpdate}
        res.set_content(com_str, "text/plain");

        if (cnt_com < 1) {
            for (int j = 0; j < get_com_bool.size(); j++) {
                get_com_bool[j] = false;
            }
            haveUpdate = false;
            cnt_com = max_player;
            com_str.clear();
        }
    }
}

// 玩家聊天
void on_call(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(g_mutex);
    com_str += req.get_header_value("Name") + " 全图呼叫：" +
               req.get_header_value("Message") + '\n';
}

int main(int argc, char** argv) {
    system("chcp 65001");
    // 读取最大玩家数
    {
        std::ifstream file("max_player.cfg");
        if (!file.good()) {
            std::cout << "读取失败！\n";
            exit(-1);
        }
        std::stringstream str_string;
        str_string << file.rdbuf();
        max_player = str_string.str()[0] - '0';
        file.close();
    }
    max_rob = 4 - max_player;
    cnt_com = max_player;
    get_com_bool.resize(max_player);
    for (int i = 0; i < get_com_bool.size(); i++) {
        get_com_bool[i] = false;
    }

    // 生成牌堆
    generateCard(warehouse);
    // 108张空间
    usedCard.reserve(108);
    // 发牌
    startCard(warehouse, playersHands);
    std::cout << "Game Start!\n\n";
    // 获取第一张非功能牌
    first = warehouse.back();
    startGame(warehouse, first, usedCard);
    lastCard = first;

    // 服务端Post
    httplib::Server server;
    server.Post("/hello", on_hello);
    server.Post("/login", on_login);
    server.Post("/get_start", on_get_start);
    server.Post("/get_players", on_get_players);
    server.Post("/get_first", on_get_first);
    server.Post("/show_card", on_show_card);
    server.Post("/query_turn", on_query_turn);
    server.Post("/updata", on_updata);
    server.Post("/computer_update", on_computer_update);
    server.Post("/query_add", on_query_add);
    server.Post("/add", on_add);
    server.Post("/query_change", on_query_change);
    server.Post("/change_color", on_change_color);
    server.Post("/call", on_call);
    server.listen("0.0.0.0", 25565);
    // server.listen("172.16.2.173", 25565);
}
/*
x86_64-w64-mingw32-g++ -static-libgcc -static-libstdc++ server.cpp -o uno.exe -lws2_32
*/