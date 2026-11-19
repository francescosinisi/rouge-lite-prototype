#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "rlights.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

enum GamePhase {
    TURN_ORDER_DEFINITION,
    PLAY
};

typedef struct {
    Model model;
    char* name;
    Transform transform;
    size_t team;
    size_t initiative;
} Entity;

typedef struct {
    Entity *items;
    size_t count;
    size_t capacity;
} Entities;

typedef struct {
    int *items;
    size_t count;
    size_t capacity;
} EntitiesReferences;

typedef struct {
    Camera3D camera;
    Ray pickingRay;
    Entities entities;
    EntitiesReferences playableEntities;
    size_t target;

    enum GamePhase phase;
    Entities scene;


} GameState;

void game_init(GameState* state);
void game_update(GameState* state);
void game_shutdown(GameState* state);

int AddEntity(Entities* entities, Entity entity);
void AddEntityReference(EntitiesReferences* entities, int entityIndex);
void InitializeWorld(GameState* state);
void InitializeEntities(GameState* state);

void SortPlayableEntitiesByInitiative(GameState *state);

#endif
