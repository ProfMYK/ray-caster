#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "definitions.h"

#include "new_steel.ppm"

#define PI 3.1415926535
#define DEGTORAD 0.0174533
#define RADTODEG 57.2957795

#define IS_DEBUG false

#define SCREEN_WIDTH 1440
#define SCREEN_HEIGHT 960

#define UNIT_SIZE 64
#define NUMBER_OF_RAYS 1440
#define FOV 60
#define MAX_DOF 20

#define GAME_HEIGHT 960
#define GAME_WIDTH  1440

ButtonKeys keys;

#define PLAYER_SPEED 300
Vec2 playerPos, playerDelta;
float playerAngle;

int pixels[SCREEN_HEIGHT*SCREEN_WIDTH];

#define MAP_X 20
#define MAP_Y 10
int mapW[MAP_X*MAP_Y] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

uint32_t modify_rgba(uint32_t hex, float delta) {
    // Extract RGBA components from the hex value
    uint8_t r = (hex >> 24) & 0xFF;
    uint8_t g = (hex >> 16) & 0xFF;
    uint8_t b = (hex >> 8) & 0xFF;
    uint8_t a = hex & 0xFF;

    // Lower each component by delta value, ensuring they don't go below 0
    r *= delta;
    g *= delta;
    b *= delta;

    // Combine modified components back into a single hex value
    return (r << 24) | (g << 16) | (b << 8) | a;
}

float fix_angle(float angle) {
    if (angle < 0) return (angle + 2*PI);
    else if (angle > 2*PI) return (angle - 2*PI);
    else return angle;
}

