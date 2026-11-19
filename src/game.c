#include "game.h"
#include <raylib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rcamera.h"
#include "raymath.h"

Vector3 CAMERA_OFFSET = (Vector3){ 0.0f, 6.0f, 10.0f };

// Comparison function
int compare(const void* a, const void* b) {
   return (*(int*)a - *(int*)b);
}

void game_init(GameState *state) {
    Camera3D camera = { 0 };
    camera.position = CAMERA_OFFSET;
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;

    camera.projection = CAMERA_PERSPECTIVE;

    state->camera = camera;

    state->pickingRay = (Ray){0};
    state->target = 0;

    // ----- Entities Building
    InitializeEntities(state);

    // ----- Playable Entities


    // ----- World Building
    InitializeWorld(state);
}

void game_update(GameState *state) {
    float cameraYaw = 0.0f;
    float zoom = - GetMouseWheelMove();

    switch (state->phase) {
        case PLAY:
            if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
                cameraYaw = -GetMouseDelta().x * 0.005f;
            }

            if(IsKeyReleased(KEY_TAB)) {
                state->target = (state->target + 1) % state->playableEntities.count;

                int nextTargetIndex = state->playableEntities.items[state->target];

                state->camera.target = Vector3Add(state->entities.items[nextTargetIndex].transform.translation, (Vector3){ 0.0f, 0.4f, 0.0f });
                state->camera.position = Vector3Add(CAMERA_OFFSET, state->camera.target);
            }

            CameraYaw(&state->camera, cameraYaw, true);
            CameraMoveToTarget(&state->camera, zoom);

            state->pickingRay = GetScreenToWorldRay(GetMousePosition(), state->camera);

            BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(state->camera);
                for(size_t i = 0; i < state->entities.count; ++i) {
                    Entity entity = state->entities.items[i];
                    Vector3 rotationAxis = {0};
                    float rotationAngle = 0.0f;
                    QuaternionToAxisAngle(entity.transform.rotation, &rotationAxis, &rotationAngle);

                    DrawModelEx(entity.model, entity.transform.translation, rotationAxis, RAD2DEG * rotationAngle, entity.transform.scale, RAYWHITE);
                }

                for(size_t i = 0; i < state->scene.count; ++i) {
                    Entity entity = state->scene.items[i];
                    DrawModelEx(entity.model, entity.transform.translation, (Vector3){0, 1, 0}, 0.0f, entity.transform.scale, RAYWHITE);
                }
            EndMode3D();

            for(size_t i = 0; i < state->playableEntities.count; i++) {
                int entityIndex = state->playableEntities.items[i];
                Entity entity = state->entities.items[entityIndex];

                char* initiativeLabel = (char*)TextFormat("%s: < ? >", entity.name);
                if(entity.initiative > 0) {
                    initiativeLabel = (char*)TextFormat("%s: < %zu >", entity.name, entity.initiative);
                }

                DrawText(initiativeLabel, 50, 50 + 25 * i, 24, BLACK);
            }

            EndDrawing();
            break;
        case TURN_ORDER_DEFINITION:
            BeginDrawing();
            ClearBackground(GRAY);

            char* text = "Press space to roll for initiative";
            int fontSize = 36;
            DrawText(text, (GetRenderWidth() - MeasureText(text, fontSize)) / 2, GetRenderHeight() - 50, fontSize, BLACK);

            state->camera.target = (Vector3){ state->target, 0.0f, 0.0f }; //state->entities.items[state->playableEntities.items[0]].transform.translation;
            state->camera.position = Vector3Add(CAMERA_OFFSET, state->camera.target);

            if (IsKeyReleased(KEY_SPACE)) {
                int entityIndex = state->playableEntities.items[state->target];
                Entity* entity = &state->entities.items[entityIndex];

                entity->initiative = GetRandomValue(1, 20);

                state->target = (state->target + 1) % state->playableEntities.count;
                printf("%zu\n", state->target);

                if(state->target == 0) {
                    SortPlayableEntitiesByInitiative(state);

                    for (size_t i = 0; i < state->playableEntities.count; ++i) {
                        printf("%d ", state->playableEntities.items[i]);
                    }

                    fflush(stdout);

                    // End of initiative phase
                    state->phase = PLAY;

                    state->target = 0;
                    int entityIndex = state->playableEntities.items[0];
                    Entity* entity = &state->entities.items[entityIndex];

                    state->camera.target = Vector3Add(entity->transform.translation, (Vector3){ 0.0f, 0.4f, 0.0f });
                    state->camera.position = Vector3Add(CAMERA_OFFSET, state->camera.target);
                }
            }

            BeginMode3D(state->camera);
            for(size_t i = 0; i < state->playableEntities.count; i++) {
                int entityIndex = state->playableEntities.items[i];
                Entity entity = state->entities.items[entityIndex];

                DrawModelEx(entity.model, (Vector3){ i, 0.0f, 0.0f }, (Vector3){0}, 0.0f, entity.transform.scale, RAYWHITE);
            }
            EndMode3D();

            for(size_t i = 0; i < state->playableEntities.count; i++) {
                int entityIndex = state->playableEntities.items[i];
                Entity entity = state->entities.items[entityIndex];

                char* initiativeLabel = (char*)TextFormat("%s: < ? >", entity.name);
                if(entity.initiative > 0) {
                    initiativeLabel = (char*)TextFormat("%s: < %zu >", entity.name, entity.initiative);
                }

                DrawText(initiativeLabel, 50, 50 + 25 * i, 24, BLACK);
            }

            EndDrawing();
            break;
    }
}

