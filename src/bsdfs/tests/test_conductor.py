import pytest
import drjit as dr
import mitsuba as mi

def test01_create(variant_scalar_mono_polarized):
    b = mi.load_dict({'type': 'conductor'})
    assert b is not None
    assert b.component_count() == 1
    assert b.flags(0) == mi.BSDFFlags.FrontSide | mi.BSDFFlags.DeltaReflection
    assert b.flags() == b.flags(0)


def test02_sample_pol_local(variant_scalar_mono_polarized):
    def spectrum_from_stokes(v):
        res = mi.Spectrum(0.0)
        for i in range(4):
            res[i, 0] = v[i]
        return res

    # Create a Silver conductor BSDF and reflect different polarization states
    # at a 45˚ angle.
    # All tests take place directly in local BSDF coordinates. Here we only
    # want to make sure that the output of this looks like what you would
    # expect from a Mueller matrix describing specular reflection on a mirror.

    eta = 0.136125 + 4.010625j # IoR values of Ag at 635.816284nm
    bsdf = mi.load_dict({
        'type': 'conductor',
        'eta': eta.real,
        'k': eta.imag,
    })

    theta_i = 45 * dr.pi / 180.0
    wi = mi.Vector3f([-dr.sin(theta_i), 0, dr.cos(theta_i)])

    ctx = mi.BSDFContext()
    ctx.mode = mi.TransportMode.Importance
    si = mi.SurfaceInteraction3f()
    si.p = mi.Point3f(0, 0, 0)
    si.wi = wi
    n = mi.Normal3f(0, 0, 1)
    si.sh_frame = mi.Frame3f(n)

    bs, M_local = bsdf.sample(ctx, si, 0.0, [0.0, 0.0])
    wo = bs.wo

    # Rotate into standard coordinates for specular reflection
    bi_s = dr.normalize(dr.cross(n, -wi))
    bi_p = dr.normalize(dr.cross(-wi, bi_s))
    bo_s = dr.normalize(dr.cross(n, wo))
    bo_p = dr.normalize(dr.cross(wo, bi_s))

    M_local = mi.mueller.rotate_mueller_basis(
        M_local,
        -wi, mi.mueller.stokes_basis(-wi), bi_s,
        wo, mi.mueller.stokes_basis(wo), bo_s
    )

    # Apply to unpolarized light and verify that it is equivalent to normal Fresnel
    a0 = M_local @ spectrum_from_stokes([1, 0, 0, 0])
    F = mi.fresnel_conductor(dr.cos(theta_i), dr.scalar.Complex2f(eta.real, eta.imag))
    a0 = a0[0, 0]
    assert dr.allclose(a0[0], F, atol=1e-3)

    # Apply to horizontally polarized light (linear at 0˚)
    # Test that it is..
    # 1) still fully polarized (though with reduced intensity)
    # 2) still horizontally polarized
    a1 = M_local @ spectrum_from_stokes([1, +1, 0, 0])
    assert dr.allclose(a1[0, 0], dr.abs(a1[1, 0]), atol=1e-3)              # 1)
    a1 = a1 * dr.rcp(a1[0, 0][0])
    assert dr.allclose(a1, spectrum_from_stokes([1, 1, 0, 0]), atol=1e-3)  # 2)

    # Apply to vertically polarized light (linear at 90˚)
    # Test that it is..
    # 1) still fully polarized (though with reduced intensity)
    # 2) still vertically polarized
    a2 = M_local @ spectrum_from_stokes([1, -1, 0, 0])
    assert dr.allclose(a2[0, 0], dr.abs(a2[1, 0]), atol=1e-3)              # 1)
    a2 = a2 * dr.rcp(a2[0, 0][0])
    assert dr.allclose(a2, spectrum_from_stokes([1, -1, 0, 0]), atol=1e-3) # 2)

    # Apply to (positive) diagonally polarized light (linear at +45˚)
    # Test that..
    # 1) The polarization is flipped to -45˚
    # 2) There is now also some (right) circular polarization
    a3 = M_local @ spectrum_from_stokes([1, 0, +1, 0])
    assert dr.all(a3[2, 0] < mi.UnpolarizedSpectrum(0.0))
    assert dr.all(a3[3, 0] > mi.UnpolarizedSpectrum(0.0))

    # Apply to (negative) diagonally polarized light (linear at -45˚)
    # Test that..
    # 1) The polarization is flipped to +45˚
    # 2) There is now also some (left) circular polarization
    a4 = M_local @ spectrum_from_stokes([1, 0, -1, 0])
    assert dr.all(a4[2, 0] > mi.UnpolarizedSpectrum(0.0))
    assert dr.all(a4[3, 0] < mi.UnpolarizedSpectrum(0.0))

    # Apply to right circularly polarized light
    # Test that the polarization is flipped to left circular
    a5 = M_local @ spectrum_from_stokes([1, 0, 0, +1])
    assert dr.all(a5[3, 0] < mi.UnpolarizedSpectrum(0.0))

    # Apply to left circularly polarized light
    # Test that the polarization is flipped to right circular
    a6 = M_local @ spectrum_from_stokes([1, 0, 0, -1])
    assert dr.all(a6[3, 0] > mi.UnpolarizedSpectrum(0.0))


