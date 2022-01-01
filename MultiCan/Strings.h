#if defined(_LANG_ENG)
	#include "../Languages/English.h"
#elif defined(_LANG_RUS)
	#include "../Languages/Russian.h"
#else
	#error "Language variable is not set"
#endif
