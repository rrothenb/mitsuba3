import pytest
import drjit as dr
import mitsuba as mi

# TODO this is way too slow
# @pytest.mark.slow
# def test01_chi2_smooth(variants_vec_backends_once_rgb):
#     from mitsuba.core import mi.ScalarVector3f
#     xml = """<float name="alpha" value="0.05"/>"""
#     wi = dr.normalize(mi.ScalarVector3f(0.8, 0.3, 0.05))
#     sample_func, pdf_func = mi.chi2.BSDFAdapter("roughdielectric", xml, wi=wi)

#     chi2 = mi.chi2.ChiSquareTest(
#         domain=mi.chi2.SphericalDomain(),
#         sample_func=sample_func,
#         pdf_func=pdf_func,
#         sample_dim=3,
#         res=201,
#         ires=32
#     )

#     assert chi2.run()


def test02_chi2_rough_grazing(variants_vec_backends_once_rgb):
    xml = """<float name="alpha" value="0.5"/>"""
    wi = dr.normalize(mi.ScalarVector3f(0.8, 0.3, 0.05))
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughdielectric", xml, wi=wi)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        seed=4
    )

    assert chi2.run()


def test03_chi2_rough_beckmann_all(variants_vec_backends_once_rgb):
    xml = """<float name="alpha" value="0.5"/>
             <boolean name="sample_visible" value="false"/>
             <string name="distribution" value="beckmann"/>
          """
    wi = dr.normalize(mi.ScalarVector3f(0.5, 0.0, 0.5))
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughdielectric", xml, wi=wi)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        res=201
    )

    assert chi2.run()


def test04_chi2_rough_beckmann_visible(variants_vec_backends_once_rgb):
    xml = """<float name="alpha" value="0.5"/>
             <boolean name="sample_visible" value="true"/>
             <string name="distribution" value="beckmann"/>
          """
    wi = dr.normalize(mi.ScalarVector3f(0.5, 0.0, 0.5))
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughdielectric", xml, wi=wi)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        seed=2
    )

    assert chi2.run()


def test05_chi2_rough_ggx_all(variants_vec_backends_once_rgb):
    xml = """<float name="alpha" value="0.5"/>
             <boolean name="sample_visible" value="false"/>
             <string name="distribution" value="ggx"/>
          """
    wi = dr.normalize(mi.ScalarVector3f(0.5, 0.0, 0.5))
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughdielectric", xml, wi=wi)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        res=201
    )

    assert chi2.run()


def test06_chi2_rough_ggx_visible(variants_vec_backends_once_rgb):
    xml = """<float name="alpha" value="0.5"/>
             <boolean name="sample_visible" value="true"/>
             <string name="distribution" value="ggx"/>
          """
    wi = dr.normalize(mi.ScalarVector3f(0.5, 0.5, 0.001))
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughdielectric", xml, wi=wi)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        seed=2
    )

    assert chi2.run()


def test07_chi2_rough_from_inside(variants_vec_backends_once_rgb):
    xml = """<float name="alpha" value="0.5"/>"""
    wi = dr.normalize(mi.ScalarVector3f(0.2, -0.6, -0.5))
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughdielectric", xml, wi=wi)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        res=201
    )

    assert chi2.run()


def test08_chi2_rough_from_inside_tir(variants_vec_backends_once_rgb):
    xml = """<float name="alpha" value="0.5"/>"""
    wi = dr.normalize(mi.ScalarVector3f(0.8, 0.3, -0.05))
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughdielectric", xml, wi=wi)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        seed=8
    )

    assert chi2.run()


def test09_chi2_rough_from_denser(variants_vec_backends_once_rgb):
    xml = """<float name="alpha" value="0.5"/>
             <float name="ext_ior" value="1.5"/>
             <float name="int_ior" value="1"/>"""
    wi = dr.normalize(mi.ScalarVector3f(0.2, -0.6, 0.5))
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughdielectric", xml, wi=wi)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3
    )

    assert chi2.run()


def test10_chi2_lobe_refl(variants_vec_backends_once_rgb):
    xml = """
    <float name="alpha_u" value="0.5"/>
    <float name="alpha_v" value="0.2"/>
    """
    wi = dr.normalize(mi.ScalarVector3f(-0.5, -0.5, 0.1))
    ctx = mi.BSDFContext()
    ctx.component = 0
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughdielectric", xml, wi=wi, ctx=ctx)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        res=201,
        seed=2
    )

    assert chi2.run()


