/**
 * @file engine.cpp
 * @author Dhiraj Adhikary (IIT Guwahati)
 * @brief Main execution loop and WASM bridge for the dynamic game engine.
 * @context Parses JSON payloads and routes logic to specific game modes seamlessly.
 */

#include "engine_types.h"
#include "game_modes.h"
#include "renderer.h"
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <string>

using namespace std;

// ==========================================
// JAVASCRIPT BRIDGES (EMSCRIPTEN INTEROP)
// ==========================================
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
EM_JS(void, triggerGameOverUI, (int finalScore), { 
    document.getElementById('game-over-overlay').classList.remove('hidden'); 
    document.getElementById('final-score-text').innerText = "Metrics: " + finalScore; 
    window.submitScoreToCloud(finalScore); 
});
EM_JS(void, triggerVictoryUI, (int finalScore), { 
    document.getElementById('victory-overlay').classList.remove('hidden'); 
    document.getElementById('victory-score-text').innerText = "Metrics: " + finalScore; 
    window.submitScoreToCloud(finalScore); 
});
#else
void triggerGameOverUI(int score) { cout << "GAME OVER!\n"; } 
void triggerVictoryUI(int score) { cout << "VICTORY!\n"; }
#endif

GameContext* globalCtx = nullptr;

float getF(json& j, const string& k, float d) {
    if(!j.contains(k)) return d;
    if(j[k].is_number()) return j[k].get<float>();
    if(j[k].is_string()) { try { return stof(j[k].get<string>()); } catch(...) {} }
    return d;
}

int getI(json& j, const string& k, int d) {
    if(!j.contains(k)) return d;
    if(j[k].is_number()) return j[k].get<int>();
    if(j[k].is_string()) { try { return stoi(j[k].get<string>()); } catch(...) {} }
    return d;
}

