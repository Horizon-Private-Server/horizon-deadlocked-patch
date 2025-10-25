
  .section .text.functions
	.globl	_lodScale;
_lodScale: 
  .align 4
  .float 1.0
  .float 0.0
  .float 0.0
  .float 0.0


	.globl	_correctTieLod;
_correctTieLod:
  add	        $s6,   $s6,   $v1;
  la          $s2, _lodScale;
  lqc2        $vf2,  0($s2);
  vmulx.xyz   vf16, vf16, vf2x;
  j 0x005930C0;
  nop;

	.globl	_playerSyncPatchHeroTransAnim;
_playerSyncPatchHeroTransAnim:

  addiu $sp, $sp, -0x10;
  sq $ra, 0x00($sp);

  # ensure anim seq is valid
  slti $v0, $s2, 0x62
  bne $v0, $zero, _playerSyncPatchHeroTransAnim_exit;

  # default to 0
  addiu $s2, $zero, 0

_playerSyncPatchHeroTransAnim_exit:
  jal 0x004fb018;

  lq $ra, 0x00($sp)
  addiu $sp, $sp, 0x10;
  jr $ra;
