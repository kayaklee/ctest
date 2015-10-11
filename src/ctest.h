#ifndef CTEST_TEST_H_
#define CTEST_TEST_H_

#include <ctest_define.h>
#include <ctest_list.h>
#include <ctest_hash.h>
#include <ctest_pool.h>
#include <ctest_atomic.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/time.h>
#include <ctest_string.h>

CTEST_CPP_START

typedef struct ctest_test_func_t ctest_test_func_t;
typedef struct ctest_test_case_t ctest_test_case_t;
typedef struct cmdline_param_t cmdline_param_t;
typedef void ctest_test_func_pt();

struct ctest_test_case_t {
    const char                *case_name;
    ctest_test_func_pt         *fcsetup;
    ctest_test_func_pt         *fcdown;
    ctest_test_func_pt         *fsetup;
    ctest_test_func_pt         *fdown;
    ctest_list_t               listnode;
    ctest_hash_list_t          hash_node;
    ctest_list_t               list;
    int                       list_cnt;
};

// struct test
struct ctest_test_func_t {
    ctest_test_case_t          *tc;
    const char                *func_name;
    ctest_test_func_pt         *func;
    ctest_list_t               listnode;
    int                       ret;
};
// cmdline parameter
struct cmdline_param_t {
    const char                *filter_str;
    int                       filter_str_len;
    int                       filter_flags;
};

#define CTEST_TEST_COLOR_RED   1
#define CTEST_TEST_COLOR_GREEN 2

// extern
extern ctest_pool_t      *ctest_test_pool;
extern ctest_hash_t      *ctest_test_case_table;
extern ctest_list_t      ctest_test_case_list;
extern int              ctest_test_retval;
extern ctest_atomic_t    ctest_test_alloc_byte;

// color printf
static inline void ctest_test_color_printf(int color, const char *fmt, ...)
{
    va_list                 args;
    va_start(args, fmt);
    printf("\033[0;3%dm", color);
    vprintf(fmt, args);
    printf("\033[m");
    va_end(args);
}

static int64_t ctest_test_now()
{
    struct timeval          tv;
    gettimeofday (&tv, 0);
    return 1000L * tv.tv_sec + tv.tv_usec / 1000;
}

static int ctest_test_case_cmp(const void *a, const void *b)
{
    ctest_test_case_t        *tc = (ctest_test_case_t *) b;
    return strcmp(tc->case_name, (const char *)a);
}

static ctest_test_case_t *ctest_test_get_tc(const char *case_name)
{
    ctest_test_case_t        *tc;
    uint64_t                key;

    if (ctest_test_pool == NULL) {
        ctest_test_pool = ctest_pool_create(1024);
        ctest_test_case_table = ctest_hash_create(ctest_test_pool, 128,
                                                offsetof(ctest_test_case_t, hash_node));
    }

    // 处理case
    key = ctest_hash_code(case_name, strlen(case_name), 3);
    tc = (ctest_test_case_t *)ctest_hash_find_ex(ctest_test_case_table, key, ctest_test_case_cmp, case_name);

    if (tc == NULL) {
        tc = (ctest_test_case_t *)ctest_pool_calloc(ctest_test_pool, sizeof(ctest_test_case_t));
        tc->case_name = case_name;
        ctest_list_init(&tc->list);
        ctest_hash_add(ctest_test_case_table, key, &tc->hash_node);
        ctest_list_add_head(&tc->listnode, &ctest_test_case_list);
    }

    return tc;
}

static inline void ctest_test_reg_func(const char *case_name, const char *func_name,
                                      ctest_test_func_pt *func, int before)
{
    ctest_test_func_t        *t;
    ctest_test_case_t        *tc;

    tc = ctest_test_get_tc(case_name);
    t = (ctest_test_func_t *)ctest_pool_calloc(ctest_test_pool, sizeof(ctest_test_func_t));
    t->func_name = func_name;
    t->func = func;
    t->tc = tc;

    ctest_list_add_head(&t->listnode, &tc->list);
    tc->list_cnt ++;
}

static inline void ctest_test_print_usage(char *prog_name)
{
    fprintf(stderr, "%s [-f [-]filter_string]\n"
            "    -f, --filter            filter string\n"
            "    -l, --list              list tests\n"
            "    -h, --help              display this help and exit\n"
            "    -V, --version           version and build time\n\n", prog_name);
}

/**
 * 打印tests case
 */
