/**
 * @file renderer.h
 * @author Dhiraj Adhikary (IIT Guwahati)
 * @brief Handles all SDL2 drawing routines for UI, particles, and mode-specific scenes.
 */

#pragma once
#include "engine_types.h"

using namespace std;

// ==========================================
// RENDER HELPERS
// ==========================================

/// @brief Dynamically selects font based on the active JSON payload game mode.
inline TTF_Font* GetModeFont(GameContext* ctx) {
    if (ctx->gameMode == "quiz_mcq" || ctx->gameMode == "algo_arena") return ctx->fontQuiz;
    return ctx->fontArcade;
}

/// @brief Draws solid or outline rectangles.
inline void DrawRectTracker(GameContext* ctx, SDL_Rect* rect, bool filled) {
    if (filled) SDL_RenderFillRect(ctx->renderer, rect); else SDL_RenderDrawRect(ctx->renderer, rect); 
}

/// @brief Processes alpha-blended particle rendering.
inline void RenderParticles(GameContext* ctx) {
    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);
    for (auto& p : ctx->particles) {
        Uint8 alpha = (Uint8)((p.life / (float)p.maxLife) * 255.0f); SDL_SetRenderDrawColor(ctx->renderer, p.color.r, p.color.g, p.color.b, alpha);
        SDL_Rect pr = { (int)p.x, (int)p.y, 6, 6 }; SDL_RenderFillRect(ctx->renderer, &pr); 
    }
    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_NONE);
}

/// @brief Renders standard text at specific coordinates.
inline void RenderText(GameContext* ctx, const string& text, int x, int y, SDL_Color color) {
    if (text.empty()) return; SDL_Surface* surf = TTF_RenderText_Solid(GetModeFont(ctx), text.c_str(), color); if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ctx->renderer, surf); SDL_Rect rect = { x, y, surf->w, surf->h };
    SDL_RenderCopy(ctx->renderer, tex, NULL, &rect); SDL_DestroyTexture(tex); SDL_FreeSurface(surf); 
}

/// @brief Renders and scales text to fit within a specific bounding box.
inline void RenderTextBounded(GameContext* ctx, const string& text, SDL_Rect bounds, SDL_Color color) {
    if (text.empty()) return; SDL_Surface* surf = TTF_RenderText_Solid(GetModeFont(ctx), text.c_str(), color); if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ctx->renderer, surf);
    int w = surf->w; int h = surf->h; int maxW = bounds.w - 10; int maxH = bounds.h - 10;
    if (w > maxW) { float scale = (float)maxW / (float)w; w = maxW; h = (int)(h * scale); }
    if (h > maxH) { float scale = (float)maxH / (float)h; h = maxH; w = (int)(w * scale); }
    SDL_Rect dest = { bounds.x + (bounds.w - w)/2, bounds.y + (bounds.h - h)/2, w, h };
    SDL_RenderCopy(ctx->renderer, tex, NULL, &dest); SDL_DestroyTexture(tex); SDL_FreeSurface(surf); 
}

// ==========================================
// UI / OVERLAYS
// ==========================================

inline void RenderMenuOverlay(GameContext* ctx) {
    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ctx->renderer, 5, 7, 10, 200);
    SDL_Rect bg = {0,0,800,600}; SDL_RenderFillRect(ctx->renderer, &bg);
    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_NONE);
    RenderTextBounded(ctx, "SYSTEM READY", {200, 200, 400, 80}, {0, 229, 255, 255});
    RenderTextBounded(ctx, "PRESS ANY KEY OR CLICK TO INITIATE", {150, 350, 500, 40}, {255, 255, 255, 255});
}

inline void RenderGlobalOverlays(GameContext* ctx) {
    SDL_Color white = {255, 255, 255, 255};
    RenderText(ctx, "Level: " + to_string(ctx->currentLevelIndex + 1) + " / " + to_string(ctx->levelsConfig.size()), 10, 10, {0, 255, 255, 255});
    if (ctx->levelAdvanceTimer > 0) {
        SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 150);
        SDL_Rect bg = {0,0,800,600}; SDL_RenderFillRect(ctx->renderer, &bg);
        SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_NONE);
        RenderTextBounded(ctx, "LEVEL CLEAR", {200, 200, 400, 200}, {50, 255, 50, 255});
    }
}

