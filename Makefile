

all:
	$(MAKE) -C patch
	$(MAKE) -C unpatch
	$(MAKE) -C spleef
	$(MAKE) -C infected
	$(MAKE) -C gun-game
	$(MAKE) -C infinite-climber
	$(MAKE) -C race
	$(MAKE) -C snd
	$(MAKE) -C duckhunt
	$(MAKE) -C gridiron
	$(MAKE) -C thousand-kills
	$(MAKE) -C survival
	$(MAKE) -C payload
	$(MAKE) -C training
	$(MAKE) -C benchmark
	$(MAKE) -C team-defender
	$(MAKE) -C thousand-kills
	$(MAKE) -C mapdownloader
	$(MAKE) -C anim-extractor
	$(MAKE) -C elfloader
	
clean:
	$(MAKE) -C patch clean
	$(MAKE) -C unpatch clean
	$(MAKE) -C spleef clean
	$(MAKE) -C infected clean
	$(MAKE) -C gun-game clean
	$(MAKE) -C infinite-climber clean
	$(MAKE) -C race clean
	$(MAKE) -C snd clean
	$(MAKE) -C duckhunt clean
	$(MAKE) -C gridiron clean
	$(MAKE) -C thousand-kills clean
	$(MAKE) -C survival clean
	$(MAKE) -C payload clean
	$(MAKE) -C training clean
	$(MAKE) -C benchmark clean
	$(MAKE) -C team-defender clean
	$(MAKE) -C thousand-kills clean
	$(MAKE) -C mapdownloader clean
	$(MAKE) -C anim-extractor clean
	$(MAKE) -C elfloader clean