static inline void ctest_test_print_list()
{
    ctest_test_case_t        *tc;
    ctest_test_func_t        *t;

    // foreach
    ctest_list_for_each_entry(tc, &ctest_test_case_list, listnode) {
        printf("%s.\n", tc->case_name);
        ctest_list_for_each_entry(t, &tc->list, listnode) {
            printf("  %s.%s\n", tc->case_name, t->func_name);
        }
    }
}

static inline int ctest_test_is_skip(const char *str, cmdline_param_t *cp)
{
    int                     ret;

    if (cp->filter_str_len > 0) {
        ret = strncmp(cp->filter_str, str, cp->filter_str_len);

        if ((ret != 0 && cp->filter_flags == 0) || (ret == 0 && cp->filter_flags != 0))
            return 1;
    } else if (cp->filter_str_len < 0) {
        ret = strcmp(cp->filter_str, str);

        if ((ret != 0 && cp->filter_flags == 0) || (ret == 0 && cp->filter_flags != 0))
            return 1;
    }

    return 0;
}

/**
 * 解析命令行
 */
static inline int ctest_test_parse_cmd_line(int argc, char *const argv[], cmdline_param_t *cp)
{
    int                     opt, len;
    const char              *opt_string = "hVf:l";
    struct option           long_opts[] = {
        {"filter", 1, NULL, 'f'},
        {"list", 0, NULL, 'l'},
        {"help", 0, NULL, 'h'},
        {"version", 0, NULL, 'V'},
        {0, 0, 0, 0}
    };

    opterr = 0;

    while ((opt = getopt_long(argc, argv, opt_string, long_opts, NULL)) != -1) {
        switch (opt) {
        case 'f':

            if (*optarg == '-') {
                cp->filter_flags = 1;
                cp->filter_str = optarg + 1;
            } else {
                cp->filter_str = optarg;
            }

            len = strlen(cp->filter_str);

            if (len > 0 && (cp->filter_str[len - 1] == '*' || cp->filter_str[len - 1] == '?'))
                cp->filter_str_len = len - 1;
            else
                cp->filter_str_len = -1;

            break;

        case 'l':
            ctest_test_print_list();
            return CTEST_ERROR;

        case 'V':
            fprintf(stderr, "BUILD_TIME: %s %s\n", __DATE__, __TIME__);
            return CTEST_ERROR;

        case 'h':
        default:
            ctest_test_print_usage(argv[0]);
            return CTEST_ERROR;
        }
    }

    return CTEST_OK;
}

static inline void *ctest_test_realloc (void *ptr, size_t size)
{
    char                    *p1, *p = (char *)ptr;

    if (p) {
        p -= 8;
        ctest_atomic_add(&ctest_test_alloc_byte, -(*((int *)p)));
    }

    if (size) {
        ctest_atomic_add(&ctest_test_alloc_byte, size);
        p1 = (char *)ctest_realloc(p, size + 8);

        if (p1) {
            *((int *)p1) = size;
            p1 += 8;
        }

        return p1;
    } else if (p) {
        ctest_free(p);
    }

    return NULL;
}

static int ctest_test_exec_case(ctest_test_case_t *tc)
{
    ctest_test_func_t        *t;
    int64_t                 t1, t2;
    int                     failcnt = 0;

    ctest_test_color_printf(CTEST_TEST_COLOR_GREEN, "[----------]");
    printf(" %d tests from %s\n", tc->list_cnt, tc->case_name);

    if (tc->fcsetup) (*tc->fcsetup)();

    ctest_list_for_each_entry(t, &tc->list, listnode) {
        // start run
        ctest_test_color_printf(CTEST_TEST_COLOR_GREEN, "[ RUN      ]");
        printf(" %s.%s\n", tc->case_name, t->func_name);

        ctest_test_retval = 0;
        t1 = ctest_test_now();

        if (tc->fsetup) (*tc->fsetup)();

        (t->func)();

        if (tc->fdown) (*tc->fdown)();

        t2 = ctest_test_now();
        t->ret = ctest_test_retval;

        // failure
        if (ctest_test_retval) {
            ctest_test_color_printf(CTEST_TEST_COLOR_RED, "[  FAILED  ]");
            failcnt ++;
        } else {
            ctest_test_color_printf(CTEST_TEST_COLOR_GREEN, "[       OK ]");
        }

        printf(" %s.%s (%d ms)\n", tc->case_name, t->func_name, (int)(t2 - t1));
    }

    if (tc->fcdown) (*tc->fcdown)();

    ctest_test_color_printf(CTEST_TEST_COLOR_GREEN, "[----------]");
    printf(" %d tests from %s\n\n", tc->list_cnt, tc->case_name);
    return failcnt;
}

