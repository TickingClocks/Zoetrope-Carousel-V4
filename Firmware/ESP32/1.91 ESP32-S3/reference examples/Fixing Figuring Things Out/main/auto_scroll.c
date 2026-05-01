#include "auto_scroll.h"
#include "ui.h"

#define AUTO_SCROLL_DELAY_MS   3000
#define AUTO_SCROLL_OFFSET_PX  -260
#define AUTO_SCROLL_ANIM_TIME  1200   // ms

typedef struct {
    lv_obj_t *obj;
    int32_t last_value;
} scroll_anim_ctx_t;

/* Animation exec callback — scroll by delta */
static void scroll_anim_exec_cb(void *var, int32_t value)
{
    scroll_anim_ctx_t *ctx = (scroll_anim_ctx_t *)var;
    int32_t delta = value - ctx->last_value;
    ctx->last_value = value;

    lv_obj_scroll_by(ctx->obj, 0, delta, LV_ANIM_OFF);
}

/* Proper deleted callback */
static void scroll_anim_deleted_cb(lv_anim_t *a)
{
    if (a && a->var) {
        lv_mem_free(a->var);
    }
}

static void auto_scroll_cb(lv_timer_t *timer)
{
    (void)timer;

    if (!ui_Screen1) return;

    lv_obj_update_layout(ui_Screen1);

    scroll_anim_ctx_t *ctx = lv_mem_alloc(sizeof(scroll_anim_ctx_t));
    if (!ctx) return;

    ctx->obj = ui_Screen1;
    ctx->last_value = 0;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ctx);
    lv_anim_set_exec_cb(&a, scroll_anim_exec_cb);
    lv_anim_set_deleted_cb(&a, scroll_anim_deleted_cb);
    lv_anim_set_time(&a, AUTO_SCROLL_ANIM_TIME);
    lv_anim_set_values(&a, 0, AUTO_SCROLL_OFFSET_PX);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);

    /* One-shot timer */
    lv_timer_del(timer);
}

void auto_scroll_start(void)
{
    if (!ui_Screen1) return;

    lv_obj_add_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);

    lv_timer_t *timer = lv_timer_create(
        auto_scroll_cb,
        AUTO_SCROLL_DELAY_MS,
        NULL
    );

    lv_timer_set_repeat_count(timer, 1);
}
