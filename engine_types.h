/**
 * @file engine_types.h
 * @author Dhiraj Adhikary (IIT Guwahati)
 * @brief Core data structures and state definitions for the WebAssembly Game Engine.
 * @context TapTap Game Engine Hackathon 2026
 */

#pragma once

#include <string>
#include <vector>
#include <deque>
#include <cmath>
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include "json.hpp"

using json = nlohmann::json;
using namespace std; 

/// @brief Represents the high-level states of the application lifecycle.
enum GameState { MENU, PLAYING, GAME_OVER, VICTORY };

/// @brief Utility for basic Axis-Aligned Bounding Box (AABB) collision detection.
inline bool CheckCollision(SDL_Rect a, SDL_Rect b) {
    return !(a.y + a.h <= b.y || a.y >= b.y + b.h || a.x + a.w <= b.x || a.x >= b.x + b.w);
}

// ==========================================
// GAMEPLAY STRUCTURES
// ==========================================

struct Particle { float x, y, vx, vy; int life, maxLife; SDL_Color color; };
struct FallingWord { float x, y, speed; string text; bool active; SDL_Rect GetRect() { return { (int)x, (int)y, (int)(text.length() * 15), 30 }; } };
struct MemoryCard { SDL_Rect rect; string value; int state; int matchId; };
struct Obstacle { float x, y, speed; int width, height; bool active; bool passedPlayer; SDL_Rect GetRect() { return { (int)x, (int)y, width, height }; } };
struct AlgoEntity { float x, y, speed; string text; bool isCorrect; bool active; SDL_Rect GetRect() { return { (int)x, (int)y, (int)(text.length() * 15), 40 }; } };
struct QuizOption { SDL_Rect rect; string text; bool isCorrect; int state; };

/// @brief Base class for physics-enabled objects (Player, Platform, Projectiles).
class Entity {
public:
    float x, y, velocityX, velocityY; int width, height, hp, maxHp; bool hasGravity, onGround; SDL_Texture* texture;
    Entity() : texture(nullptr) {}
    
    void Init(float sx, float sy, int w, int h, bool grav, int health) {
        x = sx; y = sy; width = w; height = h; hasGravity = grav; hp = health; maxHp = health; velocityX = 0; velocityY = 0; onGround = false;
    }
    
    /// @brief Applies velocity and handles basic floor boundary collisions.
    void UpdatePhysics(float floorY) {
        x += velocityX;
        if (hasGravity) {
            velocityY += 0.8f; if (velocityY > 15.0f) velocityY = 15.0f; y += velocityY;
            if (y + height >= floorY) { y = floorY - height; velocityY = 0.0f; onGround = true; } else { onGround = false; }
        } else {
            y += velocityY; if (y < 0) y = 0; if (y > 600 - height) y = 600 - height;
        }
    }
    SDL_Rect GetRect() { return { (int)x, (int)y, width, height }; }
};

/// @brief God-object containing the entire state of the engine, passed through the WASM loop.
struct GameContext {
    // SDL & Rendering State
    SDL_Renderer* renderer; 
    TTF_Font* fontArcade; 
    TTF_Font* fontQuiz; 
    TTF_Font* fontFps;
    GameState currentState; 
    string gameMode; 
    int score; 
    bool gameIsRunning;
    
    // Audio State
    Mix_Music* bgm; Mix_Chunk* sfxCorrect; Mix_Chunk* sfxWrong; Mix_Chunk* sfxJump; bool bgmStarted; 
    
    // Telemetry & Timing
    Uint32 lastFrameTicks; Uint32 accumulator; Uint32 lastTime; int frames, fps; bool showFps;

    // Level Management (Parsed from JSON)
    vector<json> levelsConfig;
    int currentLevelIndex;
    int levelAdvanceTimer;
    int levelScoreStart;

    // Mode-Specific Data Containers
    Entity player, platform, ball; vector<Particle> particles;
    vector<FallingWord> words; vector<string> vocabulary; string typeBuffer; int wordSpawnTimer; float globalFallSpeed;
    vector<MemoryCard> cards; int firstClickIdx, secondClickIdx, flipTimer;
    vector<Obstacle> obstacles; float obsSpeed; int obsSpawnRate, obsTimer;
    deque<Entity> historyPlayer; deque<vector<Obstacle>> historyObs; bool isRewinding;
    string algoQuestion; vector<AlgoEntity> algoOptions; int algoSpawnTimer;
    string currentQuizQuestion; vector<QuizOption> currentQuizOptions;
    float ropePosition; float aiPullSpeed; float playerPullPower;
    vector<SDL_Rect> bricks; 
    struct RhythmNote { float x, y; bool active; }; vector<RhythmNote> notes; int noteTimer;
};