inline void RenderTelemetry(GameContext* ctx) {
    if (!ctx->showFps) return;
    SDL_Color yellow = { 255, 255, 0, 255 }; 
    string text = "FPS: " + to_string(ctx->fps);
    SDL_Surface* surf = TTF_RenderText_Solid(ctx->fontFps, text.c_str(), yellow);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ctx->renderer, surf);
    SDL_Rect dest = { 790 - surf->w, 10, surf->w, surf->h };
    SDL_RenderCopy(ctx->renderer, tex, NULL, &dest);
    SDL_DestroyTexture(tex); SDL_FreeSurface(surf);
}

// ==========================================
// SCENE RENDERERS (MODE SPECIFIC)
// ==========================================

inline void RenderRunnerMode(GameContext* ctx) {
    if (ctx->isRewinding) SDL_SetRenderDrawColor(ctx->renderer, 50, 20, 50, 255); else SDL_SetRenderDrawColor(ctx->renderer, 20, 20, 30, 255); SDL_RenderClear(ctx->renderer);
    SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, 255); SDL_Rect plat = ctx->platform.GetRect(); DrawRectTracker(ctx, &plat, true);
    SDL_SetRenderDrawColor(ctx->renderer, 0, 255, 100, 255); SDL_Rect pRect = ctx->player.GetRect(); DrawRectTracker(ctx, &pRect, true);
    SDL_SetRenderDrawColor(ctx->renderer, 255, 50, 50, 255);
    for (auto& obs : ctx->obstacles) { if (obs.active) { SDL_Rect r = obs.GetRect(); DrawRectTracker(ctx, &r, true); } }
    RenderParticles(ctx); SDL_Color white = { 255, 255, 255, 255 }; RenderText(ctx, "Score: " + to_string(ctx->score), 10, 40, white);
    if (ctx->isRewinding) RenderText(ctx, "<< REWINDING TIME <<", 300, 100, {255, 100, 255, 255});
    RenderGlobalOverlays(ctx);
}

inline void RenderAlgoMode(GameContext* ctx) {
    SDL_SetRenderDrawColor(ctx->renderer, 20, 20, 30, 255); SDL_RenderClear(ctx->renderer);
    SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, 255); SDL_Rect plat = ctx->platform.GetRect(); DrawRectTracker(ctx, &plat, true);
    SDL_SetRenderDrawColor(ctx->renderer, 50, 150, 255, 255); SDL_Rect pRect = ctx->player.GetRect(); DrawRectTracker(ctx, &pRect, true);
    SDL_Color white = {255, 255, 255, 255}; RenderText(ctx, "Q: " + ctx->algoQuestion, 10, 40, {255, 255, 0, 255});
    for (auto& opt : ctx->algoOptions) { if (opt.active) { SDL_SetRenderDrawColor(ctx->renderer, 200, 50, 50, 255); SDL_Rect r = opt.GetRect(); DrawRectTracker(ctx, &r, true); RenderTextBounded(ctx, opt.text, r, white); } }
    RenderParticles(ctx); RenderGlobalOverlays(ctx);
}

inline void RenderQuizMode(GameContext* ctx) {
    SDL_SetRenderDrawColor(ctx->renderer, 20, 20, 30, 255); SDL_RenderClear(ctx->renderer); SDL_Color white = {255, 255, 255, 255}; 
    RenderTextBounded(ctx, ctx->currentQuizQuestion, {50, 40, 700, 80}, white);
    for (auto& opt : ctx->currentQuizOptions) {
        if (opt.state == 0) SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 200, 255); else if (opt.state == 1) SDL_SetRenderDrawColor(ctx->renderer, 200, 50, 50, 255); else if (opt.state == 2) SDL_SetRenderDrawColor(ctx->renderer, 50, 200, 50, 255);
        SDL_Rect r = opt.rect; DrawRectTracker(ctx, &r, true); RenderTextBounded(ctx, opt.text, r, white);
    }
    RenderParticles(ctx); RenderText(ctx, "Score: " + to_string(ctx->score), 10, 560, white); RenderGlobalOverlays(ctx);
}

