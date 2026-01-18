#include "orthography.h"
#include <stdlib.h>

//~ acadia: GameState
typedef enum : U64
{
    GameState_MainMenu,
    GameState_Playing,
} GameState;

//~ angn: EventType
typedef enum : U64
{
    EventType_Shoot, // angn: i have no idea what we want
    EventType__Count,
} EventType;

//~ daria: Render/Animation
#define MAX_ANIMATIONS 1
#define ANIMATION_CAPACITY 5

//~ daria: Audio
#define SOUND_EFFECT_CAPACITY 5 // used in Entity

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
    if(a->frame_duration >= a->frames[a->current_frame].duration)
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
    EntityFlagsIndex_Collider,
    EntityFlagsIndex_Trigger,
    EntityFlagsIndex_ApplyFriction,
    EntityFlagsIndex_ApplyBounce,
    EntityFlagsIndex_ShootOnClick,
    EntityFlagsIndex_Player,
    EntityFlagsIndex_RenderTexture,
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

    //- angn: state
    EntityState player_state;

    //- angn: motion
    Vector2 position;
    Vector2 velocity;
    F32 friction;
    Rectangle collision;

    //- angn: audio
    Sound sound_effects[EventType__Count];

    //- angn: TODO: render / animations
    Animation animations[ANIMATION_CAPACITY];
};

//~ angn: Game
#define ENTITIES_CAPACITY 4096

//~ nick: Physics
#define COLLISIONS_MAX 128

typedef struct Game Game;
struct Game
{
    Vec2S32 screen;

    Entity entities[ENTITIES_CAPACITY];
    U64 entities_count;

    Sound sound_effects[SoundName__Count];
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

        // angn: should we feature flag this?
        entity->animations[entity->player_state].frame_duration++;

        // nick: velocity we started the frame with
        Vector2 initial_velocity = entity->velocity;
        Entity *collided_with = 0;

        if(entity_flags_contains(&entity->flags, EntityFlagsIndex_WASDMotion))
        {
            Vector2 dir = {0};

            PlayerState old_state = entity->player_state;

            // daria: NOTE: this assumes it's a player
            // TODO(daria): determine entity type
            if(inputs[InputTypes_W] & InputState_Down)
            {
                dir.y -= 1.0f;
                entity->player_state = PlayerState_Up;
            }
            else if(inputs[InputTypes_S] & InputState_Down)
            {
                dir.y += 1.0f;
                entity->player_state = PlayerState_Down;
            }
            else if(inputs[InputTypes_D] & InputState_Down)
            {
                dir.x += 1.0f;
                entity->player_state = PlayerState_Right;
            }
            else if(inputs[InputTypes_A] & InputState_Down)
            {
                dir.x -= 1.0f;
                entity->player_state = PlayerState_Left;
            }
            else
            {
                entity->player_state = PlayerState_Idle;
            }

            if(old_state != entity->player_state)
            {
                entity->animations[old_state].frame_duration = 0;
            }

            dir = Vector2ClampValue(dir, 0.0f, 1.0f);

            if(entity_flags_contains(&entity->flags, EntityFlagsIndex_ApplyFriction))
            {
                entity->velocity =
                    Vector2Add(
                            entity->velocity,
                            Vector2Scale(dir, entity->friction * 1000.0f * dt));
            }
            else
            {
                entity->velocity =
                    Vector2Add(
                            entity->velocity,
                            Vector2Scale(dir, 1000.0f * dt));
            }
        }

        if(entity_flags_contains(&entity->flags, EntityFlagsIndex_ApplyFriction))
        {
            entity->velocity =
                Vector2Add(
                        entity->velocity,
                        Vector2Scale(initial_velocity, -(entity->friction * dt)));
        }

        // angn: TODO: this is just an example
        if(entity_flags_contains(&entity->flags, EntityFlagsIndex_ShootOnClick))
        {
            if(inputs[InputTypes_Shoot] & InputState_Pressed)
            {
                PlaySound(entity->sound_effects[EventType_Shoot]);
                puts("sound");
                Entity *ball = alloc_entity(game);
                Assert(ball);
                entity_flags_set(&ball->flags, EntityFlagsIndex_ApplyVelocity);
                ball->position = entity->position;
                ball->velocity = entity->velocity;
            }
        }

