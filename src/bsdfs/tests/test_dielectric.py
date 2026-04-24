import pytest
import drjit as dr
import mitsuba as mi

def example_bsdf(reflectance=0.3, transmittance=0.6):
    return mi.load_dict({
        'type': 'dielectric',
        'specular_reflectance': reflectance,
        'specular_transmittance': transmittance,
        'int_ior': 1.5,
        'ext_ior': 1.0,
    })


def test01_create(variant_scalar_rgb):
    b = mi.load_dict({'type': 'dielectric'})
    assert b is not None
    assert b.component_count() == 2
    assert b.flags(0) == (mi.BSDFFlags.DeltaReflection | mi.BSDFFlags.FrontSide |
                          mi.BSDFFlags.BackSide)
    assert b.flags(1) == (mi.BSDFFlags.DeltaTransmission | mi.BSDFFlags.FrontSide |
                          mi.BSDFFlags.BackSide | mi.BSDFFlags.NonSymmetric)
    assert b.flags() == b.flags(0) | b.flags(1)

    # Should not accept negative IORs
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'dielectric', 'int_ior': -0.5})


def test02_sample(variant_scalar_rgb):
    si = mi.SurfaceInteraction3f()
    si.wi = [0, 0, 1]
    bsdf = example_bsdf()

    for i in range(2):
        ctx = mi.BSDFContext(mi.TransportMode.Importance if i == 0
                          else mi.TransportMode.Radiance)

        # Sample reflection
        bs, spec = bsdf.sample(ctx, si, 0, [0, 0])
        assert dr.allclose(spec, [0.3] * 3)
        assert dr.allclose(bs.pdf, 0.04)
        assert dr.allclose(bs.eta, 1.0)
        assert dr.allclose(bs.wo, [0, 0, 1])
        assert bs.sampled_component == 0
        assert bs.sampled_type == +mi.BSDFFlags.DeltaReflection

        # Sample refraction
        bs, spec = bsdf.sample(ctx, si, 0.05, [0, 0])
        if i == 0:
            assert dr.allclose(spec, [0.6] * 3)
        else:
            assert dr.allclose(spec, [0.6 / 1.5**2] * 3)
        assert dr.allclose(bs.pdf, 1 - 0.04)
        assert dr.allclose(bs.eta, 1.5)
        assert dr.allclose(bs.wo, [0, 0, -1])
        assert bs.sampled_component == 1
        assert bs.sampled_type == +mi.BSDFFlags.DeltaTransmission


def test03_sample_reverse(variant_scalar_rgb):
    si = mi.SurfaceInteraction3f()
    si.wi = [0, 0, -1]
    bsdf = example_bsdf()

    for i in range(2):
        ctx = mi.BSDFContext(mi.TransportMode.Importance if i == 0
                          else mi.TransportMode.Radiance)

        # Sample reflection
        bs, spec = bsdf.sample(ctx, si, 0, [0, 0])
        assert dr.allclose(spec, [0.3] * 3)
        assert dr.allclose(bs.pdf, 0.04)
        assert dr.allclose(bs.eta, 1.0)
        assert dr.allclose(bs.wo, [0, 0, -1])
        assert bs.sampled_component == 0
        assert bs.sampled_type == +mi.BSDFFlags.DeltaReflection

        # Sample refraction
        bs, spec = bsdf.sample(ctx, si, 0.05, [0, 0])
        if i == 0:
            assert dr.allclose(spec, [0.6] * 3)
        else:
            assert dr.allclose(spec, [0.6 * 1.5**2] * 3)
        assert dr.allclose(bs.pdf, 1 - 0.04)
        assert dr.allclose(bs.eta, 1 / 1.5)
        assert dr.allclose(bs.wo, [0, 0, 1])
        assert bs.sampled_component == 1
        assert bs.sampled_type == +mi.BSDFFlags.DeltaTransmission


