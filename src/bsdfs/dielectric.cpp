#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/fresnel.h>
#include <mitsuba/render/texture.h>
#include <mitsuba/render/ior.h>

NAMESPACE_BEGIN(mitsuba)

/**!

.. _bsdf-dielectric:

Smooth dielectric material (:monosp:`dielectric`)
-------------------------------------------------

.. pluginparameters::
 :extra-rows: 8

 * - int_ior
   - |float| or |string|
   - Interior index of refraction specified numerically or using a known material name. (Default: bk7 / 1.5046)

 * - ext_ior
   - |float| or |string|
   - Exterior index of refraction specified numerically or using a known material name.  (Default: air / 1.000277)

 * - abbe
   - |float|
   - Optional Abbe number :math:`V` of the interface (dimensionless). Enables
     wavelength-dependent refraction in spectral variants. Lower values give
     stronger dispersion: :math:`V \approx 60` is crown-glass-like,
     :math:`V \approx 35` is flint-like, :math:`V \approx 20` is
     heavy-flint / "diamond fire". Internally converted to ``cauchy_b`` via
     :math:`B = (\eta_0 - 1) / (1.9085\,V)`. Mutually exclusive with
     ``cauchy_b``. Ignored in RGB / monochromatic variants.
     (Default: 0, no dispersion)

 * - cauchy_b
   - |float|
   - Cauchy dispersion coefficient :math:`B` (in :math:`\mu m^2`), for users
     who want to specify dispersion directly. When nonzero in a spectral
     variant, the relative index of refraction varies with wavelength as
     :math:`\eta(\lambda) = \eta_0 + B\,(1/\lambda^2 - 1/\lambda_\text{ref}^2)`,
     with :math:`\lambda_\text{ref} = 0.5893\,\mu m` (sodium D line).
     ``int_ior`` / ``ext_ior`` then describe the refractive index at
     :math:`\lambda_\text{ref}`. Mutually exclusive with ``abbe``. Ignored in
     RGB / monochromatic variants. (Default: 0, no dispersion)

 * - film_thickness
   - |float|
   - Optional thickness of a single non-absorbing dielectric film coating the
     interface, in nanometers. When nonzero in a spectral variant, the model
     replaces the standard Fresnel reflectance with the Airy formula for an
     ambient → film → substrate stack, producing wavelength-dependent
     interference (iridescence). Typical values: 100–600 nm for AR coatings,
     400–2000 nm for soap-film / oil-slick effects. Not yet supported in
     polarized variants — instantiation throws if both are requested. Ignored
     in RGB / monochromatic variants. (Default: 0, no film)

 * - film_ior
   - |float| or |string|
   - Refractive index of the thin film, specified numerically or via a known
     material name (same conventions as ``int_ior`` / ``ext_ior``). Only
     consulted when ``film_thickness > 0``. (Default: air / 1.000277)

 * - specular_reflectance
   - |spectrum| or |texture|
   - Optional factor that can be used to modulate the specular reflection component. Note that for physical realism, this parameter should never be touched. (Default: 1.0)
   - |exposed|, |differentiable|

 * - specular_transmittance
   - |spectrum| or |texture|
   - Optional factor that can be used to modulate the specular transmission component. Note that for physical realism, this parameter should never be touched. (Default: 1.0)
   - |exposed|, |differentiable|

 * - eta
   - |float|
   - Relative index of refraction from the exterior to the interior
   - |exposed|

.. subfigstart::
.. subfigure:: ../../resources/data/docs/images/render/bsdf_dielectric_glass.jpg
    :caption: Air ↔ Water (IOR: 1.33) interface.
.. subfigure:: ../../resources/data/docs/images/render/bsdf_dielectric_diamond.jpg
    :caption: Air ↔ Diamond (IOR: 2.419)
.. subfigend::
    :label: fig-bsdf-dielectric

This plugin models an interface between two dielectric materials having mismatched
indices of refraction (for instance, water ↔ air). Exterior and interior IOR values
can be specified independently, where "exterior" refers to the side that contains
the surface normal. When no parameters are given, the plugin activates the defaults, which
describe a borosilicate glass (BK7) ↔ air interface.

In this model, the microscopic structure of the surface is assumed to be perfectly
smooth, resulting in a degenerate BSDF described by a Dirac delta distribution.
This means that for any given incoming ray of light, the model always scatters into
a discrete set of directions, as opposed to a continuum. For a similar model that
instead describes a rough surface microstructure, take a look at the
:ref:`roughdielectric <bsdf-roughdielectric>` plugin.

This snippet describes a simple air-to-water interface

.. tabs::
    .. code-tab:: xml
        :name: dielectric-water

        <shape type="...">
            <bsdf type="dielectric">
                <string name="int_ior" value="water"/>
                <string name="ext_ior" value="air"/>
            </bsdf>
        </shape>

    .. code-tab:: python

        'type': 'dielectric',
        'int_ior': 'water',
        'ext_ior': 'air'

When using this model, it is crucial that the scene contains
meaningful and mutually compatible indices of refraction changes---see the
section about :ref:`correctness considerations <bsdf-correctness>` for a
description of what this entails.

In many cases, we will want to additionally describe the *medium* within a
dielectric material. This requires the use of a rendering technique that is
aware of media (e.g. the :ref:`volumetric path tracer <integrator-volpath>`).
An example of how one might describe a slightly absorbing piece of glass is shown below:

.. tabs::
    .. code-tab:: xml
        :name: dielectric-glass

        <shape type="...">
            <bsdf type="dielectric">
                <float name="int_ior" value="1.504"/>
                <float name="ext_ior" value="1.0"/>
            </bsdf>

            <medium type="homogeneous" name="interior">
                <float name="scale" value="4"/>
                <rgb name="sigma_t" value="1, 1, 0.5"/>
                <rgb name="albedo" value="0.0, 0.0, 0.0"/>
            </medium>
        </shape>

    .. code-tab:: python

        'type': '...',
        'glass':  {
            'type': 'dielectric',
            'int_ior': 1.504,
            'ext_ior': 1.0
        },
        'interior': {
            'type': 'homogeneous',
            'scale': 4,
            'sigma_t': {
                'type': 'rgb',
                'value': [1, 1, 0.5]
            },
            'albedo': {
                'type': 'rgb',
                'value': [0.0, 0.0, 0.0]
            }
        }

In *polarized* rendering modes, the material automatically switches to a polarized
implementation of the underlying Fresnel equations that quantify the reflectance and
transmission.

Dispersion (spectral variants only)
***********************************

Passing a nonzero ``abbe`` (or ``cauchy_b``) enables wavelength-dependent
refraction via Cauchy's two-term equation. The ``abbe`` parameter is the
recommended entry point: it is dimensionless, matches values listed in glass
datasheets, and has an intuitive range.

Typical Abbe numbers:

.. list-table::
    :widths: 25 25 50
    :header-rows: 1

    * - :math:`V`
      - Character
      - Real-world analog
    * - 80–95
      - Nearly achromatic
      - Fluorite, fluorocrown
    * - 55–70
      - Subtle rainbow
      - BK7, water (:math:`V \approx 55`)
    * - 35–55
      - Visible rainbow
      - Ordinary flint glass
    * - 20–35
      - "Fire"
      - Heavy flint, cut crystal
    * - 12–20
      - Strong "diamond fire"
      - Diamond

Sampling uses a hero-wavelength scheme: one wavelength drives the refracted
direction, and the remaining wavelengths in the packet are masked out on
transmission. This trades variance (dispersive paths have higher variance per
sample) for an unbiased estimate. Reflection is wavelength-independent and
all wavelengths in the packet contribute.

Thin-film interference (spectral variants only)
***********************************************

Setting ``film_thickness`` (in nanometers) coats the interface with a single
non-absorbing dielectric film of refractive index ``film_ior``. The reflectance
becomes wavelength-dependent via the Airy formula for a thin layer, producing
iridescence — the colored sheen of soap films, oil slicks, and anti-reflective
coatings on lenses. Typical thickness ranges:

.. list-table::
    :widths: 25 25 50
    :header-rows: 1

    * - Thickness
      - Effect
      - Real-world analog
    * - 100–200 nm
      - Color shift, broad
      - AR coatings (single-layer MgF₂)
    * - 300–600 nm
      - Strong color
      - Soap films, oil on water
    * - 600–2000 nm
      - High-order rainbows
      - Thicker soap films, peacock feathers
    * - > 2000 nm
      - Washes out
      - Interference fringes too dense for the spectrum

The film does not displace the refracted ray (films are vanishingly thin
compared to the geometric ray), so refraction direction and ``bs.eta`` are
determined by the substrate IOR alone. Thin-film and dispersion compose
naturally — when both are enabled, the substrate IOR varies with wavelength
inside the Airy formula. **Not yet supported in polarized variants**:
constructing a dielectric with ``film_thickness > 0`` in a polarized variant
throws.

Instead of specifying numerical values for the indices of refraction, Mitsuba 3
comes with a list of presets that can be specified with the :paramtype:`material`
parameter:

.. figtable::
    :label: ior-table-list
    :caption: This table lists all supported material names
       along with along with their associated index of refraction at standard conditions.
       These material names can be used with the plugins :ref:`dielectric <bsdf-dielectric>`,
       :ref:`roughdielectric <bsdf-roughdielectric>`, :ref:`plastic <bsdf-plastic>`
       , as well as :ref:`roughplastic <bsdf-roughplastic>`.
    :alt: List table

    .. list-table::
        :widths: 35 25 35 25
        :header-rows: 1

        * - Name
          - Value
          - Name
          - Value
        * - :paramtype:`vacuum`
          - 1.0
          - :paramtype:`acetone`
          - 1.36
        * - :paramtype:`bromine`
          - 1.661
          - :paramtype:`bk7`
          - 1.5046
        * - :paramtype:`helium`
          - 1.00004
          - :paramtype:`ethanol`
          - 1.361
        * - :paramtype:`water ice`
          - 1.31
          - :paramtype:`sodium chloride`
          - 1.544
        * - :paramtype:`hydrogen`
          - 1.00013
          - :paramtype:`carbon tetrachloride`
          - 1.461
        * - :paramtype:`fused quartz`
          - 1.458
          - :paramtype:`amber`
          - 1.55
        * - :paramtype:`air`
          - 1.00028
          - :paramtype:`glycerol`
          - 1.4729
        * - :paramtype:`pyrex`
          - 1.470
          - :paramtype:`pet`
          - 1.575
        * - :paramtype:`carbon dioxide`
          - 1.00045
          - :paramtype:`benzene`
          - 1.501
        * - :paramtype:`acrylic glass`
          - 1.49
          - :paramtype:`diamond`
          - 2.419
        * - :paramtype:`water`
          - 1.3330
          - :paramtype:`silicone oil`
          - 1.52045
        * - :paramtype:`polypropylene`
          - 1.49
          -
          -
 */

