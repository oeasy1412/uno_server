#ifndef CSUNO_H_
#define CSUNO_H_

#include <map>
#include <string>
#include <vector>

void generateCard(std::vector<std::string>& warehouse);
void toRandom(std::map<int ,std::string>& ToPlayer, std::map<std::string, bool>& IsPlayer, int max_player);
bool isPlayer(const int i);
void startCard(std::vector<std::string>& warehouse, std::vector<std::vector<std::string>>& playersHands);
void startGame(std::vector<std::string>& warehouse, std::string& first, std::vector<std::string>& usedCard);
void washedCard(std::vector<std::string>& usedCard);
void getShow(std::string& ret, std::vector<std::vector<std::string>>& playersHands, const int i);
void toNext(int& i, const int Reverse);
int getNext(int i, const int Reverse);
void changeColor(int playerColor, std::string& lastCard, std::vector<std::vector<std::string>>& playersHands, 
                const int i,std::string& ret);
void computer_Add(const char T, bool& boolOfAdd, std::string& lastCard, int& addNum, std::vector<std::vector<std::string>>& playersHands,
                std::vector<std::string>& warehouse, const int i, std::string& ret);
void getCard(std::vector<std::string>& warehouse, std::vector<std::vector<std::string>>& playersHands, const int i);
bool playCard(int& playerChoice, int i, bool& isSkip, bool& isAddTwo, int& Reverse, int playerColor, bool& isAdd4, int& addNum,
                std::string& lastCard, std::vector<std::vector<std::string>>& playersHands, std::vector<std::string>& warehouse,
                std::vector<std::string>& usedCard, bool& hadChanged);
void computerPlay(int playerChoice, const int i, bool& isSkip, bool& isAddTwo, int& Reverse, int playerColor, bool& isAdd4, int& addNum,
                std::string& lastCard, std::vector<std::vector<std::string>>& playersHands, std::vector<std::string>& warehouse,
                std::vector<std::string>& usedCard, bool& hadChanged, const bool mode, int& win, std::string& ret);


#endif