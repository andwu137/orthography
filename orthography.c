#include "orthography.h"
#include <stdlib.h>

//~ daria: Render/Animation
#define MAX_ANIMATIONS 1
#define ANIMATION_CAPACITY 5

//~ daria: Audio
#define MAX_SOUNDS 1
#define SOUND_EFFECT_CAPACITY 5 // used in Entity

#define SOUNDS_LIST \
    SOUND(CatMeow, 0, "WAV/Cat_Meow.wav") \

typedef enum : U32
{
#define SOUND(n, e, path) SoundType_##n = (1 << e),
    SOUNDS_LIST
#undef SOUND
} SoundType;

char * sound_names[MAX_SOUNDS] =
{
#define SOUND(n, e, path) path,
    SOUNDS_LIST
#undef SOUND
};

//~ daria: Animations
// TODO(daria): place this elsewhere
typedef struct AnimationFrame AnimationFrame;
struct AnimationFrame
{
    U64 sprite_map_index; // starting frame index
    U32 duration; // per frame
};

typedef struct Animation Animation;
struct Animation
{
    Texture2D texture;

    U32 x_cell_count;
    U32 y_cell_count;

    AnimationFrame *frames;
    // left to right, top to bottom
    U32 frames_size;
    U32 current_frame;
    U32 frame_duration;
    U32 cell_size; // px
};

internal Animation
animation_load(
        Arena *arena,
        Texture2D texture,
        U8 frames_size,
        U8 cell_size)
{
    // Animation
    Animation a =
    {
        .texture = texture,

        .x_cell_count = (F32) a.texture.width / cell_size,
        .y_cell_count = (F32) a.texture.height / cell_size,
        .frames = arena_push_array(arena, AnimationFrame, frames_size),
        .frames_size = frames_size,
        .cell_size = cell_size
    };

    return a;
}

internal void
animation_next_frame(
        Animation *a)
{
    if (a->frame_duration >= a->frames[a->current_frame].duration)
    {
        a->current_frame = (a->current_frame + 1) % a->frames_size;
    }
}

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
    EntityFlagsIndex_Player,
    EntityFlagsIndex_RenderTexture,
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

//~ daria: entity states
typedef U64 EntityState;
typedef enum : EntityState
{
    PlayerState_Idle,
    PlayerState_Down,
    PlayerState_Up,
    PlayerState_Left,
    PlayerState_Right,
} PlayerState;

//~ angn: Entities
typedef struct Entity Entity;
struct Entity
{
    EntityFlags flags;
    Handle handle;

    //- angn: motion
    Vector2 position;
    Vector2 velocity;

    EntityState player_state;

    //- angn: TODO: audio
    Sound sound_effects[SOUND_EFFECT_CAPACITY];

    //- angn: TODO: render / animations
    Animation animations[ANIMATION_CAPACITY];
};

//~ angn: Game
#define ENTITIES_CAPACITY 4096

typedef struct Game Game;
struct Game
{
    Vec2S32 screen;

    Entity entities[ENTITIES_CAPACITY];
    U64 entities_count;

    Sound sound_effects[MAX_SOUNDS];
    Animation animations[MAX_ANIMATIONS];
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

        if(entity_flags_contains(&entity->flags, EntityFlagsIndex_WASD_Motion))
        {
            Vector2 dir = {0};

            PlayerState old_state = entity->player_state;

            // daria: NOTE: this assumes it's a player
            // TODO(daria): determine entity type
            if(inputs[InputTypes_W] & InputState_Down)
            {
                dir.y -= 1.0f;
                PlaySound(entity->sound_effects[0]);
                entity->player_state = PlayerState_Up;
            }
            else if(inputs[InputTypes_S] & InputState_Down)
            {
                dir.y += 1.0f;
                PlaySound(entity->sound_effects[0]);
                entity->player_state = PlayerState_Down;
            }
            else if(inputs[InputTypes_D] & InputState_Down)
            {
                dir.x += 1.0f;
                PlaySound(entity->sound_effects[0]);
                entity->player_state = PlayerState_Right;
            }
            else if(inputs[InputTypes_A] & InputState_Down)
            {
                dir.x -= 1.0f;
                PlaySound(entity->sound_effects[0]);
                entity->player_state = PlayerState_Left;
            }
            else
            // Check if the player is idle
            {
                entity->player_state = PlayerState_Idle;
            }

            if (old_state != entity->player_state)
            {
                entity->animations[old_state].frame_duration = 0;
            }

            entity->animations[entity->player_state].frame_duration++;

            dir = Vector2ClampValue(dir, 0.0f, 1.0f);
            entity->velocity =
                Vector2Add(
                        entity->velocity,
                        Vector2Scale(dir, 1000.0f * dt));
        }