void LoadCurrentLevel(GameContext* ctx) {
    if (ctx->currentLevelIndex >= (int)ctx->levelsConfig.size()) return;
    json lvl = ctx->levelsConfig[ctx->currentLevelIndex];
    
    ctx->particles.clear(); 
    ctx->player.hp = 100; 
    ctx->levelScoreStart = ctx->score; 
    
    if (ctx->gameMode == "endless_runner") {
        ctx->player.Init(100, 500, 40, 40, true, 100); ctx->platform.Init(0, 550, 800, 50, false, 9999);
        ctx->obstacles.clear(); ctx->historyPlayer.clear(); ctx->historyObs.clear();
        ctx->obsSpeed = getF(lvl, "obstacle_speed", 7.0f); ctx->obsSpawnRate = getI(lvl, "spawn_rate", 60); ctx->obsTimer = 0;
    } 
    else if (ctx->gameMode == "quiz_mcq") {
        ctx->currentQuizQuestion = lvl.value("question", "?"); ctx->currentQuizOptions.clear();
        struct TempOpt { string text; bool correct; }; vector<TempOpt> shuffleQueue;
        shuffleQueue.push_back({lvl.value("correct", "True"), true});
        if (lvl.contains("wrong") && lvl["wrong"].is_array()) { for (auto& w : lvl["wrong"]) { if (w.is_string()) shuffleQueue.push_back({w.get<string>(), false}); } }
        for (size_t i = 0; i < shuffleQueue.size(); i++) swap(shuffleQueue[i], shuffleQueue[rand() % shuffleQueue.size()]);
        int startY = 150; for (auto& sq : shuffleQueue) { ctx->currentQuizOptions.push_back({{ 100, startY, 600, 80 }, sq.text, sq.correct, 0}); startY += 100; }
    }
    else if (ctx->gameMode == "algo_arena") {
        ctx->player.Init(380, 500, 40, 40, false, 100); ctx->platform.Init(0, 550, 800, 50, false, 9999);
        ctx->algoQuestion = lvl.value("question", "?"); ctx->algoOptions.clear(); float startX = 100.0f;
        ctx->algoOptions.push_back({startX, 0.0f, 2.0f, lvl.value("correct", "True"), true, true});
        if (lvl.contains("wrong") && lvl["wrong"].is_array()) { for (auto& w : lvl["wrong"]) { startX += 200.0f; ctx->algoOptions.push_back({startX, -((float)(rand()%100)), 2.0f, w.is_string() ? w.get<string>() : "False", false, true}); } }
    }
    else if (ctx->gameMode == "typing_defense") {
        ctx->words.clear(); ctx->vocabulary.clear();
        if (lvl.contains("words") && lvl["words"].is_array()) { for (auto& w : lvl["words"]) ctx->vocabulary.push_back(w.get<string>()); } else ctx->vocabulary.push_back("err");
        ctx->typeBuffer = ""; ctx->wordSpawnTimer = 0; ctx->globalFallSpeed = getF(lvl, "fall_speed", 1.0f);
    }
    else if (ctx->gameMode == "memory_match") {
        ctx->cards.clear(); int rows = getI(lvl, "rows", 2); int cols = getI(lvl, "cols", 3); int startX = 100, startY = 100, padding = 20, w = 150, h = 150; 
        struct CardSetup { string text; int matchId; }; vector<CardSetup> deck; int currentId = 0;
        if (lvl.contains("pairs") && lvl["pairs"].is_array()) { for (auto& p : lvl["pairs"]) { deck.push_back({p.contains("a") ? p["a"].get<string>() : "A", currentId}); deck.push_back({p.contains("b") ? p["b"].get<string>() : "B", currentId}); currentId++; } }
        for (size_t i = 0; i < deck.size(); i++) swap(deck[i], deck[rand() % deck.size()]); int idx = 0;
        for (int r = 0; r < rows; r++) { for (int c = 0; c < cols; c++) { if (idx < (int)deck.size()) { ctx->cards.push_back({{ startX + c * (w + padding), startY + r * (h + padding), w, h }, deck[idx].text, 0, deck[idx].matchId }); idx++; } } }
        ctx->firstClickIdx = -1; ctx->secondClickIdx = -1; ctx->flipTimer = 0;
    }
    else if (ctx->gameMode == "tug_of_war") {
        ctx->ropePosition = 400.0f; ctx->aiPullSpeed = getF(lvl, "ai_pull_speed", 1.5f); ctx->playerPullPower = getF(lvl, "player_pull_power", 10.0f);
    }
    else if (ctx->gameMode == "rhythm_catch") {
        ctx->notes.clear(); ctx->noteTimer = 0; ctx->obsSpawnRate = getI(lvl, "spawn_rate", 30); ctx->globalFallSpeed = getF(lvl, "fall_speed", 5.0f);
    }
    else if (ctx->gameMode == "flappy_heli") {
        ctx->player.Init(100, 300, 40, 40, true, 100); ctx->obstacles.clear(); ctx->obsSpeed = getF(lvl, "obstacle_speed", 5.0f); ctx->obsSpawnRate = getI(lvl, "spawn_rate", 90); ctx->obsTimer = 0;
    }
    else if (ctx->gameMode == "bullet_hell") {
        ctx->player.Init(400, 300, 40, 40, false, 100); ctx->obstacles.clear(); ctx->obsSpeed = getF(lvl, "projectile_speed", 4.0f); ctx->obsSpawnRate = getI(lvl, "spawn_rate", 10); ctx->obsTimer = 0;
    }
    else if (ctx->gameMode == "breakout_pong") {
        ctx->player.Init(350, 550, 100, 20, false, 100); ctx->ball.Init(390, 500, 20, 20, false, 100); ctx->ball.velocityX = 5.0f; ctx->ball.velocityY = -5.0f; ctx->bricks.clear();
        int rows = getI(lvl, "rows", 4); int cols = getI(lvl, "cols", 8); int bw = 80; int bh = 30; int pad = 10; int startX = (800 - (cols*(bw+pad)))/2;
        for(int r=0; r<rows; r++) { for(int c=0; c<cols; c++) { ctx->bricks.push_back({startX + c*(bw+pad), 50 + r*(bh+pad), bw, bh}); } }
    }
    else if (ctx->gameMode == "precision_jump") {
        ctx->player.Init(100, 500, 40, 40, true, 100); ctx->platform.Init(0, 550, 800, 50, false, 9999); ctx->obstacles.clear(); ctx->obsSpeed = getF(lvl, "scroll_speed", 6.0f);
        if (lvl.contains("obstacles") && lvl["obstacles"].is_array()) { for (auto& o : lvl["obstacles"]) { ctx->obstacles.push_back({getF(o, "x", 800.0f), 550.0f - getF(o, "h", 40.0f), ctx->obsSpeed, getI(o, "w", 40), getI(o, "h", 40), true, false}); } }
    }
}