def test11_chi2_aniso_lobe_trans(variants_vec_backends_once_rgb):
    xml = """
    <float name="alpha_u" value="0.5"/>
    <float name="alpha_v" value="0.2"/>
    """
    wi = dr.normalize(mi.ScalarVector3f(-0.5, -0.5, 0.1))
    ctx = mi.BSDFContext()
    ctx.component = 1
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughdielectric", xml, wi=wi, ctx=ctx)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        res=201
    )

    assert chi2.run()


def test12_eval_pdf(variant_scalar_rgb):
    bsdf = mi.load_dict({'type': 'roughdielectric'})

    si    = mi.SurfaceInteraction3f()
    si.p  = [0, 0, 0]
    si.n  = [0, 0, 1]
    si.wi = [0, 0, 1]
    si.sh_frame = mi.Frame3f(si.n)

    ctx = mi.BSDFContext()

    for i in range(20):
        theta = i / 19.0 * (dr.pi / 2)
        wo = [dr.sin(theta), 0, dr.cos(theta)]

        v_pdf  = bsdf.pdf(ctx, si, wo=wo)
        v_eval = bsdf.eval(ctx, si, wo=wo)[0]
        v_eval_pdf = bsdf.eval_pdf(ctx, si, wo=wo)
        assert dr.allclose(v_eval, v_eval_pdf[0])
        assert dr.allclose(v_pdf, v_eval_pdf[1])


def test13_attached_sampling(variants_all_ad_rgb):
    bsdf = mi.load_dict({'type': 'roughdielectric', 'alpha' : 0.5})

    angle = mi.Float(0)
    dr.enable_grad(angle)

    si    = mi.SurfaceInteraction3f()
    si.p  = [0, 0, 0]
    si.n  = [dr.sin(angle), 0, dr.cos(angle)]
    si.sh_frame = mi.Frame3f(si.n)
    si.wi = si.sh_frame.to_local([0, 0, 1])

    bs, weight = bsdf.sample(mi.BSDFContext(), si, 0, [0.3, 0.3])
    assert dr.grad_enabled(weight)

    dr.forward(angle)
    assert dr.allclose(mi.unpolarized_spectrum(dr.grad(weight)), 0.02079637348651886)


def test14_thin_film_construct(variant_scalar_rgb):
    # Film params accepted in non-spectral variants but silently inert.
    b = mi.load_dict({'type': 'roughdielectric', 'alpha': 0.1,
                      'film_thickness': 300.0, 'film_ior': 1.38})
    assert b is not None

    # film_ior accepted as material name.
    b = mi.load_dict({'type': 'roughdielectric', 'alpha': 0.05,
                      'film_thickness': 200.0, 'film_ior': 'water'})
    assert b is not None

    # Negative thickness rejected.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'roughdielectric', 'film_thickness': -10.0})


def test15_thin_film_spectral(variant_scalar_spectral):
    """In a spectral variant, an enabled thin film should give a
    wavelength-dependent reflectance even on a rough microfacet surface."""
    bsdf_film = mi.load_dict({
        'type': 'roughdielectric', 'alpha': 0.05, 'int_ior': 1.5,
        'film_thickness': 400.0, 'film_ior': 2.4,
    })
    bsdf_no_film = mi.load_dict({
        'type': 'roughdielectric', 'alpha': 0.05, 'int_ior': 1.5,
    })

    si = mi.SurfaceInteraction3f()
    angle = 30 * dr.pi / 180
    si.wi = [dr.sin(angle), 0, dr.cos(angle)]
    si.wavelengths = [430.0, 510.0, 590.0, 670.0]

    ctx = mi.BSDFContext()
    ctx.type_mask = mi.BSDFFlags.GlossyReflection  # force reflection lobe

    _, w_film    = bsdf_film.sample(ctx, si, 0.0, [0.5, 0.5])
    _, w_no_film = bsdf_no_film.sample(ctx, si, 0.0, [0.5, 0.5])

    w_no_film_arr = mi.unpolarized_spectrum(w_no_film)
    w_film_arr    = mi.unpolarized_spectrum(w_film)

    # No-film: reflectance flat across lanes.
    assert dr.allclose(w_no_film_arr[0], w_no_film_arr[3])

    # Film: clearly varies across lanes.
    spread = float(dr.max(w_film_arr) - dr.min(w_film_arr))
    assert spread > 0.05, f'film reflectance should vary; spread={spread}'