        if(entity_flags_contains(&entity->flags, EntityFlagsIndex_Apply_Velocity))
        {
            entity->position =
                Vector2Add(
                        entity->position,
                        Vector2Scale(entity->velocity, dt));
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

    //- daria: init audio
    InitAudioDevice();
    if (!IsAudioDeviceReady())
    {
        Assert(0 && "failed to init audio device");
        fprintf(stderr, "smth wrong with audio :(");
        exit(-1);
    }

    for (int i = 0; i < MAX_SOUNDS; i++)
    {
        game->sound_effects[i] = LoadSound(sound_names[i]);
    }

    //- daria: animations
    // TODO(daria): have separate spritesheets or a single one
    // add arena param load
    Texture2D player_texture = LoadTexture("textures/Creature.png");
    Animation player_animation_down = animation_load(global_arena, player_texture, 1, 32);
    player_animation_down.frames[0] = (AnimationFrame) { .sprite_map_index = 0, .duration = 1 };

    Animation player_animation_up = animation_load(global_arena, player_texture, 1, 32);
    player_animation_up.frames[0] = (AnimationFrame) { .sprite_map_index = 1, .duration = 1 };

    Animation player_animation_left = animation_load(global_arena, player_texture, 1, 32);
    player_animation_left.frames[0] = (AnimationFrame) { .sprite_map_index = 2, .duration = 1 };

    Animation player_animation_right = animation_load(global_arena, player_texture, 1, 32);
    player_animation_right.frames[0] = (AnimationFrame) { .sprite_map_index = 3, .duration = 1 };

    Animation player_animation_idle = animation_load(global_arena, player_texture, 4, 32);
    player_animation_idle.frames[0] = (AnimationFrame) { .sprite_map_index = 0, .duration = 1 };
    player_animation_idle.frames[1] = (AnimationFrame) { .sprite_map_index = 1, .duration = 1 };
    player_animation_idle.frames[2] = (AnimationFrame) { .sprite_map_index = 2, .duration = 1 };
    player_animation_idle.frames[3] = (AnimationFrame) { .sprite_map_index = 3, .duration = 1 };

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
        Entity *player = alloc_entity(game);
        Assert(player);
        printf("%lu\n", player - game->entities);
        entity_flags_set(&player->flags, EntityFlagsIndex_WASD_Motion);
        entity_flags_set(&player->flags, EntityFlagsIndex_Apply_Velocity);
        entity_flags_set(&player->flags, EntityFlagsIndex_Player);
        entity_flags_set(&player->flags, EntityFlagsIndex_RenderTexture);
        player->position = (Vector2){ 0.0, Cast(F32, game->screen.y) * 0.1 };
        for (int i = 0; i < MAX_SOUNDS && i < SOUND_EFFECT_CAPACITY; i++)
        {
            player->sound_effects[i] = LoadSoundAlias(game->sound_effects[i]);
        }
        player->player_state = PlayerState_Up;

        player->animations[PlayerState_Down] = player_animation_down;
        player->animations[PlayerState_Up] = player_animation_up;
        player->animations[PlayerState_Left] = player_animation_left;
        player->animations[PlayerState_Right] = player_animation_right;
        player->animations[PlayerState_Idle] = player_animation_idle;
    }

    /*
    {
        Entity *ball = alloc_entity(game);
        Assert(ball);
        printf("%lu\n", ball - game->entities);
        entity_flags_set(&ball->flags, EntityFlagsIndex_WASD_Motion);
        entity_flags_set(&ball->flags, EntityFlagsIndex_Apply_Velocity);
        ball->position = (Vector2){ 0.0, Cast(F32, game->screen.y) * 0.1 };
        for (int i = 0; i < MAX_SOUNDS && i < SOUND_EFFECT_CAPACITY; i++)
        {
            ball->sound_effects[i] = LoadSoundAlias(game->sound_effects[i]);
        }
    }
    */

    {
        Entity *ball = alloc_entity(game);
        Assert(ball);
        printf("%lu\n", ball - game->entities);
        entity_flags_set(&ball->flags, EntityFlagsIndex_WASD_Motion);
        entity_flags_set(&ball->flags, EntityFlagsIndex_Apply_Velocity);
        ball->position = (Vector2){ 0.0, Cast(F32, game->screen.y) * 0.3 };
    }

    //- angn: game loop
    U8 quit = 0;
    for(;!quit;) // angn: TODO: remove that
    {
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

        BeginDrawing();
        {
            ClearBackground(RAYWHITE);

            for(U64 ei = 0;
                    ei < ENTITIES_CAPACITY;
                    ++ei)
            {
                Entity *entity = &game->entities[ei];
                if(!entity_flags_contains(&entity->flags, EntityFlagsIndex_Alive))
                {
                    continue;
                }

                //- daria: render entity
                if(entity_flags_contains(&entity->flags, EntityFlagsIndex_RenderTexture))
                {
                    Animation *animation = &entity->animations[entity->player_state];
                    AnimationFrame *frame = &animation->frames[animation->current_frame];

                    // daria: TODO: precompute?
                    U32 row_size = animation->texture.width / animation->cell_size;

                    // daria: NOTE: the y value is hard coded
                    Rectangle frame_rec =
                    {
                        .x = (F32)((frame->sprite_map_index % row_size) * animation->cell_size),
                        .y = (F32)((frame->sprite_map_index / row_size) * animation->cell_size),
                        .width = animation->cell_size,
                        .height = animation->cell_size,
                    };

                    Rectangle dest_rec =
                    {
                        .x = entity->position.x,
                        .y = entity->position.y,
                        .width = 128,
                        .height = 128
                    };

                    Vector2 origin = (Vector2)
                    {
                        .x = animation->cell_size,
                        .y = animation->cell_size
                    };

                    DrawTexturePro(
                            entity->animations[entity->player_state].texture,
                            frame_rec,
                            dest_rec,
                            origin,
                            0,
                            WHITE);

                    // daria: TODO: go to next frame
                    animation_next_frame(animation);
                }
                else
                {
                    DrawCircleV(entity->position, 25.0f, SKYBLUE);
                }
            }
        }
        EndDrawing();
    }

    //- angn: cleanup
    CloseWindow();

    //- daria: audio cleanup
    for (int i = 0; i < MAX_SOUNDS; i++)
    {
        UnloadSound(game->sound_effects[i]);
    }

    CloseAudioDevice();
    return(0);
}