static inline int ctest_test_main(int argc, char *argv[])
{
    ctest_test_case_t        *tc, *tc1;
    ctest_test_func_t        *t, *nt;
    int64_t                 t1, t2;
    int                     total_failcnt, total_func_cnt, total_case_cnt;
    char                    test_func_name[256];
    cmdline_param_t         cp;

    // parse cmd
    memset(&cp, 0, sizeof(cmdline_param_t));

    if (ctest_test_parse_cmd_line(argc, argv, &cp) == CTEST_ERROR) {
        return -1;
    }

    // init
    ctest_test_color_printf(CTEST_TEST_COLOR_GREEN, "[==========]");

    // 计算个数
    total_func_cnt = 0;
    total_case_cnt = 0;
    total_failcnt = 0;
    ctest_list_for_each_entry_safe(tc, tc1, &ctest_test_case_list, listnode) {
        if (cp.filter_str_len) {
            ctest_list_for_each_entry_safe(t, nt, &tc->list, listnode) {
                snprintf(test_func_name, 256, "%s.%s", tc->case_name, t->func_name);

                if (ctest_test_is_skip(test_func_name, &cp)) {
                    ctest_list_del(&t->listnode);
                    tc->list_cnt --;
                }
            }
        }

        if (tc->list_cnt == 0) {
            ctest_list_del(&tc->listnode);
        } else {
            total_func_cnt += tc->list_cnt;
            total_case_cnt ++;
        }
    }

    printf(" Running %d tests from %d cases.\n", total_func_cnt, total_case_cnt);

    t1 = ctest_test_now();
    ctest_pool_set_allocator(ctest_test_realloc);
    ctest_list_for_each_entry(tc, &ctest_test_case_list, listnode) {
        total_failcnt += ctest_test_exec_case(tc);
    }
    t2 = ctest_test_now();

    ctest_test_color_printf(CTEST_TEST_COLOR_GREEN, "[==========]");
    printf(" %d tests ran. (%d ms total)\n", total_func_cnt, (int)(t2 - t1));
    ctest_test_color_printf(CTEST_TEST_COLOR_GREEN, "[  PASSED  ]");
    printf(" %d tests.\n", total_func_cnt - total_failcnt);

    if (total_failcnt > 0) {
        ctest_test_color_printf(CTEST_TEST_COLOR_RED, "[  FAILED  ]");
        printf(" %d tests, listed below:\n", total_failcnt);
        ctest_list_for_each_entry(tc, &ctest_test_case_list, listnode) {
            ctest_list_for_each_entry(t, &tc->list, listnode) {
                if (!t->ret) continue;

                ctest_test_color_printf(CTEST_TEST_COLOR_RED, "[  FAILED  ]");
                printf(" %s.%s\n", tc->case_name, t->func_name);
            }
        }

        printf(" %d FAILED TEST\n", total_failcnt);
    }

    if (ctest_test_alloc_byte) {
        printf("no free memory: %ld\n", (long)ctest_test_alloc_byte);
    }

    return (total_failcnt > 0 ? 1 : 0);
}

#define CTEST_TEST_MAIN_DEFINE                                                           \
    int                     ctest_test_retval = 0;                                                           \
    ctest_atomic_t           ctest_test_alloc_byte = 0;                                             \
    ctest_pool_t             *ctest_test_pool = NULL;                                                 \
    ctest_hash_t             *ctest_test_case_table = NULL;                                           \
    ctest_list_t             ctest_test_case_list = CTEST_LIST_HEAD_INIT(ctest_test_case_list);         \
    __attribute__((destructor)) void unregtest_main() {                                 \
        ctest_pool_set_allocator(NULL);                                                  \
        if (ctest_test_pool) ctest_pool_destroy(ctest_test_pool);                          \
    }

#define RUN_TEST_MAIN                                                                   \
    CTEST_TEST_MAIN_DEFINE                                                               \
    int main(int argc, char *argv[]) {                                                  \
        return ctest_test_main(argc, argv);                                              \
    }

#define TEST_NAME(case_name, func_name) ctest_testf_##case_name##_##func_name
#define TEST_CASE(case_name, func_name) ctest_testc_##case_name##_##func_name
// TEST
#define TEST(case_name, func_name)                                                      \
    void TEST_NAME(case_name, func_name)();                                             \
    __attribute__((constructor)) void ctest_testg_##case_name##_##func_name() {          \
        ctest_test_reg_func(#case_name, #func_name,                                      \
                           TEST_NAME(case_name, func_name), 1);                         \
    }                                                                                   \
    void TEST_NAME(case_name, func_name)()

