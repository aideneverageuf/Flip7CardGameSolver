// Flip7CardGameSolver

#include <algorithm>
#include <cctype>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Cards
// ---------------------------------------------------------------------------

enum class Kind { Number, ModifierAdd, ModifierX2, Freeze, FlipThree, SecondChance };

struct Card {
    Kind kind;
    int value = 0; // number value, or the +N amount for ModifierAdd

    std::string label() const {
        switch (kind) {
            case Kind::Number:       return std::to_string(value);
            case Kind::ModifierAdd:  return "+" + std::to_string(value);
            case Kind::ModifierX2:   return "x2";
            case Kind::Freeze:       return "FREEZE";
            case Kind::FlipThree:    return "FLIP THREE";
            case Kind::SecondChance: return "SECOND CHANCE";
        }
        return "?";
    }
};

// The full 94-card composition: {card, count}.
std::vector<std::pair<Card, int>> deckSpec() {
    std::vector<std::pair<Card, int>> spec;
    spec.push_back({Card{Kind::Number, 0}, 1});
    for (int v = 1; v <= 12; ++v)
        spec.push_back({Card{Kind::Number, v}, v});
    for (int v : {2, 4, 6, 8, 10})
        spec.push_back({Card{Kind::ModifierAdd, v}, 1});
    spec.push_back({Card{Kind::ModifierX2, 0}, 1});
    spec.push_back({Card{Kind::Freeze, 0}, 3});
    spec.push_back({Card{Kind::FlipThree, 0}, 3});
    spec.push_back({Card{Kind::SecondChance, 0}, 3});
    return spec;
}

// ---------------------------------------------------------------------------
// Player
// ---------------------------------------------------------------------------

struct Player {
    std::string name;
    std::vector<Card> numberCards;
    std::vector<Card> modifierCards;
    bool hasSecondChance = false;
    Card secondChanceCard{Kind::SecondChance, 0};
    bool active = true;   // still in the round (not busted / not stayed)
    bool busted = false;
    int totalScore = 0;

    std::vector<int> numberValues() const {
        std::vector<int> vals;
        for (const auto& c : numberCards) vals.push_back(c.value);
        return vals;
    }

    bool hasFlip7() const {
        std::vector<int> vals = numberValues();
        std::sort(vals.begin(), vals.end());
        vals.erase(std::unique(vals.begin(), vals.end()), vals.end());
        return vals.size() >= 7;
    }

    int roundScore() const {
        if (busted) return 0;
        int total = 0;
        for (const auto& c : numberCards) total += c.value;
        bool hasX2 = std::any_of(modifierCards.begin(), modifierCards.end(),
                                  [](const Card& m) { return m.kind == Kind::ModifierX2; });
        if (hasX2) total *= 2;
        for (const auto& m : modifierCards)
            if (m.kind == Kind::ModifierAdd) total += m.value;
        if (hasFlip7()) total += 15;
        return total;
    }

    void resetRound() {
        numberCards.clear();
        modifierCards.clear();
        hasSecondChance = false;
        active = true;
        busted = false;
    }

    std::string handStr() const {
        std::vector<int> vals = numberValues();
        std::sort(vals.begin(), vals.end());
        std::ostringstream nums, mods;
        for (size_t i = 0; i < vals.size(); ++i) {
            if (i) nums << ",";
            nums << vals[i];
        }
        for (size_t i = 0; i < modifierCards.size(); ++i) {
            if (i) mods << ",";
            mods << modifierCards[i].label();
        }
        std::string numStr = vals.empty() ? "-" : nums.str();
        std::string modStr = modifierCards.empty() ? "-" : mods.str();
        return "numbers=[" + numStr + "] modifiers=[" + modStr + "] second_chance=" +
               (hasSecondChance ? "yes" : "no");
    }
};

// ---------------------------------------------------------------------------
// Game: tracks exact remaining/discarded card counts (no simulated shuffle).
// ---------------------------------------------------------------------------

