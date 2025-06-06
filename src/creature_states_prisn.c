/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file creature_states_prisn.c
 *     Creature state machine functions for their job in various rooms.
 * @par Purpose:
 *     Defines elements of states[] array, containing valid creature states.
 * @par Comment:
 *     None.
 * @author   KeeperFX Team
 * @date     23 Sep 2009 - 05 Jan 2011
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "creature_states_prisn.h"
#include "globals.h"

#include "bflib_math.h"
#include "bflib_planar.h"
#include "creature_states.h"
#include "creature_states_spdig.h"
#include "thing_list.h"
#include "creature_control.h"
#include "creature_instances.h"
#include "creature_states_combt.h"
#include "config_creature.h"
#include "config_rules.h"
#include "config_terrain.h"
#include "thing_stats.h"
#include "thing_objects.h"
#include "thing_effects.h"
#include "thing_navigate.h"
#include "room_data.h"
#include "room_jobs.h"
#include "room_list.h"
#include "gui_soundmsgs.h"
#include "game_legacy.h"
#include "player_instances.h"
#include "creature_senses.h"
#include "post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
#ifdef __cplusplus
}
#endif
/******************************************************************************/
TbBool jailbreak_possible(struct Room *room, PlayerNumber creature_owner)
{
    struct SlabMap *slb;
    // Neutral creatures (in any player's prison)
    // and creatures in the prisons of their owner can't jailbreak
    if (creature_owner == game.neutral_player_num || room->owner == creature_owner)
    {
        return false;
    }
    unsigned long k = 0;
    unsigned long i = room->slabs_list;
    while (i > 0)
    {
        slb = get_slabmap_direct(i);
        if (slabmap_block_invalid(slb))
        {
            ERRORLOG("Jump to invalid room slab detected");
            break;
        }
        if (slab_by_players_land(creature_owner, slb_num_decode_x(i), slb_num_decode_y(i)))
        {
            return true;
        }
        i = get_next_slab_number_in_room(i);
        k++;
        if (k > game.map_tiles_x * game.map_tiles_y)
        {
            ERRORLOG("Infinite loop detected when sweeping room slabs");
            break;
        }
    }
    return false;
}

short cleanup_prison(struct Thing *thing)
{
  struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
  cctrl->flgfield_1 &= (CCFlg_Exists | CCFlg_PreventDamage | CCFlg_Unknown08 | CCFlg_Unknown10 | CCFlg_IsInRoomList | CCFlg_MoveX | CCFlg_MoveY);
  state_cleanup_in_room(thing);
  return 1;
}

short creature_arrived_at_prison(struct Thing *creatng)
{
    TRACE_THING(creatng);
    struct CreatureControl* cctrl = creature_control_get_from_thing(creatng);
    cctrl->target_room_id = 0;
    struct Room* room = get_room_thing_is_on(creatng);
    if (!room_initially_valid_as_type_for_thing(room, get_room_role_for_job(Job_CAPTIVITY), creatng))
    {
        WARNLOG("Room %s owned by player %d is invalid for %s index %d",room_code_name(room->kind),(int)room->owner,thing_model_name(creatng),(int)creatng->index);
        set_start_state(creatng);
        return 0;
    }
    if (!add_creature_to_work_room(creatng, room, Job_CAPTIVITY))
    {
        output_room_message(room->owner, room->kind, OMsg_RoomTooSmall);
        cctrl->flgfield_1 &= ~CCFlg_NoCompControl;
        set_start_state(creatng);
        return 0;
    }
    cctrl->turns_at_job = game.play_gameturn;
    cctrl->imprison.start_gameturn = game.play_gameturn;
    cctrl->imprison.last_mood_sound_turn = game.play_gameturn;
    cctrl->flgfield_1 |= CCFlg_NoCompControl;
    internal_set_thing_state(creatng, get_continue_state_for_job(Job_CAPTIVITY));
    if (creature_under_spell_effect(creatng, CSAfF_Speed))
    {
        clean_spell_effect(creatng, CSAfF_Speed);
    }
    if (creature_under_spell_effect(creatng, CSAfF_Invisibility))
    {
        clean_spell_effect(creatng, CSAfF_Invisibility);
    }
    if (creatng->light_id != 0) {
        light_delete_light(creatng->light_id);
        creatng->light_id = 0;
    }
    return 1;

}