float get_distance(float x1, float y1, float x2, float y2) {
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

void draw_pixel(int x, int y, int c) {
    pixels[(y * SCREEN_WIDTH) + x] = c;
}

void draw_rectangle(int x, int y, int w, int h, int c) {
    for (int _x = x; _x < x + w; _x++) {
        for (int _y = y; _y < y + h; _y++) {
            draw_pixel(_x, _y, c);
        }
    }
}

void draw_ray_3d(SDL_Renderer *renderer) {
    Vec2 ray, offset, dists, hor, ver;
    float rayAngle, distance;
    int dof;

    rayAngle = playerAngle - (FOV/2)*DEGTORAD; rayAngle = fix_angle(rayAngle);
    for (int r = 0; r < NUMBER_OF_RAYS; r++) {
        // Check Horizontal
        dof = 0;
        dists.x = 10000000;
        hor = playerPos;
        float aTan = -1/tan(rayAngle);
        if (rayAngle > PI) {
            ray.y = (((int)playerPos.y / UNIT_SIZE) * UNIT_SIZE) - 0.0001;
            ray.x = (playerPos.y - ray.y) * aTan + playerPos.x;
            offset.y = -UNIT_SIZE;
            offset.x = -offset.y * aTan;
        } if (rayAngle < PI) {
            ray.y = (((int)playerPos.y / UNIT_SIZE) * UNIT_SIZE) + UNIT_SIZE;
            ray.x = (playerPos.y - ray.y) * aTan + playerPos.x;
            offset.y = UNIT_SIZE;
            offset.x = -offset.y * aTan;
        } if (rayAngle == 0 || rayAngle == PI) {
            ray.y = playerPos.y;
            ray.x = playerPos.x;
            dof = MAX_DOF;
        }

        while (dof < MAX_DOF) {
            IVec2 mapPos;
            mapPos.x = (int)ray.x / UNIT_SIZE;
            mapPos.y = (int)ray.y / UNIT_SIZE;
            int mapIndex = mapPos.y*MAP_X + mapPos.x;

            if (mapIndex > 0 && mapIndex < MAP_X*MAP_Y && mapW[mapIndex] > 0) {
                dof = MAX_DOF;
                hor = ray;
                dists.x = get_distance(playerPos.x, playerPos.y, ray.x, ray.y);
            } else {
                ray.x += offset.x;
                ray.y += offset.y;
                dof++;
            }
        }

        // Check Vertical
        dof = 0;
        dists.y = 1000000;
        ver = playerPos;
        float nTan = -tan(rayAngle);
        if (cos(rayAngle) < 0) {
            ray.x = (((int)playerPos.x / UNIT_SIZE) * UNIT_SIZE) - 0.0001;
            ray.y = (playerPos.x - ray.x) * nTan + playerPos.y;
            offset.x = -UNIT_SIZE;
            offset.y = -offset.x * nTan;
        } if (cos(rayAngle) > 0) {
            ray.x = (((int)playerPos.x / UNIT_SIZE) * UNIT_SIZE) + UNIT_SIZE;
            ray.y = (playerPos.x - ray.x) * nTan + playerPos.y;
            offset.x = UNIT_SIZE;
            offset.y = -offset.x * nTan;
        } if (rayAngle == 0 || rayAngle == PI) {
            ray.y = playerPos.y;
            ray.x = playerPos.x;
            dof = MAX_DOF;
        }

        while (dof < MAX_DOF) {
            IVec2 mapPos;
            mapPos.x = (int)ray.x / UNIT_SIZE;
            mapPos.y = (int)ray.y / UNIT_SIZE;
            int mapIndex = mapPos.y*MAP_X + mapPos.x;

            if (mapIndex > 0 && mapIndex < MAP_X*MAP_Y && mapW[mapIndex] > 0) {
                dof = MAX_DOF;
                ver.x = ray.x;
                ver.y = ray.y;
                dists.y = get_distance(playerPos.x, playerPos.y, ray.x, ray.y);
            } else {
                ray.x += offset.x;
                ray.y += offset.y;
                dof++;
            }
        }

        float shade = 1;
        int color = 0xFF00FFFF;
        int *texture = T_wall;
        if (dists.x < dists.y) {
            distance = dists.x;
            ray = hor;
            shade = 1;
        } if (dists.y < dists.x) {
            distance = dists.y;
            ray = ver;
            shade = .9;
        }

        if (IS_DEBUG) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 100);
            SDL_RenderDrawLine(renderer, playerPos.x, playerPos.y, ray.x, ray.y);
        }

        Vec2 tStep = {0,0};
        Vec2 text = {0, 0};
        if (shade == 1) text.x = (int)ray.x % UNIT_SIZE;
        else text.x = (int)ray.y % UNIT_SIZE;
        float lineHeight = (UNIT_SIZE * GAME_HEIGHT) / (distance * cos(playerAngle - rayAngle));
        tStep.y = UNIT_SIZE / lineHeight;
        if (lineHeight > GAME_HEIGHT) {
            text.y = tStep.y * ((lineHeight - GAME_HEIGHT) / 2);
            lineHeight = GAME_HEIGHT;
        }
        float lineOffset = GAME_HEIGHT/2 - lineHeight/2;

        int off = 0;
        if (IS_DEBUG)
            off = 648;
        float lineLength = (float)GAME_WIDTH / (float)NUMBER_OF_RAYS;

        for (int y = 0; y < lineHeight; y++) {
            //color = modify_rgba(T_wall[(int)text.x*UNIT_SIZE + (int)text.y], shade);
            color = texture[(int)text.x*UNIT_SIZE + (int)text.y];
            for (int x = 0; x < lineLength; x++) {
                draw_pixel(r * lineLength + off + x, lineOffset + y, color);
            }
            text.y += tStep.y;
        }

        rayAngle += DEGTORAD*((float)FOV/(float)NUMBER_OF_RAYS); rayAngle = fix_angle(rayAngle);
    }
}

void draw_map_2d(SDL_Renderer *renderer) {
    for (int x = 0; x < MAP_X; x++) {
        for (int y = 0; y < MAP_Y; y++) {
            SDL_Rect box = {x*UNIT_SIZE+1, y*UNIT_SIZE+1, UNIT_SIZE-1, UNIT_SIZE-1};
            if (mapW[y*MAP_X + x] == 1) SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            if (mapW[y*MAP_X + x] == 0) SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(renderer, &box);
        }
    }
}