def test04_sample_specific_component(variant_scalar_rgb):
    si = mi.SurfaceInteraction3f()
    si.wi = [0, 0, 1]
    bsdf = example_bsdf()

    for i in range(2):
        for sample in [0.0, 0.5, 1.0]:
            for sel_type in range(2):
                ctx = mi.BSDFContext(mi.TransportMode.Importance if i == 0
                                     else mi.TransportMode.Radiance)

                # Sample reflection
                if sel_type == 0:
                    ctx.type_mask = mi.BSDFFlags.DeltaReflection
                else:
                    ctx.component = 0
                bs, spec = bsdf.sample(ctx, si, sample, [0, 0])
                assert dr.allclose(spec, [0.3 * 0.04] * 3)
                assert dr.allclose(bs.pdf, 1.0)
                assert dr.allclose(bs.eta, 1.0)
                assert dr.allclose(bs.wo, [0, 0, 1])
                assert bs.sampled_component == 0
                assert bs.sampled_type == +mi.BSDFFlags.DeltaReflection

                # Sample refraction
                if sel_type == 0:
                    ctx.type_mask = mi.BSDFFlags.DeltaTransmission
                else:
                    ctx.component = 1
                bs, spec = bsdf.sample(ctx, si, sample, [0, 0])
                if i == 0:
                    assert dr.allclose(spec, [0.6 * (1 - 0.04)] * 3)
                else:
                    assert dr.allclose(spec, [0.6 * (1 - 0.04) / 1.5**2] * 3)
                assert dr.allclose(bs.pdf, 1.0)
                assert dr.allclose(bs.eta, 1.5)
                assert dr.allclose(bs.wo, [0, 0, -1])
                assert bs.sampled_component == 1
                assert bs.sampled_type == +mi.BSDFFlags.DeltaTransmission

    ctx = mi.BSDFContext()
    ctx.component = 3
    bs, spec = bsdf.sample(ctx, si, 0, [0, 0])
    assert dr.all(spec == [0] * 3)


def test05_spot_check(variant_scalar_rgb):
    angle = 80 * dr.pi / 180
    ctx = mi.BSDFContext()
    si = mi.SurfaceInteraction3f()
    wi = [dr.sin(angle), 0, dr.cos(angle)]
    si.wi = wi
    bsdf = example_bsdf()

    bs, spec = bsdf.sample(ctx, si, 0, [0, 0])
    assert dr.allclose(bs.pdf, 0.387704354691473)
    assert dr.allclose(bs.wo, [-dr.sin(angle), 0, dr.cos(angle)])

    bs, spec = bsdf.sample(ctx, si, 1, [0, 0])
    angle = 41.03641052520335 * dr.pi / 180
    assert dr.allclose(bs.pdf, 1 - 0.387704354691473)
    assert dr.allclose(bs.wo, [-dr.sin(angle), 0, -dr.cos(angle)])

    si.wi = bs.wo
    bs, spec = bsdf.sample(ctx, si, 1, [0, 0])
    assert dr.allclose(bs.pdf, 1 - 0.387704354691473)
    assert dr.allclose(bs.wo, wi)


def test07_dispersion_construct(variant_scalar_rgb):
    # Both abbe and cauchy_b should accept and instantiate in any variant
    # (they are silently inert in non-spectral variants).
    for params in [{'abbe': 50.0}, {'cauchy_b': 0.0042}]:
        b = mi.load_dict({'type': 'dielectric', 'int_ior': 1.5, **params})
        assert b is not None

    # Mutual exclusion: passing both should raise.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'dielectric', 'abbe': 50.0, 'cauchy_b': 0.004})

    # Negative abbe should raise.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'dielectric', 'abbe': -10.0})


def test08_dispersion_spectral(variant_scalar_spectral):
    """In a spectral variant, an abbe-enabled BSDF should produce
    wavelength-dependent transmission directions when refracting."""
    bsdf = mi.load_dict({'type': 'dielectric', 'int_ior': 1.5, 'abbe': 30.0})
    bsdf_no_disp = mi.load_dict({'type': 'dielectric', 'int_ior': 1.5})

    si = mi.SurfaceInteraction3f()
    angle = 30 * dr.pi / 180
    si.wi = [dr.sin(angle), 0, dr.cos(angle)]
    # Use wavelengths far from the 589.3 nm reference so the dispersive
    # eta differs noticeably from m_eta. Lane 0 (the hero) gets 450 nm.
    si.wavelengths = [450.0, 550.0, 650.0, 700.0]

    ctx = mi.BSDFContext()

    # Force transmission by sampling at high probability.
    bs_disp,    _ = bsdf.sample(ctx, si, 1.0, [0, 0])
    bs_no_disp, _ = bsdf_no_disp.sample(ctx, si, 1.0, [0, 0])

    assert bs_disp.sampled_type    == +mi.BSDFFlags.DeltaTransmission
    assert bs_no_disp.sampled_type == +mi.BSDFFlags.DeltaTransmission

    # The dispersive bsdf at 450 nm refracts more strongly than 1.5,
    # so the refracted z-component should differ from the non-dispersive
    # case (which uses eta = 1.5 across the board).
    assert not dr.allclose(bs_disp.eta, bs_no_disp.eta)
    assert bs_disp.eta > bs_no_disp.eta  # blue light bends more


