	.data
#ifdef _WIN32
	.globl	__binary_dot_cardpeek_tar_gz_start
	.globl  __binary_dot_cardpeek_tar_gz_size
	.globl	__binary_dot_cardpeek_tar_gz_end

__binary_dot_cardpeek_tar_gz_size:
	.int	__binary_dot_cardpeek_tar_gz_end-__binary_dot_cardpeek_tar_gz_start
__binary_dot_cardpeek_tar_gz_start:
	.incbin	"dot_cardpeek.tar.gz"
__binary_dot_cardpeek_tar_gz_end:

#else

	.globl	_binary_dot_cardpeek_tar_gz_start
	.globl  _binary_dot_cardpeek_tar_gz_size
	.globl	_binary_dot_cardpeek_tar_gz_end

_binary_dot_cardpeek_tar_gz_size:
	.int	_binary_dot_cardpeek_tar_gz_end-_binary_dot_cardpeek_tar_gz_start
_binary_dot_cardpeek_tar_gz_start:
	.incbin	"dot_cardpeek.tar.gz"        
_binary_dot_cardpeek_tar_gz_end:

#endif
#if defined __ELF__ && defined __linux__
	.section	.note.GNU-stack,"",@progbits
#endif
