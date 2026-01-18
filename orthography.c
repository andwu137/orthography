#include "orthography.h"
#include <stdlib.h>

//~ angn: Inputs
typedef enum InputState : U8
{
    InputState_Down = (1<<0),
    InputState_Up = (1<<1),
    InputState_Pressed = (1<<2),
    InputState_Released = (1<<3),
} InputState;

typedef enum InputTypes : U64
{
    InputTypes_W,
    InputTypes_A,
    InputTypes_S,
    InputTypes_D,
    InputTypes__Count,
} InputTypes;

typedef InputState Inputs[InputTypes__Count];
typedef KeyboardKey KeyMap[InputTypes__Count];

//~ angn: Handle
typedef struct Handle Handle;
struct Handle
{
    U64 index;
    U64 gen;
};

//~ angn: EntityFlags
typedef struct EntityFlags EntityFlags;
struct EntityFlags
{
    U64 f[1];
};

typedef enum : U64
{
    EntityFlagsIndex_Alive,
    EntityFlagsIndex_Apply_Velocity,
    EntityFlagsIndex_WASD_Motion,
    EntityFlagsIndex_Collider,
    EntityFlagsIndex_Trigger,
    EntityFlagsIndex_Apply_Friction,
    EntityFlagsIndex_Apply_Bounce,
} EntityFlagsIndex;

#define EntityFlags_Assert_IndexValid(index) do { \
    U64 max_number_of_bits = sizeof(MemberOf(EntityFlags, f)) * 8; \
    Assert(index < max_number_of_bits); \
} while(0)

internal void
entity_flags_set(
        EntityFlags *flags,
        EntityFlagsIndex index)
{
    EntityFlags_Assert_IndexValid(index);
    U64 number_of_bits_in_cell = sizeof(*MemberOf(EntityFlags, f)) * 8;
    U64 quo = index / number_of_bits_in_cell;
    U64 rem = index % number_of_bits_in_cell;
    flags->f[quo] |= (1 << rem);
}

internal void
entity_flags_unset(
        EntityFlags *flags,
        EntityFlagsIndex index)
{
    EntityFlags_Assert_IndexValid(index);
    U64 number_of_bits_in_cell = sizeof(*MemberOf(EntityFlags, f)) * 8;
    U64 quo = index / number_of_bits_in_cell;
    U64 rem = index % number_of_bits_in_cell;
    flags->f[quo] &= ~(1 << rem);
}

internal B32
entity_flags_contains(
        EntityFlags *flags,
        EntityFlagsIndex index)
{
    EntityFlags_Assert_IndexValid(index);
    U64 number_of_bits_in_cell = sizeof(*MemberOf(EntityFlags, f)) * 8;
    U64 quo = index / number_of_bits_in_cell;
    U64 rem = index % number_of_bits_in_cell;
    return(flags->f[quo] & (1 << rem));
}

//~ angn: Entities
typedef struct Entity Entity;
struct Entity
{
    EntityFlags flags;
    Handle handle;

    //- angn: motion
    Vector2 position;
    Vector2 velocity;
    F32 friction;
    Rectangle collision;

    //- angn: TODO: audio

    //- angn: TODO: render / animations
};

//~ angn: Game
#define ENTITIES_CAPACITY 4096

//~ nick: Physics
#define COLLISIONS_MAX 128
#define BACKPEDAL_STEP 1.0f

typedef struct Game Game;
struct Game
{
    Vec2S32 screen;

    Entity entities[ENTITIES_CAPACITY];
    U64 entities_count;
};

internal Entity *
get_entity_from_handle(
        Game *game,
        Handle handle)
{
    Assert(handle.index < ENTITIES_CAPACITY);
    Entity *entity = &game->entities[handle.index];
    if(entity->handle.gen != handle.gen) { entity = 0; }
    return(entity);
}

