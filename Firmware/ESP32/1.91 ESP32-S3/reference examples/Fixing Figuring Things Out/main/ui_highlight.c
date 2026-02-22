#include "ui_highlight.h"

/* ===== External system data ===== */
#include "uart_protocol.h"

/* ===== Configuration ===== */
#define UI_HIGHLIGHT_ITEM_COUNT 23

/* ===== External UI objects (SquareLine-generated) ===== */
extern lv_obj_t *ui_index0;
extern lv_obj_t *ui_index1;
extern lv_obj_t *ui_index2;
extern lv_obj_t *ui_index3;
extern lv_obj_t *ui_index4;
extern lv_obj_t *ui_index5;
extern lv_obj_t *ui_index6;
extern lv_obj_t *ui_index7;
extern lv_obj_t *ui_index8;
extern lv_obj_t *ui_index9;
extern lv_obj_t *ui_index10;
extern lv_obj_t *ui_index11;
extern lv_obj_t *ui_index12;
extern lv_obj_t *ui_index13;
extern lv_obj_t *ui_index14;
extern lv_obj_t *ui_index15;
extern lv_obj_t *ui_index16;
extern lv_obj_t *ui_index17;
extern lv_obj_t *ui_index18;
extern lv_obj_t *ui_index19;
extern lv_obj_t *ui_index20;
extern lv_obj_t *ui_index21;
extern lv_obj_t *ui_index22;

/* ===== Second-column objects ===== */
extern lv_obj_t *ui_labelFrameCount;  /* vertical index 2 */
extern lv_obj_t *ui_labelFrameOnT;    /* vertical index 3 */

/* ===== Internal state ===== */
static lv_obj_t *highlight_container = NULL;
static lv_obj_t *items[UI_HIGHLIGHT_ITEM_COUNT];

/* Track focus separately per column */
static lv_obj_t *last_focused_obj = NULL;

static int16_t last_vertical_index = -1;
static int8_t  last_horizontal_index = -1;
static uint8_t last_highlight_flag = 0;

/* ===== Helper: resolve which object should be focused ===== */
static lv_obj_t *get_target_object(int16_t vertical, int8_t horizontal)
{
    /* Column 0 = normal vertical list */
    if (horizontal == 0) {
        return items[vertical];
    }

    /* Column 1 = secondary column (limited rows for now) */
    if (horizontal == 1) {
        switch (vertical) {
            case 2: return ui_labelFrameCount;
            case 3: return ui_labelFrameOnT;
            default: return NULL; /* Not allowed */
        }
    }

    return NULL;
}

void ui_highlight_init(lv_obj_t *scroll_container)
{
    highlight_container = scroll_container;

    /* Store pointers in order (top → bottom) */
    items[0]  = ui_index0;
    items[1]  = ui_index1;
    items[2]  = ui_index2;
    items[3]  = ui_index3;
    items[4]  = ui_index4;
    items[5]  = ui_index5;
    items[6]  = ui_index6;
    items[7]  = ui_index7;
    items[8]  = ui_index8;
    items[9]  = ui_index9;
    items[10] = ui_index10;
    items[11] = ui_index11;
    items[12] = ui_index12;
    items[13] = ui_index13;
    items[14] = ui_index14;
    items[15] = ui_index15;
    items[16] = ui_index16;
    items[17] = ui_index17;
    items[18] = ui_index18;
    items[19] = ui_index19;
    items[20] = ui_index20;
    items[21] = ui_index21;
    items[22] = ui_index22;

    /* Clear any residual focus */
    for (int i = 0; i < UI_HIGHLIGHT_ITEM_COUNT; i++) {
        lv_obj_clear_state(items[i], LV_STATE_FOCUSED);
    }

    lv_obj_clear_state(ui_labelFrameCount, LV_STATE_FOCUSED);
    lv_obj_clear_state(ui_labelFrameOnT, LV_STATE_FOCUSED);

    last_focused_obj = NULL;
    last_vertical_index = -1;
    last_horizontal_index = -1;
    last_highlight_flag = 0;
}

void ui_highlight_update(void)
{
    int16_t vertical = g_system_rx.verticalHighlight;
    int8_t  horizontal = g_system_rx.horizontalHighlight;
    uint8_t highlight = g_system_rx.highlightFlag;

    /* Clamp vertical */
    if (vertical < 0) vertical = 0;
    if (vertical >= UI_HIGHLIGHT_ITEM_COUNT)
        vertical = UI_HIGHLIGHT_ITEM_COUNT - 1;

    lv_obj_t *target = get_target_object(vertical, horizontal);

    /* ===== Highlight OFF ===== */
    if (!highlight) {
        if (last_highlight_flag && last_focused_obj) {
            lv_obj_clear_state(last_focused_obj, LV_STATE_FOCUSED);
            lv_obj_update_layout(last_focused_obj);
            lv_obj_invalidate(last_focused_obj);
        }

        last_focused_obj = NULL;
        last_highlight_flag = 0;
        last_vertical_index = vertical;
        last_horizontal_index = horizontal;
        return;
    }

    /* ===== Highlight ON ===== */
    last_highlight_flag = 1;

    /* If target is invalid (horizontal not allowed on this row) */
    if (!target) {
        return;
    }

    /* Remove focus from previous object */
    if (last_focused_obj && last_focused_obj != target) {
        lv_obj_clear_state(last_focused_obj, LV_STATE_FOCUSED);
        lv_obj_update_layout(last_focused_obj);
        lv_obj_invalidate(last_focused_obj);
    }

    /* Apply focus to new object */
    lv_obj_add_state(target, LV_STATE_FOCUSED);
    lv_obj_update_layout(target);
    lv_obj_invalidate(target);

    /* Scroll only when in main column */
    if (highlight_container && horizontal == 0) {
        lv_obj_update_layout(highlight_container);
        lv_obj_scroll_to_view(target, LV_ANIM_ON);
    }

    last_focused_obj = target;
    last_vertical_index = vertical;
    last_horizontal_index = horizontal;
}
