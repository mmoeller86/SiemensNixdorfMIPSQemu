/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2021 Loongson Technology Corporation Limited
 */

static bool gen_rr(DisasContext *ctx, arg_rr *a,
                   DisasExtend src_ext, DisasExtend dst_ext,
                   void (*func)(TCGv, TCGv))
{
    TCGv dest = gpr_dst(ctx, a->rd, dst_ext);
    TCGv src1 = gpr_src(ctx, a->rj, src_ext);

    func(dest, src1);
    gen_set_gpr(a->rd, dest, dst_ext);

    return true;
}

static void gen_bytepick_w(TCGv dest, TCGv src1, TCGv src2, target_long sa)
{
    tcg_gen_concat_tl_i64(dest, src1, src2);
    tcg_gen_sextract_i64(dest, dest, (32 - sa * 8), 32);
}

static void gen_bytepick_d(TCGv dest, TCGv src1, TCGv src2, target_long sa)
{
    tcg_gen_extract2_i64(dest, src1, src2, (64 - sa * 8));
}

static bool gen_bstrins(DisasContext *ctx, arg_rr_ms_ls *a,
                        DisasExtend dst_ext)
{
    TCGv src1 = gpr_src(ctx, a->rd, EXT_NONE);
    TCGv src2 = gpr_src(ctx, a->rj, EXT_NONE);
    TCGv dest = gpr_dst(ctx, a->rd, EXT_NONE);

    if (a->ls > a->ms) {
        return false;
    }

    tcg_gen_deposit_tl(dest, src1, src2, a->ls, a->ms - a->ls + 1);
    gen_set_gpr(a->rd, dest, dst_ext);
    return true;
}

static bool gen_bstrpick(DisasContext *ctx, arg_rr_ms_ls *a,
                         DisasExtend dst_ext)
{
    TCGv dest = gpr_dst(ctx, a->rd, EXT_NONE);
    TCGv src1 = gpr_src(ctx, a->rj, EXT_NONE);

    if (a->ls > a->ms) {
        return false;
    }

    tcg_gen_extract_tl(dest, src1, a->ls, a->ms - a->ls + 1);
    gen_set_gpr(a->rd, dest, dst_ext);
    return true;
}

static void gen_clz_w(TCGv dest, TCGv src1)
{
    tcg_gen_clzi_tl(dest, src1, TARGET_LONG_BITS);
    tcg_gen_subi_tl(dest, dest, TARGET_LONG_BITS - 32);
}

static void gen_clo_w(TCGv dest, TCGv src1)
{
    tcg_gen_not_tl(dest, src1);
    tcg_gen_ext32u_tl(dest, dest);
    gen_clz_w(dest, dest);
}

static void gen_ctz_w(TCGv dest, TCGv src1)
{
    tcg_gen_ori_tl(dest, src1, (target_ulong)MAKE_64BIT_MASK(32, 32));
    tcg_gen_ctzi_tl(dest, dest, TARGET_LONG_BITS);
}

static void gen_cto_w(TCGv dest, TCGv src1)
{
    tcg_gen_not_tl(dest, src1);
    gen_ctz_w(dest, dest);
}

static void gen_clz_d(TCGv dest, TCGv src1)
{
    tcg_gen_clzi_i64(dest, src1, TARGET_LONG_BITS);
}

static void gen_clo_d(TCGv dest, TCGv src1)
{
    tcg_gen_not_tl(dest, src1);
    gen_clz_d(dest, dest);
}

static void gen_ctz_d(TCGv dest, TCGv src1)
{
    tcg_gen_ctzi_tl(dest, src1, TARGET_LONG_BITS);
}

static void gen_cto_d(TCGv dest, TCGv src1)
{
    tcg_gen_not_tl(dest, src1);
    gen_ctz_d(dest, dest);
}

static void gen_revb_2w(TCGv dest, TCGv src1)
{
    tcg_gen_bswap64_i64(dest, src1);
    tcg_gen_rotri_i64(dest, dest, 32);
}

static void gen_revb_2h(TCGv dest, TCGv src1)
{
    TCGv mask = tcg_constant_tl(0x00FF00FF);
    TCGv t0 = tcg_temp_new();
    TCGv t1 = tcg_temp_new();

    tcg_gen_shri_tl(t0, src1, 8);
    tcg_gen_and_tl(t0, t0, mask);
    tcg_gen_and_tl(t1, src1, mask);
    tcg_gen_shli_tl(t1, t1, 8);
    tcg_gen_or_tl(dest, t0, t1);
}

static void gen_revb_4h(TCGv dest, TCGv src1)
{
    TCGv mask = tcg_constant_tl(0x00FF00FF00FF00FFULL);
    TCGv t0 = tcg_temp_new();
    TCGv t1 = tcg_temp_new();

    tcg_gen_shri_tl(t0, src1, 8);
    tcg_gen_and_tl(t0, t0, mask);
    tcg_gen_and_tl(t1, src1, mask);
    tcg_gen_shli_tl(t1, t1, 8);
    tcg_gen_or_tl(dest, t0, t1);
}

