/* Native Linux flex emits POSIX spellings; Windows UCRT uses these names. */
#if defined(_WIN32) && defined(_MSC_VER) && !defined(__ASSEMBLER__)
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#endif
