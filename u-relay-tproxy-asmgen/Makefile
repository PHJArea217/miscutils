# Can be any big-endian arch
CROSS_COMPILE := mips-linux-gnu-
AS := $(CROSS_COMPILE)as
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
NR_DOMAINS := 1
MASTER_ELF_FLAGS := --defsym=ipv6_prefix_hi=0x26020806 --defsym=ipv6_prefix_lo=0xa0020000
DOMAIN_ELF_FLAGS :=
all: obj/domains.bin obj/master.bin
%-asm.o: %.S
	$(AS) -o $@ $<
obj/domains.bin: obj/domains.elf
	$(OBJCOPY) -O binary obj/domains.elf obj/domains.bin -j .data
obj/master.bin: obj/master.elf
	$(OBJCOPY) -O binary obj/master.elf obj/master.bin.a -j .data
	$(OBJCOPY) -O binary obj/master.elf obj/master.bin.b -j .master-data
	cat obj/master.bin.a obj/master.bin.b > obj/master.bin
obj/domains.elf: obj/domains.o
	$(LD) $(DOMAIN_ELF_FLAGS) -o $@ $<
obj/master.elf: obj/master.o
	$(LD) $(MASTER_ELF_FLAGS) -o $@ --section-start=.master-data=0 $<
obj/domains.o: obj/domains.lds gen_domain_s
	$(LD) -T obj/domains.lds -r -o $@
obj/master.o: obj/master.lds gen_master_s
	$(LD) -T obj/master.lds -r -o $@
obj/domains.lds:
	./gen_seq $(NR_DOMAINS) domains_lds
obj/master.lds:
	./gen_seq $(NR_DOMAINS) master_lds
gen_domain_s:
	./gen_seq $(NR_DOMAINS) domains_s
	./gen_seq $(NR_DOMAINS) compile_domains_s $(AS)
gen_master_s:
	./gen_seq $(NR_DOMAINS) master_s
	./gen_seq $(NR_DOMAINS) compile_master_s $(AS)
clean:
	rm -f obj/*.o obj/*.gen.S obj/*.lds
.PHONY: all clean gen_domain_s