inline void RenderTypingMode(GameContext* ctx) {
    SDL_SetRenderDrawColor(ctx->renderer, 20, 20, 30, 255); SDL_RenderClear(ctx->renderer); SDL_Color white = { 255, 255, 255, 255 }; SDL_Color red = { 255,  50,  50, 255 };
    SDL_SetRenderDrawColor(ctx->renderer, 50, 200, 50, 255); SDL_Rect base = { 0, 550, 800, 50 }; DrawRectTracker(ctx, &base, true);
    for (auto& w : ctx->words) { if (w.active) RenderText(ctx, w.text, (int)w.x, (int)w.y, white); }
    RenderParticles(ctx); RenderText(ctx, "Type: " + ctx->typeBuffer, 300, 560, red); RenderText(ctx, "City HP: " + to_string(ctx->player.hp), 10, 40, white); RenderText(ctx, "Score: " + to_string(ctx->score), 10, 70, white);
    RenderGlobalOverlays(ctx);
}

inline void RenderMemoryMode(GameContext* ctx) {
    SDL_SetRenderDrawColor(ctx->renderer, 20, 20, 30, 255); SDL_RenderClear(ctx->renderer); SDL_Color black = { 0, 0, 0, 255 }; SDL_Color white = { 255, 255, 255, 255 };
    for (auto& c : ctx->cards) {
        switch (c.state) {
            case 0:  SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 250, 255); break; 
            case 1:  SDL_SetRenderDrawColor(ctx->renderer, 250, 250, 250, 255); break; 
            default: SDL_SetRenderDrawColor(ctx->renderer,  50, 200,  50, 255); break; 
        }
        SDL_RenderFillRect(ctx->renderer, &c.rect); SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 255); SDL_RenderDrawRect(ctx->renderer, &c.rect);
        if (c.state > 0) RenderTextBounded(ctx, c.value, c.rect, black);
    }
    RenderParticles(ctx); RenderText(ctx, "Score: " + to_string(ctx->score), 10, 40, white); RenderGlobalOverlays(ctx);
}

inline void RenderTugOfWarMode(GameContext* ctx) {
    SDL_SetRenderDrawColor(ctx->renderer, 20, 20, 30, 255); SDL_RenderClear(ctx->renderer); SDL_Color white = { 255, 255, 255, 255 };
    SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, 255); SDL_Rect centerLine = {398, 100, 4, 400}; SDL_RenderFillRect(ctx->renderer, &centerLine);
    SDL_SetRenderDrawColor(ctx->renderer, 200, 200, 150, 255); SDL_Rect rope = {200, 290, 400, 20}; SDL_RenderFillRect(ctx->renderer, &rope);
    SDL_SetRenderDrawColor(ctx->renderer, 255, 50, 50, 255); SDL_Rect marker = {(int)ctx->ropePosition - 10, 270, 20, 60}; SDL_RenderFillRect(ctx->renderer, &marker);
    SDL_SetRenderDrawColor(ctx->renderer, 50, 255, 50, 255); SDL_Rect leftBound = {190, 270, 10, 60}; SDL_RenderFillRect(ctx->renderer, &leftBound);
    SDL_SetRenderDrawColor(ctx->renderer, 255, 50, 50, 255); SDL_Rect rightBound = {600, 270, 10, 60}; SDL_RenderFillRect(ctx->renderer, &rightBound);
    RenderParticles(ctx); RenderTextBounded(ctx, "SPAM SPACEBAR!", {200, 50, 400, 50}, white); RenderText(ctx, "PLAYER", 80, 285, {50, 255, 50, 255}); RenderText(ctx, "AI", 630, 285, {255, 50, 50, 255}); RenderText(ctx, "Score: " + to_string(ctx->score), 10, 40, white);
    RenderGlobalOverlays(ctx);
}

inline void RenderRhythmMode(GameContext* ctx) {
    SDL_SetRenderDrawColor(ctx->renderer, 20, 20, 30, 255); SDL_RenderClear(ctx->renderer); SDL_Color white = {255,255,255,255};
    SDL_SetRenderDrawColor(ctx->renderer, 50, 50, 50, 255);
    for(int i=0; i<4; i++) { SDL_Rect lane = {250 + i*100 - 25, 0, 50, 600}; SDL_RenderFillRect(ctx->renderer, &lane); }
    SDL_SetRenderDrawColor(ctx->renderer, 255, 0, 255, 255); SDL_Rect hitLine = {200, 500, 400, 10}; SDL_RenderFillRect(ctx->renderer, &hitLine);
    SDL_SetRenderDrawColor(ctx->renderer, 0, 255, 255, 255);
    for(auto& n : ctx->notes) { if(n.active) { SDL_Rect nr = {(int)n.x - 25, (int)n.y, 50, 20}; SDL_RenderFillRect(ctx->renderer, &nr); } }
    RenderParticles(ctx); RenderText(ctx, "D", 240, 520, white); RenderText(ctx, "F", 340, 520, white); RenderText(ctx, "J", 440, 520, white); RenderText(ctx, "K", 540, 520, white);
    RenderText(ctx, "HP: " + to_string(ctx->player.hp), 10, 40, white); RenderText(ctx, "Score: " + to_string(ctx->score), 10, 70, white); RenderGlobalOverlays(ctx);
}