short creature_drop_body_in_prison(struct Thing *thing)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    struct Thing* dragtng = thing_get(cctrl->dragtng_idx);
    if (!thing_exists(dragtng) || !creature_is_being_unconscious(dragtng)) {
        set_start_state(thing);
        return 0;
    }
    if (!subtile_is_room(thing->mappos.x.stl.num, thing->mappos.y.stl.num)) {
        set_start_state(thing);
        return 0;
    }
    struct Room* room = get_room_thing_is_on(thing);
    if ((room->owner != thing->owner) || !room_role_matches(room->kind, RoRoF_Prison) || (room->used_capacity >= room->total_capacity)) {
        if (creature_drop_thing_to_another_room(thing, room, RoRoF_Prison)) {
            thing->continue_state = CrSt_CreatureDropBodyInPrison;
            return 1;
        }
        set_start_state(thing);
        return 0;
    }
    make_creature_conscious(dragtng);
    initialise_thing_state(dragtng, CrSt_CreatureArrivedAtPrison);
    struct CreatureControl* dragctrl = creature_control_get_from_thing(dragtng);
    dragctrl->flgfield_1 |= CCFlg_NoCompControl;
    set_start_state(thing);
    return 1;

}

struct Thing *find_prisoner_to_freeze(struct Thing *creatng, SpellKind spell_idx)
{
    long i;
    TRACE_THING(creatng);
    struct Room* room = INVALID_ROOM;
    if (!is_neutral_thing(creatng)) {
        room = find_nearest_room_of_role_for_thing_with_used_capacity(creatng, creatng->owner, RoRoF_Prison, NavRtF_Default, 1);
    }
    if (room_exists(room)) {
        i = room->creatures_list;
    } else {
        i = 0;
    }
    struct Thing* out_creatng = INVALID_THING;
    long out_delay = LONG_MAX;
    unsigned long k = 0;
    while (i != 0)
    {
        struct Thing* thing = thing_get(i);
        TRACE_THING(thing);
        struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
        if (!creature_control_exists(cctrl))
        {
            ERRORLOG("Jump to invalid creature %ld detected",i);
            break;
        }
        i = cctrl->next_in_room;
        // Per creature code
        long dist = get_chessboard_distance(&creatng->mappos, &thing->mappos);
        if (out_delay < 0)
        {
            // If we have a victim which isn't frozen, accept only other unfrozen creatures
            if ((dist <= LONG_MAX) && !creature_under_spell_effect(thing, CSAfF_Freeze)) {
                out_creatng = thing;
                out_delay = -1;
            }
        } else
        if (creature_under_spell_effect(thing, CSAfF_Freeze))
        {
            // If the victim is frozen, select one which will unfreeze sooner
            long durt = get_spell_duration_left_on_thing(thing, spell_idx);
            if ((durt > 0) && (out_delay > durt)) {
                out_creatng = thing;
                out_delay = durt;
            }
        } else
        {
            // Found first unfrozen victim - change out_delay to mark thet we no longer want frozen ones
            out_creatng = thing;
            out_delay = -1;
        }
        // Per creature code ends
        k++;
        if (k > THINGS_COUNT)
        {
          ERRORLOG("Infinite loop detected when sweeping creatures list");
          break;
        }
    }
    return out_creatng;
}

short creature_freeze_prisoners(struct Thing *creatng)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(creatng);
    if (cctrl->instance_id != CrInst_NULL) {
        return 1;
    }
    // Look for an available instance that can freeze.
    CrInstance inst_idx = get_available_instance_with_spell_effect(creatng, CSAfF_Freeze);
    if (inst_idx == CrInst_NULL)
    {
        set_start_state(creatng);
        return 0;
    }
    if (!creature_instance_has_reset(creatng, inst_idx))
    {
        if (creature_choose_random_destination_on_valid_adjacent_slab(creatng)) {
            creatng->continue_state = CrSt_CreatureFreezePrisoners;
        }
        return 1;
    }
    SpellKind spell_idx = get_spell_kind_from_instance(inst_idx);
    struct Thing *victng = find_prisoner_to_freeze(creatng, spell_idx);
    if (thing_is_invalid(victng)) {
        set_start_state(creatng);
        return 0;
    }
    long dist = get_combat_distance(creatng, victng);
    if (dist < 156) {
        creature_retreat_from_combat(creatng, victng, CrSt_CreatureFreezePrisoners, 0);
    } else
    if ((dist <= 2048) && (creature_can_see_combat_path(creatng, victng, dist)))
    {
        set_creature_instance(creatng, inst_idx, victng->index, 0);
    } else
    {
        creature_move_to(creatng, &victng->mappos, cctrl->max_speed, 0, 0);
    }
    return 1;
}