struct Game {
    std::vector<Player> players;
    std::map<std::string, int> remaining;   // label -> count left in the real deck
    std::map<std::string, int> discarded;   // label -> count in the real discard pile
    std::map<std::string, Card> byLabel;    // label -> representative Card
    int dealerIndex = 0;

    explicit Game(std::vector<Player> p) : players(std::move(p)) {
        for (const auto& [card, count] : deckSpec()) {
            std::string lbl = card.label();
            remaining[lbl] = count;
            discarded[lbl] = 0;
            byLabel[lbl] = card;
        }
    }

    int totalRemaining() const {
        int total = 0;
        for (const auto& [lbl, c] : remaining) total += c;
        return total;
    }

    int totalDiscarded() const {
        int total = 0;
        for (const auto& [lbl, c] : discarded) total += c;
        return total;
    }

    void discardOne(const Card& c) { discarded[c.label()]++; }

    // If the real deck has run out, the discard pile gets shuffled back in.
    void reshuffleIfNeeded() {
        if (totalRemaining() > 0) return;
        std::cout << "\n[Deck is empty - shuffle the discard pile into a new deck]\n";
        remaining = discarded;
        for (auto& [lbl, c] : discarded) c = 0;
    }

    // -- turn order: dealer acts LAST -----------------------------------
    std::vector<int> turnOrder() const {
        std::vector<int> order;
        int n = static_cast<int>(players.size());
        for (int i = 1; i <= n; ++i) order.push_back((dealerIndex + i) % n);
        return order;
    }

    void printProbabilities() const {
        int total = totalRemaining();
        std::cout << "    Deck remaining: " << total << " cards"
                  << " | discard: " << totalDiscarded() << " cards\n";
        if (total == 0) return;
        std::vector<std::pair<std::string, int>> items;
        for (const auto& [lbl, count] : remaining)
            if (count > 0) items.emplace_back(lbl, count);
        std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
            bool aNum = !a.first.empty() && std::all_of(a.first.begin(), a.first.end(), ::isdigit);
            bool bNum = !b.first.empty() && std::all_of(b.first.begin(), b.first.end(), ::isdigit);
            if (aNum != bNum) return aNum > bNum;
            if (aNum && bNum) return std::stoi(a.first) < std::stoi(b.first);
            return a.first < b.first;
        });
        std::cout << "    ";
        for (size_t i = 0; i < items.size(); ++i) {
            if (i) std::cout << "  ";
            double pct = 100.0 * items[i].second / total;
            std::cout << items[i].first << ":" << items[i].second << "(" << static_cast<int>(pct + 0.5) << "%)";
        }
        std::cout << "\n";
    }

    double bustProbability(const Player& p) const {
        int total = totalRemaining();
        if (total == 0) return 0.0;
        if (p.hasSecondChance) return 0.0; // a duplicate would be absorbed, not a bust
        std::vector<int> have = p.numberValues();
        int bad = 0;
        for (int v : have) {
            auto it = remaining.find(std::to_string(v));
            if (it != remaining.end()) bad += it->second;
        }
        return 100.0 * bad / total;
    }
};

bool roundIsOver(const Game& game) {
    for (const auto& p : game.players)
        if (p.hasFlip7()) return true;
    for (const auto& p : game.players)
        if (p.active) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Input abstraction
// ---------------------------------------------------------------------------

std::function<std::string(const std::string&)> gAsk = [](const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cout << "\n[No more input - exiting.]\n";
        std::exit(0);
    }
    return line;
};

// ---------------------------------------------------------------------------
// Parsing a real drawn card from user input
// ---------------------------------------------------------------------------

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