def test02_sample_pol_world(variant_scalar_mono_polarized):

    def spectrum_from_stokes(v):
        res = mi.Spectrum(0.0)
        for i in range(4):
            res[i,0] = v[i]
        return res

    # Create a Silver conductor BSDF and reflect different polarization states
    # at a 45˚ angle.
    # This test takes place in world coordinates and thus involves additional
    # coordinate system rotations.
    #
    # The setup is as follows:
    # - The mirror is positioned at [0, 0, 0], angled s.t. the surface normal
    #   is along [1, 1, 0].
    # - Incident ray w1 = [-1, 0, 0] strikes the mirror at a 45˚ angle and
    #   reflects into direction w2 = [0, 1, 0]
    # - The corresponding Stokes bases are b1 = [0, 1, 0] and b2 = [1, 0, 0].

    # Setup
    eta = 0.136125 + 4.010625j # IoR values of Ag at 635.816284nm
    bsdf = mi.load_dict({
        'type': 'conductor',
        'eta': eta.real,
        'k': eta.imag,
    })

    ctx = mi.BSDFContext()
    ctx.mode = mi.TransportMode.Importance
    si = mi.SurfaceInteraction3f()
    si.p = mi.Point3f(0, 0, 0)
    si.n = dr.normalize(mi.Normal3f(1.0, 1.0, 0.0))
    si.sh_frame = mi.Frame3f(si.n)

    # Incident / outgoing directions and their Stokes bases
    w1 = mi.Vector3f(-1, 0, 0); b1 = mi.Vector3f(0, 1, 0)
    w2 = mi.Vector3f(0, 1, 0);  b2 = mi.Vector3f(1, 0, 0)

    # Perform actual reflection
    si.wi = si.to_local(-w1)
    bs, M_tmp = bsdf.sample(ctx, si, 0.0, [0.0, 0.0])
    M_world = si.to_world_mueller(M_tmp, -si.wi, bs.wo)

    # Test that outgoing direction is as predicted
    assert dr.allclose(si.to_world(bs.wo), w2, atol=1e-5)

    # Align reference frames s.t. polarization is expressed w.r.t. b1 & b2
    M_world = mi.mueller.rotate_mueller_basis(
        M_world,
        w1, mi.mueller.stokes_basis(w1), b1,
        w2, mi.mueller.stokes_basis(w2), b2
    )

    # Apply to unpolarized light and verify that it is equivalent to normal Fresnel
    a0 = M_world @ spectrum_from_stokes([1, 0, 0, 0])
    F = mi.fresnel_conductor(si.wi[2], dr.scalar.Complex2f(eta.real, eta.imag))
    a0 = a0[0, 0]
    assert dr.allclose(a0[0], F, atol=1e-3)

    # Apply to horizontally polarized light (linear at 0˚)
    # Test that it is..
    # 1) still fully polarized (though with reduced intensity)
    # 2) still horizontally polarized
    a1 = M_world @ spectrum_from_stokes([1, +1, 0, 0])
    assert dr.allclose(a1[0, 0], dr.abs(a1[1, 0]), atol=1e-3)              # 1)
    a1 = a1 * dr.rcp(a1[0, 0][0])
    assert dr.allclose(a1, spectrum_from_stokes([1, 1, 0, 0]), atol=1e-3)  # 2)

    # Apply to vertically polarized light (linear at 90˚)
    # Test that it is..
    # 1) still fully polarized (though with reduced intensity)
    # 2) still vertically polarized
    a2 = M_world @ spectrum_from_stokes([1, -1, 0, 0])
    assert dr.allclose(a2[0, 0], dr.abs(a2[1, 0]), atol=1e-3)              # 1)
    a2 = a2 * dr.rcp(a2[0, 0][0])
    assert dr.allclose(a2, spectrum_from_stokes([1, -1, 0, 0]), atol=1e-3) # 2)

    # Apply to (positive) diagonally polarized light (linear at +45˚)
    # Test that..
    # 1) The polarization is flipped to -45˚
    # 2) There is now also some (left) circular polarization
    a3 = M_world @ spectrum_from_stokes([1, 0, +1, 0])
    assert dr.all(a3[2, 0] < mi.UnpolarizedSpectrum(0.0))
    assert dr.all(a3[3, 0] < mi.UnpolarizedSpectrum(0.0))

    # Apply to (negative) diagonally polarized light (linear at -45˚)
    # Test that..
    # 1) The polarization is flipped to +45˚
    # 2) There is now also some (right) circular polarization
    a4 = M_world @ spectrum_from_stokes([1, 0, -1, 0])
    assert dr.all(a4[2, 0] > mi.UnpolarizedSpectrum(0.0))
    assert dr.all(a4[3, 0] > mi.UnpolarizedSpectrum(0.0))

    # Apply to right circularly polarized light
    # Test that the polarization is flipped to left circular
    a5 = M_world @ spectrum_from_stokes([1, 0, 0, +1])
    assert dr.all(a5[3, 0] < mi.UnpolarizedSpectrum(0.0))

    # Apply to left circularly polarized light
    # Test that the polarization is flipped to right circular
    a6 = M_world @ spectrum_from_stokes([1, 0, 0, -1])
    assert dr.all(a6[3, 0] > mi.UnpolarizedSpectrum(0.0))