CrStateRet process_prison_visuals(struct Thing *creatng, struct Room *room)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(creatng);
    if (cctrl->instance_id != CrInst_NULL) {
        return CrStRet_Unchanged;
    }
    if (game.play_gameturn - cctrl->turns_at_job > 200)
    {
        if (game.play_gameturn - cctrl->turns_at_job < 250)
        {
            set_creature_instance(creatng, CrInst_MOAN, 0, 0);
            event_create_event_or_update_nearby_existing_event(creatng->mappos.x.val, creatng->mappos.y.val, EvKind_PrisonerStarving, room->owner, creatng->index);
            if (game.play_gameturn - cctrl->imprison.last_mood_sound_turn > 32)
            {
                play_creature_sound(creatng, CrSnd_Sad, 2, 0);
                cctrl->imprison.last_mood_sound_turn = game.play_gameturn;
            }
            return CrStRet_Modified;
        }
        cctrl->turns_at_job = game.play_gameturn;
    }
    if (!creature_setup_adjacent_move_for_job_within_room(creatng, room, Job_CAPTIVITY)) {
        return CrStRet_Unchanged;
    }
    creatng->continue_state = get_continue_state_for_job(Job_CAPTIVITY);
    return CrStRet_Modified;
}

CrStateRet creature_in_prison(struct Thing *thing)
{
    TRACE_THING(thing);
    struct Room* room = get_room_thing_is_on(thing);
    if (creature_job_in_room_no_longer_possible(room, Job_CAPTIVITY, thing))
    {
        remove_creature_from_work_room(thing);
        set_start_state(thing);
        return CrStRet_ResetFail;
    }
    if (room->used_capacity > room->total_capacity)
    {
        output_room_message(room->owner, room->kind, OMsg_RoomTooSmall);
        set_start_state(thing);
        return CrStRet_ResetOk;
    }
    switch (process_prison_function(thing))
    {
    case CrCkRet_Deleted:
        return CrStRet_Deleted;
    case CrCkRet_Available:
        process_prison_visuals(thing, room);
        return CrStRet_Modified;
    default:
        return CrStRet_ResetOk;
    }
}

TbBool prison_convert_creature_to_skeleton(struct Room *room, struct Thing *thing)
{
    struct CreatureModelConfig* crconf = creature_stats_get_from_thing(thing);
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    struct Thing* crthing = INVALID_THING;
    ThingModel crmodel = crconf->prison_kind;
    if ((crmodel > game.conf.crtr_conf.model_count) || (crmodel <= 0))
    {
        // If not assigned or is unknown, default to the room creature creation.
        crmodel = get_room_create_creature_model(room->kind);
    }
    if (creature_count_below_map_limit(1))
    {
        crthing = create_creature(&thing->mappos, crmodel, room->owner);
        if (thing_is_invalid(crthing))
        {
            ERRORLOG("Couldn't create creature %s in prison", creature_code_name(crmodel));
            return false;
        }
        init_creature_level(crthing, cctrl->exp_level);
        set_start_state(crthing);
        struct Dungeon* dungeon = get_dungeon(room->owner);
        if (!dungeon_invalid(dungeon)) {
            dungeon->lvstats.skeletons_raised++;
        }
    }
    else
    {
        WARNLOG("Could not create creature %s to transform %s to due to creature limit", creature_code_name(crmodel), thing_model_name(thing));
    }
    if (creature_model_bleeds(thing->model)) {
        create_effect_around_thing(thing, TngEff_Blood5); // TODO CONFIG: make this effect configurable?
    }
    kill_creature(thing, INVALID_THING, -1, CrDed_NoEffects);
    return !thing_is_invalid(crthing);
}

TbBool process_prisoner_skelification(struct Thing *thing, struct Room *room)
{
    struct CreatureModelConfig* crconf = creature_stats_get_from_thing(thing);
    if ((thing->health >= 0) || ((!crconf->humanoid_creature) && ((crconf->prison_kind > game.conf.crtr_conf.model_count) || (crconf->prison_kind <= 0)))) {
        return false;
    }
    // TODO CONFIG: (?) Allow 'skelification' only if spent specific amount of turns in prison (set it to low value). (?)
    if (CREATURE_RANDOM(thing, 101) > game.conf.rules.rooms.prison_skeleton_chance) {
        return false;
    }
    if (prison_convert_creature_to_skeleton(room, thing))
    {
        if (is_my_player_number(room->owner))
        {
            output_message(SMsg_PrisonMadeSkeleton, 0);
        }
    }
    return true; // Return true even if no skeleton could be created due to creature limit. Otherwise there's a confusing sound message. 
}

void food_set_wait_to_be_eaten(struct Thing *thing)
{
    TRACE_THING(thing);
    if (thing_is_creature(thing))
    {
        struct CreatureControl *cctrl;
        cctrl = creature_control_get_from_thing(thing);
        set_flag(cctrl->stateblock_flags, CCSpl_ChickenRel);
    }
    else
    {
        thing->food.byte_15 = -1;
        thing->food.byte_16 = 127;
    }
}

