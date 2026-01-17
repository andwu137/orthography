#include "orthography.h"
#include <stdlib.h>

//~ angn: EventType
typedef enum : U64
{
    EventType_Shoot, // angn: i have no idea what we want
    EventType__Count,
} EventType;

//~ daria: Audio
#define SOUNDS_LIST \
    SOUNDS_LIST_X(CatMeow, "audio/Cat_Meow.wav") \

typedef enum : U32
{
#define SOUNDS_LIST_X(n, path) SoundName_##n,
    SOUNDS_LIST
#undef SOUNDS_LIST_X
    SoundName__Count,
} SoundName;

char *sound_names[] =
{
#define SOUNDS_LIST_X(n, path) path,
    SOUNDS_LIST
#undef SOUNDS_LIST_X
};

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
    InputTypes_Shoot,
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
    EntityFlagsIndex_ApplyVelocity,
    EntityFlagsIndex_WASDMotion,
    EntityFlagsIndex_ShootOnClick,
    EntityFlagsIndex__Count,
} EntityFlagsIndex;

#define EntityFlags_Assert_IndexValid(index) do { \
    Assert(index < EntityFlagsIndex__Count); \
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

    //- angn: audio
    Sound sound_effects[EventType__Count];

    //- angn: TODO: render / animations
};

//~ angn: Game
#define ENTITIES_CAPACITY 4096

typedef struct Game Game;
struct Game
{
    Vec2S32 screen;

    Entity entities[ENTITIES_CAPACITY];
    U64 entities_count;

    Sound sound_effects[SoundName__Count];
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
        Handle handle)
{
    Assert(handle.index < ENTITIES_CAPACITY);
    Entity *entity = game->entities + handle.index;
    if(entity_flags_contains(&entity->flags, EntityFlagsIndex_Alive))
    {
        entity_flags_unset(&entity->flags, EntityFlagsIndex_Alive);
    }
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

        if(entity_flags_contains(&entity->flags, EntityFlagsIndex_WASDMotion))
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
            entity->velocity =
                Vector2Add(
                        entity->velocity,
                        Vector2Scale(dir, 1000.0f * dt));
        }

        // angn: TODO: this is just an example
        if(entity_flags_contains(&entity->flags, EntityFlagsIndex_ShootOnClick))
        {
            if(inputs[InputTypes_Shoot] & InputState_Pressed)
            {
                PlaySound(entity->sound_effects[EventType_Shoot]);
                puts("sound");
            }
        }

        if(entity_flags_contains(&entity->flags, EntityFlagsIndex_ApplyVelocity))
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

    for (U64 i = 0;
            i < SoundName__Count;
            i += 1)
    {
        game->sound_effects[i] = LoadSound(sound_names[i]);
    }

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
    key_map[InputTypes_Shoot] = KEY_E;

    //- angn: entities
    {
        Entity *ball = alloc_entity(game);
        Assert(ball);
        entity_flags_set(&ball->flags, EntityFlagsIndex_WASDMotion);
        entity_flags_set(&ball->flags, EntityFlagsIndex_ApplyVelocity);
        entity_flags_set(&ball->flags, EntityFlagsIndex_ShootOnClick);
        ball->position = (Vector2){ 0.0, Cast(F32, game->screen.y) * 0.1 };
        ball->sound_effects[EventType_Shoot] = LoadSoundAlias(game->sound_effects[SoundName_CatMeow]);
    }

    {
        Entity *ball = alloc_entity(game);
        Assert(ball);
        entity_flags_set(&ball->flags, EntityFlagsIndex_WASDMotion);
        entity_flags_set(&ball->flags, EntityFlagsIndex_ApplyVelocity);
        ball->position = (Vector2){ 0.0, Cast(F32, game->screen.y) * 0.3 };
    }

    //- angn: game loop
    U8 quit = 0;
    Inputs frame_input = {0};
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
        if(IsKeyPressed(KEY_ESCAPE)) { quit = 1; } // angn: TODO: remove this

        for(InputTypes ki = 0;
                ki < StaticArrayLength(key_map);
                ki += 1)
        {
            KeyboardKey key = key_map[ki];
            frame_input[ki] &= ~(InputState_Up | InputState_Down); // angn: reset input state for non-event actions
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

            //- angn: unset the pressed and released flags
            // angn: NOTE: ideally we would obtain new inputs, be we seem to be
            // having issues getting the Pressed event when calling PollInputEvents
            for(InputTypes ki = 0;
                    ki < InputTypes__Count;
                    ki += 1)
            {
                frame_input[ki] &= ~(InputState_Pressed | InputState_Released);
            }
        }

        BeginDrawing();
        {
            ClearBackground(RAYWHITE);

            for(U64 ei = 0;
                    ei < ENTITIES_CAPACITY;
                    ei += 1)
            {
                Entity *entity = &game->entities[ei];
                if(!entity_flags_contains(&entity->flags, EntityFlagsIndex_Alive))
                {
                    continue;
                }

                // angn: TODO: only render if asked
                DrawCircleV(entity->position, 25.0f, SKYBLUE);
            }
        }
        EndDrawing();
    }

    //- angn: cleanup
    CloseWindow();

    //- daria: audio cleanup
    for (U64 i = 0;
            i < SoundName__Count;
            i += 1)
    {
        UnloadSound(game->sound_effects[i]);
    }
    CloseAudioDevice();

    return(0);
}