inline void RenderFlappyMode(GameContext* ctx) {
    SDL_SetRenderDrawColor(ctx->renderer, 100, 150, 255, 255); SDL_RenderClear(ctx->renderer);
    SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 50, 255); SDL_Rect pRect = ctx->player.GetRect(); DrawRectTracker(ctx, &pRect, true);
    SDL_SetRenderDrawColor(ctx->renderer, 50, 200, 50, 255);
    for (auto& obs : ctx->obstacles) { if (obs.active) { SDL_Rect r = obs.GetRect(); DrawRectTracker(ctx, &r, true); } }
    RenderParticles(ctx); RenderText(ctx, "Score: " + to_string(ctx->score), 10, 40, {255,255,255,255}); RenderGlobalOverlays(ctx);
}

inline void RenderBulletHellMode(GameContext* ctx) {
    SDL_SetRenderDrawColor(ctx->renderer, 10, 10, 20, 255); SDL_RenderClear(ctx->renderer);
    SDL_SetRenderDrawColor(ctx->renderer, 100, 200, 255, 255); SDL_Rect pRect = ctx->player.GetRect(); DrawRectTracker(ctx, &pRect, true);
    SDL_SetRenderDrawColor(ctx->renderer, 255, 50, 50, 255);
    for (auto& obs : ctx->obstacles) { if (obs.active) { SDL_Rect r = obs.GetRect(); DrawRectTracker(ctx, &r, true); } }
    RenderParticles(ctx); RenderText(ctx, "Score: " + to_string(ctx->score), 10, 40, {255,255,255,255}); RenderGlobalOverlays(ctx);
}

inline void RenderBreakoutMode(GameContext* ctx) {
    SDL_SetRenderDrawColor(ctx->renderer, 20, 20, 30, 255); SDL_RenderClear(ctx->renderer);
    SDL_SetRenderDrawColor(ctx->renderer, 100, 255, 100, 255); SDL_Rect pRect = ctx->player.GetRect(); DrawRectTracker(ctx, &pRect, true);
    SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 255); SDL_Rect bRect = ctx->ball.GetRect(); DrawRectTracker(ctx, &bRect, true);
    SDL_SetRenderDrawColor(ctx->renderer, 255, 150, 50, 255);
    for(auto& b : ctx->bricks) { SDL_RenderFillRect(ctx->renderer, &b); SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255); SDL_RenderDrawRect(ctx->renderer, &b); SDL_SetRenderDrawColor(ctx->renderer, 255, 150, 50, 255); }
    RenderParticles(ctx); RenderText(ctx, "Score: " + to_string(ctx->score), 10, 40, {255,255,255,255}); RenderGlobalOverlays(ctx);
}

inline void RenderPrecisionMode(GameContext* ctx) {
    SDL_SetRenderDrawColor(ctx->renderer, 30, 20, 40, 255); SDL_RenderClear(ctx->renderer);
    SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, 255); SDL_Rect plat = ctx->platform.GetRect(); DrawRectTracker(ctx, &plat, true);
    SDL_SetRenderDrawColor(ctx->renderer, 0, 255, 200, 255); SDL_Rect pRect = ctx->player.GetRect(); DrawRectTracker(ctx, &pRect, true);
    SDL_SetRenderDrawColor(ctx->renderer, 200, 50, 150, 255);
    for (auto& obs : ctx->obstacles) { if (obs.active) { SDL_Rect r = obs.GetRect(); DrawRectTracker(ctx, &r, true); } }
    RenderParticles(ctx); RenderText(ctx, "Progress: " + to_string(ctx->score), 10, 40, {255,255,255,255}); RenderGlobalOverlays(ctx);
}