def test03_thin_film_construct(variant_scalar_rgb):
    # Film params accepted in non-spectral variants but silently inert.
    b = mi.load_dict({'type': 'conductor', 'material': 'Au',
                      'film_thickness': 100.0, 'film_ior': 1.38})
    assert b is not None

    # film_ior accepted as material name.
    b = mi.load_dict({'type': 'conductor', 'material': 'Cu',
                      'film_thickness': 200.0, 'film_ior': 'water'})
    assert b is not None

    # Negative thickness rejected.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'conductor', 'material': 'Cu',
                      'film_thickness': -10.0})


def test04_thin_film_spectral(variant_scalar_spectral):
    """In a spectral variant, an enabled thin film should change the
    conductor's spectral reflectance signature (the difference vs the
    no-film case must vary with wavelength)."""
    bsdf_film = mi.load_dict({
        'type': 'conductor', 'material': 'Al',
        'film_thickness': 200.0, 'film_ior': 1.38,
    })
    bsdf_no_film = mi.load_dict({'type': 'conductor', 'material': 'Al'})

    si = mi.SurfaceInteraction3f()
    angle = 30 * dr.pi / 180
    si.wi = [dr.sin(angle), 0, dr.cos(angle)]
    si.wavelengths = [430.0, 510.0, 590.0, 670.0]
    ctx = mi.BSDFContext()

    _, w_film    = bsdf_film.sample(ctx, si, 0, [0, 0])
    _, w_no_film = bsdf_no_film.sample(ctx, si, 0, [0, 0])

    w_film_arr    = mi.unpolarized_spectrum(w_film)
    w_no_film_arr = mi.unpolarized_spectrum(w_no_film)

    # The film modulates Al's already-varying reflectance differently at
    # each wavelength — the film/no-film difference should not be uniform.
    delta = w_film_arr - w_no_film_arr
    delta_spread = float(dr.max(delta) - dr.min(delta))
    assert delta_spread > 0.02, \
        f'film should change spectral signature non-uniformly; spread={delta_spread}'


def test05_thin_film_polarized_guard():
    """A polarized spectral variant should reject thin-film at construction."""
    polarized = [v for v in mi.variants() if 'polarized' in v and 'spectral' in v]
    if not polarized:
        pytest.skip('no polarized spectral variant in this build')
    mi.set_variant(polarized[0])
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'conductor', 'material': 'Au',
                      'film_thickness': 200.0})