#ifdef __EMSCRIPTEN__
extern "C" { 
    EMSCRIPTEN_KEEPALIVE void LoadDynamicConfig(const char* jsonString) {
        if (!globalCtx) return;
        try {
            json config = json::parse(jsonString);
            if (config.contains("game_mode") && config["game_mode"].is_string()) globalCtx->gameMode = config["game_mode"].get<string>(); else globalCtx->gameMode = "unknown";
            globalCtx->score = 0; 
            
            globalCtx->currentState = MENU; 
            
            globalCtx->levelsConfig.clear();
            if (config.contains("levels") && config["levels"].is_array()) {
                for (auto& l : config["levels"]) globalCtx->levelsConfig.push_back(l);
            } else if (config.contains("questions") && config["questions"].is_array()) {
                for (auto& q : config["questions"]) globalCtx->levelsConfig.push_back(q);
            } else {
                globalCtx->levelsConfig.push_back(config); 
            }
            
            globalCtx->currentLevelIndex = 0;
            globalCtx->levelAdvanceTimer = 0;
            LoadCurrentLevel(globalCtx);

        } catch (...) { cout << "JSON Parse Error: Invalid Schema\n"; }
    }

    EMSCRIPTEN_KEEPALIVE void SetAudioState(int active) {
        if (active == 0) {
            Mix_PauseMusic();
            Mix_Pause(-1); 
        } else {
            Mix_ResumeMusic();
            Mix_Resume(-1);
        }
    }
} 
#endif