template <typename Float, typename Spectrum>
class SmoothDielectric final : public BSDF<Float, Spectrum> {
public:
    MI_IMPORT_BASE(BSDF, m_flags, m_components)
    MI_IMPORT_TYPES(Texture)

    SmoothDielectric(const Properties &props) : Base(props) {

        // Specifies the internal index of refraction at the interface
        ScalarFloat int_ior = lookup_ior(props, "int_ior", "bk7");

        // Specifies the external index of refraction at the interface
        ScalarFloat ext_ior = lookup_ior(props, "ext_ior", "air");

        if (int_ior < 0 || ext_ior < 0)
            Throw("The interior and exterior indices of refraction must"
                  " be positive!");

        m_eta = int_ior / ext_ior;

        /* Optional dispersion. Users can either specify the Cauchy coefficient
           B directly (cauchy_b, in μm²), or the Abbe number of the interface
           (abbe, dimensionless), which is more intuitive and matches values
           listed in glass datasheets. When abbe is given, it is converted to
           B via B = (m_eta - 1) / (K * abbe), where
           K = 1/λ_F² - 1/λ_C² ≈ 1.9085 μm⁻² with λ_F = 0.48613 μm and
           λ_C = 0.65627 μm (Fraunhofer F and C lines). The two parameters
           are mutually exclusive. Both are ignored in RGB / monochromatic
           variants. */
        ScalarFloat cauchy_b = props.get<ScalarFloat>("cauchy_b", 0.f);
        ScalarFloat abbe     = props.get<ScalarFloat>("abbe", 0.f);

        if (cauchy_b != 0.f && abbe != 0.f)
            Throw("Specify either 'cauchy_b' or 'abbe', not both.");
        if (abbe < 0.f)
            Throw("'abbe' must be positive (got %f).", abbe);

        if (abbe != 0.f)
            cauchy_b = (m_eta - 1.f) / (1.9085f * abbe);

        m_cauchy_b = cauchy_b;

        /* Optional thin-film interference. film_thickness is in nm; film_ior
           is parsed the same way as int_ior/ext_ior and stored relative to
           ext_ior so it lives in the same coordinate system as m_eta. The
           polarized Mueller path does not yet handle thin-film phase shifts;
           we throw rather than silently produce wrong polarization output. */
        ScalarFloat film_thickness = props.get<ScalarFloat>("film_thickness", 0.f);
        ScalarFloat film_ior_abs   = lookup_ior(props, "film_ior", "air");

        if (film_thickness < 0.f)
            Throw("'film_thickness' must be non-negative (got %f).", film_thickness);
        if (film_ior_abs <= 0.f)
            Throw("'film_ior' must be positive (got %f).", film_ior_abs);

        if constexpr (is_polarized_v<Spectrum>) {
            if (film_thickness > 0.f)
                Throw("'film_thickness' is not yet supported in polarized "
                      "variants. Use a non-polarized spectral variant for "
                      "thin-film effects.");
        }

        m_film_thickness = film_thickness;
        m_film_ior       = film_ior_abs / ext_ior;

        if (props.has_property("specular_reflectance"))
            m_specular_reflectance   = props.get_texture<Texture>("specular_reflectance", 1.f);
        if (props.has_property("specular_transmittance"))
            m_specular_transmittance = props.get_texture<Texture>("specular_transmittance", 1.f);

        m_components.push_back(BSDFFlags::DeltaReflection | BSDFFlags::FrontSide |
                               BSDFFlags::BackSide);
        m_components.push_back(BSDFFlags::DeltaTransmission | BSDFFlags::FrontSide |
                               BSDFFlags::BackSide | BSDFFlags::NonSymmetric);

        m_flags = m_components[0] | m_components[1];
    }

