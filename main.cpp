#include <iostream>
#include <cmath>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "vector.h"

const int WIDTH=640, HEIGHT=360;
const float F_DELTA_TIME = 16.0/1000.0;
static SDL_Window *window = nullptr;
static SDL_Renderer *renderer = nullptr;

void sdl_init() {
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow("Hello", 
                                          SDL_WINDOWPOS_UNDEFINED, 
                                          SDL_WINDOWPOS_UNDEFINED,
                                          WIDTH,
                                          HEIGHT,
                                          SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        std::cout << "Could not create window, error " << SDL_GetError() << std::endl;
        abort();
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cout << "Could not create renderer, error " << SDL_GetError() << std::endl;
        abort();
    }

    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
}

void sdl_teardown(SDL_Window *window) {
    SDL_DestroyWindow(window);
    SDL_Quit();
}

SDL_Texture *sdl_load_texture(char *filename)
{
    SDL_Texture* newTexture = nullptr;
    newTexture = IMG_LoadTexture(renderer, filename);

    return newTexture;
}

void sdl_blit(SDL_Texture *texture, int x, int y) {
	SDL_Rect dest;

	dest.x = x;
	dest.y = y;
	SDL_QueryTexture(texture, NULL, NULL, &dest.w, &dest.h);

	SDL_RenderCopy(renderer, texture, NULL, &dest);
}

enum MouseState {
    up,
    short_hold_release,
    long_hold_release,
    hold
};

struct MOUSEINFO {
    MouseState state = MouseState::up;
    uint32_t timestamp = 0;
    int press_x, press_y;
    int x, y;

    const uint32_t PRESS_TIME = 10*16; // 10 frames
};

class Player {
    public:
        enum PlayerState {
            airborne,
            grounded
        };
        enum TetherState {
            tethered,
            untethered
        };
        PlayerState player_state;
        TetherState tether_state;

        float x, y, x_vel, y_vel;
        int target_x;

        SDL_Texture *player_texture;

        Player() {
            Initialize();
        }

        void Initialize() {
            PlayerState player_state = PlayerState::airborne;
            TetherState tether_state = TetherState::untethered;
            
            x = 10; 
            y = 0;
            x_vel = 0;
            y_vel = 0;

            player_texture = sdl_load_texture((char *)"resource/yellow.png");
        }

        void Update(MOUSEINFO mouse_info) {
            switch (player_state) {
                case PlayerState::airborne:
                    if (y + y_vel < HEIGHT - 16) {
                        y_vel += 10.0 * F_DELTA_TIME;
                    } else {
                        y_vel = 0;
                        y = HEIGHT - 16;
                        player_state = PlayerState::grounded;
                        break;
                    }
                    break;
                case PlayerState::grounded:
                    if (mouse_info.state == MouseState::hold) {
                        int mx, my;
                        SDL_GetMouseState(&mx, &my);
                        float time_since_hold_start = (float)(SDL_GetTicks() - mouse_info.timestamp);
                        // time_since_hold_start = (time_since_hold_start > 1000) ? 1000 : time_since_hold_start;
                        float f_sign = mx > x ? 1.0 : -1.0;
                        x_vel = f_sign * (4.0 / (1.0 + exp(-time_since_hold_start / 1000.0 * 2.0)));
                        // x += (target_x - x) * 0.1;
                    }
                    break;
                default:
                    break;
            }
            x += x_vel;
            y += y_vel;
            switch (tether_state) {
                case TetherState::untethered:
                    if (mouse_info.state == MouseState::short_hold_release) {
                        tether_state = TetherState::tethered;
                    }
                    break;
                case TetherState::tethered:
                    if (mouse_info.state == MouseState::short_hold_release) {
                        tether_state = TetherState::untethered;
                    }
                    break;
            }
        }

        void Draw() {
            sdl_blit(player_texture, (int)x, (int)y);
        }
};

class Rope {
    public:
        Vector2D v_position_, v_velocity_, v_acceleration_;
        Vector2D v_fixed_point_ {100.0, 100.0};
        float k_ = 100;
        float distance_;
        SDL_Texture *node_texture;

