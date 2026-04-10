# Spectrum Game Engine

**Developed by:** Dhiraj Adhikary and Samyak Anil Gaikwad  
**Event:** TapTap Game Engine Hackathon 2026

## Overview
Spectrum Game Engine is a C++-based WebAssembly (WASM) game engine designed to run natively in the browser. Instead of hardcoding game logic, the engine's core architecture dynamically reconfigures itself at runtime using JSON payloads. It integrates a generative AI pipeline that allows users to type text prompts to generate fully playable games, which are then compiled on the fly without reloading the webpage.

## Features
* **WebAssembly Execution:** C++ core compiled to WASM via Emscripten for high-performance, low-latency browser execution.
* **Dynamic JSON Architecture:** The engine parses JSON schemas to instantiate entities, physics rules, and win/loss states dynamically.
* **Generative AI Integration:** Uses the Groq API (Llama-3.3-70b-versatile) to translate user text prompts into valid game configuration payloads.
* **11 Playable Engine Modes:** * `quiz_mcq` (Multiple Choice Questions)
    * `endless_runner` (Obstacle avoidance)
    * `algo_arena` (Falling algorithms challenge)
    * `typing_defense` (Type-to-destroy)
    * `memory_match` (Card flipping)
    * `tug_of_war` (Spacebar spam against AI)
    * `rhythm_catch` (4-lane rhythm game)
    * `flappy_heli` (Gravity-based navigation)
    * `bullet_hell` (Omnidirectional projectile evasion)
    * `breakout_pong` (Classic brick breaker)
    * `precision_jump` (Platforming)
* **Firebase Authentication:** Implements a custom UI for secure Email/Password login and a Synthetic Email system for returning Guest IDs.
* **Cloud Leaderboards:** Integrates Firebase Firestore to upload scores, track global leaderboards per game, and share AI-generated game payloads to the cloud.
* **C++/JavaScript Interop:** Uses Emscripten `ccall` to pass data between the web DOM (AI, Auth) and the C++ memory space (Engine config, Audio muting).
* **Custom SDL Graphics & Audio:** Utilizes SDL2, SDL2_ttf, and SDL2_mixer for rendering, particle systems, fonts, and volume-controlled background music/SFX.

## File Structure
* `engine.cpp` - Main C++ execution loop, SDL initialization, and WASM bindings.
* `engine_types.h` - Core data structures, entity physics classes, and global context variables.
* `game_modes.h` - Logic controllers and input handling for all 11 game modes.
* `renderer.h` - SDL2 rendering pipelines for UI, particles, and mode-specific scenes.
* `json.hpp` - Nlohmann's JSON library for C++.
* `index.html` - The frontend hub, AI prompt interface, Firebase configuration, and WASM canvas slot.
* `config.js` - External, git-ignored configuration file for API keys.
* `assets/` - Directory containing fonts (`.ttf`) and audio (`.wav`, `.ogg`, `.mp3`).

## Setup and Configuration
To run this project locally or fork it, you must provide your own API keys. 

1. Create a `config.js` file in the root directory.
2. Add the following structure and insert your keys:

```javascript
const CONFIG = {
    GROQ_API_KEY: "your_groq_api_key_here",
    FIREBASE: {
        apiKey: "your_firebase_api_key",
        authDomain: "your_auth_domain",
        projectId: "your_project_id",
        storageBucket: "your_storage_bucket",
        messagingSenderId: "your_sender_id",
        appId: "your_app_id"
    }
};
```
*Note: Ensure `config.js` is added to your `.gitignore`.*

## Compilation Instructions
To compile the C++ source files into WebAssembly, you must have the [Emscripten SDK (emsdk)](https://emscripten.org/docs/getting_started/downloads.html) installed and activated on your system.

Run the following command in your terminal from the project root:

```bash
emcc engine.cpp -o index.js -O3 \
  -s USE_SDL=2 \
  -s USE_SDL_IMAGE=2 \
  -s USE_SDL_TTF=2 \
  -s USE_SDL_MIXER=2 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
  -s EXPORTED_FUNCTIONS='["_main", "_LoadDynamicConfig", "_SetAudioState"]' \
  --preload-file assets
```

### Emscripten Flags Explained:
* `-O3`: Aggressive optimization for performance and file size.
* `-s USE_SDL=2 ...`: Links the necessary SDL2 web ports.
* `-s ALLOW_MEMORY_GROWTH=1`: Allows the WASM heap to expand dynamically if the game requires more RAM.
* `-s EXPORTED_FUNCTIONS`: Exposes the C++ functions so they can be triggered by JavaScript button clicks.
* `--preload-file assets`: Packages the `assets` folder into a virtual file system (`index.data`) so the WASM binary can read fonts and audio.

## Running Locally
Because browsers block fetching local files (CORS policy), you cannot just double-click `index.html`. You must serve the directory using a local web server.

If you have Python installed, run:
```bash
python -m http.server 8000
```
Then navigate to `http://localhost:8000` in your web browser.
