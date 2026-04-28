import pytest
import drjit as dr
import mitsuba as mi


@pytest.mark.slow
def test01_chi2_smooth(variants_vec_backends_once_rgb):
    xml = """<float name="alpha" value="0.05"/>
             <rgb name="specular_reflectance" value="0.7"/>
             <rgb name="diffuse_reflectance" value="0.1"/>"""
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughplastic", xml)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
        ires=16,
        res=201
    )

    assert chi2.run()


@pytest.mark.slow
def test02_chi2_rough(variants_vec_backends_once_rgb):
    xml = """<float name="alpha" value="0.25"/>
             <rgb name="specular_reflectance" value="0.7"/>
             <rgb name="diffuse_reflectance" value="0.1"/>"""
    sample_func, pdf_func = mi.chi2.BSDFAdapter("roughplastic", xml)

    chi2 = mi.chi2.ChiSquareTest(
        domain=mi.chi2.SphericalDomain(),
        sample_func=sample_func,
        pdf_func=pdf_func,
        sample_dim=3,
    )

    assert chi2.run()


def test03_eval_pdf(variant_scalar_rgb):
    bsdf = mi.load_dict({'type': 'roughplastic'})

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


def test04_eval_attribute(variants_all_rgb):
    reflectance = [0.5, 0.6, 0.7]
    roughness = 0.4
    bsdf = mi.load_dict({
        'type': 'roughplastic',
        'diffuse_reflectance': { 'type': 'rgb', 'value': reflectance},
        'alpha': roughness
    })

    si = mi.SurfaceInteraction3f()

    assert dr.allclose(bsdf.eval_attribute('diffuse_reflectance', si), reflectance)
    assert dr.allclose(bsdf.eval_attribute_3('diffuse_reflectance', si), reflectance)
    assert dr.allclose(bsdf.eval_attribute_1('alpha', si), roughness)


def test_thin_film_construct(variant_scalar_rgb):
    # Film + dispersion params accepted in non-spectral variants but inert.
    b = mi.load_dict({'type': 'roughplastic', 'alpha': 0.05,
                      'film_thickness': 300.0, 'film_ior': 1.38})
    assert b is not None

    b = mi.load_dict({'type': 'roughplastic', 'alpha': 0.05, 'abbe': 30.0})
    assert b is not None

    # Negative thickness rejected.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'roughplastic', 'film_thickness': -10.0})

    # Mutual exclusion of dispersion params.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'roughplastic', 'abbe': 30.0,
                      'cauchy_b': 0.005})

    # Negative abbe rejected.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'roughplastic', 'abbe': -10.0})


def test_thin_film_spectral(variant_scalar_spectral):
    """In a spectral variant, an enabled thin film should give a
    wavelength-dependent specular highlight on the rough plastic surface."""
    bsdf_film = mi.load_dict({
        'type': 'roughplastic', 'alpha': 0.05, 'int_ior': 1.5,
        'film_thickness': 400.0, 'film_ior': 2.4,
        'diffuse_reflectance': 0.5,
    })
    bsdf_no_film = mi.load_dict({
        'type': 'roughplastic', 'alpha': 0.05, 'int_ior': 1.5,
        'diffuse_reflectance': 0.5,
    })

    si = mi.SurfaceInteraction3f()
    angle = 30 * dr.pi / 180
    si.wi = [dr.sin(angle), 0, dr.cos(angle)]
    si.wavelengths = [430.0, 510.0, 590.0, 670.0]

    # Evaluate at a near-mirror direction to probe the per-microfacet
    # specular Fresnel.
    wo = [-dr.sin(angle), 0, dr.cos(angle)]
    ctx = mi.BSDFContext()

    val_film    = mi.unpolarized_spectrum(bsdf_film.eval(ctx, si, wo))
    val_no_film = mi.unpolarized_spectrum(bsdf_no_film.eval(ctx, si, wo))

    # No film: flat across lanes.
    assert dr.allclose(val_no_film[0], val_no_film[3])

    # Film: per-wavelength variation visible.
    spread = float(dr.max(val_film) - dr.min(val_film))
    assert spread > 1e-3, f'film should vary highlight by λ; spread={spread}'


def test_dispersion_spectral(variant_scalar_spectral):
    """Dispersion alone (no film) should also vary the highlight."""
    bsdf_disp = mi.load_dict({
        'type': 'roughplastic', 'alpha': 0.05, 'int_ior': 1.5,
        'abbe': 25.0, 'diffuse_reflectance': 0.5,
    })
    bsdf_no_disp = mi.load_dict({
        'type': 'roughplastic', 'alpha': 0.05, 'int_ior': 1.5,
        'diffuse_reflectance': 0.5,
    })

    si = mi.SurfaceInteraction3f()
    angle = 70 * dr.pi / 180  # glancing — strong Fresnel angular variation
    si.wi = [dr.sin(angle), 0, dr.cos(angle)]
    si.wavelengths = [430.0, 510.0, 590.0, 670.0]
    wo = [-dr.sin(angle), 0, dr.cos(angle)]

    ctx = mi.BSDFContext()

    val_disp    = mi.unpolarized_spectrum(bsdf_disp.eval(ctx, si, wo))
    val_no_disp = mi.unpolarized_spectrum(bsdf_no_disp.eval(ctx, si, wo))

    # No dispersion → flat across lanes.
    assert dr.allclose(val_no_disp[0], val_no_disp[3])

    # With dispersion, per-λ Fresnel makes the value differ across lanes.
    spread = float(dr.max(val_disp) - dr.min(val_disp))
    assert spread > 1e-5, f'dispersion should vary highlight; spread={spread}'