static void gen_revh_2w(TCGv dest, TCGv src1)
{
    TCGv_i64 t0 = tcg_temp_new_i64();
    TCGv_i64 t1 = tcg_temp_new_i64();
    TCGv_i64 mask = tcg_constant_i64(0x0000ffff0000ffffull);

    tcg_gen_shri_i64(t0, src1, 16);
    tcg_gen_and_i64(t1, src1, mask);
    tcg_gen_and_i64(t0, t0, mask);
    tcg_gen_shli_i64(t1, t1, 16);
    tcg_gen_or_i64(dest, t1, t0);
}

static void gen_revh_d(TCGv dest, TCGv src1)
{
    TCGv t0 = tcg_temp_new();
    TCGv t1 = tcg_temp_new();
    TCGv mask = tcg_constant_tl(0x0000FFFF0000FFFFULL);

    tcg_gen_shri_tl(t1, src1, 16);
    tcg_gen_and_tl(t1, t1, mask);
    tcg_gen_and_tl(t0, src1, mask);
    tcg_gen_shli_tl(t0, t0, 16);
    tcg_gen_or_tl(t0, t0, t1);
    tcg_gen_rotri_tl(dest, t0, 32);
}

static void gen_maskeqz(TCGv dest, TCGv src1, TCGv src2)
{
    TCGv zero = tcg_constant_tl(0);

    tcg_gen_movcond_tl(TCG_COND_EQ, dest, src2, zero, zero, src1);
}

static void gen_masknez(TCGv dest, TCGv src1, TCGv src2)
{
    TCGv zero = tcg_constant_tl(0);

    tcg_gen_movcond_tl(TCG_COND_NE, dest, src2, zero, zero, src1);
}

TRANS(ext_w_h, ALL, gen_rr, EXT_NONE, EXT_NONE, tcg_gen_ext16s_tl)
TRANS(ext_w_b, ALL, gen_rr, EXT_NONE, EXT_NONE, tcg_gen_ext8s_tl)
TRANS(clo_w, ALL, gen_rr, EXT_NONE, EXT_NONE, gen_clo_w)
TRANS(clz_w, ALL, gen_rr, EXT_ZERO, EXT_NONE, gen_clz_w)
TRANS(cto_w, ALL, gen_rr, EXT_NONE, EXT_NONE, gen_cto_w)
TRANS(ctz_w, ALL, gen_rr, EXT_NONE, EXT_NONE, gen_ctz_w)
TRANS(clo_d, 64, gen_rr, EXT_NONE, EXT_NONE, gen_clo_d)
TRANS(clz_d, 64, gen_rr, EXT_NONE, EXT_NONE, gen_clz_d)
TRANS(cto_d, 64, gen_rr, EXT_NONE, EXT_NONE, gen_cto_d)
TRANS(ctz_d, 64, gen_rr, EXT_NONE, EXT_NONE, gen_ctz_d)
TRANS(revb_2h, ALL, gen_rr, EXT_NONE, EXT_SIGN, gen_revb_2h)
TRANS(revb_4h, 64, gen_rr, EXT_NONE, EXT_NONE, gen_revb_4h)
TRANS(revb_2w, 64, gen_rr, EXT_NONE, EXT_NONE, gen_revb_2w)
TRANS(revb_d, 64, gen_rr, EXT_NONE, EXT_NONE, tcg_gen_bswap64_i64)
TRANS(revh_2w, 64, gen_rr, EXT_NONE, EXT_NONE, gen_revh_2w)
TRANS(revh_d, 64, gen_rr, EXT_NONE, EXT_NONE, gen_revh_d)
TRANS(bitrev_4b, ALL, gen_rr, EXT_ZERO, EXT_SIGN, gen_helper_bitswap)
TRANS(bitrev_8b, 64, gen_rr, EXT_NONE, EXT_NONE, gen_helper_bitswap)
TRANS(bitrev_w, ALL, gen_rr, EXT_NONE, EXT_SIGN, gen_helper_bitrev_w)
TRANS(bitrev_d, 64, gen_rr, EXT_NONE, EXT_NONE, gen_helper_bitrev_d)
TRANS(maskeqz, ALL, gen_rrr, EXT_NONE, EXT_NONE, EXT_NONE, gen_maskeqz)
TRANS(masknez, ALL, gen_rrr, EXT_NONE, EXT_NONE, EXT_NONE, gen_masknez)
TRANS(bytepick_w, ALL, gen_rrr_sa, EXT_NONE, EXT_NONE, gen_bytepick_w)
TRANS(bytepick_d, 64, gen_rrr_sa, EXT_NONE, EXT_NONE, gen_bytepick_d)
TRANS(bstrins_w, ALL, gen_bstrins, EXT_SIGN)
TRANS(bstrins_d, 64, gen_bstrins, EXT_NONE)
TRANS(bstrpick_w, ALL, gen_bstrpick, EXT_SIGN)
TRANS(bstrpick_d, 64, gen_bstrpick, EXT_NONE)