def test16_dispersion_construct(variant_scalar_rgb):
    # abbe / cauchy_b accept and instantiate (silently inert in RGB).
    for params in [{'abbe': 30.0}, {'cauchy_b': 0.005}]:
        b = mi.load_dict({'type': 'roughdielectric', 'alpha': 0.05,
                          'int_ior': 1.5, **params})
        assert b is not None

    # Mutual exclusion.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'roughdielectric', 'abbe': 30.0,
                      'cauchy_b': 0.005})

    # Negative abbe rejected.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'roughdielectric', 'abbe': -10.0})


def test17_dispersion_spectral(variant_scalar_spectral):
    """In a spectral variant with dispersion, force a glancing-angle
    near-mirror reflection. F_spec varies with wavelength via per-λ Fresnel,
    so the reflection weight should not be flat across lanes."""
    bsdf_disp = mi.load_dict({
        'type': 'roughdielectric', 'alpha': 0.05, 'int_ior': 1.5,
        'abbe': 25.0,
    })
    bsdf_no_disp = mi.load_dict({
        'type': 'roughdielectric', 'alpha': 0.05, 'int_ior': 1.5,
    })

    si = mi.SurfaceInteraction3f()
    angle = 70 * dr.pi / 180  # glancing — strong Fresnel angular variation
    si.wi = [dr.sin(angle), 0, dr.cos(angle)]
    si.wavelengths = [430.0, 510.0, 590.0, 670.0]

    ctx = mi.BSDFContext()
    ctx.type_mask = mi.BSDFFlags.GlossyReflection

    _, w_disp    = bsdf_disp.sample(ctx, si, 0.0, [0.5, 0.5])
    _, w_no_disp = bsdf_no_disp.sample(ctx, si, 0.0, [0.5, 0.5])

    w_no_disp_arr = mi.unpolarized_spectrum(w_no_disp)
    w_disp_arr    = mi.unpolarized_spectrum(w_disp)

    # No dispersion → flat across lanes.
    assert dr.allclose(w_no_disp_arr[0], w_no_disp_arr[3])

    # With dispersion, the per-λ Fresnel makes the reflection weight differ.
    spread = float(dr.max(w_disp_arr) - dr.min(w_disp_arr))
    assert spread > 1e-4, f'dispersion should vary reflection weight; spread={spread}'


def test18_dispersion_transmission_hero_only(variant_scalar_spectral):
    """With dispersion, transmission sampling should mask non-hero
    wavelengths to zero (the refraction direction is wavelength-dependent
    and only the hero wavelength refracts to the sampled direction)."""
    bsdf = mi.load_dict({
        'type': 'roughdielectric', 'alpha': 0.05, 'int_ior': 1.5,
        'abbe': 25.0,
    })

    si = mi.SurfaceInteraction3f()
    si.wi = [0.0, 0.0, 1.0]  # straight on
    si.wavelengths = [430.0, 510.0, 590.0, 670.0]

    ctx = mi.BSDFContext()
    ctx.type_mask = mi.BSDFFlags.GlossyTransmission

    bs, w = bsdf.sample(ctx, si, 0.0, [0.5, 0.5])
    assert bs.sampled_type == +mi.BSDFFlags.GlossyTransmission

    w_arr = mi.unpolarized_spectrum(w)
    # Hero wavelength carries weight; non-hero are zero.
    assert float(w_arr[0]) > 0.0
    assert dr.allclose(w_arr[1], 0.0)
    assert dr.allclose(w_arr[2], 0.0)
    assert dr.allclose(w_arr[3], 0.0)