    void traverse(TraversalCallback *cb) override {
        cb->put("eta", m_eta, ParamFlags::NonDifferentiable);
        if (m_specular_reflectance)
            cb->put("specular_reflectance", m_specular_reflectance, ParamFlags::Differentiable);
        if (m_specular_transmittance)
            cb->put("specular_transmittance", m_specular_transmittance, ParamFlags::Differentiable);
    }

    /// Per-wavelength eta via Cauchy's equation around the sodium D line.
    UnpolarizedSpectrum eval_eta(const Wavelength &wavelengths) const {
        if constexpr (is_spectral_v<Spectrum>) {
            if (m_cauchy_b != 0.f) {
                UnpolarizedSpectrum lambda_um = wavelengths * ScalarFloat(1e-3f);
                UnpolarizedSpectrum inv_lambda_sq = dr::rcp(dr::square(lambda_um));
                ScalarFloat lambda_ref = 0.5893f;
                ScalarFloat inv_ref_sq = 1.f / (lambda_ref * lambda_ref);
                return m_eta + m_cauchy_b * (inv_lambda_sq - inv_ref_sq);
            }
            DRJIT_MARK_USED(wavelengths);
            return UnpolarizedSpectrum(m_eta);
        } else {
            DRJIT_MARK_USED(wavelengths);
            return UnpolarizedSpectrum(m_eta);
        }
    }

