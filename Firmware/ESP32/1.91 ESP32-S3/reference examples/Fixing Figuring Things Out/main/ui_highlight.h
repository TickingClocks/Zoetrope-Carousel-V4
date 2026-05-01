#ifndef UI_HIGHLIGHT_H
#define UI_HIGHLIGHT_H

#include "lvgl.h"

/* Initialize highlight system
 * scroll_container = the vertical scrollable container
 */
void ui_highlight_init(lv_obj_t *scroll_container);

/* Call periodically to update highlight state */
void ui_highlight_update(void);

#endif /* UI_HIGHLIGHT_H */
