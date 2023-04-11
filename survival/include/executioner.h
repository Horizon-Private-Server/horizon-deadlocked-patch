#ifndef SURVIVAL_MOB_EXECUTIONER_H
#define SURVIVAL_MOB_EXECUTIONER_H

#define EXECUTIONER_BASE_REACTION_TICKS						  (0.25 * TPS)
#define EXECUTIONER_BASE_ATTACK_COOLDOWN_TICKS			(5 * TPS)
#define EXECUTIONER_BASE_EXPLODE_RADIUS						  (5)
#define EXECUTIONER_MELEE_HIT_RADIUS								(1.75)
#define EXECUTIONER_EXPLODE_HIT_RADIUS							(5)
#define EXECUTIONER_MELEE_ATTACK_RADIUS						  (5)
#define EXECUTIONER_TOO_CLOSE_TO_TARGET_RADIUS		  (4)

#define EXECUTIONER_TARGET_KEEP_CURRENT_FACTOR      (10)

#define EXECUTIONER_MAX_WALKABLE_SLOPE              (40 * MATH_DEG2RAD)
#define EXECUTIONER_BASE_STEP_HEIGHT								(2)
#define EXECUTIONER_TURN_RADIANS_PER_SEC            (45 * MATH_DEG2RAD)
#define EXECUTIONER_TURN_AIR_RADIANS_PER_SEC        (15 * MATH_DEG2RAD)
#define EXECUTIONER_MOVE_ACCELERATION               (25)
#define EXECUTIONER_MOVE_AIR_ACCELERATION           (5)

#define EXECUTIONER_ANIM_ATTACK_TICKS							  (30)
#define EXECUTIONER_TIMEBOMB_TICKS									(60 * 2)
#define EXECUTIONER_FLINCH_COOLDOWN_TICKS					  (60 * 7)
#define EXECUTIONER_ACTION_COOLDOWN_TICKS					  (30)
#define EXECUTIONER_RESPAWN_AFTER_TICKS						  (60 * 30)
#define EXECUTIONER_BASE_COLL_RADIUS								(0.5)
#define EXECUTIONER_MAX_COLL_RADIUS								  (4)
#define EXECUTIONER_AMBSND_MIN_COOLDOWN_TICKS    	  (60 * 2)
#define EXECUTIONER_AMBSND_MAX_COOLDOWN_TICKS    	  (60 * 3)
#define EXECUTIONER_FLINCH_PROBABILITY             (0.05)

enum ExecutionerAnimId
{
	EXECUTIONER_ANIM_IDLE,
	EXECUTIONER_ANIM_LOOK_ON_ALERT,
	EXECUTIONER_ANIM_STOMP_ON_GROUND,
	EXECUTIONER_ANIM_WALK,
	EXECUTIONER_ANIM_WALK_BACKWARD,
	EXECUTIONER_ANIM_RUN,
	EXECUTIONER_ANIM_WOBBLE_ON_GROUND,
	EXECUTIONER_ANIM_WOBBLE_ON_GROUND2,
	EXECUTIONER_ANIM_JUMP,
	EXECUTIONER_ANIM_SWING,
	EXECUTIONER_ANIM_FIRE,
	EXECUTIONER_ANIM_THROW,
	EXECUTIONER_ANIM_FIRE_READY,
	EXECUTIONER_ANIM_FLINCH,
	EXECUTIONER_ANIM_BIG_FLINCH,
	EXECUTIONER_ANIM_FLINCH_FALL_DOWN,
	EXECUTIONER_ANIM_SPAWN
};

enum ExecutionerBangles {
	EXECUTIONER_BANGLE_LEFT_CHEST_PLATE =  	  (1 << 0),
	EXECUTIONER_BANGLE_RIGHT_CHEST_PLATE =  	(1 << 1),
	EXECUTIONER_BANGLE_HELMET =  	            (1 << 2),
	EXECUTIONER_BANGLE_LEFT_COLLAR_BONE =  	  (1 << 3),
	EXECUTIONER_BANGLE_RIGHT_COLLAR_BONE =  	(1 << 4),
	EXECUTIONER_BANGLE_BRAIN =  	            (1 << 5),
	EXECUTIONER_BANGLE_HIDE_BODY =	          (1 << 15)
};

#endif // SURVIVAL_MOB_EXECUTIONER_H