// Returns true and sets `out` if `raw` parses to a recognized card label.
bool parseCard(const std::string& raw, Card& out) {
    std::string s = raw;
    // trim whitespace
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    if (s.empty()) return false;

    std::string low = toLower(s);

    if (low == "x2") { out = Card{Kind::ModifierX2, 0}; return true; }
    if (low == "freeze" || low == "fz") { out = Card{Kind::Freeze, 0}; return true; }
    if (low == "flip3" || low == "flipthree" || low == "ft" || low == "flip three") {
        out = Card{Kind::FlipThree, 0}; return true;
    }
    if (low == "sc" || low == "second" || low == "secondchance" || low == "second chance") {
        out = Card{Kind::SecondChance, 0}; return true;
    }
    if (s[0] == '+' && s.size() > 1 &&
        std::all_of(s.begin() + 1, s.end(), ::isdigit)) {
        int v = std::stoi(s.substr(1));
        if (v == 2 || v == 4 || v == 6 || v == 8 || v == 10) {
            out = Card{Kind::ModifierAdd, v};
            return true;
        }
        return false;
    }
    if (std::all_of(s.begin(), s.end(), ::isdigit)) {
        int v = std::stoi(s);
        if (v >= 0 && v <= 12) {
            out = Card{Kind::Number, v};
            return true;
        }
    }
    return false;
}

// Prompts the user for the card that was actually drawn for `who`, validates
// it against what's really left in the deck, and removes it from the pool.
Card promptDrawnCard(Game& game, const std::string& who) {
    game.reshuffleIfNeeded();
    while (true) {
        game.printProbabilities();
        std::string raw = gAsk("Card drawn for " + who +
                                " (number 0-12, +2/+4/+6/+8/+10, x2, freeze, flip3, sc): ");
        Card c;
        if (!parseCard(raw, c)) {
            std::cout << "  Didn't recognize that card, try again.\n";
            continue;
        }
        std::string lbl = c.label();
        auto it = game.remaining.find(lbl);
        if (it == game.remaining.end() || it->second <= 0) {
            std::cout << "  There are 0 of that card left in the deck - try again.\n";
            continue;
        }
        it->second--;
        return c;
    }
}

// ---------------------------------------------------------------------------
// Action / card resolution
// ---------------------------------------------------------------------------

int choosePlayer(const std::string& prompt, const std::vector<int>& candidates, const Game& game) {
    std::cout << prompt << "\n";
    for (size_t i = 0; i < candidates.size(); ++i)
        std::cout << "  " << (i + 1) << ". " << game.players[candidates[i]].name << "\n";
    while (true) {
        std::string sel = gAsk("Choice: ");
        try {
            int idx = std::stoi(sel);
            if (idx >= 1 && static_cast<size_t>(idx) <= candidates.size())
                return candidates[idx - 1];
        } catch (...) {
            // fall through to retry
        }
        std::cout << "Invalid choice, try again.\n";
    }
}

