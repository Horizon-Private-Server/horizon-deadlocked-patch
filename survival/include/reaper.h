#ifndef SURVIVAL_MOB_REAPER_H
#define SURVIVAL_MOB_REAPER_H

#define REAPER_BASE_REACTION_TICKS						(0.25 * TPS)
#define REAPER_BASE_ATTACK_COOLDOWN_TICKS			(2 * TPS)
#define REAPER_BASE_EXPLODE_RADIUS						(5)
#define REAPER_MELEE_HIT_RADIUS								(1.00)
#define REAPER_EXPLODE_HIT_RADIUS							(5)
#define REAPER_MELEE_ATTACK_RADIUS						(5)
#define REAPER_AGGRO_DAMAGE_MULTIPLIER        (1.5)

#define REAPER_TARGET_KEEP_CURRENT_FACTOR     (3)

#define REAPER_MAX_WALKABLE_SLOPE             (40 * MATH_DEG2RAD)
#define REAPER_BASE_STEP_HEIGHT								(2)
#define REAPER_TURN_RADIANS_PER_SEC           (45 * MATH_DEG2RAD)
#define REAPER_TURN_AIR_RADIANS_PER_SEC       (15 * MATH_DEG2RAD)
#define REAPER_MOVE_ACCELERATION              (25)
#define REAPER_MOVE_AIR_ACCELERATION          (5)

#define REAPER_ANIM_ATTACK_TICKS							(30)
#define REAPER_TIMEBOMB_TICKS									(60 * 2)
#define REAPER_FLINCH_COOLDOWN_TICKS					(60 * 7)
#define REAPER_ACTION_COOLDOWN_TICKS					(30)
#define REAPER_RESPAWN_AFTER_TICKS						(60 * 30)
#define REAPER_BASE_COLL_RADIUS								(0.5)
#define REAPER_MAX_COLL_RADIUS								(4)
#define REAPER_AMBSND_MIN_COOLDOWN_TICKS    	(60 * 2)
#define REAPER_AMBSND_MAX_COOLDOWN_TICKS    	(60 * 3)
#define REAPER_FLINCH_PROBABILITY             (1.0)

enum ReaperAnimId
{
	REAPER_ANIM_IDLE,
	REAPER_ANIM_HOLD_HANDS_LOOK_RIGHT,
	REAPER_ANIM_AGGRO_ROAR,
	REAPER_ANIM_WALK,
	REAPER_ANIM_RUN,
	REAPER_ANIM_ENTER_FIRE_SPIKES_FROM_BACK,
	REAPER_ANIM_LOOP_FIRE_SPIKES_FROM_BACK,
	REAPER_ANIM_EXIT_FIRE_SPIKES_FROM_BACK,
	REAPER_ANIM_VOMIT,
	REAPER_ANIM_SWING,
	REAPER_ANIM_TAUNT_1_SUCCESS_FIST_PUMP,
	REAPER_ANIM_TAUNT_2_COME_HERE_GESTURE,
	REAPER_ANIM_SPAWN,
	REAPER_ANIM_FLINCH,
	REAPER_ANIM_FLINCH_FALL_DOWN,
	REAPER_ANIM_FLINCH_KNOCKBACK,
	REAPER_ANIM_FALL_AND_DIE,
};

enum ReaperBangles {
	REAPER_BANGLE_SHOULDER_PAD_LEFT =  	(1 << 0),
	REAPER_BANGLE_SHOULDER_PAD_RIGHT =  (1 << 1),
	REAPER_BANGLE_HIDE_BODY =	(1 << 15)
};

enum ReaperAction
{
	REAPER_ACTION_SPAWN,
	REAPER_ACTION_IDLE,
	REAPER_ACTION_JUMP,
	REAPER_ACTION_WALK,
	REAPER_ACTION_AGGRO,
	REAPER_ACTION_FLINCH,
	REAPER_ACTION_BIG_FLINCH,
	REAPER_ACTION_LOOK_AT_TARGET,
  REAPER_ACTION_DIE,
	REAPER_ACTION_ATTACK,
};

enum ReaperSubskeletonJoints
{
  REAPER_SUBSKELETON_SPIKE_0 = 0,
  REAPER_SUBSKELETON_SPIKE_1 = 1,
  REAPER_SUBSKELETON_SPIKE_2 = 2,
  REAPER_SUBSKELETON_SPIKE_3 = 3,
  REAPER_SUBSKELETON_SPIKE_4 = 4,
  REAPER_SUBSKELETON_SPIKE_5 = 5,
  REAPER_SUBSKELETON_SPIKE_6 = 6,
  REAPER_SUBSKELETON_SPIKE_7 = 7,
  REAPER_SUBSKELETON_SPIKE_8 = 8,
  REAPER_SUBSKELETON_SPIKE_9 = 9,
  REAPER_SUBSKELETON_SPIKE_10 = 10,
  REAPER_SUBSKELETON_SPIKE_11 = 11,
  REAPER_SUBSKELETON_SPIKE_12 = 12,
  REAPER_SUBSKELETON_SPIKE_13 = 13,
  REAPER_SUBSKELETON_SPIKE_14 = 14,
  REAPER_SUBSKELETON_HEAD = 15,
  REAPER_SUBSKELETON_JAW = 16,
  REAPER_SUBSKELETON_LEFT_SHOULDER = 17,
  REAPER_SUBSKELETON_LEFT_ELBOW = 18,
  REAPER_SUBSKELETON_LEFT_HAND = 19,
  REAPER_SUBSKELETON_RIGHT_HAND = 20,
  REAPER_SUBSKELETON_HIPS = 21,
  REAPER_SUBSKELETON_CHEST = 22,
  REAPER_SUBSKELETON_NECK = 23,
};

typedef struct ReaperMobVars
{
  char AggroTriggered;
} ReaperMobVars_t;

#endif // SURVIVAL_MOB_REAPER_H
