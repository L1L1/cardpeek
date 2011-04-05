	.globl	_binary_dot_cardpeek_tar_gz_start
_binary_dot_cardpeek_tar_gz_start:
	.incbin	"dot_cardpeek.tar.gz"        
	.globl	_binary_dot_cardpeek_tar_gz_end
_binary_dot_cardpeek_tar_gz_end:
#if defined __ELF__ && defined __linux__
	.section	.note.GNU-stack,"",@progbits
#endif
