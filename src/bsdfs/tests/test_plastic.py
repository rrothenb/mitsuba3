import pytest
import drjit as dr
import mitsuba as mi


def test01_thin_film_construct(variant_scalar_rgb):
    # Film params accepted in non-spectral variants but silently inert.
    b = mi.load_dict({'type': 'plastic', 'film_thickness': 300.0,
                      'film_ior': 1.38})
    assert b is not None

    # film_ior accepted as material name, mirroring int_ior/ext_ior.
    b = mi.load_dict({'type': 'plastic', 'film_thickness': 200.0,
                      'film_ior': 'water'})
    assert b is not None

    # Negative thickness rejected.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'plastic', 'film_thickness': -10.0})


def test02_thin_film_spectral(variant_scalar_spectral):
    """In a spectral variant, an enabled thin film should give a strongly
    wavelength-dependent specular reflectance and a complementary tint on
    the diffuse component."""
    bsdf_film = mi.load_dict({
        'type': 'plastic', 'int_ior': 1.5,
        'film_thickness': 400.0, 'film_ior': 2.4,
        'diffuse_reflectance': 0.5,
    })
    bsdf_no_film = mi.load_dict({
        'type': 'plastic', 'int_ior': 1.5,
        'diffuse_reflectance': 0.5,
    })

    si = mi.SurfaceInteraction3f()
    angle = 30 * dr.pi / 180
    si.wi = [dr.sin(angle), 0, dr.cos(angle)]
    si.wavelengths = [430.0, 510.0, 590.0, 670.0]

    ctx = mi.BSDFContext()
    ctx.type_mask = mi.BSDFFlags.DeltaReflection  # force specular

    _, w_film    = bsdf_film.sample(ctx, si, 0, [0, 0])
    _, w_no_film = bsdf_no_film.sample(ctx, si, 0, [0, 0])

    # Without a film, specular weight is the scalar Fresnel reflectance — flat
    # across wavelengths.
    w_no_film_arr = mi.unpolarized_spectrum(w_no_film)
    assert dr.allclose(w_no_film_arr[0], w_no_film_arr[1])
    assert dr.allclose(w_no_film_arr[0], w_no_film_arr[3])

    # With a high-index film, the Airy formula produces a strongly
    # wavelength-dependent specular reflectance.
    w_film_arr = mi.unpolarized_spectrum(w_film)
    spread = float(dr.max(w_film_arr) - dr.min(w_film_arr))
    assert spread > 0.05, f'film specular reflectance should vary; spread={spread}'


def test03_thin_film_diffuse_tint(variant_scalar_spectral):
    """The diffuse contribution is gated by (1 - F_film) on the way in and
    out, so the diffuse evaluation should also be wavelength-dependent when
    the film is enabled, and flat when it isn't."""
    bsdf_film = mi.load_dict({
        'type': 'plastic', 'int_ior': 1.5,
        'film_thickness': 400.0, 'film_ior': 2.4,
        'diffuse_reflectance': 0.5,
    })
    bsdf_no_film = mi.load_dict({
        'type': 'plastic', 'int_ior': 1.5,
        'diffuse_reflectance': 0.5,
    })

    si = mi.SurfaceInteraction3f()
    angle = 30 * dr.pi / 180
    si.wi = [dr.sin(angle), 0, dr.cos(angle)]
    si.wavelengths = [430.0, 510.0, 590.0, 670.0]
    wo = [-dr.sin(angle), 0, dr.cos(angle)]  # diffuse direction (any non-mirror)

    ctx = mi.BSDFContext()

    val_film    = mi.unpolarized_spectrum(bsdf_film.eval(ctx, si, wo))
    val_no_film = mi.unpolarized_spectrum(bsdf_no_film.eval(ctx, si, wo))

    # No-film diffuse is flat across wavelengths (reflectance is wavelength-
    # independent, Fresnel is wavelength-independent).
    assert dr.allclose(val_no_film[0], val_no_film[1])
    assert dr.allclose(val_no_film[0], val_no_film[3])

    # Film diffuse picks up a wavelength-dependent (1 - F_film) factor.
    spread = float(dr.max(val_film) - dr.min(val_film))
    assert spread > 1e-3, f'diffuse should pick up film tint; spread={spread}'


def test04_dispersion_construct(variant_scalar_rgb):
    # Both abbe and cauchy_b accept and instantiate (silently inert in RGB).
    for params in [{'abbe': 50.0}, {'cauchy_b': 0.005}]:
        b = mi.load_dict({'type': 'plastic', 'int_ior': 1.5, **params})
        assert b is not None

    # Mutual exclusion.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'plastic', 'abbe': 50.0, 'cauchy_b': 0.005})

    # Negative abbe rejected.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'plastic', 'abbe': -10.0})


def test05_dispersion_spectral(variant_scalar_spectral):
    """With dispersion, the specular highlight reflectance varies with
    wavelength; without dispersion, it's flat (Fresnel is wavelength-
    independent for a constant IOR)."""
    bsdf_disp = mi.load_dict({
        'type': 'plastic', 'int_ior': 1.5, 'abbe': 25.0,
        'diffuse_reflectance': 0.5,
    })
    bsdf_no_disp = mi.load_dict({
        'type': 'plastic', 'int_ior': 1.5,
        'diffuse_reflectance': 0.5,
    })

    si = mi.SurfaceInteraction3f()
    angle = 60 * dr.pi / 180  # near-grazing — dispersion most visible
    si.wi = [dr.sin(angle), 0, dr.cos(angle)]
    si.wavelengths = [430.0, 510.0, 590.0, 670.0]

    ctx = mi.BSDFContext()
    ctx.type_mask = mi.BSDFFlags.DeltaReflection  # force specular

    _, w_disp    = bsdf_disp.sample(ctx, si, 0, [0, 0])
    _, w_no_disp = bsdf_no_disp.sample(ctx, si, 0, [0, 0])

    w_no_disp_arr = mi.unpolarized_spectrum(w_no_disp)
    w_disp_arr    = mi.unpolarized_spectrum(w_disp)

    # No dispersion → constant across lanes.
    assert dr.allclose(w_no_disp_arr[0], w_no_disp_arr[3])

    # With dispersion, the per-wavelength Fresnel reflectance differs across
    # lanes. The effect is subtle compared to thin-film but still measurable.
    spread = float(dr.max(w_disp_arr) - dr.min(w_disp_arr))
    assert spread > 1e-4, f'dispersion should vary specular reflectance; spread={spread}'