void draw_player(SDL_Renderer *renderer, int playerSize) {
    SDL_FRect player = {playerPos.x - playerSize/2, playerPos.y - playerSize/2, playerSize, playerSize};
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderFillRectF(renderer, &player);   

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawLineF(renderer, playerPos.x, playerPos.y, playerPos.x+playerDelta.x*1000, playerPos.y+playerDelta.y*1000);
}

void start() {
    playerPos.x = 900;
    playerPos.y = 300;
    playerAngle = 0;
    playerDelta.x = cos(playerAngle);
    playerDelta.y = sin(playerAngle);
}

void update(SDL_Renderer *renderer, double dt) {
    IVec2 mapPos = {playerPos.x / UNIT_SIZE, playerPos.y / UNIT_SIZE}, 
        mapAddPos = {(playerPos.x + 20*playerDelta.x) / UNIT_SIZE, (playerPos.y + 20*playerDelta.y) / UNIT_SIZE}, 
        mapSubPos = {(playerPos.x - 20*playerDelta.x) / UNIT_SIZE, (playerPos.y - 20*playerDelta.y) / UNIT_SIZE}; 

    if (keys.w) {
        if (mapW[mapPos.y*MAP_X + mapAddPos.x] == 0) 
            playerPos.x += playerDelta.x*PLAYER_SPEED*dt;
        if (mapW[mapAddPos.y*MAP_X + mapPos.x] == 0) 
            playerPos.y += playerDelta.y*PLAYER_SPEED*dt;
    } else if (keys.s) {
        if (mapW[mapPos.y*MAP_X + mapSubPos.x] == 0) 
            playerPos.x -= playerDelta.x*PLAYER_SPEED*dt;
        if (mapW[mapSubPos.y*MAP_X + mapPos.x] == 0) 
            playerPos.y -= playerDelta.y*PLAYER_SPEED*dt;
    }

    if (keys.a && !keys.d) {
        playerAngle -= 3*dt; playerAngle = fix_angle(playerAngle);
        playerDelta.x = cos(playerAngle);
        playerDelta.y = sin(playerAngle);
    } 
    if (keys.d && !keys.a) {
        playerAngle += 3*dt; playerAngle = fix_angle(playerAngle);
        playerDelta.x = cos(playerAngle);
        playerDelta.y = sin(playerAngle);
    }

    if (IS_DEBUG) {
        draw_map_2d(renderer);
        draw_player(renderer, 10);
    }

    draw_rectangle(0, 0, GAME_WIDTH, GAME_HEIGHT / 2, 0x555555FF);
    draw_rectangle(0, GAME_HEIGHT / 2, GAME_WIDTH, GAME_HEIGHT / 2, 0x777777FF);

    draw_ray_3d(renderer);
}

int main(int argc, char** argv){
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow("THE COOLEST GAME EVER", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    Uint32 frame1 = SDL_GetPerformanceCounter(), frame2 = 0; 
    double deltaTime = 0;

    start();
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    bool running = true;
    while(running){
        frame2 = frame1;
        frame1 = SDL_GetPerformanceCounter();

        deltaTime = (double)((frame1 - frame2) / (double)SDL_GetPerformanceFrequency() );

        SDL_Event event;
        while(SDL_PollEvent(&event)){
            SDL_KeyCode key = event.key.keysym.sym;
            switch(event.type){
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    if (key == SDLK_w) keys.w = 1;
                    if (key == SDLK_s) keys.s = 1;
                    if (key == SDLK_a) keys.a = 1;
                    if (key == SDLK_d) keys.d = 1;
                    break;
                case SDL_KEYUP:
                    if (key == SDLK_w) keys.w = 0;
                    if (key == SDLK_s) keys.s = 0;
                    if (key == SDLK_a) keys.a = 0;
                    if (key == SDLK_d) keys.d = 0;
                    break;

                default:
                    break;
            }
        }

        SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH*4);
        SDL_RenderCopyEx(renderer, texture, NULL, NULL, 0.0, NULL, SDL_FLIP_NONE);

        update(renderer, deltaTime);

        SDL_RenderPresent(renderer);
    }

    return 0;
}