void game_shutdown(GameState *state) {
}



int AddEntity(Entities* entities, Entity entity) {
    if (entities->count >= entities->capacity) {
        if (entities->capacity == 0) entities->capacity = 256;
        else entities->capacity *= 2;

        entities->items = realloc(entities->items, entities->capacity * sizeof(*entities->items));
    }
    entities->items[entities->count++] = entity;

    return entities->count - 1;
}

// TODO: Understand and use a macro here
void AddEntityReference(EntitiesReferences* entities, int entityIndex) {
    if (entities->count >= entities->capacity) {
        if (entities->capacity == 0) entities->capacity = 256;
        else entities->capacity *= 2;

        entities->items = realloc(entities->items, entities->capacity * sizeof(*entities->items));
    }
    entities->items[entities->count++] = entityIndex;
}

void InitializeWorld(GameState* state) {
    Model models[256] = {0};

    models[0] = LoadModel("assets/kenney_mini_dungeon/Models/GLB format/floor.glb");
    models[5] = LoadModel("assets/kenney_mini_dungeon/Models/GLB format/stones.glb");
    models[10] = LoadModel("assets/kenney_mini_dungeon/Models/GLB format/floor-detail.glb");
    models[15] = LoadModel("assets/kenney_mini_dungeon/Models/GLB format/wall.glb");
    models[20] = LoadModel("assets/kenney_mini_dungeon/Models/GLB format/stairs.glb");

    Image sceneLayout = LoadImage("assets/maps/prototype-level-001.png");
    Color* pixels = LoadImageColors(sceneLayout);

    for(int x = 0; x < sceneLayout.width; ++x) {
        for(int y = 0; y < sceneLayout.height; ++y) {
            Color pixel = (pixels[x + y * sceneLayout.width]);
            unsigned char id = pixel.r;
            unsigned char elevation = pixel.g;
            unsigned char detail = pixel.b;

            if (IsModelValid(models[id])) {
                Entity entity = (Entity) {
                    .model = models[id],
                    .transform = {
                        .translation = (Vector3){ x - sceneLayout.width / 2.0f, elevation / 10.0f, y - sceneLayout.height / 2.0f },
                        .rotation = (Quaternion){0},
                        .scale = (Vector3){ 1.0f, 1.0f, 1.0f }
                    }
                };

                AddEntity(&state->scene, entity);
            }

            if (detail > 0 && IsModelValid(models[detail])) {
                Entity entity = (Entity) {
                    .model = models[detail],
                    .transform = {
                        .translation = (Vector3){ x - sceneLayout.width / 2.0f, elevation / 10.0f, y - sceneLayout.height / 2.0f },
                        .rotation = (Quaternion){0},
                        .scale = (Vector3){ 1.0f, 1.0f, 1.0f }
                    }
                };

                AddEntity(&state->scene, entity);
            }
        }
    }
}

void InitializeEntities(GameState* state) {
    Model orc = LoadModel("assets/kenney_mini_dungeon/Models/GLB format/character-orc.glb");
    Model human = LoadModel("assets/kenney_mini_dungeon/Models/GLB format/character-human.glb");
    Model chair = LoadModel("assets/kenney_mini_dungeon/Models/GLB format/chair.glb");

    Entity chairEntity = (Entity){
        .model = chair,
        .transform = (Transform){
            .translation = (Vector3){ -2.0f, 0.0f, 2.0f },
            .rotation = QuaternionFromAxisAngle((Vector3){ 0.0f, 1.0f, 0.0f}, 45.0f),
            .scale = (Vector3){ 1.0f, 1.0f, 1.0f }
        }
    };

    Entity orcEntity = (Entity){
        .model = orc,
        .name = "Bob",
        .team = 1,
        .transform = (Transform){
            .translation = (Vector3){ -1.0f, 0.0f, 0.0f },
            .rotation = QuaternionFromAxisAngle((Vector3){ 0.0f, 1.0f, 0.0f}, 0.0f),
            .scale = (Vector3){ 1.0f, 1.0f, 1.0f }
        }
    };

    Entity humanEntity = (Entity){
        .model = human,
        .name = "Greg",
        .team = 2,
        .transform = (Transform){
            .translation = (Vector3){ 1.0f, 0.0f, 1.0f },
            .rotation = QuaternionFromAxisAngle((Vector3){ 0.0f, 1.0f, 0.0f}, 0.0f),
            .scale = (Vector3){ 1.0f, 1.0f, 1.0f }
        }
    };

    AddEntity(&state->entities, chairEntity);
    int orcIndex = AddEntity(&state->entities, orcEntity);
    int humanIndex = AddEntity(&state->entities, humanEntity);

    AddEntityReference(&state->playableEntities, orcIndex);
    AddEntityReference(&state->playableEntities, humanIndex);
}

void SortPlayableEntitiesByInitiative(GameState *state) {
    size_t n = state->playableEntities.count;
    int   *items = state->playableEntities.items;

    for (size_t i = 0; i + 1 < n; ++i) {
        bool swapped = false;

        for (size_t j = 0; j + 1 < n - i; ++j) {
            size_t a = state->entities.items[items[j]].initiative;
            size_t b = state->entities.items[items[j + 1]].initiative;

            if (a < b) {              // descending: highest initiative first
                int tmp      = items[j];
                items[j]     = items[j + 1];
                items[j + 1] = tmp;
                swapped = true;
            }
        }

        if (!swapped) break;          // already sorted
    }
}