    bool has_dispersion() const {
        return is_spectral_v<Spectrum> && m_cauchy_b != 0.f;
    }

    bool has_thin_film() const {
        return is_spectral_v<Spectrum> && !is_polarized_v<Spectrum> &&
               m_film_thickness > 0.f;
    }

    /// True when any per-wavelength effect (dispersion or thin-film) is on,
    /// i.e. when r_i_spec / t_i_spec must drive the unpolarized weight.
    bool has_spectral_fresnel() const {
        return has_dispersion() || has_thin_film();
    }

    /// Intensity reflectance per wavelength for a single non-absorbing film
    /// (ambient → film → substrate stack), via the Airy formula. Refraction
    /// direction is unaffected — only reflectance varies with λ.
    UnpolarizedSpectrum thin_film_reflectance(
            Float cos_theta_i,
            const UnpolarizedSpectrum &eta_substrate,
            const Wavelength &wavelengths) const {
        if constexpr (is_spectral_v<Spectrum> && !is_polarized_v<Spectrum>) {
            Float cos_a = dr::abs(cos_theta_i);
            Float sin2_a = 1.f - dr::square(cos_a);

            // Snell into the film. n_a = 1 by convention (film/substrate IORs
            // are already expressed relative to the exterior medium).
            ScalarFloat inv_nf_sq = 1.f / (m_film_ior * m_film_ior);
            Float sin2_f = sin2_a * inv_nf_sq;
            // If n_f < 1 and incidence is grazing, sin2_f could exceed 1 →
            // TIR at the top interface. safe_sqrt clamps this; the resulting
            // Airy value is degenerate but bounded.
            Float cos_f = dr::safe_sqrt(1.f - sin2_f);

            // Snell into the substrate, per wavelength.
            UnpolarizedSpectrum inv_ns_sq = dr::rcp(dr::square(eta_substrate));
            UnpolarizedSpectrum sin2_s = sin2_a * inv_ns_sq;
            // TIR at substrate: full reflection regardless of film. Clamp the
            // intermediate value to avoid NaN, then mask in the final answer.
            auto tir = sin2_s >= 1.f;
            sin2_s = dr::minimum(sin2_s, UnpolarizedSpectrum(1.f - 1e-7f));
            UnpolarizedSpectrum cos_s = dr::safe_sqrt(1.f - sin2_s);

            // Fresnel amplitude reflection coefficients at each interface.
            // Top (ambient → film): n_a = 1.
            Float r01_s = (cos_a - m_film_ior * cos_f) /
                          (cos_a + m_film_ior * cos_f);
            Float r01_p = (m_film_ior * cos_a - cos_f) /
                          (m_film_ior * cos_a + cos_f);
            // Bottom (film → substrate), per wavelength.
            UnpolarizedSpectrum r12_s = (m_film_ior * cos_f - eta_substrate * cos_s) /
                                        (m_film_ior * cos_f + eta_substrate * cos_s);
            UnpolarizedSpectrum r12_p = (eta_substrate * cos_f - m_film_ior * cos_s) /
                                        (eta_substrate * cos_f + m_film_ior * cos_s);

            // Phase accumulated by one round-trip through the film:
            //   φ = 4π · n_f · d · cos(θ_f) / λ
            // d and λ both in nm → unitless φ.
            UnpolarizedSpectrum phi =
                (4.f * dr::Pi<ScalarFloat>) * m_film_ior * m_film_thickness *
                cos_f * dr::rcp(wavelengths);
            UnpolarizedSpectrum cos_phi = dr::cos(phi);

            // Airy intensity reflectance per polarization.
            auto airy = [&](Float r01, const UnpolarizedSpectrum &r12) {
                UnpolarizedSpectrum r01_sq(r01 * r01);
                UnpolarizedSpectrum r12_sq = dr::square(r12);
                UnpolarizedSpectrum two_r  = 2.f * r01 * r12 * cos_phi;
                return (r01_sq + r12_sq + two_r) /
                       (1.f + r01_sq * r12_sq + two_r);
            };

            UnpolarizedSpectrum R = 0.5f * (airy(r01_s, r12_s) + airy(r01_p, r12_p));
            return dr::select(tir, UnpolarizedSpectrum(1.f), R);
        } else {
            DRJIT_MARK_USED(cos_theta_i);
            DRJIT_MARK_USED(eta_substrate);
            DRJIT_MARK_USED(wavelengths);
            return UnpolarizedSpectrum(0.f);  // unreachable: has_thin_film() is false
        }
    }