void MainLoopStep(void* arg) {
    GameContext* ctx = (GameContext*)arg; 
    
    Uint32 now = SDL_GetTicks(); Uint32 frameTime = now - ctx->lastFrameTicks; ctx->lastFrameTicks = now;
    if (frameTime > 250) frameTime = 250; ctx->accumulator += frameTime;

    if (!HandleInput(ctx)) return;

    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (ctx->currentState == GAME_OVER && keys[SDL_SCANCODE_R]) { 
        ctx->currentState = PLAYING;
#ifdef __EMSCRIPTEN__
        EM_ASM({ document.getElementById('game-over-overlay').classList.add('hidden'); });
#endif
    }

    while (ctx->accumulator >= 16) {
        if (ctx->levelAdvanceTimer > 0) {
            ctx->levelAdvanceTimer--;
            if (ctx->levelAdvanceTimer == 0) { ctx->currentLevelIndex++; LoadCurrentLevel(ctx); }
            UpdateParticles(ctx); 
        } 
        else if (ctx->currentState == PLAYING) {
            if      (ctx->gameMode == "endless_runner") UpdateRunnerMode(ctx);
            else if (ctx->gameMode == "algo_arena")     UpdateAlgoMode(ctx);
            else if (ctx->gameMode == "typing_defense") UpdateTypingMode(ctx);
            else if (ctx->gameMode == "memory_match")   UpdateMemoryMode(ctx);
            else if (ctx->gameMode == "quiz_mcq")       ; 
            else if (ctx->gameMode == "tug_of_war")     UpdateTugOfWarMode(ctx);
            else if (ctx->gameMode == "rhythm_catch")   UpdateRhythmMode(ctx);
            else if (ctx->gameMode == "flappy_heli")    UpdateFlappyMode(ctx);
            else if (ctx->gameMode == "bullet_hell")    UpdateBulletHellMode(ctx);
            else if (ctx->gameMode == "breakout_pong")  UpdateBreakoutMode(ctx);
            else if (ctx->gameMode == "precision_jump") UpdatePrecisionMode(ctx);
        }
        ctx->accumulator -= 16;
    }

    ctx->frames++;
    if (now - ctx->lastTime >= 1000) {
        ctx->fps = ctx->frames; ctx->frames = 0; ctx->lastTime = now;
    }

    if      (ctx->gameMode == "endless_runner") RenderRunnerMode(ctx);
    else if (ctx->gameMode == "algo_arena")     RenderAlgoMode(ctx);
    else if (ctx->gameMode == "typing_defense") RenderTypingMode(ctx);
    else if (ctx->gameMode == "memory_match")   RenderMemoryMode(ctx);
    else if (ctx->gameMode == "quiz_mcq")       RenderQuizMode(ctx);
    else if (ctx->gameMode == "tug_of_war")     RenderTugOfWarMode(ctx);
    else if (ctx->gameMode == "rhythm_catch")   RenderRhythmMode(ctx);
    else if (ctx->gameMode == "flappy_heli")    RenderFlappyMode(ctx);
    else if (ctx->gameMode == "bullet_hell")    RenderBulletHellMode(ctx);
    else if (ctx->gameMode == "breakout_pong")  RenderBreakoutMode(ctx);
    else if (ctx->gameMode == "precision_jump") RenderPrecisionMode(ctx);
    else { SDL_SetRenderDrawColor(ctx->renderer, 20, 20, 30, 255); SDL_RenderClear(ctx->renderer); }
    
    if (ctx->currentState == MENU) RenderMenuOverlay(ctx);
    
    RenderTelemetry(ctx); 
    SDL_RenderPresent(ctx->renderer);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO); TTF_Init(); Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048); 
    SDL_Window* window = SDL_CreateWindow("WASM Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0"); SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    GameContext ctx; ctx.renderer = renderer; ctx.currentState = MENU; ctx.gameIsRunning = true; 
    ctx.lastFrameTicks = SDL_GetTicks(); ctx.accumulator = 0; ctx.bgmStarted = false; globalCtx = &ctx;

    ctx.fontArcade = TTF_OpenFont("assets/font_arcade.ttf", 24);
    ctx.fontQuiz = TTF_OpenFont("assets/font_quiz.ttf", 24);
    ctx.fontFps = TTF_OpenFont("assets/font_fps.ttf", 18);
    
    if (!ctx.fontArcade) ctx.fontArcade = TTF_OpenFont("assets/font.ttf", 24);
    if (!ctx.fontQuiz) ctx.fontQuiz = TTF_OpenFont("assets/font.ttf", 24);
    if (!ctx.fontFps) ctx.fontFps = TTF_OpenFont("assets/font.ttf", 18);
    
    ctx.showFps = true;

    ctx.bgm = Mix_LoadMUS("assets/bgm.wav"); 
    if(!ctx.bgm) ctx.bgm = Mix_LoadMUS("assets/bgm.ogg");
    if(!ctx.bgm) ctx.bgm = Mix_LoadMUS("assets/bgm.mp3");

    ctx.sfxCorrect = Mix_LoadWAV("assets/correct.wav"); ctx.sfxWrong = Mix_LoadWAV("assets/wrong.wav"); ctx.sfxJump = Mix_LoadWAV("assets/jump.wav");

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(MainLoopStep, &ctx, 0, 1);
#else
    while (ctx.gameIsRunning) { MainLoopStep(&ctx); SDL_Delay(16); }
#endif

    Mix_FreeMusic(ctx.bgm); Mix_FreeChunk(ctx.sfxCorrect); Mix_FreeChunk(ctx.sfxWrong); Mix_FreeChunk(ctx.sfxJump); Mix_CloseAudio();
    return 0;
}