void resolveCard(Game& game, int playerIdx, Card card) {
    Player& player = game.players[playerIdx];

    if (card.kind == Kind::Number) {
        std::vector<int> vals = player.numberValues();
        if (std::find(vals.begin(), vals.end(), card.value) != vals.end()) {
            if (player.hasSecondChance) {
                std::cout << "  " << player.name << " drew a duplicate " << card.value
                          << " -> SECOND CHANCE cancels it!\n";
                game.discardOne(player.secondChanceCard);
                game.discardOne(card);
                player.hasSecondChance = false;
            } else {
                std::cout << "  " << player.name << " drew a duplicate " << card.value << " -> BUSTS!\n";
                player.busted = true;
                player.active = false;
                game.discardOne(card);
            }
            return;
        }
        player.numberCards.push_back(card);
        std::vector<int> newVals = player.numberValues();
        std::sort(newVals.begin(), newVals.end());
        std::cout << "  " << player.name << " drew " << card.value << ". Hand: [";
        for (size_t i = 0; i < newVals.size(); ++i) {
            if (i) std::cout << ", ";
            std::cout << newVals[i];
        }
        std::cout << "]\n";
        if (player.hasFlip7())
            std::cout << "  *** " << player.name << " achieved FLIP 7! Round ends! ***\n";
        return;
    }

    if (card.kind == Kind::ModifierAdd || card.kind == Kind::ModifierX2) {
        player.modifierCards.push_back(card);
        std::cout << "  " << player.name << " drew modifier " << card.label() << ".\n";
        return;
    }

    if (card.kind == Kind::SecondChance) {
        if (!player.hasSecondChance) {
            player.hasSecondChance = true;
            player.secondChanceCard = card;
            std::cout << "  " << player.name << " keeps a SECOND CHANCE card.\n";
        } else {
            std::vector<int> others;
            for (size_t i = 0; i < game.players.size(); ++i)
                if (game.players[i].active && static_cast<int>(i) != playerIdx)
                    others.push_back(static_cast<int>(i));
            if (others.empty()) {
                std::cout << "  " << player.name
                          << " already holds a Second Chance and no other active players remain"
                             " -> discarded.\n";
                game.discardOne(card);
            } else {
                int targetIdx = choosePlayer(
                    "  " + player.name + " already has a Second Chance. Choose who receives the new one:",
                    others, game);
                Player& target = game.players[targetIdx];
                if (!target.hasSecondChance) {
                    target.hasSecondChance = true;
                    target.secondChanceCard = card;
                    std::cout << "  " << target.name << " receives the extra SECOND CHANCE card.\n";
                } else {
                    std::cout << "  " << target.name << " already has one too -> card discarded.\n";
                    game.discardOne(card);
                }
            }
        }
        return;
    }

    if (card.kind == Kind::Freeze) {
        std::vector<int> candidates;
        for (size_t i = 0; i < game.players.size(); ++i)
            if (game.players[i].active) candidates.push_back(static_cast<int>(i));
        int targetIdx;
        if (candidates.size() == 1) {
            targetIdx = candidates[0];
            std::cout << "  Only one active player -> FREEZE auto-targets "
                      << game.players[targetIdx].name << ".\n";
        } else {
            targetIdx = choosePlayer("  " + player.name + " drew FREEZE. Choose the target:", candidates, game);
        }
        Player& target = game.players[targetIdx];
        target.active = false;
        std::cout << "  " << target.name << " is FROZEN and banks " << target.roundScore() << " points.\n";
        game.discardOne(card);
        return;
    }

    if (card.kind == Kind::FlipThree) {
        std::vector<int> candidates;
        for (size_t i = 0; i < game.players.size(); ++i)
            if (game.players[i].active) candidates.push_back(static_cast<int>(i));
        int targetIdx;
        if (candidates.size() == 1) {
            targetIdx = candidates[0];
            std::cout << "  Only one active player -> FLIP THREE auto-targets "
                      << game.players[targetIdx].name << ".\n";
        } else {
            targetIdx = choosePlayer("  " + player.name + " drew FLIP THREE. Choose the target:", candidates, game);
        }
        game.discardOne(card);

        std::vector<Card> deferred;
        int drawn = 0;
        while (drawn < 3) {
            Player& target = game.players[targetIdx];
            if (target.busted || !target.active || target.hasFlip7()) break;
            ++drawn;
            Card next = promptDrawnCard(game, target.name + " (Flip Three " + std::to_string(drawn) + "/3)");
            if (next.kind == Kind::Freeze || next.kind == Kind::FlipThree) {
                deferred.push_back(next);
                std::cout << "  (" << next.label()
                          << " revealed during Flip Three -> resolved after all 3 cards are dealt)\n";
                continue;
            }
            resolveCard(game, targetIdx, next);
        }
        for (const auto& dc : deferred) {
            Player& target = game.players[targetIdx];
            if (target.busted || !target.active || target.hasFlip7()) {
                std::cout << "  " << dc.label() << " discarded (target no longer eligible).\n";
                game.discardOne(dc);
                continue;
            }
            resolveCard(game, targetIdx, dc);
        }
        return;
    }
}

// ---------------------------------------------------------------------------
// Round / game flow
// ---------------------------------------------------------------------------

void showStatus(const Game& game, const Player& player) {
    std::cout << "\n" << player.name << "'s turn -- " << player.handStr()
              << " | score if stay now: " << player.roundScore() << "\n";
    game.printProbabilities();
    std::cout << "    Bust probability if you HIT: "
              << std::fixed << std::setprecision(1) << game.bustProbability(player) << "%\n";
}