    std::pair<BSDFSample3f, Spectrum> sample(const BSDFContext &ctx,
                                             const SurfaceInteraction3f &si,
                                             Float sample1,
                                             const Point2f & /* sample2 */,
                                             Mask active) const override {
        MI_MASKED_FUNCTION(ProfilerPhase::BSDFSample, active);

        bool has_reflection   = ctx.is_enabled(BSDFFlags::DeltaReflection, 0),
             has_transmission = ctx.is_enabled(BSDFFlags::DeltaTransmission, 1);

        // Evaluate the Fresnel equations for unpolarized illumination
        Float cos_theta_i = Frame3f::cos_theta(si.wi);

        /* Per-wavelength relative IOR. Equals m_eta everywhere unless the
           user enabled Cauchy dispersion in a spectral variant. The hero
           wavelength (lane 0) drives the sampled direction and lobe choice;
           the remaining lanes contribute via weight scaling (reflection) or
           are masked out (transmission). */
        UnpolarizedSpectrum eta_spec = eval_eta(si.wavelengths);
        Float eta_hero = has_dispersion() ? Float(eta_spec[0]) : Float(m_eta);

        auto [r_i, cos_theta_t, eta_it, eta_ti] = fresnel(cos_theta_i, eta_hero);
        Float t_i = 1.f - r_i;

        UnpolarizedSpectrum r_i_spec(r_i), t_i_spec(t_i);
        if (has_dispersion()) {
            auto [r_s, cos_theta_t_s, eta_it_s, eta_ti_s] =
                fresnel(UnpolarizedSpectrum(cos_theta_i), eta_spec);
            (void) cos_theta_t_s; (void) eta_it_s; (void) eta_ti_s;
            r_i_spec = r_s;
            t_i_spec = 1.f - r_s;
        }

        /* Thin-film interference, if enabled, replaces the per-wavelength
           reflectance with the Airy formula. cos_theta_t / eta_it / eta_ti
           are unaffected — the film is too thin to displace the refracted
           ray, so refraction direction is set by substrate IOR alone. */
        if (has_thin_film()) {
            UnpolarizedSpectrum r_film =
                thin_film_reflectance(cos_theta_i, eta_spec, si.wavelengths);
            r_i_spec = r_film;
            t_i_spec = 1.f - r_film;
            r_i = Float(r_film[0]);  // hero λ drives lobe choice
            t_i = 1.f - r_i;
        }

        // Lobe selection
        BSDFSample3f bs = dr::zeros<BSDFSample3f>();
        Mask selected_r;
        if (likely(has_reflection && has_transmission)) {
            selected_r = sample1 <= r_i && active;
            bs.pdf = dr::detach(dr::select(selected_r, r_i, t_i));
        } else {
            if (has_reflection || has_transmission) {
                selected_r = Mask(has_reflection) && active;
                bs.pdf = 1.f;
            } else {
                return { bs, 0.f };
            }
        }
        Mask selected_t = !selected_r && active;

        bs.sampled_component = dr::select(selected_r, UInt32(0), UInt32(1));
        bs.sampled_type      = dr::select(selected_r, UInt32(+BSDFFlags::DeltaReflection),
                                                      UInt32(+BSDFFlags::DeltaTransmission));

        bs.wo = dr::select(selected_r,
                           reflect(si.wi),
                           refract(si.wi, cos_theta_t, eta_ti));

        bs.eta = dr::select(selected_r, Float(1.f), eta_it);

        UnpolarizedSpectrum reflectance = 1.f, transmittance = 1.f;
        if (m_specular_reflectance)
            reflectance = m_specular_reflectance->eval(si, selected_r);
        if (m_specular_transmittance)
            transmittance = m_specular_transmittance->eval(si, selected_t);

        Spectrum weight(0.f);
        if constexpr (is_polarized_v<Spectrum>) {
            /* Due to the coordinate system rotations for polarization-aware
               pBSDFs below we need to know the propagation direction of light.
               In the following, light arrives along `-wo_hat` and leaves along
               `+wi_hat`. */
            Vector3f wo_hat = ctx.mode == TransportMode::Radiance ? bs.wo : si.wi,
                     wi_hat = ctx.mode == TransportMode::Radiance ? si.wi : bs.wo;

            /* BSDF weights are Mueller matrices now. */
            Float cos_theta_o_hat = Frame3f::cos_theta(wo_hat);
            Spectrum R = mueller::specular_reflection(UnpolarizedSpectrum(cos_theta_o_hat), eta_spec),
                     T = mueller::specular_transmission(UnpolarizedSpectrum(cos_theta_o_hat), eta_spec);

            if (likely(has_reflection && has_transmission)) {
                weight = dr::select(selected_r, R, T) / bs.pdf;
            } else if (has_reflection || has_transmission) {
                weight = has_reflection ? R : T;
                bs.pdf = 1.f;
            }

            /* The Stokes reference frame vector of this matrix lies perpendicular
               to the plane of reflection. */
            Vector3f n(0, 0, 1);
            Vector3f s_axis_in  = dr::cross(n, -wo_hat);
            Vector3f s_axis_out = dr::cross(n, wi_hat);

            // Singularity when the input & output are collinear with the normal
            Mask collinear = dr::all(s_axis_in == Vector3f(0));
            s_axis_in  = dr::select(collinear, Vector3f(1, 0, 0),
                                               dr::normalize(s_axis_in));
            s_axis_out = dr::select(collinear, Vector3f(1, 0, 0),
                                               dr::normalize(s_axis_out));

            /* Rotate in/out reference vector of `weight` s.t. it aligns with the
               implicit Stokes bases of -wo_hat & wi_hat. */
            weight = mueller::rotate_mueller_basis(weight,
                                                   -wo_hat, s_axis_in, mueller::stokes_basis(-wo_hat),
                                                    wi_hat, s_axis_out, mueller::stokes_basis(wi_hat));

            if (dr::any_or<true>(selected_r))
                weight[selected_r] *= mueller::absorber(reflectance);

            if (dr::any_or<true>(selected_t))
                weight[selected_t] *= mueller::absorber(transmittance);

        } else {
            if (likely(has_reflection && has_transmission)) {
                weight = 1.f;
                /* For differentiable variants, lobe choice has to be detached to avoid bias.
                    Sampling weights should be computed accordingly. */
                if constexpr (dr::is_diff_v<Float>) {
                    if (dr::grad_enabled(r_i)) {
                        Float r_diff = dr::replace_grad(Float(1.f), r_i / dr::detach(r_i));
                        Float t_diff = dr::replace_grad(Float(1.f), t_i / dr::detach(t_i));
                        weight = dr::select(selected_r, r_diff, t_diff);
                    }
                }

                if (has_spectral_fresnel()) {
                    /* Hero wavelength drove the lobe decision with probability r_hero.
                       Reflection: all wavelengths share the sampled direction, so each
                       lane receives r_λ / r_hero. Transmission: only the hero wavelength
                       refracts into the sampled direction — the other lanes are killed. */
                    /* Guard against thin-film resonances where r_i (or
                       t_i = 1 - r_i) approaches zero — direct division
                       would produce NaN/inf and corrupt the path. When
                       the sampling pdf is below threshold, we accept the
                       tiny bias of zeroing the contribution rather than
                       letting NaN propagate through MIS / volumetric
                       transport. */
                    Float det_r = dr::detach(r_i),
                          det_t = dr::detach(t_i);
                    UnpolarizedSpectrum w_r = dr::select(
                        det_r > 1e-6f,
                        r_i_spec / dr::maximum(det_r, 1e-6f),
                        UnpolarizedSpectrum(0.f));
                    UnpolarizedSpectrum w_t = dr::zeros<UnpolarizedSpectrum>();
                    w_t[0] = dr::select(det_t > 1e-6f,
                                        t_i_spec[0] / dr::maximum(det_t, 1e-6f),
                                        Float(0.f));
                    weight = dr::select(selected_r, Spectrum(w_r), Spectrum(w_t));
                }
            } else if (has_reflection || has_transmission) {
                weight = has_reflection ? r_i : t_i;
                if (has_spectral_fresnel()) {
                    if (has_reflection) {
                        weight = Spectrum(r_i_spec);
                    } else {
                        UnpolarizedSpectrum w_t = dr::zeros<UnpolarizedSpectrum>();
                        w_t[0] = t_i_spec[0];
                        weight = Spectrum(w_t);
                    }
                }
            }

            if (dr::any_or<true>(selected_r))
                weight[selected_r] *= reflectance;

            if (dr::any_or<true>(selected_t))
                weight[selected_t] *= transmittance;
        }

        if (dr::any_or<true>(selected_t)) {
            /* For transmission, radiance must be scaled to account for the solid
               angle compression that occurs when crossing the interface. */
            Float factor = (ctx.mode == TransportMode::Radiance) ? eta_ti : Float(1.f);
            weight[selected_t] *= dr::square(factor);
        }

        return { bs, weight & active };
    }