        Rope() {
            Initialize();
        }
        void Initialize() {
            node_texture = sdl_load_texture((char *)"resource/yellow.png");
            v_position_.x = 200.0;
            v_position_.y = 200.0;
            distance_ = (v_fixed_point_ - v_position_).Magnitude();
        }
        void Update(MOUSEINFO mouse_info) {
            Vector2D v_displacement = v_fixed_point_ - v_position_;
            Vector2D v_displacement_dir = v_displacement.Normalized();
            float x = v_displacement.Magnitude() - distance_;

            Vector2D v_gravity { 0.0, 10.0 };
            v_acceleration_ = v_displacement_dir * k_ * x + v_gravity;
            v_velocity_ += v_acceleration_ * F_DELTA_TIME;

            v_position_ += v_velocity_;
        }
        void Draw() {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            SDL_RenderDrawLine(
                renderer,  
                (int)v_position_.x + 8, 
                (int)v_position_.y + 8,
                (int)v_fixed_point_.x,
                (int)v_fixed_point_.y               
            );
            sdl_blit(node_texture, (int)v_position_.x, (int)v_position_.y);
        }
};

class Multispring {
    public:
        const static int N_ = 10;
        float friction_constant_ = 0.25;
        float length_ = 0.25;

        Vector2D v_positions_[N_];
        Vector2D v_velocities_[N_];
        Vector2D v_accelerations_[N_];
        Vector2D v_forces_[N_];
        float masses_[N_]; //  = {1.0, 1.0, 1.0};
        float k_ = 1;
        float distance_;
        SDL_Texture *node_texture;

        Multispring() {
            Initialize();
        }
        void Initialize() {
            node_texture = sdl_load_texture((char *)"resource/yellow.png");
            v_positions_[N_ - 1].x = 100.0;
            v_positions_[N_ - 1].y = 100.0;
            for (int m = 0; m < N_; m++) masses_[m] = 1.0;
            masses_[0] = 2.0;
            for (int v = 2; v <= N_; v++) {
                v_positions_[N_ - v].x = 100.0 + 2.0 * (v - 1) * length_;
                v_positions_[N_ - v].y = 100.0 + (v - 1) * length_;
            }
            distance_ = (v_positions_[N_ - 1] - v_positions_[N_ - 2]).Magnitude();
        }
        void Update(MOUSEINFO mouse_info) {
            v_positions_[N_ - 1].x = mouse_info.x;
            v_positions_[N_ - 1].y = mouse_info.y;

            // N_ - 1 is reserved for fixed point
            for (int i = 0; i < N_ - 1; i++) {
                Vector2D v_displacement = v_positions_[i + 1] - v_positions_[i];
                Vector2D v_displacement_dir = v_displacement.Normalized();
                float x = v_displacement.Magnitude() - distance_;

                Vector2D v_gravity { 0.0, 10.0 };
                Vector2D prev_force = { 0.0, 0.0 };
                Vector2D v_displacement_prev = { 0.0, 0.0 };
                Vector2D v_displacement_prev_dir = { 0.0, 0.0 };
                float x_prev = 0;
                if (i > 0) {
                    v_displacement_prev = v_positions_[i - 1] - v_positions_[i];
                    v_displacement_prev_dir = v_displacement_prev.Normalized();
                    x_prev = v_displacement_prev.Magnitude() - distance_;
                    if (x_prev < 0.0) x_prev = 0.0;
                    v_forces_[i] = v_displacement_prev_dir * k_ * x_prev + v_displacement_dir * k_ * x + v_gravity * masses_[i];
                } else {
                    if (x < 0.0) x = 0.0;
                    v_forces_[i] = v_displacement_dir * k_ * x + v_gravity * masses_[i];
                }
                v_forces_[i] -=  v_velocities_[i] * friction_constant_;
                v_accelerations_[i] = v_forces_[i] / masses_[i];
                v_velocities_[i] += v_accelerations_[i] * F_DELTA_TIME;
            }
            for (int j = 0; j < N_ - 1; j++) {
                v_positions_[j] += v_velocities_[j];
            }
        }
        void Draw() {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            for (int i = 0; i < N_ - 1; i++) {
                SDL_RenderDrawLine(
                    renderer,  
                    (int)v_positions_[i].x, 
                    (int)v_positions_[i].y,
                    (int)v_positions_[i + 1].x,
                    (int)v_positions_[i + 1].y               
                );
            }
            sdl_blit(node_texture, (int)v_positions_[0].x, (int)v_positions_[0].y);
        }
};

class ConstrainedRope {
    public:
        const static int N_ = 10;
        const static int passes_ = 5;
        float length_ = 10;

        Vector2D v_positions_[N_];
        Vector2D v_positions_prev_[N_];
        Vector2D v_velocities_[N_];
        Vector2D v_accelerations_[N_];
        Vector2D v_forces_[N_];
        float masses_[N_]; //  = {1.0, 1.0, 1.0};
        float k_ = 1;
        float distance_;
        SDL_Texture *node_texture;

