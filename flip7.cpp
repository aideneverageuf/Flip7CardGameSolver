//Flip7CardGameSolver

#include <algorithm>
#include <cassert>
#include <cctype>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

enum class Kind { Number, ModifierAdd, ModifierX2, Freeze, FlipThree, SecondChance };

struct Card {
    Kind kind;
    int value = 0;

    string label() const {
        switch (kind) {
            case Kind::Number: return to_string(value);
            case Kind::ModifierAdd: return "+" + to_string(value);
            case Kind::ModifierX2: return "x2";
            case Kind::Freeze: return "FREEZE";
            case Kind::FlipThree: return "FLIP THREE";
            case Kind::SecondChance: return "SSECOND CHANCE";
        }
        return "?";
    }
};

static mt19937 g_rng(random_device{}());

vector<Card> buildDeck() {
    vector<Card> cards;
    //number cards: n copies of value n: 12 twelves, 11 elevens, etc...
    for (int v = 1; v <= 12; ++v) {
        for (int i = 0; i < v; ++i)
            cards.push_back({Kind::Number, v});
    cards.push_back({Kind::Number, 0});
    
    //modifier cards: one of each: +2, +4, +6, +8, +10, x2
    for (int v : {2, 4, 6, 8, 10})
        cards.push_back({Kind::ModifierAdd, v});

    //action cards: three of each: freeze, flip three, second chance
    for (int i = 0; i < 3; ++i) {
        cards.push_back({Kind::Freeze, 0});
        cards.push_back({Kind::FlipThree, 0});
        cards.push_back({Kind::SecondChance, 0});
    }
    assert(cards.size() == 94 && "deck build error");
    shuffle(cards.begin(), cards.end(), g_rng);
    return cards;
    }
}
//------------------------------------
//Player
//------------------------------------

struct Player {
    string name;
    vector<Card> numberCards;
    vector<Card> modifierCards;
    bool hasSecondChance = false;
    Card secondChanceCard{Kind::SecondChance, 0};
    bool active = true; // in the round
    bool busted = false;
    int totalScore = 0;

    vector<int> numberValues() const {
        vector<int> vals;
        for (const auto& c : numberCards) vals.push_back(c.value);
        return vals;
    }

    bool hasFlip7() const {
        vector<int> vals = numberValues();
        sort(vals.begin(), vals.end());
        vals.erase(unique(vals.begin(), vals.end()), vals.end());
        return vals.size() >= 7;
    }

    int roundScore() const {
        if (busted) return 0;
        int total = 0;
        for (const auto& c : numberCards) total += c.value;
        bool hasX2 = std::any_of(modifierCards.begin(), modifierCards.end(), [](const Card& m) { return m.kind == Kind::ModifierX2; });
        if (hasX2) total *= 2;
        for (const auto& m : modifierCards)
            if (m.kind == Kind::ModifierAdd) total += m.value;
        if (hasFlip7()) total += 15;
        return total;
    }

    void resetRound() {
        numberCards.clear();
        modifierCards.clear();
        active = true;
        busted = false;
    }

    string handStr() const {
        vector<int> vals = numberValues();
        sort(vals.begin(), vals.end());
        ostringstream nums, mods;
        for (size_t i = 0; i < vals.size(); ++i) {
            if (i) mods << ",";
            mods << modifierCards[i].label();
        }
        string numStr = vals.empty() ? "-" : nums.str();
        string modStr = modifierCards.empty() ? "-" : mods.str();
        return "numbers=[" + numStr + "], modifiers=[" + modStr + "] second_chance=" + (hasSecondChance ? "yes" : "no");
    }
};