        if(entity_flags_contains(&entity->flags, EntityFlagsIndex_ApplyVelocity))
        {
            if(entity_flags_contains(&entity->flags, EntityFlagsIndex_Collider))
            {

                for(U64 ci = 0;
                        ci < ENTITIES_CAPACITY;
                        ci += 1)
                {
                    Entity *other = &game->entities[ci];

                    // do not self-intersect.
                    if(entity == other)
                    {
                        continue;
                    }
                    // not of concern
                    if(!entity_flags_contains(&other->flags, EntityFlagsIndex_Alive
                                || !entity_flags_contains(&other->flags, EntityFlagsIndex_Collider)))
                    {
                        continue;
                    }

                    Rectangle entity_box =
                    {
                        entity->position.x + entity->collision.x + entity->velocity.x * dt,
                        entity->position.y + entity->collision.y + entity->velocity.y * dt,
                        entity->collision.width,
                        entity->collision.height
                    };

                    Rectangle other_box =
                    {
                        other->position.x + other->collision.x,
                        other->position.y + other->collision.y,
                        other->collision.width,
                        other->collision.height
                    };

                    _Bool collides = CheckCollisionRecs(entity_box, other_box);

                    if(collides)
                    {
                        collided_with = other;

                        Vector2 entity_center =
                        {
                            entity->position.x + entity->collision.width / 2.0f,
                            entity->position.y + entity->collision.height / 2.0f
                        };

                        Vector2 other_center =
                        {
                            other->position.x + other->collision.width / 2.0f,
                            other->position.y + other->collision.height / 2.0f
                        };

                        Vector2 other_verts[4] =
                        {
                            other->position,
                            Vector2Add(other->position, (Vector2){other->collision.width, 0.0f}),
                            Vector2Add(other->position, (Vector2){other->collision.width, other->collision.height}),
                            Vector2Add(other->position, (Vector2){0.0f, other->collision.height})
                        };

                        Vector2 collision_point = {0.0f, 0.0f};
                        Vector2 tangel = {0.0f, 0.0f};

                        if(CheckCollisionLines(entity_center, other_center, other_verts[0], other_verts[1], &collision_point))
                        {
                            tangel = (Vector2){1.0f, 0.0f};
                        }

                        if(CheckCollisionLines(entity_center, other_center, other_verts[1], other_verts[2], &collision_point))
                        {
                            tangel = (Vector2){0.0f, 1.0f};
                        }

                        if(CheckCollisionLines(entity_center, other_center, other_verts[2], other_verts[3], &collision_point))
                        {
                            tangel = (Vector2){1.0f, 0.0f};
                        }

                        if(CheckCollisionLines(entity_center, other_center, other_verts[3], other_verts[0], &collision_point))
                        {
                            tangel = (Vector2){0.0f, 1.0f};
                        }

                        entity->velocity = Vector2Scale(tangel, Vector2DotProduct(entity->velocity, tangel));
                    }
                }
            }

            entity->position = Vector2Add(entity->position, Vector2Scale(entity->velocity, dt));
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
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(0, 0, argv[0]);

    //- angn: game
    Game *game = arena_push_array(global_arena, Game, 1);
    game->screen.x = GetScreenWidth();
    game->screen.y = GetScreenHeight();

    //- daria: init audio
    InitAudioDevice();
    if(!IsAudioDeviceReady())
    {
        Assert(0 && "failed to init audio device");
        fprintf(stderr, "smth wrong with audio :(");
        exit(-1);
    }

    for(U64 i = 0;
            i < SoundName__Count;
            i += 1)
    {
        game->sound_effects[i] = LoadSound(sound_names[i]);
    }

    //- daria: animations
    // daria: TODO: have separate spritesheets or a single one
    Texture2D player_texture = LoadTexture("textures/Creature.png");
    Animation player_animation_down = animation_load(global_arena, player_texture, 1, 32);
    player_animation_down.frames[0] = (AnimationFrame){ .sprite_map_index = 0, .duration = 1 };

    Animation player_animation_up = animation_load(global_arena, player_texture, 1, 32);
    player_animation_up.frames[0] = (AnimationFrame){ .sprite_map_index = 1, .duration = 1 };

    Animation player_animation_left = animation_load(global_arena, player_texture, 1, 32);
    player_animation_left.frames[0] = (AnimationFrame){ .sprite_map_index = 2, .duration = 1 };

    Animation player_animation_right = animation_load(global_arena, player_texture, 1, 32);
    player_animation_right.frames[0] = (AnimationFrame){ .sprite_map_index = 3, .duration = 1 };

    Animation player_animation_idle = animation_load(global_arena, player_texture, 4, 32);
    player_animation_idle.frames[0] = (AnimationFrame){ .sprite_map_index = 0, .duration = 1 };
    player_animation_idle.frames[1] = (AnimationFrame){ .sprite_map_index = 1, .duration = 1 };
    player_animation_idle.frames[2] = (AnimationFrame){ .sprite_map_index = 2, .duration = 1 };
    player_animation_idle.frames[3] = (AnimationFrame){ .sprite_map_index = 3, .duration = 1 };

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
        Entity *player = alloc_entity(game);
        Assert(player);
        entity_flags_set(&player->flags, EntityFlagsIndex_WASDMotion);
        entity_flags_set(&player->flags, EntityFlagsIndex_ApplyVelocity);
        entity_flags_set(&player->flags, EntityFlagsIndex_Player);
        entity_flags_set(&player->flags, EntityFlagsIndex_RenderTexture);
        player->position = (Vector2){ 0.0, Cast(F32, game->screen.y) * 0.1 };
        for(U64 i = 0;
                i < SoundName__Count && i < SOUND_EFFECT_CAPACITY;
                i++)
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
        entity_flags_set(&ball->flags, EntityFlagsIndex_WASDMotion);
        entity_flags_set(&ball->flags, EntityFlagsIndex_ApplyVelocity);
        entity_flags_set(&ball->flags, EntityFlagsIndex_ApplyFriction);
        entity_flags_set(&ball->flags, EntityFlagsIndex_Collider);
        entity_flags_set(&ball->flags, EntityFlagsIndex_ShootOnClick);
        ball->position = (Vector2){ 0.0, Cast(F32, game->screen.y) * 0.1 };
        ball->sound_effects[EventType_Shoot] = LoadSoundAlias(game->sound_effects[SoundName_CatMeow]);
        ball->friction = 15.0f;
        ball->collision = (Rectangle){0.0f, 0.0f, 50.0f, 50.0f};
    }
    */

