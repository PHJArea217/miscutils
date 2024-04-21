domains.elf: domains.lds
%-asm.o: %.S
	$(AS) -o $@ $<
domains.lds:
	./gen_seq $(NR_DOMAINS)
