#include <emscripten.h>
#include <vector>
#include <stack>
#include <algorithm>
#include <ctime>
#include <cstdlib>

extern "C" {

static int cards[16];
static bool revealed[16];
static bool matchedArr[16];
static std::stack<int> historyStack; // stores flip actions (index) so undo can pop them
static std::vector<int> currentFlipped; // indices flipped this turn (0..2)

// helper
static int matchedCount = 0;
static int score = 0;

// initialize / shuffle
EMSCRIPTEN_KEEPALIVE
void initGame(){
    std::srand((unsigned)time(NULL));
    std::vector<int> vals;
    for(int i=1;i<=8;i++){
        vals.push_back(i);
        vals.push_back(i);
    }
    // Fisher-Yates shuffle
    for(int i=(int)vals.size()-1;i>0;i--){
        int j = std::rand() % (i+1);
        std::swap(vals[i], vals[j]);
    }
    for(int i=0;i<16;i++){
        cards[i] = vals[i];
        revealed[i] = false;
        matchedArr[i] = false;
    }
    while(!historyStack.empty()) historyStack.pop();
    currentFlipped.clear();
    matchedCount = 0;
    score = 0;
}

// get card value if revealed/matched, else -1
EMSCRIPTEN_KEEPALIVE
int getCardValue(int idx){
    if(idx < 0 || idx >= 16) return -1;
    if(revealed[idx] || matchedArr[idx]) return cards[idx];
    return -1;
}

// flip a card (called from JS)
EMSCRIPTEN_KEEPALIVE
int flipCard(int idx){
    if(idx < 0 || idx >= 16) return 0;
    if(matchedArr[idx]) return 0; // already matched
    if(revealed[idx]) return 0; // already revealed

    revealed[idx] = true;
    historyStack.push(idx);
    currentFlipped.push_back(idx);

    // if two are flipped, check immediately
    if(currentFlipped.size() == 2){
        int a = currentFlipped[0];
        int b = currentFlipped[1];
        if(cards[a] == cards[b]){
            matchedArr[a] = true;
            matchedArr[b] = true;
            matchedCount++;
            score += 1; // one point per pair
            currentFlipped.clear();
        } else {
            // do not auto-hide here; JS will call hideLastTwo after delay
            // keep currentFlipped as is so JS can instruct hide
        }
    }
    return 1;
}

// hide last two flipped (used when two don't match)
EMSCRIPTEN_KEEPALIVE
void hideLastTwo(){
    if(currentFlipped.size() < 2) return;
    int a = currentFlipped[0];
    int b = currentFlipped[1];
    // make sure they're not matched
    if(!matchedArr[a]) revealed[a] = false;
    if(!matchedArr[b]) revealed[b] = false;
    currentFlipped.clear();
}

// undo last flip (pop stack) — only undoes flips that are currently face-up and not matched
EMSCRIPTEN_KEEPALIVE
void undo(){
    if(historyStack.empty()) return;
    int idx = historyStack.top();
    historyStack.pop();

    // only undo if it's currently revealed and not matched
    if(idx >=0 && idx < 16 && revealed[idx] && !matchedArr[idx]){
        revealed[idx] = false;
        // also remove from currentFlipped vector if present
        for(auto it = currentFlipped.begin(); it != currentFlipped.end(); ++it){
            if(*it == idx){ currentFlipped.erase(it); break; }
        }
    } else {
        // if it was matched already, we can't undo that flip (leave it)
    }
}

// isMatched
EMSCRIPTEN_KEEPALIVE
int isMatched(int idx){
    if(idx < 0 || idx >= 16) return 0;
    return matchedArr[idx] ? 1 : 0;
}

// matchedCount getter
EMSCRIPTEN_KEEPALIVE
int getMatchedCount(){
    return matchedCount;
}

// score getter
EMSCRIPTEN_KEEPALIVE
int getScore(){
    return score;
}

// win check
EMSCRIPTEN_KEEPALIVE
int isWin(){
    return (matchedCount >= 8) ? 1 : 0;
}

} 