TbBool process_prison_food(struct Thing *creatng, struct Room *room)
{
    struct Thing *foodtng;
    struct CreatureControl* cctrl = creature_control_get_from_thing(creatng);
    struct CreatureModelConfig* crconf = creature_stats_get(creatng->model);

    if ( crconf->hunger_rate == 0 )
        return false;
    foodtng = get_food_at_subtile_available_to_eat_and_owned_by(creatng->mappos.x.stl.num,creatng->mappos.y.stl.num, -1);

    if ( thing_is_invalid(foodtng) )
    {
        long offsetted_gameturn = game.play_gameturn + creatng->index;
       if ((offsetted_gameturn % 64 == 0)
        && thing_is_invalid(get_food_at_subtile_available_to_eat_and_owned_by(cctrl->moveto_pos.x.stl.num,cctrl->moveto_pos.y.stl.num, -1)))
        {
            foodtng = find_random_thing_in_room(TCls_Object, 10, room); //10 = mature_food
            if ( !thing_is_invalid(foodtng) )
            {
                if ( !is_thing_directly_controlled(foodtng)
                && line_of_room_move_2d(
                    &creatng->mappos,
                    &foodtng->mappos,
                    room)
                && setup_person_move_to_position(
                    creatng,
                    foodtng->mappos.x.stl.num,
                    foodtng->mappos.y.stl.num,
                    0) )
                {
                creatng->continue_state = CrSt_CreatureInPrison;
                food_set_wait_to_be_eaten(foodtng);
                return 1;
                }
            }
        }
        return false;
    }
    if ( is_thing_directly_controlled(foodtng) )
        return false;

    thing_play_sample(creatng, 112 + UNSYNC_RANDOM(3), NORMAL_PITCH, 0, 3, 0, 2, FULL_LOUDNESS);
    if ( creatng->active_state != CrSt_CreatureInPrison )
        internal_set_thing_state(creatng, CrSt_CreatureInPrison);
    set_creature_instance(creatng, CrInst_EAT, 0, 0);
    delete_thing_structure(foodtng, 0);
    
    struct Dungeon* dungeon = get_players_num_dungeon(room->owner);
    dungeon->lvstats.chickens_eaten++;
    return true;
}

/* Does a step of being imprisoned. * Informs if the imprisoning cycle should end. * @param thing */
CrCheckRet process_prison_function(struct Thing *creatng)
{
    struct Room* room = get_room_creature_works_in(creatng);
    if (!room_still_valid_as_type_for_thing(room, RoRoF_Prison, creatng))
    {
        WARNLOG("Room %s owned by player %d is bad work place for %s index %d owner %d", room_code_name(room->kind), (int)room->owner, thing_model_name(creatng), (int)creatng->index, (int)creatng->owner);
        set_start_state(creatng);
        return CrCkRet_Continue;
    }
    process_creature_hunger(creatng);
    struct CreatureModelConfig* crconf = creature_stats_get_from_thing(creatng);
    if (process_prisoner_skelification(creatng, room))
    {
        return CrCkRet_Deleted;
    }
    else if ((creatng->health < 0) && ((!crconf->humanoid_creature) && ((crconf->prison_kind > game.conf.crtr_conf.model_count) || (crconf->prison_kind <= 0))))
    {
        if (is_my_player_number(room->owner))
        {
            output_message(SMsg_PrisonersStarving, MESSAGE_DURATION_STARVING);
        }
    }
    struct CreatureControl* cctrl = creature_control_get_from_thing(creatng);
    if ((cctrl->instance_id == CrInst_NULL) && process_prison_food(creatng, room))
        return CrCkRet_Continue;
    // Breaking from jail is only possible once per some amount of turns, and only if creature sits in jail for long enough.
    if (((game.play_gameturn % game.conf.rules.rooms.time_between_prison_break) == 0) &&
        (game.play_gameturn > cctrl->imprison.start_gameturn + game.conf.rules.rooms.time_in_prison_without_break))
    {
        // Check the base jail break condition - whether prison touches enemy land.
        if (jailbreak_possible(room, creatng->owner) && (CREATURE_RANDOM(creatng, 100) < game.conf.rules.rooms.prison_break_chance))
        {
            if (is_my_player_number(room->owner)) {
                output_message(SMsg_PrisonersEscaping, 40);
            }
            else if (is_my_player_number(room->owner)) {
                output_message(SMsg_CreatrFreedPrison, 40);
            }
            set_start_state(creatng);
            return CrCkRet_Continue;
        }
    }
    return CrCkRet_Available;
}

/******************************************************************************/
