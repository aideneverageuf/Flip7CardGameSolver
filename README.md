# Flip7CardGameSolver

A companion toolkit for playing the physical card game **Flip 7** (The Op / USAOPOLY, Ruleset Ed. 3.1). Instead of simulating a random deck, these tools track the *real* deck as you play: you tell the tool what card actually got drawn, and it keeps exact score, deck, and discard state, resolves action cards, and shows live draw/bust probabilities based on what's genuinely left in the deck.

Two versions of the same engine are included:

| File | What it is |
|---|---|
| `flip7_tracker.html` | **Recommended.** A self-contained web app — click cards instead of typing them, so there's no room for input errors. Includes a full undo system. |
| `flip7.cpp` | A console version of the same tracker, driven by typed input. |

Both implement identical rules and scoring; the HTML version is the more polished, error-resistant front end built on top of the same logic.

---

## Quick start (web app — recommended)

1. Download `flip7_tracker.html`.
2. Double-click it, or open it in any browser (Chrome, Edge, Firefox, Safari). No install, no server, no build step.
3. Choose the number of players and (optionally) name them. Player 1 deals the first round.
4. Play the physical game as normal. Whenever a card is revealed, tap the matching button in the app.

### How a round works in the app

- **Dealing:** the app deals one card to each player in turn order, with the dealer always dealt to **last**. Tap the card that actually came off the deck for each player.
- **Hit / Stay:** once everyone has their first card, the app asks each active player (in the same order, dealer last) to Hit or Stay. Tap the real decision made at the table, then tap the drawn card if they hit.
- **Action cards:** Freeze, Flip Three, and Second Chance are resolved automatically — the app will prompt you to pick a target when needed, and Flip Three walks through its 3 forced draws (including the "any Freeze/Flip Three revealed mid-sequence resolves after the 3 cards" rule).
- **Busts, Flip 7, and scoring** are computed automatically per round, including the ×2-then-add-modifiers order and the 15-point Flip 7 bonus.
- **Deck & discard** are tracked exactly. When the real deck runs out, the app tells you to shuffle the discard pile back in before continuing.
- **Dealer rotation:** after each round, the deal passes to the next player, who then deals everyone else first and themselves last.

### Undo

If you tap the wrong card or the wrong Hit/Stay by mistake, hit **↺ Undo last entry** (top right, next to the round/dealer info). It steps back one recorded action at a time — card draw, hit/stay call, target choice, or "start next round" — and correctly rewinds scores, deck counts, dealer state, and any in-progress Flip Three sequence, not just the on-screen log.

A couple of things worth knowing:
- Undo only works within the current game session — once you click "Start a new game," history resets.
- There's no redo. If you undo too far, just re-enter the cards forward again.
- The dealer rotates the instant a round ends (not when you click "Start next round"), so undoing that click restores the round's final scores but won't un-rotate the dealer — you'd need to undo further back into the round for that.

### Live probabilities

At every decision point the app shows:
- The exact composition of what's left in the deck (every remaining card and its count).
- The bust probability for whoever's deciding whether to Hit — the exact chance their next card duplicates one they already have.

These are exact, not estimated — since you're telling the app every card that's been played, it always knows precisely what's left.

---

## Console version (`flip7.cpp`)

The same engine, driven from a terminal instead of buttons. Useful if you'd rather type card codes than click, or want to build on the logic yourself.

### Build

Requires a C++17 compiler.

```
g++ -std=c++17 -O2 -o flip7 flip7.cpp
./flip7
```

On Windows with MSVC/Visual Studio, just build `flip7.cpp` as a normal console project targeting the C++17 standard.

### Usage

The program will ask for the number of players and their names, then prompt for each card as it's drawn:

```
Card drawn for Player1 (number 0-12, +2/+4/+6/+8/+10, x2, freeze, flip3, sc):
```

Accepted shorthand: a plain number (`7`), a modifier (`+4`), `x2`, `freeze`, `flip3`, or `sc`. Invalid or depleted cards are rejected with a re-prompt instead of crashing.

The console version does not currently have an undo command — if you mistype, the web app is the better choice.

---

## Game rules reference

Based on the official rulebook (Ruleset Edition 3.1). The full 94-card deck:

**Number cards (79 total):** one 0, then *N* copies of each number *N* for 1 through 12 (one 1, two 2s, … twelve 12s).

**Modifier cards (6 total):** one each of +2, +4, +6, +8, +10, and one ×2.

**Action cards (9 total):** three each of Freeze, Flip Three, and Second Chance.

**Scoring for a round:**
1. Sum your number cards.
2. If you have the ×2 modifier, double that sum.
3. Add any +N modifiers.
4. If you collected 7 unique number cards ("Flip 7"), add 15 bonus points and the round ends immediately for everyone.
5. Busting (drawing a number you already have, without a Second Chance to cancel it) scores 0 for the round.

First player to 200+ points at the end of a round wins.

---

## Notes on accuracy

This engine has been tested extensively against the rulebook:
- Card counts verified against the physical rulebook (94 cards total, exact breakdown above).
- Deck/discard conservation (always exactly 94 cards across deck + discard + all players' hands) has been verified across dozens of automated randomized games, including multiple mid-round reshuffles.
- The full click-driven web UI has been tested end-to-end via automated browser-DOM simulation, not just the underlying logic.
- The undo system has been tested for correctness of state rollback (deck counts, scores, dealer position), not just log display.