def test09_thin_film_construct(variant_scalar_rgb):
    # film params accepted in non-spectral variants but silently inert.
    b = mi.load_dict({'type': 'dielectric', 'film_thickness': 300.0,
                      'film_ior': 1.38})
    assert b is not None

    # film_ior accepted as material name, mirroring int_ior/ext_ior.
    b = mi.load_dict({'type': 'dielectric', 'film_thickness': 200.0,
                      'film_ior': 'water'})
    assert b is not None

    # Negative thickness rejected.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'dielectric', 'film_thickness': -10.0})


def test10_thin_film_spectral(variant_scalar_spectral):
    """In a spectral variant, an enabled thin film should produce a
    wavelength-dependent reflectance weight on the reflection lobe;
    the non-film case should be flat across wavelengths."""
    # High-index film (TiO2-like) on glass: strong interference visible
    # across the visible spectrum.
    bsdf_film = mi.load_dict({
        'type': 'dielectric', 'int_ior': 1.5,
        'film_thickness': 400.0, 'film_ior': 2.4,
    })
    bsdf_no_film = mi.load_dict({'type': 'dielectric', 'int_ior': 1.5})

    si = mi.SurfaceInteraction3f()
    angle = 30 * dr.pi / 180
    si.wi = [dr.sin(angle), 0, dr.cos(angle)]
    # Span the visible range so phase φ differs noticeably across lanes.
    si.wavelengths = [430.0, 510.0, 590.0, 670.0]

    ctx = mi.BSDFContext()
    ctx.type_mask = mi.BSDFFlags.DeltaReflection  # force reflection

    _, w_film    = bsdf_film.sample(ctx, si, 0, [0, 0])
    _, w_no_film = bsdf_no_film.sample(ctx, si, 0, [0, 0])

    # Without a film, the reflection weight is the scalar Fresnel reflectance
    # broadcast across all lanes — all four entries should match.
    w_no_film_arr = mi.unpolarized_spectrum(w_no_film)
    assert dr.allclose(w_no_film_arr[0], w_no_film_arr[1])
    assert dr.allclose(w_no_film_arr[0], w_no_film_arr[3])

    # With a high-index film, the Airy formula produces a strongly
    # wavelength-dependent reflectance.
    w_film_arr = mi.unpolarized_spectrum(w_film)
    spread = float(dr.max(w_film_arr) - dr.min(w_film_arr))
    assert spread > 0.05, f'film reflectance should vary across λ; spread={spread}'


def test11_thin_film_polarized_guard():
    """If a polarized spectral variant is built, constructing a dielectric
    with film_thickness > 0 should throw at construction time."""
    polarized = [v for v in mi.variants() if 'polarized' in v and 'spectral' in v]
    if not polarized:
        pytest.skip('no polarized spectral variant in this build')
    mi.set_variant(polarized[0])
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'dielectric', 'film_thickness': 300.0})


def test06_attached_sampling(variants_all_ad_rgb):
    bsdf = mi.load_dict({'type': 'dielectric'})

    angle = mi.Float(10 * dr.pi / 180)
    dr.enable_grad(angle)

    si    = mi.SurfaceInteraction3f()
    si.p  = [0, 0, 0]
    si.n  = [dr.sin(angle), 0, dr.cos(angle)]
    si.sh_frame = mi.Frame3f(si.n)
    si.wi = si.sh_frame.to_local([0, 0, 1])

    bs, weight = bsdf.sample(mi.BSDFContext(), si, 0, [0, 0])
    assert dr.grad_enabled(weight)
    
    dr.forward(angle)
    assert dr.allclose(mi.unpolarized_spectrum(dr.grad(weight)), 0.008912204764783382)
    