def test19_dispersion_film_via_instance(variant_scalar_spectral):
    """Dispersive thin-film BSDFs are pure surface BSDFs with no
    world-space state, so they should behave identically when accessed
    through a shape group + instance vs. as a plain shape. This test
    exercises the most complex code path (microfacet specular + per-λ
    Fresnel + thin-film Airy + hero-wavelength on dispersive transmission)
    and verifies the spectral response is preserved through instancing."""
    bsdf_dict = {
        'type': 'roughdielectric',
        'alpha': 0.05,
        'int_ior': 1.5,
        'abbe': 25.0,
        'film_thickness': 400.0,
        'film_ior': 2.4,
    }

    # Plain sphere at the origin.
    scene_plain = mi.load_dict({
        'type': 'scene',
        'sphere': {'type': 'sphere', 'bsdf': bsdf_dict},
    })

    # Same sphere via shape group + instance (identity transform).
    scene_inst = mi.load_dict({
        'type': 'scene',
        'group': {
            'type': 'shapegroup',
            'sphere': {'type': 'sphere', 'bsdf': bsdf_dict},
        },
        'inst': {
            'type': 'instance',
            'group': {'type': 'ref', 'id': 'group'},
        },
    })

    ray = mi.Ray3f(o=[0, 0, -3], d=[0, 0, 1], time=0.0,
                   wavelengths=[430.0, 510.0, 590.0, 670.0])
    si_plain = scene_plain.ray_intersect(ray)
    si_inst  = scene_inst.ray_intersect(ray)

    assert si_plain.is_valid()
    assert si_inst.is_valid()
    assert si_inst.instance is not None  # confirm we hit an instance
    assert si_plain.instance is None     # plain shape has no instance

    bsdf_p = si_plain.bsdf()
    bsdf_i = si_inst.bsdf()

    ctx = mi.BSDFContext()
    ctx.type_mask = mi.BSDFFlags.GlossyReflection
    _, w_p = bsdf_p.sample(ctx, si_plain, 0.0, [0.5, 0.5])
    _, w_i = bsdf_i.sample(ctx, si_inst, 0.0, [0.5, 0.5])

    w_p_arr = mi.unpolarized_spectrum(w_p)
    w_i_arr = mi.unpolarized_spectrum(w_i)

    # Sanity: the thin-film + dispersion combo really did vary across λ.
    spread = float(dr.max(w_p_arr) - dr.min(w_p_arr))
    assert spread > 0.05, f'thin-film + dispersion should vary by λ; spread={spread}'

    # Spectral BSDF response must be preserved through instancing.
    assert dr.allclose(w_p_arr, w_i_arr, atol=1e-4), \
        f'instance should preserve spectral BSDF response: plain={w_p_arr} inst={w_i_arr}'


def test20_thin_film_resonance_no_nan(variant_scalar_spectral):
    """Regression: a roughdielectric with thin-film parameters that produce
    destructive interference (F → 0) on the hero wavelength must not
    generate NaN sample weights. The previous code divided F_spec by
    dr::detach(F) directly, which produced NaN when F was near zero at
    Airy resonances — those NaNs then propagated through MIS / volumetric
    transport and corrupted ~30% of pixels in long renders. The fix
    guards the division with dr::select."""
    # Parameters approximate an AR-coating sweet spot: film_ior ≈ √(n_a·n_s)
    # at ~138 nm on glass produces near-zero reflectance near 550 nm.
    bsdf = mi.load_dict({
        'type': 'roughdielectric',
        'alpha': 0.05,
        'int_ior': 1.5,
        'film_thickness': 138.0,
        'film_ior': 1.225,
    })

    # Sweep incident angle and wavelength packets to hit the resonance from
    # multiple directions. Even a single NaN is a regression.
    rng = dr.opaque
    ctx = mi.BSDFContext()
    for angle_deg in [5.0, 25.0, 45.0, 65.0, 80.0]:
        a = angle_deg * dr.pi / 180
        si = mi.SurfaceInteraction3f()
        si.wi = [dr.sin(a), 0, dr.cos(a)]
        # Pick wavelengths spanning the expected R≈0 region.
        for wls in ([430., 510., 550., 670.],
                    [380., 450., 555., 720.],
                    [400., 500., 600., 700.]):
            si.wavelengths = wls
            for sample1 in [0.0, 0.25, 0.5, 0.75, 1.0]:
                for u, v in [(0.1, 0.1), (0.5, 0.5), (0.9, 0.9)]:
                    bs, w = bsdf.sample(ctx, si, sample1, [u, v])
                    arr = mi.unpolarized_spectrum(w)
                    for i in range(4):
                        assert dr.all(dr.isfinite(arr[i])), \
                            (f'NaN/Inf at angle={angle_deg}° λ={wls} '
                             f'sample1={sample1} uv=({u},{v}): w={arr}')
