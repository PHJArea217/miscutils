.pushsection .data.domain-data
domains_start_local = .
.popsection
.altmacro
.macro domain_stream i, d
.pushsection .domain-stream
.2byte 1
.2byte domain_stream_len_\@
.4byte \i
1:
.asciz \d
.balign 8
domain_stream_len_\@ = . - 1b
.popsection
.endm
.macro domain_ent_nr n, i, d
.pushsection .data.domain-ents
.4byte \i, domains_\n\()_base+dis_\@
.popsection
.pushsection .data.domain-data
.balign 4
dis_\@ = (.-domains_start_local)/4
.asciz \d
.popsection
.endm
.macro master_ent ip0, ip1, ip2, ip3, mask, type, typemacro, typeargs:vararg
.pushsection .data.master-ents
.4byte \ip0, \ip1, \ip2, \ip3
.byte \mask, \type
.2byte ent_\@_size
.4byte ent_\@_off
.4byte 0, 0
.popsection
.pushsection .data.master-data
.balign 8
ent_\@_off = .
\typemacro \typeargs
ent_\@_size = . - ent_\@_off
.popsection
.endm
.macro type1_data domain
.asciz \domain
.endm
.macro type3_data ip0, ip1, ip2, ip3, ifname, bip0, bip1, bip2, bip3, flags
.4byte \ip0, \ip1, \ip2, \ip3
1:
.asciz \ifname
. = 1b+16
.4byte \bip0, \bip1, \bip2, \bip3
.4byte \flags
.endm
.macro type4_data flags, idx_shift, ent_shift, idx_mask, ent_mask, idx_offset, ent_offset, socket
.byte 1, \flags, \idx_shift, \ent_shift
.4byte \idx_mask, \ent_mask, \idx_offset, \ent_offset, 0, 0, 0
.asciz \socket
.endm