internal Entity *
alloc_entity(
        Game *game)
    // angn: SUGGEST: should we allow for bulk allocations?
{
    Entity *entity = 0;

    if(game->entities_count + 1 >= ENTITIES_CAPACITY)
    {
        Assert(0 && "out of memory");
        fprintf(stderr, "you won i guess??\n");
        exit(-1);
    }
    else
    {
        for(U64 ei = 0;
                ei < ENTITIES_CAPACITY;
                ei += 1)
        {
            if(entity_flags_contains(&game->entities[ei].flags, EntityFlagsIndex_Alive))
            {
                continue;
            }
            entity = game->entities + ei;
            entity->handle.gen += 1;
            game->entities_count += 1;
            break;
        }
    }

    if(entity)
    {
        entity_flags_set(&entity->flags, EntityFlagsIndex_Alive);
    }

    return(entity);
}

internal void
destroy_entity(
        Game *game,
        Handle entity)
{
    NotUsed(game);
    NotUsed(entity);
}

internal void
game_update(
        Game *game,
        Inputs inputs,
        F32 dt) // angn: NOTE: technically always constant, useful
                // for extra updates if required
{
    for(U64 ei = 0;
            ei < ENTITIES_CAPACITY;
            ei += 1)
    {
        Entity *entity = &game->entities[ei];
        if(!entity_flags_contains(&entity->flags, EntityFlagsIndex_Alive)
                || entity->handle.gen == 0)
        {
            continue;
        }

        //~ nick: velocity we started the frame with
        Vector2 initial_velocity = entity->velocity;
        Entity *collided_with = 0;

        if(entity_flags_contains(&entity->flags, EntityFlagsIndex_WASD_Motion))
        {
            Vector2 dir = {0};

            if(inputs[InputTypes_W] & InputState_Down)
            {
                dir.y -= 1.0f;
            }
            if(inputs[InputTypes_S] & InputState_Down)
            {
                dir.y += 1.0f;
            }
            if(inputs[InputTypes_D] & InputState_Down)
            {
                dir.x += 1.0f;
            }
            if(inputs[InputTypes_A] & InputState_Down)
            {
                dir.x -= 1.0f;
            }

            dir = Vector2ClampValue(dir, 0.0f, 1.0f);

            if(entity_flags_contains(&entity->flags, EntityFlagsIndex_Apply_Friction)) {
                entity->velocity =
                    Vector2Add(
                            entity->velocity,
                            Vector2Scale(dir, entity->friction * 1000.0f * dt));
            } else {
                entity->velocity =
                    Vector2Add(
                            entity->velocity,
                            Vector2Scale(dir, 1000.0f * dt));
            }
        }

        if(entity_flags_contains(&entity->flags, EntityFlagsIndex_Apply_Friction))
        {
            entity->velocity =
                Vector2Add(
                        entity->velocity,
                        Vector2Scale(initial_velocity, -(entity->friction * dt)));
        }

        if(entity_flags_contains(&entity->flags, EntityFlagsIndex_Apply_Velocity))
        {
            if(entity_flags_contains(&entity->flags, EntityFlagsIndex_Collider))
            {

                for (
                        U64 ci = 0;
                        ci < ENTITIES_CAPACITY;
                        ci ++)
                {
                    Entity *other = &game->entities[ci];

                    if (entity == other)
                    {
                        // do not self-intersect.
                        continue;
                    }
                    if (
                            !entity_flags_contains(&other->flags, EntityFlagsIndex_Alive ||
                            !entity_flags_contains(&other->flags, EntityFlagsIndex_Collider)))
                    {
                        // not of concern
                        continue;
                    }

                    Rectangle entity_box = {
                        entity->position.x + entity->collision.x + entity->velocity.x * dt,
                        entity->position.y + entity->collision.y + entity->velocity.y * dt,
                        entity->collision.width,
                        entity->collision.height};

                    Rectangle other_box = {
                            other->position.x + other->collision.x,
                            other->position.y + other->collision.y,
                            other->collision.width,
                            other->collision.height};

                    _Bool collides = CheckCollisionRecs(
                            entity_box, other_box);

                    if (collides) {
                        collided_with = other;

                        // Vector2 entity_center = {
                        //         entity->position.x + entity->collision.width / 2.0f,
                        //         entity->position.y + entity->collision.height / 2.0f};

                        // Vector2 other_center = {
                        //         other->position.x + other->collision.width / 2.0f,
                        //         other->position.y + other->collision.height / 2.0f};

                        // Vector2 nudge_step = Vector2Scale(Vector2Normalize(Vector2Subtract(entity_center, other_center)), BACKPEDAL_STEP);
                        // Vector2 nudge_acc = {0.0f, 0.0f};

                        // while (CheckCollisionRecs(
                        //             (Rectangle){
                        //                 entity_box.x + nudge_acc.x,
                        //                 entity_box.y + nudge_acc.y,
                        //                 entity_box.width,
                        //                 entity_box.height},
                        //             other_box))
                        // {
                        //     nudge_acc = Vector2Subtract(nudge_acc, nudge_step);
                        // }

                        // entity->position = Vector2Add(entity->position, nudge_step);
                        entity->velocity = (Vector2){0.0f, 0.0f};
                    } else {
                        entity->position = Vector2Add(entity->position, Vector2Scale(entity->velocity, dt));
                    }
                }
            //     _Bool collided = 0;
            //     Vector2 smallest_resolve = {0};

            //     // Rectangle move_box = {
            //     //         entity->position.x,
            //     //         entity->position.y,
            //     //         entity->position.x + entity->velocity.x * dt + entity->collision.width,
            //     //         entity->position.y + entity->velocity.y * dt + entity->collision.height
            //     // };

            //     Vector2 entity_verts_i[4] = {
            //         entity->position,
            //         Vector2Add(entity->position, (Vector2){entity->collision.x, 0.0f}),
            //         Vector2Add(entity->position, (Vector2){entity->collision.x, entity->collision.y}),
            //         Vector2Add(entity->position, (Vector2){0.0f, entity->collision.y})
            //     };

            //     Vector2 entity_verts_f[4] = {
            //         Vector2Add(entity_verts_i[0], Vector2Scale(entity->velocity, dt)),
            //         Vector2Add(entity_verts_i[1], Vector2Scale(entity->velocity, dt)),
            //         Vector2Add(entity_verts_i[2], Vector2Scale(entity->velocity, dt)),
            //         Vector2Add(entity_verts_i[3], Vector2Scale(entity->velocity, dt))
            //     };

            //     float min_x = entity_verts_i[0].x;
            //     float max_x = entity_verts_i[0].x;
            //     float min_y = entity_verts_i[0].y;
            //     float max_y = entity_verts_i[0].y;

            //     for (U8 i = 0; i < 4; i++) {
            //         if (entity_verts_i[i].x < min_x) min_x = entity_verts_i[i].x;
            //         if (entity_verts_i[i].y < min_y) min_y = entity_verts_i[i].y;
            //         if (entity_verts_i[i].x > max_x) max_x = entity_verts_i[i].x;
            //         if (entity_verts_i[i].y > max_y) max_y = entity_verts_i[i].y;
            //     }

            //     for (U8 i = 0; i < 4; i++) {
            //         if (entity_verts_f[i].x < min_x) min_x = entity_verts_f[i].x;
            //         if (entity_verts_f[i].y < min_y) min_y = entity_verts_f[i].y;
            //         if (entity_verts_f[i].x > max_x) max_x = entity_verts_f[i].x;
            //         if (entity_verts_f[i].y > max_y) max_y = entity_verts_f[i].y;
            //     }

            //     Rectangle move_box = (Rectangle) {
            //             min_x - entity->collision.width,
            //             min_y - entity->collision.height,
            //             (max_x - min_x) + 2.0f * entity->collision.width,
            //             (max_y - min_y) + 2.0f * entity->collision.height};

            //     printf("position (%f, %f), box: x[%f, %f] y[%f, %f]\n",
            //             entity->position.x, entity->position.y,
            //             min_x, max_x, min_y, max_y);

            //     // staticly sized move-box (bounding box)
            //     // Rectangle move_box = {
            //     //     entity->position.x - (entity->collision.width),
            //     //     entity->position.y - (entity->collision.height),
            //     //     entity->collision.width * 3.0f,
            //     //     entity->collision.height * 3.0f
            //     // };

            //     // DrawRectangleRec(move_box, RED);
            //     // DrawRectangleLinesEx(move_box, 2.0f, RED);
            //     DrawLineV(entity_verts_i[0], Vector2Add(Vector2Scale(Vector2Subtract(entity_verts_f[0], entity_verts_i[0]), 100.0f), entity_verts_i[0]), RED);
            //     DrawLineV(entity_verts_i[1], Vector2Add(Vector2Scale(Vector2Subtract(entity_verts_f[1], entity_verts_i[1]), 100.0f), entity_verts_i[1]), RED);
            //     DrawLineV(entity_verts_i[2], Vector2Add(Vector2Scale(Vector2Subtract(entity_verts_f[2], entity_verts_i[2]), 100.0f), entity_verts_i[2]), RED);
            //     DrawLineV(entity_verts_i[3], Vector2Add(Vector2Scale(Vector2Subtract(entity_verts_f[3], entity_verts_i[3]), 100.0f), entity_verts_i[3]), RED);

            //     for(U64 ci = 0;
            //             ci < ENTITIES_CAPACITY;
            //             ci += 1)
            //     {
            //         // TODO(nick):
            //         // resolve.

            //         Entity *other = &game->entities[ci];

            //         Rectangle other_collision = other->collision;
            //         other_collision.x = other->collision.x + other->position.x;
            //         other_collision.y = other->collision.y + other->position.y;

            //         if (entity == other) { continue; }  //~ nick: do not self-intersect
            //         if (                                //~ nick: check validity
            //                 !entity_flags_contains(&other->flags, EntityFlagsIndex_Alive) ||
            //                 !entity_flags_contains(&other->flags, EntityFlagsIndex_Collider))
            //         {
            //             continue;
            //         }

            //         if (!CheckCollisionRecs(move_box, other_collision)) { continue; } //~ nick: not a threat.
            //         printf("> Threat!\n");

            //         //~ nick: try rect collision at minimum passing distance

            //         // Vector2 entity_position_center = Vector2Add(entity->position, Vector2Scale((Vector2){entity->collision.width, entity->collision.height}, 0.5));
            //         // Vector2 other_position_center = Vector2Add(other->position, Vector2Scale((Vector2){other->collision.width, other->collision.height}, 0.5));

            //         // Vector2 v_normal = Vector2Normalize(
            //         //         (Vector2){-entity->velocity.y, entity->velocity.x});

            //         // float min_distance = Vector2DotProduct(
            //         //         Vector2Subtract(other_position_center, entity_position_center),
            //         //         v_normal);

            //         // Vector2 entity_virtual_origin = Vector2Add(
            //         //         Vector2Add(other_position_center, Vector2Scale(v_normal, -min_distance)),
            //         //         Vector2Scale((Vector2){entity->collision.width, entity->collision.height}, -0.5));

            //         // Rectangle entity_virtual_rectangle = entity->collision;
            //         // entity_virtual_rectangle.x = entity_virtual_origin.x;
            //         // entity_virtual_rectangle.y = entity_virtual_origin.y;

            //         // if (CheckCollisionRecs(entity_virtual_rectangle, other_collision)) {
            //         //     // TODO(Nick): this collides. record the move vector

            //         //     printf("> Prox-collision!\n");

            //         //     collided = 1;

            //         //     Vector2 backpedal = {0};
            //         //     Vector2 backpedal_dir = Vector2Normalize(Vector2Subtract(
            //         //                 entity_position_center,
            //         //                 other_position_center));

            //         //     while (
            //         //             CheckCollisionRecs(
            //         //                 (Rectangle){
            //         //                     entity_virtual_rectangle.x + backpedal.x,
            //         //                     entity_virtual_rectangle.y + backpedal.y,
            //         //                     entity_virtual_rectangle.width,
            //         //                     entity_virtual_rectangle.height},
            //         //                 other_collision))
            //         //     {
            //         //         backpedal = Vector2Subtract(backpedal, Vector2Scale(backpedal_dir, BACKPEDAL_STEP));
            //         //         printf("    ... backpedal (%f, %f)\n", backpedal.x, backpedal.y);
            //         //     }

            //         //     Vector2 resolve = Vector2Subtract(
            //         //             Vector2Add(entity_virtual_origin, backpedal), entity->position);

            //         //     if (Vector2Length(resolve) < Vector2Length(smallest_resolve)) {
            //         //         smallest_resolve = resolve;
            //         //     }

            //         //     continue;
            //         // }

            //         //~ nick: move lines extend from entity's corners,
            //         //intersect against other's edges.

            //         Vector2 other_verts[4] = {
            //             other->position,
            //             Vector2Add(other->position, (Vector2){other->collision.x, 0.0f}),
            //             Vector2Add(other->position, (Vector2){other->collision.x, other->collision.y}),
            //             Vector2Add(other->position, (Vector2){0.0f, other->collision.y})
            //         };

            //         Vector2 collision_location[16] = {0};

            //         for (U8 li = 0;
            //                 li < 4;
            //                 li++)
            //         {
            //             if (CheckCollisionLines(
            //                     entity_verts_i[li], entity_verts_f[li],
            //                     other_verts[0], other_verts[1],
            //                     &collision_location[0 + (li * 4)]))
            //             {
            //                 collided = 1;
            //             }

            //             if (CheckCollisionLines(
            //                     entity_verts_i[li], entity_verts_f[li],
            //                     other_verts[1], other_verts[2],
            //                     &collision_location[1 + (li * 4)]))
            //             {
            //                 collided = 1;
            //             }

            //             if (CheckCollisionLines(
            //                     entity_verts_i[li], entity_verts_f[li],
            //                     other_verts[2], other_verts[3],
            //                     &collision_location[2 + (li * 4)]))
            //             {
            //                 collided = 1;
            //             }

            //             if (CheckCollisionLines(
            //                     entity_verts_i[li], entity_verts_f[li],
            //                     other_verts[3], other_verts[0],
            //                     &collision_location[3 + (li * 4)]))
            //             {
            //                 collided = 1;
            //             }
            //         }

            //         if (collided == false) { continue; }

            //         printf("> line-collision!");

            //         // TODO(Nick): this collides. record the move vector
            //         Vector2 shortest_origin = {0};
            //         Vector2 shortest_vec = {0};
            //         float shortest_length = 0.0;

            //         for (U8 ci = 0; ci < 16; ci++) {
            //             Vector2 origin = entity_verts_i[ci / 4];
            //             Vector2 challenger = Vector2Subtract(collision_location[ci], origin);
            //             float challenger_length = Vector2Length(challenger);

            //             if (challenger_length < shortest_length) {
            //                 shortest_length = challenger_length;
            //                 shortest_vec = challenger;
            //                 shortest_origin = origin;
            //             }
            //         }

            //         if (shortest_length < Vector2Length(smallest_resolve)) {
            //             smallest_resolve = shortest_vec;
            //         }

            //         continue;
            //     }

            //     // TODO(Nick): resolve to nearest collision if it occured.
            //     if (collided) {
            //         if (entity_flags_contains(&entity->flags, EntityFlagsIndex_Apply_Bounce)) {
            //             // TODO(nick): bouncy boi
            //         } else {
            //             // not bouncy
            //             entity->position = Vector2Add(
            //                     entity->position,
            //                     Vector2Subtract(
            //                         smallest_resolve,
            //                         Vector2Scale(
            //                             Vector2Normalize(smallest_resolve),
            //                             BACKPEDAL_STEP)));
            //             entity->velocity = (Vector2){0.0, 0.0};
            //         }
            //     } else {
            //         entity->position =
            //             Vector2Add(
            //                     entity->position,
            //                     Vector2Scale(entity->velocity, dt));
            //     }

            // } else {
            //     entity->position =
            //         Vector2Add(
            //                 entity->position,
            //                 Vector2Scale(entity->velocity, dt));
            }
        }
    }
}