void endRound(Game& game) {
    std::cout << "\n----- Round Results -----\n";
    for (auto& p : game.players) {
        int pts = p.roundScore();
        p.totalScore += pts;
        std::string status = p.busted ? "BUSTED" : (p.hasFlip7() ? "FLIP 7!" : "Stayed");
        std::cout << "  " << p.name << ": " << status << "  +" << pts
                  << " pts  (total: " << p.totalScore << ")\n";

        for (const auto& c : p.numberCards) game.discardOne(c);
        for (const auto& c : p.modifierCards) game.discardOne(c);
        if (p.hasSecondChance) game.discardOne(p.secondChanceCard);

        p.numberCards.clear();
        p.modifierCards.clear();
        p.hasSecondChance = false;
    }
    game.dealerIndex = (game.dealerIndex + 1) % static_cast<int>(game.players.size());
}

void playRound(Game& game) {
    for (auto& p : game.players) p.resetRound();

    std::cout << "\n" << std::string(60, '=') << "\nNEW ROUND -- Dealer: "
              << game.players[game.dealerIndex].name << " (deals to self last)\n"
              << std::string(60, '=') << "\n";

    std::vector<int> order = game.turnOrder(); // dealer is last in this list

    // Initial deal: one card each, dealer last, pausing to resolve any
    // action card immediately before continuing the deal.
    for (int idx : order) {
        if (roundIsOver(game)) break;
        std::cout << "\n-- Dealing to " << game.players[idx].name << " --\n";
        resolveCard(game, idx, promptDrawnCard(game, game.players[idx].name));
    }

    // Hit / stay loop, same order (dealer acts last each pass).
    while (!roundIsOver(game)) {
        bool progressed = false;
        for (int idx : order) {
            if (roundIsOver(game)) break;
            Player& p = game.players[idx];
            if (!p.active) continue;
            progressed = true;
            showStatus(game, p);
            std::string choice = gAsk(p.name + ": (H)it or (S)tay? ");
            if (!choice.empty() && std::tolower(choice[0]) == 's') {
                p.active = false;
                std::cout << "  " << p.name << " stays with " << p.roundScore() << " points.\n";
            } else {
                resolveCard(game, idx, promptDrawnCard(game, p.name));
            }
        }
        if (!progressed) break;
    }

    endRound(game);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main() {
    std::cout << "=== FLIP 7 SOLVER - Live Game Tracker ===\n";
    std::cout << "Enter each card as it's actually drawn from your physical deck.\n";
    std::cout << "Player 1 deals first (and deals to themselves last); the deal\n";
    std::cout << "then rotates to the next player each round.\n\n";

    int n = 0;
    while (true) {
        std::string raw = gAsk("How many players (3-18 recommended)? ");
        try {
            n = std::stoi(raw);
            if (n >= 1) break;
        } catch (...) {}
        std::cout << "Please enter a whole number.\n";
    }

    std::vector<Player> players;
    for (int i = 0; i < n; ++i) {
        std::string name = gAsk("Name for player " + std::to_string(i + 1) + " (blank = Player" +
                                 std::to_string(i + 1) + "): ");
        players.push_back(Player{name.empty() ? ("Player" + std::to_string(i + 1)) : name});
    }

    Game game(players);
    // Player 1 (index 0) deals the first round.
    game.dealerIndex = 0;

    while (true) {
        playRound(game);
        bool someoneWon = false;
        for (const auto& p : game.players)
            if (p.totalScore >= 200) someoneWon = true;
        if (someoneWon) break;
        gAsk("\nPress Enter to start the next round...");
    }

    std::cout << "\n" << std::string(60, '=') << "\nGAME OVER\n" << std::string(60, '=') << "\n";
    std::vector<Player> sortedPlayers = game.players;
    std::sort(sortedPlayers.begin(), sortedPlayers.end(),
              [](const Player& a, const Player& b) { return a.totalScore > b.totalScore; });
    for (const auto& p : sortedPlayers)
        std::cout << "  " << p.name << ": " << p.totalScore << "\n";

    const Player* winner = &game.players[0];
    for (const auto& p : game.players)
        if (p.totalScore > winner->totalScore) winner = &p;
    std::cout << "\n*** " << winner->name << " wins with " << winner->totalScore << " points! ***\n";

    return 0;
}