#define TEST_SETUP_DOWN(case_name, func_name)                                           \
    void TEST_CASE(case_name, func_name)();                                             \
    __attribute__((constructor)) void ctest_testd_##case_name##_##func_name() {          \
        ctest_test_case_t        *tc = ctest_test_get_tc(#case_name);                            \
        tc->f##func_name = TEST_CASE(case_name, func_name);                             \
    }                                                                                   \
    void TEST_CASE(case_name, func_name)()

#define TEST_CASE_SETUP(case_name) TEST_SETUP_DOWN(case_name, csetup)
#define TEST_CASE_DOWN(case_name) TEST_SETUP_DOWN(case_name, cdown)

#define TEST_SETUP(case_name) TEST_SETUP_DOWN(case_name, setup)
#define TEST_DOWN(case_name) TEST_SETUP_DOWN(case_name, down)

// TEST_FAIL
#define TEST_FAIL(fmt, args...)                                                         \
    printf("ERROR at %s:%d, TEST_FAIL: " fmt "\n",                                  \
           __FILE__, __LINE__, ## args); ctest_test_retval = 1;

// EXPECT_TRUE
#define EXPECT_TRUE(c) if(!(c)) {                                                       \
        printf("ERROR at %s:%d, EXPECT_TRUE(" #c ")\n",                                 \
               __FILE__, __LINE__); ctest_test_retval = 1;}

// EXPECT_FALSE
#define EXPECT_FALSE(c) if((c)) {                                                       \
        printf("ERROR at %s:%d, EXPECT_FALSE(" #c ")\n",                                \
               __FILE__, __LINE__); ctest_test_retval = 1;}

CTEST_CPP_END

#ifndef __cplusplus
// EXPECT_EQ
#define EXPECT_EQ(a, b) if(!((int64_t)(a)==(int64_t)(b))) {                                 \
        printf("ERROR at %s: %d, EXPECT_EQ(" #a ", " #b ") (a=%" PRId64 ",b=%" PRId64 ")\n",\
               __FILE__, __LINE__, (int64_t)(a), (int64_t)(b)); ctest_test_retval = 1;}

// EXPECT_NE
#define EXPECT_NE(a, b) if(((int64_t)(a)==(int64_t)(b))) {                                  \
        printf("ERROR at %s: %d, EXPECT_NE(" #a ", " #b ") (a=%" PRId64 ",b=%" PRId64 ")\n",\
               __FILE__, __LINE__, (int64_t)(a), (int64_t)(b)); ctest_test_retval = 1;}

#else

#include <string>
#define CTEST_STRINGIFY(x) #x
#define CTEST_TOSTRING(x) CTEST_STRINGIFY(x)

// EXPECT_EQ
#define EXPECT_EQ(a, b)                                                                     \
    EXPECT_CTEST_EQ(__FILE__ ":" CTEST_TOSTRING(__LINE__) ", EXPECT_EQ(" #a ", " #b ")",  \
                   (a), (b), false)

// EXPECT_NE
#define EXPECT_NE(a, b)                                                                     \
    EXPECT_CTEST_EQ(__FILE__ ":" CTEST_TOSTRING(__LINE__) ", EXPECT_NE(" #a ", " #b ")",  \
                   (a), (b), true)

template <typename A, typename B>
static inline void EXPECT_CTEST_EQ(const char *location, const A &a, const B &b, bool neq)
{
    if (!((int64_t)(a) == (int64_t)(b)) ^ neq) {
        printf("ERROR at %s (a=%" PRId64 ",b=%" PRId64 ")\n",
               location, (int64_t)(a), (int64_t)(b));
        ctest_test_retval = 1;
    }
}

static inline void EXPECT_CTEST_EQ(
    const char *location, const std::string &a, const std::string &b, bool neq)
{
    if (!(a == b) ^ neq) {
        printf("ERROR at %s\na=%s\nb=%s\n",
               location, a.c_str(), b.c_str());
        ctest_test_retval = 1;
    }
}

static inline void EXPECT_CTEST_EQ(
    const char *location, const std::string &a, const char *const bcs, bool neq)
{
    EXPECT_CTEST_EQ(location, a, std::string(bcs), neq);
}

static inline void EXPECT_CTEST_EQ(
    const char *location, const char *const acs, const std::string &b, bool neq)
{
    EXPECT_CTEST_EQ(location, std::string(acs), b, neq);
}

static inline void EXPECT_CTEST_EQ(
    const char *location, const char *const acs, const char *const bcs, bool neq)
{
    EXPECT_CTEST_EQ(location, std::string(acs), std::string(bcs), neq);
}
#endif

#endif