int
main(
        int argc,
        char **argv)
{
    NotUsed(argc);

    //- angn: os
    {
        int os_error_code = os_init();
        if(os_error_code) { return(os_error_code); }
    }
    Arena *global_arena = os_get_arena();

    //- angn: init raylib
    InitWindow(0, 0, argv[0]);

    //- angn: game
    Game *game = arena_push_array(global_arena, Game, 1);
    game->screen.x = GetScreenWidth();
    game->screen.y = GetScreenHeight();

    //- angn: fixed timestep
    // angn: NOTE: is this update rate too high?
    F32 dt_fixed = 1.0f / 60.0f; // update rate
    F32 time_accumulator = 0.0f; // carry any extra-time(under-updates) past the frame
    F32 current_time = GetTime();

    //- angn: inputs
    KeyMap key_map = {0};
    key_map[InputTypes_W] = KEY_W;
    key_map[InputTypes_A] = KEY_A;
    key_map[InputTypes_S] = KEY_S;
    key_map[InputTypes_D] = KEY_D;

    //- angn: entities
    {
        Entity *ball = alloc_entity(game);
        Assert(ball);
        printf("%lu\n", ball - game->entities);
        entity_flags_set(&ball->flags, EntityFlagsIndex_WASD_Motion);
        entity_flags_set(&ball->flags, EntityFlagsIndex_Apply_Velocity);
        entity_flags_set(&ball->flags, EntityFlagsIndex_Apply_Friction);
        entity_flags_set(&ball->flags, EntityFlagsIndex_Collider);
        ball->position = (Vector2){ 0.0f, Cast(F32, game->screen.y) * 0.1f };
        ball->friction = 15.0f;
        ball->collision = (Rectangle){0.0f, 0.0f, 50.0f, 50.0f};
    }

    {
        Entity *ball = alloc_entity(game);
        Assert(ball);
        printf("%lu\n", ball - game->entities);
        entity_flags_set(&ball->flags, EntityFlagsIndex_Apply_Velocity);
        entity_flags_set(&ball->flags, EntityFlagsIndex_Apply_Friction);
        entity_flags_set(&ball->flags, EntityFlagsIndex_Collider);
        ball->position = (Vector2){ 50.0f, 50.0f };
        ball->friction = 2.0f;
        ball->collision = (Rectangle){0.0f, 0.0f, 50.0f, 50.0f};
    }

    //- angn: game loop
    U8 quit = 0;
    for(;!quit;) // angn: TODO: remove that
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        //- angn: get information
        game->screen.x = GetScreenWidth();
        game->screen.y = GetScreenHeight();

        //- angn: fixed time
        F32 new_time = GetTime();
        F32 frame_time = new_time - current_time;
        current_time = new_time;

        frame_time = Min(0.25f, frame_time); // angn: restrict number of updates
                                             // so we don't do an update death spiral
        time_accumulator += frame_time;

        //- angn: get inputs
        Inputs frame_input = {0};
        if(IsKeyPressed(KEY_ESCAPE)) { quit = 1; } // angn: TODO: remove this

        for(InputTypes ki = 0;
                ki < StaticArrayLength(key_map);
                ki += 1)
        {
            KeyboardKey key = key_map[ki];
            if(IsKeyDown(key)) { frame_input[ki] |= InputState_Down; }
            if(IsKeyUp(key)) { frame_input[ki] |= InputState_Up; }
            if(IsKeyPressed(key)) { frame_input[ki] |= InputState_Pressed; }
            if(IsKeyReleased(key)) { frame_input[ki] |= InputState_Released; }
        }

        //- angn: update
        for(;
                time_accumulator > dt_fixed;
                time_accumulator -= dt_fixed)
        {
            game_update(game, frame_input, dt_fixed);
        }


        for(U64 ei = 0;
                ei < ENTITIES_CAPACITY;
                ++ei)
        {
            Entity *entity = &game->entities[ei];
            if(!entity_flags_contains(&entity->flags, EntityFlagsIndex_Alive))
            {
                continue;
            }

            // angn: TODO: only render if asked
            DrawRectangleLines(
                    entity->collision.x + entity->position.x,
                    entity->collision.y + entity->position.y,
                    entity->collision.width, entity->collision.height,
                    GREEN);
            DrawCircleV(Vector2Add(entity->position, (Vector2){25.0f, 25.0f}), 25.0f, SKYBLUE);
        }
        EndDrawing();
    }

    //- angn: cleanup
    CloseWindow();
    return(0);
}
