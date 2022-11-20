#include <stdio.h>
#include <immintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <gtest/gtest.h>

struct local_tsc_clocksoure {
    union {
        __uint128_t align_var;
        struct {
            double  coef;
            double offset;
        }__;
    }_;
};

struct tsc_clocksource {
    union {
        __uint128_t align_var;
        struct {
            volatile double     coef;
            volatile double    offset;
        }__;
    }_;
    uint64_t            magic;
} __attribute__((aligned(64)));

TEST(testsimd, testfma)
{
    struct local_tsc_clocksoure local_clocksource;
    local_clocksource._.__.coef = 3.14;
    local_clocksource._.__.offset = 6.28;
    int64_t tsc = 315;
    __m128d d;
    __m128d ftsc = _mm_cvtu64_sd(d, tsc);
    __m128d coef = _mm_load_pd((double const*)&local_clocksource);
    __m128d offset = (__m128d)_mm_movehl_ps((__m128)coef, (__m128)coef);
    __m128d ts = _mm_fmadd_pd(ftsc, coef, offset);
    int64_t nanots = _mm_cvttsd_i64(ts);
    int64_t res = local_clocksource._.__.coef * tsc + local_clocksource._.__.offset;
    ASSERT_EQ(nanots, res);
}

TEST(testsimd, testsse_eq)
{
    struct local_tsc_clocksoure local_clocksource;
    struct tsc_clocksource global_clocksource;
    global_clocksource._.__.coef = 3.14;
    global_clocksource._.__.offset = 6.28;
    local_clocksource._.__.coef = 3.14;
    local_clocksource._.__.offset = 6.28;
    int64_t tsc = 315;
    __m128d d;
    __m128d ftsc = _mm_cvtu64_sd(d, tsc);
    __m128d coef = _mm_load_pd((double const*)&local_clocksource);
    __m128d offset = (__m128d)_mm_movehl_ps((__m128)coef, (__m128)coef);
    __m128d ts = _mm_fmadd_pd(ftsc, coef, offset);
    int64_t nanots = _mm_cvttsd_i64(ts);
    __m128d g_clocksource = _mm_load_pd((double const *)&global_clocksource);
    __m128 xmm_cmp_res = _mm_xor_ps((__m128)g_clocksource, (__m128)coef);
    const uint64_t* r_cmp_res = (const uint64_t*)&xmm_cmp_res;
    ASSERT_EQ(r_cmp_res[0], 0);
    ASSERT_EQ(r_cmp_res[1], 0);
}

TEST(testsimd, testsse_neq)
{
    struct local_tsc_clocksoure local_clocksource;
    struct tsc_clocksource global_clocksource;
    global_clocksource._.__.coef = 3.14;
    global_clocksource._.__.offset = 2.28;
    local_clocksource._.__.coef = 3.14;
    local_clocksource._.__.offset = 6.28;
    int64_t tsc = 315;
    __m128d d;
    __m128d ftsc = _mm_cvtu64_sd(d, tsc);
    __m128d coef = _mm_load_pd((double const*)&local_clocksource);
    __m128d offset = (__m128d)_mm_movehl_ps((__m128)coef, (__m128)coef);
    __m128d ts = _mm_fmadd_pd(ftsc, coef, offset);
    int64_t nanots = _mm_cvttsd_i64(ts);
    __m128d g_clocksource = _mm_load_pd((double const *)&global_clocksource);
    __m128 xmm_cmp_res = _mm_xor_ps((__m128)g_clocksource, (__m128)coef);
    const uint64_t* r_cmp_res = (const uint64_t*)&xmm_cmp_res;
    ASSERT_EQ(r_cmp_res[0], 0);
    ASSERT_NE(r_cmp_res[1], 0);
}

int main(int argc,char**argv){

  testing::InitGoogleTest(&argc,argv);

  return RUN_ALL_TESTS();

}