    Spectrum eval(const BSDFContext & /* ctx */, const SurfaceInteraction3f & /* si */,
                  const Vector3f & /* wo */, Mask /* active */) const override {
        return 0.f;
    }

    Float pdf(const BSDFContext & /* ctx */, const SurfaceInteraction3f & /* si */,
              const Vector3f & /* wo */, Mask /* active */) const override {
        return 0.f;
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "SmoothDielectric[" << std::endl;
        if (m_specular_reflectance)
            oss << "  specular_reflectance = " << string::indent(m_specular_reflectance) << "," << std::endl;
        if (m_specular_transmittance)
            oss << "  specular_transmittance = " << string::indent(m_specular_transmittance) << ", " << std::endl;
        oss << "  eta = " << m_eta << "," << std::endl;
        if (m_cauchy_b != 0.f)
            oss << "  cauchy_b = " << m_cauchy_b << "," << std::endl;
        if (m_film_thickness > 0.f)
            oss << "  film_thickness = " << m_film_thickness << " nm," << std::endl
                << "  film_ior = " << m_film_ior << "," << std::endl;
        oss << "]";
        return oss.str();
    }

    MI_DECLARE_CLASS(SmoothDielectric)
private:
    ScalarFloat m_eta;
    ScalarFloat m_cauchy_b;
    ScalarFloat m_film_thickness;
    ScalarFloat m_film_ior;
    ref<Texture> m_specular_reflectance;
    ref<Texture> m_specular_transmittance;

    MI_TRAVERSE_CB(Base, m_eta, m_cauchy_b, m_film_thickness, m_film_ior,
                   m_specular_reflectance, m_specular_transmittance)
};

MI_EXPORT_PLUGIN(SmoothDielectric)
NAMESPACE_END(mitsuba)