        ConstrainedRope() {
            Initialize();
        }
        void Initialize() {
            node_texture = sdl_load_texture((char *)"resource/yellow.png");
            v_positions_[N_ - 1].x = 100.0;
            v_positions_[N_ - 1].y = 100.0;
            for (int m = 0; m < N_; m++) masses_[m] = 1.0;
            masses_[0] = 2.0;
            for (int v = 2; v <= N_; v++) {
                v_positions_[N_ - v].x = 100.0 + 2.0 * (v - 1) * length_;
                v_positions_[N_ - v].y = 100.0 + (v - 1) * length_;
            }
            distance_ = (v_positions_[N_ - 1] - v_positions_[N_ - 2]).Magnitude();
        }
        void Update(MOUSEINFO mouse_info) {
            v_positions_[N_ - 1].x = mouse_info.x * 2.0;
            v_positions_[N_ - 1].y = mouse_info.y * 2.0;

            // N_ - 1 is reserved for fixed point
            for (int i = 0; i < N_ - 1; i++) {
                Vector2D v_gravity { 0.0, 10.0 };
                v_forces_[i] = v_gravity * masses_[i];
                v_accelerations_[i] = v_forces_[i] / masses_[i];
                v_velocities_[i] += v_accelerations_[i] * F_DELTA_TIME;
                v_positions_prev_[i] = v_positions_[i];
                v_positions_[i] += v_velocities_[i];
            }
            for (int p = 0; p < passes_; ++p) {
                RelaxConstraints(0, 1, 0.1, 0.9);
                for (int i = 1; i < N_ - 2; ++i) {
                    RelaxConstraints(i, i + 1, 0.5, 0.5);
                }
                RelaxConstraints(N_ - 2, N_ - 1, 1.0, 0.0);
            }
            for (int j = 0; j < N_ - 1; j++) {
                v_velocities_[j] = v_positions_[j] - v_positions_prev_[j];
            }
        }
        void RelaxConstraints(int m, int n, float m_share, float n_share) {
            Vector2D v_displacement = v_positions_[n] - v_positions_[m];
            Vector2D v_displacement_dir = v_displacement.Normalized();
            float delta_d = v_displacement.Magnitude() - distance_;

            v_positions_[m] += v_displacement_dir * delta_d * m_share;
            v_positions_[n] -= v_displacement_dir * delta_d * n_share;
        }
        void Draw() {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            for (int i = 0; i < N_ - 1; i++) {
                SDL_RenderDrawLine(
                    renderer,  
                    (int)v_positions_[i].x, 
                    (int)v_positions_[i].y,
                    (int)v_positions_[i + 1].x,
                    (int)v_positions_[i + 1].y               
                );
            }
            sdl_blit(node_texture, (int)v_positions_[0].x, (int)v_positions_[0].y);
        }
};

int main(int argc, char *argv[]) {
    sdl_init();

    MOUSEINFO mouse_info;
    Player player;
    Rope rope;
    Multispring multispring;
    ConstrainedRope constrope;

    // Game loop
    while (true) {
        // Event handler
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            if (SDL_QUIT == event.type) {
                break;
            } else if (SDL_MOUSEBUTTONDOWN == event.type) {
                if (SDL_BUTTON_LEFT == event.button.button) {
                    mouse_info.state = MouseState::hold;
                    mouse_info.timestamp = event.button.timestamp;
                    mouse_info.press_x = event.button.x;
                    mouse_info.press_y = event.button.y;
                }
            } else if (SDL_MOUSEBUTTONUP == event.type) {
                if (SDL_BUTTON_LEFT == event.button.button) { 
                    if (event.button.timestamp - mouse_info.timestamp > mouse_info.PRESS_TIME) {
                        mouse_info.state = MouseState::long_hold_release;
                    } else {
                        mouse_info.state = MouseState::short_hold_release;
                    }
                }
            } 
        }
        SDL_GetMouseState(&mouse_info.x, &mouse_info.y);

        constrope.Update(mouse_info);

        if (mouse_info.state == MouseState::long_hold_release || 
            mouse_info.state == MouseState::short_hold_release) {
            mouse_info.state = MouseState::up;
        }

        SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
        SDL_RenderClear(renderer);
        constrope.Draw();
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    sdl_teardown(window);

    return EXIT_SUCCESS;
}