    {
        Entity *ball = alloc_entity(game);
        Assert(ball);
        entity_flags_set(&ball->flags, EntityFlagsIndex_ApplyVelocity);
        entity_flags_set(&ball->flags, EntityFlagsIndex_ApplyFriction);
        entity_flags_set(&ball->flags, EntityFlagsIndex_Collider);
        ball->position = (Vector2){ 200.0, Cast(F32, game->screen.y) * 0.3 };
        ball->friction = 2.0f;
        ball->collision = (Rectangle){0.0f, 0.0f, 50.0f, 50.0f};
    }

    F32 button_hot = 0;
    F32 button_active = 0;
    F32 button_rate = 0.001;

    //- angn: game loop
    B32 quit = 0;
    Inputs frame_input = {0};
    GameState game_state = GameState_MainMenu; // acadia: TODO: save in Game
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

        switch(game_state)
        {
        case GameState_MainMenu:
        {
            Font font = GetFontDefault();
            U64 button_width = 200;
            U64 button_height = 60;
            Rectangle button_rect = (Rectangle)
            {
                (game->screen.x / 2) - (button_width / 2),
                (game->screen.y / 2) - (button_height / 2),
                button_width,
                button_height
            };
            Color button_color = RED;
            Color button_text_color = WHITE;
            char *button_text = "CLICK ME";
            int button_font_size = 40;
            bool button_clicked = false;
            bool button_hovered = false;

            //- acadia: update
            Vector2 mousePointer = GetMousePosition();
            if(CheckCollisionPointRec(mousePointer, button_rect))
            {
                button_hovered = true;
                button_hot += (1 - button_hot) * button_rate;
                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                {
                    button_clicked = true;
                    button_active = 1;
                    game_state = GameState_Playing;
                }
            }
            else
            {
                button_hot += (0 - button_hot) * button_rate;
            }
            button_active += (0 - button_active) * button_rate;

            if(button_active > 0.0001) { button_color.g = 255 * button_active; }
            else { button_color.b = 255 * button_hot; }

            // acadia: render button
            {
                Vector2 text_pos = { game->screen.x / 2, game->screen.y / 2 };

                Vector2 text_size = MeasureTextEx(font, button_text, button_font_size, button_font_size * 0.1f);

                DrawRectangleRounded(button_rect, 0.5f, 0.0f, button_color);
                DrawText(button_text,
                        button_rect.x + (button_rect.width / 2) - (text_size.x / 2),
                        button_rect.y + (button_rect.height / 2) - (text_size.y / 2),
                        button_font_size,
                        WHITE);
            }

            // acadia: render welcome text
            {
                char *text = "WELCOME";
                Vector2 text_size = MeasureTextEx(font, text, 200, 200 * 0.1f);
                DrawText(text, (game->screen.x / 2) - (text_size.x / 2), game->screen.y / 4, 200, ORANGE);
            }
        } break;

        case GameState_Playing:
        {
            //- angn: render game
            for(U64 ei = 0;
                    ei < ENTITIES_CAPACITY;
                    ei += 1)
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

                    animation_next_frame(animation);
                }
                else
                {
                    DrawCircleV(entity->position, 25.0f, SKYBLUE);
                }
            }
        } break;
        }

        EndDrawing();
    }

    //- daria: audio cleanup
    for(U64 i = 0;
            i < SoundName__Count;
            i += 1)
    {
        UnloadSound(game->sound_effects[i]);
    }

    CloseAudioDevice();

    //- angn: cleanup
    CloseWindow();
    return(0);
}
