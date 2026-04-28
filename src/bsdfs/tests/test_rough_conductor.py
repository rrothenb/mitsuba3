import pytest
import drjit as dr
import mitsuba as mi

def test01_chi2_smooth(variants_vec_backends_once_rgb):
    xml = """<float name="alpha" value="0.05"/>"""
    wi = dr.normalize(mi.ScalarVector3f(1.0, 1.0, 1.0))
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughconductor", xml, wi=wi)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        res=140,
        ires=32
    )

    assert chi2.run()


def test02_chi2_aniso_beckmann_all(variants_vec_backends_once_rgb):
    xml = """<float name="alpha_u" value="0.2"/>
             <float name="alpha_v" value="0.05"/>
             <string name="distribution" value="beckmann"/>
             <boolean name="sample_visible" value="false"/>"""
    wi = dr.normalize(mi.ScalarVector3f(1.0, 1.0, 1.0))
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughconductor", xml, wi=wi)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        ires=16
    )

    assert chi2.run()


def test03_chi2_aniso_beckmann_visible(variants_vec_backends_once_rgb):
    xml = """<float name="alpha_u" value="0.2"/>
             <float name="alpha_v" value="0.05"/>
             <string name="distribution" value="beckmann"/>
             <boolean name="sample_visible" value="true"/>"""
    wi = dr.normalize(mi.ScalarVector3f(1.0, 1.0, 1.0))
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughconductor", xml, wi=wi)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        ires=16,
        seed=2
    )

    assert chi2.run()


def test04_chi2_aniso_ggx_all(variants_vec_backends_once_rgb):
    xml = """<float name="alpha_u" value="0.2"/>
             <float name="alpha_v" value="0.05"/>
             <string name="distribution" value="ggx"/>
             <boolean name="sample_visible" value="false"/>"""
    wi = dr.normalize(mi.ScalarVector3f(1.0, 1.0, 1.0))
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughconductor", xml, wi=wi)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        ires=8
    )

    assert chi2.run()


def test05_chi2_aniso_ggx_visible(variants_vec_backends_once_rgb):
    xml = """<float name="alpha_u" value="0.2"/>
             <float name="alpha_v" value="0.05"/>
             <string name="distribution" value="ggx"/>
             <boolean name="sample_visible" value="true"/>"""
    wi = dr.normalize(mi.ScalarVector3f(1.0, 1.0, 1.0))
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughconductor", xml, wi=wi)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        ires=8
    )

    assert chi2.run()


def test06_eval_pdf(variant_scalar_rgb):
    bsdf = mi.load_dict({'type': 'roughconductor'})

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


def test_thin_film_construct(variant_scalar_rgb):
    # Film params accepted in non-spectral variants but silently inert.
    b = mi.load_dict({'type': 'roughconductor', 'material': 'Au', 'alpha': 0.1,
                      'film_thickness': 100.0, 'film_ior': 1.38})
    assert b is not None

    # film_ior accepted as material name.
    b = mi.load_dict({'type': 'roughconductor', 'material': 'Cu', 'alpha': 0.05,
                      'film_thickness': 200.0, 'film_ior': 'water'})
    assert b is not None

    # Negative thickness rejected.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'roughconductor', 'material': 'Cu',
                      'film_thickness': -10.0})


def test_thin_film_spectral(variant_scalar_spectral):
    """In a spectral variant, an enabled thin film should change the rough
    conductor's spectral reflectance signature non-uniformly across
    wavelengths (Al substrate vs same Al substrate with an MgF2-like film)."""
    bsdf_film = mi.load_dict({
        'type': 'roughconductor', 'material': 'Al', 'alpha': 0.05,
        'film_thickness': 200.0, 'film_ior': 1.38,
    })
    bsdf_no_film = mi.load_dict({
        'type': 'roughconductor', 'material': 'Al', 'alpha': 0.05,
    })

    si = mi.SurfaceInteraction3f()
    angle = 30 * dr.pi / 180
    si.wi = [dr.sin(angle), 0, dr.cos(angle)]
    si.wavelengths = [430.0, 510.0, 590.0, 670.0]

    ctx = mi.BSDFContext()

    _, w_film    = bsdf_film.sample(ctx, si, 0.0, [0.5, 0.5])
    _, w_no_film = bsdf_no_film.sample(ctx, si, 0.0, [0.5, 0.5])

    w_film_arr    = mi.unpolarized_spectrum(w_film)
    w_no_film_arr = mi.unpolarized_spectrum(w_no_film)

    # The film modulates Al's already-varying reflectance differently at each
    # wavelength — the film/no-film difference should not be uniform.
    delta = w_film_arr - w_no_film_arr
    delta_spread = float(dr.max(delta) - dr.min(delta))
    assert delta_spread > 0.02, \
        f'film should change spectral signature non-uniformly; spread={delta_spread}'


def test_thin_film_polarized_guard():
    """A polarized spectral variant should reject thin-film at construction."""
    polarized = [v for v in mi.variants() if 'polarized' in v and 'spectral' in v]
    if not polarized:
        pytest.skip('no polarized spectral variant in this build')
    mi.set_variant(polarized[0])
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'roughconductor', 'material': 'Au',
                      'alpha': 0.1, 'film_thickness': 200.0})
