#include "types.h"

#define ELECMAG_SELA 16
#define ELECMAG_SELB 17

void electromag_init();
void electromagnet_on(piece_color_t color);
void electromagnet_off();
