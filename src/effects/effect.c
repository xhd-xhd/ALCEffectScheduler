#include <string.h>
#include "effect.h"

// ---- 静态对象池，替代 malloc/free -------------------------------------------
#define EFFECT_POOL_SIZE 32

static Effect g_pool[EFFECT_POOL_SIZE];
static int    g_used[EFFECT_POOL_SIZE];

static Effect *pool_alloc(void) {
    for (int i = 0; i < EFFECT_POOL_SIZE; i++) {
        if (!g_used[i]) {
            g_used[i] = 1;
            memset(&g_pool[i], 0, sizeof(Effect));
            return &g_pool[i];
        }
    }
    return NULL;
}

static void pool_free(Effect *e) {
    for (int i = 0; i < EFFECT_POOL_SIZE; i++) {
        if (&g_pool[i] == e) { g_used[i] = 0; return; }
    }
}

// ---- 默认虚方法 ------------------------------------------------------------

static void effect_init_stub(Effect *e, TimeMs now) {
    e->start_ts = now;
    e->state    = S_ACTIVE;
}

static void effect_update_stub(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    (void)e; (void)now; (void)allowed; (void)out;
}

static void effect_deinit_stub(Effect *e) {
    (void)e;
}

// ---- 公共 API ---------------------------------------------------------------

Effect *effect_create_base(const char *name, int priority, ZoneMask mask) {
    Effect *e = pool_alloc();
    if (!e) return NULL;
    e->name     = name;
    e->priority = priority;
    e->mask     = mask;       // 管辖哪些 zone，工厂设定
    e->state    = S_IDLE;
    e->init     = effect_init_stub;
    e->update   = effect_update_stub;
    e->deinit   = effect_deinit_stub;
    return e;
}

void effect_destroy(Effect *e) {
    // 效果持续存在到 shutdown，运行时不销毁。
    // shutdown 时由 em_shutdown() 调用本函数归还对象池。
    if (!e) return;
    if (e->deinit) e->deinit(e);
    pool_free(e);
}
