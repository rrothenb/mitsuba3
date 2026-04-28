#include <mitsuba/core/fwd.h>
#include <mitsuba/core/plugin.h>
#include <mitsuba/core/string.h>
#include <mitsuba/core/warp.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/fresnel.h>
#include <mitsuba/render/ior.h>
#include <mitsuba/render/texture.h>

NAMESPACE_BEGIN(mitsuba)

/**!
.. _bsdf-plastic:

Smooth plastic material (:monosp:`plastic`)
-------------------------------------------

.. pluginparameters::
 :extra-rows: 9

 * - diffuse_reflectance
   - |spectrum| or |texture|
   - Optional factor used to modulate the diffuse reflection component. (Default: 0.5)
   - |exposed|, |differentiable|

 * - nonlinear
   - |bool|
   - Account for nonlinear color shifts due to internal scattering? See the main text for details..
     (Default: Don't account for them and preserve the texture colors, i.e. |false|)

 * - int_ior
   - |float| or |string|
   - Interior index of refraction specified numerically or using a known material name.
     (Default: polypropylene / 1.49)

 * - ext_ior
   - |float| or |string|
   - Exterior index of refraction specified numerically or using a known material name.
     (Default: air / 1.000277)

 * - abbe
   - |float|
   - Optional Abbe number :math:`V` of the plastic layer (dimensionless).
     Enables Cauchy dispersion of the dielectric layer in spectral variants —
     the specular highlight picks up a chromatic shift, and the diffuse
     contribution is tinted by the wavelength-dependent
     :math:`(1 - F(\lambda))` factor. Internally converted to ``cauchy_b``
     via :math:`B = (\eta_0 - 1) / (1.9085\,V)`. Mutually exclusive with
     ``cauchy_b``. Ignored in RGB / monochromatic variants.
     (Default: 0, no dispersion)

 * - cauchy_b
   - |float|
   - Cauchy dispersion coefficient :math:`B` (in :math:`\mu m^2`), for users
     who want to specify dispersion directly. See
     :ref:`dielectric <bsdf-dielectric>` for the formula. Mutually exclusive
     with ``abbe``. Ignored in RGB / monochromatic variants.
     (Default: 0, no dispersion)

 * - specular_reflectance
   - |spectrum| or |texture|
   - Optional factor that can be used to modulate the specular reflection component. Note that for
     physical realism, this parameter should never be touched. (Default: 1.0)
   - |exposed|, |differentiable|

 * - film_thickness
   - |float|
   - Optional thickness of a single non-absorbing dielectric film coating the
     specular interface, in nanometers. When nonzero in a spectral variant,
     the standard Fresnel reflectance is replaced by the Airy formula for an
     ambient → film → substrate stack, producing wavelength-dependent
     iridescence on the specular highlight (and a complementary tint on the
     diffuse component, since the diffuse contribution is gated by
     :math:`(1 - F)`). Ignored in RGB / monochromatic variants.
     (Default: 0, no film)

 * - film_ior
   - |float| or |string|
   - Refractive index of the thin film, specified numerically or via a known
     material name. Only consulted when ``film_thickness > 0``.
     (Default: air / 1.000277)

 * - eta
   - |float|
   - Relative index of refraction from the exterior to the interior
   - |exposed|

.. subfigstart::
.. subfigure:: ../../resources/data/docs/images/render/bsdf_plastic_default.jpg
   :caption: A rendering with the default parameters
.. subfigure:: ../../resources/data/docs/images/render/bsdf_plastic_shiny.jpg
   :caption:  A rendering with custom parameters
.. subfigend::
    :label: fig-bsdf-plastic

This plugin describes a smooth plastic-like material with internal scattering. It uses the Fresnel
reflection and transmission coefficients to provide direction-dependent specular and diffuse
components. Since it is simple, realistic, and fast, this model is often a better choice than the
:ref:`roughplastic <bsdf-roughplastic>` plugins when rendering smooth plastic-like materials.
For convenience, this model allows to specify IOR values either numerically, or based on a list of
known materials (see the corresponding table in the :ref:`dielectric <bsdf-dielectric>` reference).
When no parameters are given, the plugin activates the defaults, which describe a white polypropylene
plastic material.

The following XML snippet describes a shiny material whose diffuse reflectance is specified using
sRGB:

.. tabs::
    .. code-tab:: xml
        :name: plastic-shiny

        <bsdf type="plastic">
            <rgb name="diffuse_reflectance" value="0.1, 0.27, 0.36"/>
            <float name="int_ior" value="1.9"/>
        </bsdf>

    .. code-tab:: python

        'type': 'plastic',
        'diffuse_reflectance': {
            'type': 'rgb',
            'value': [0.1, 0.27, 0.36]
        },
        'int_ior': 1.9

Internal scattering
*******************

Internally, tis model simulates the interaction of light with a diffuse
base surface coated by a thin dielectric layer. This is a convenient
abstraction rather than a restriction. In other words, there are many
materials that can be rendered with this model, even if they might not
fit this description perfectly well.

.. subfigstart::
.. subfigure:: ../../resources/data/docs/images/bsdf/plastic_intscat_1.svg
    :caption: (**a**) At the boundary, incident illumination is partly reflected and refracted
    :label: fig-plastic-intscat-a
.. subfigure:: ../../resources/data/docs/images/bsdf/plastic_intscat_2.svg
    :caption: (**b**) The refracted portion scatters diffusely at the base layer
    :label: fig-plastic-intscat-b
.. subfigure:: ../../resources/data/docs/images/bsdf/plastic_intscat_3.svg
    :caption: (**c**) An illustration of the scattering events that are internally handled by this plugin
    :label: fig-plastic-intscat-c
.. subfigend::
    :label: fig-bsdf-plastic-intscat

Given illumination that is incident upon such a material, a portion
of the illumination is specularly reflected at the material
boundary, which results in a sharp reflection in the mirror direction (**a**).
The remaining illumination refracts into the material, where it
scatters from the diffuse base layer (**b**).
While some of the diffusely scattered illumination is able to
directly refract outwards again, the remainder is reflected from the
interior side of the dielectric boundary and will in fact remain
trapped inside the material for some number of internal scattering
events until it is finally able to escape (**c**).

Due to the mathematical simplicity of this setup, it is possible to work
out the correct form of the model without actually having to simulate
the potentially large number of internal scattering events.

Note that due to the internal scattering, the diffuse color of the
material is in practice slightly different from the color of the
base layer on its own---in particular, the material color will tend to shift towards
darker colors with higher saturation. Since this can be counter-intuitive when
using bitmap textures, these color shifts are disabled by default. Specify
the parameter :code:`nonlinear=true` to enable them. The following renderings
illustrate the resulting change:

.. subfigstart::
.. subfigure:: ../../resources/data/docs/images/render/bsdf_plastic_diffuse.jpg
   :caption: Diffuse textured rendering
.. subfigure:: ../../resources/data/docs/images/render/bsdf_plastic_preserve.jpg
   :caption: Plastic model, :code:`nonlinear=false`
.. subfigure:: ../../resources/data/docs/images/render/bsdf_plastic_nopreserve.jpg
   :caption: Plastic model, :code:`nonlinear=true`
.. subfigend::
    :label: fig-bsdf-plastic-nonlinear

This effect is also seen in real life,
for instance a piece of wood will look slightly darker after coating it
with a layer of varnish.

Dispersion (spectral variants only)
***********************************

Passing a nonzero ``abbe`` (or ``cauchy_b``) makes the plastic layer's IOR
wavelength-dependent via Cauchy's two-term formula. The visible effect is a
chromatic shift on the specular highlight (subtle but real on lacquered or
clearcoated surfaces) and a complementary tint on the diffuse return,
because diffuse light is gated by :math:`(1 - F(\lambda))` on entry and
exit. Unlike :ref:`dielectric <bsdf-dielectric>`, plastic does not produce
the "split rainbow" caustic from refraction — its internal scattering is
modeled with a closed-form approximation rather than tracked as actual
refracted rays. See the dielectric docs for the Abbe-number lookup table.

Thin-film interference (spectral variants only)
***********************************************

Setting ``film_thickness`` (in nanometers) coats the dielectric interface
with a single non-absorbing film of refractive index ``film_ior``. In a
spectral variant the specular reflectance becomes wavelength-dependent via
the Airy formula, producing iridescence on the highlight; the diffuse
component picks up the complementary tint because diffuse light enters and
exits through the same film. This is the canonical model for varnished or
wet-painted surfaces, oil-on-paint effects, and structural color on diffuse
biological substrates. See the :ref:`dielectric <bsdf-dielectric>`
documentation for the full Airy derivation and the typical-thickness
reference table. Thin-film and dispersion compose naturally — when both
are enabled, the substrate IOR fed into the Airy formula varies with
wavelength. Plastic depolarizes its output, so thin-film works in polarized
variants but the result is unpolarized (consistent with the rest of
plastic's behavior).

*/

template <typename Float, typename Spectrum>
class SmoothPlastic final : public BSDF<Float, Spectrum> {
public:
    MI_IMPORT_BASE(BSDF, m_flags, m_components)
    MI_IMPORT_TYPES(Texture)

    SmoothPlastic(const Properties &props) : Base(props) {
        // Specifies the internal index of refraction at the interface
        ScalarFloat int_ior = lookup_ior(props, "int_ior", "polypropylene");

        // Specifies the external index of refraction at the interface
        ScalarFloat ext_ior = lookup_ior(props, "ext_ior", "air");

        if (int_ior < 0.f || ext_ior < 0.f)
            Throw("The interior and exterior indices of "
                  "refraction must be positive!");

        m_eta = int_ior / ext_ior;

        /* Optional thin-film interference (see SmoothDielectric for the full
           write-up). film_thickness is in nm; film_ior is parsed like
           int_ior/ext_ior and stored relative to ext_ior so it lives in the
           same coordinate system as m_eta. Because plastic depolarizes its
           output, no special polarized handling is required — the Airy
           reflectance just modulates the depolarized intensity. */
        ScalarFloat film_thickness = props.get<ScalarFloat>("film_thickness", 0.f);
        ScalarFloat film_ior_abs   = lookup_ior(props, "film_ior", "air");

        if (film_thickness < 0.f)
            Throw("'film_thickness' must be non-negative (got %f).", film_thickness);
        if (film_ior_abs <= 0.f)
            Throw("'film_ior' must be positive (got %f).", film_ior_abs);

        m_film_thickness = film_thickness;
        m_film_ior       = film_ior_abs / ext_ior;

        /* Optional Cauchy / Abbe dispersion of the dielectric layer. Same
           parameterization as in SmoothDielectric: users specify either an
           Abbe number (preferred, dimensionless, matches glass datasheets)
           or the raw Cauchy B coefficient (in μm²). When enabled in a
           spectral variant, the layer's IOR varies with wavelength as
           η(λ) = m_eta + B·(1/λ² − 1/λ_ref²), λ_ref = 0.5893 μm. The diffuse
           component picks up the chromatic shift naturally because its
           weight is gated by (1 − F(λ)). Refraction inside the substrate
           is approximated in plastic (closed-form internal scattering),
           so dispersion appears mainly as a tint on the specular highlight,
           not as a "split rainbow" caustic. */
        ScalarFloat cauchy_b = props.get<ScalarFloat>("cauchy_b", 0.f);
        ScalarFloat abbe     = props.get<ScalarFloat>("abbe", 0.f);

        if (cauchy_b != 0.f && abbe != 0.f)
            Throw("Specify either 'cauchy_b' or 'abbe', not both.");
        if (abbe < 0.f)
            Throw("'abbe' must be positive (got %f).", abbe);

        if (abbe != 0.f)
            cauchy_b = (m_eta - 1.f) / (1.9085f * abbe);

        m_cauchy_b = cauchy_b;

        m_diffuse_reflectance  = props.get_texture<Texture>("diffuse_reflectance", .5f);

        if (props.has_property("specular_reflectance"))
            m_specular_reflectance = props.get_texture<Texture>("specular_reflectance", 1.f);

        m_nonlinear = props.get<bool>("nonlinear", false);

        m_components.push_back(BSDFFlags::DeltaReflection | BSDFFlags::FrontSide);
        m_components.push_back(BSDFFlags::DiffuseReflection | BSDFFlags::FrontSide);
        m_flags = m_components[0] | m_components[1];

        parameters_changed();
    }

    void traverse(TraversalCallback *cb) override {
        cb->put("eta", m_eta, ParamFlags::NonDifferentiable);
        cb->put("diffuse_reflectance", m_diffuse_reflectance, ParamFlags::Differentiable);

        if (m_specular_reflectance)
            cb->put("specular_reflectance", m_specular_reflectance, ParamFlags::Differentiable);
    }

    void parameters_changed(const std::vector<std::string> &/*keys*/ = {}) override {
        m_inv_eta_2 = 1.f / (m_eta * m_eta);

        // Numerically approximate the diffuse Fresnel reflectance
        m_fdr_int = fresnel_diffuse_reflectance(1.f / m_eta);
        m_fdr_ext = fresnel_diffuse_reflectance(m_eta);

        // Compute weights that further steer samples towards the specular or diffuse components
        Float d_mean = m_diffuse_reflectance->mean(),
              s_mean = 1.f;

        if (m_specular_reflectance)
            s_mean = m_specular_reflectance->mean();

        m_specular_sampling_weight = s_mean / (d_mean + s_mean);
    }

    bool has_thin_film() const {
        return is_spectral_v<Spectrum> && m_film_thickness > 0.f;
    }

    bool has_dispersion() const {
        return is_spectral_v<Spectrum> && m_cauchy_b != 0.f;
    }

    /// Per-wavelength relative IOR via Cauchy's two-term formula around the
    /// sodium D line. See SmoothDielectric::eval_eta for the derivation.
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

    /// Intensity reflectance per wavelength for a single non-absorbing film
    /// (ambient → film → substrate stack), via the Airy formula. See the
    /// matching helper in SmoothDielectric for the derivation. The substrate
    /// IOR is passed per-wavelength so that this composes with dispersion.
    UnpolarizedSpectrum thin_film_reflectance(
            Float cos_theta_i,
            const UnpolarizedSpectrum &eta_substrate,
            const Wavelength &wavelengths) const {
        if constexpr (is_spectral_v<Spectrum>) {
            Float cos_a = dr::abs(cos_theta_i);
            Float sin2_a = 1.f - dr::square(cos_a);

            ScalarFloat inv_nf_sq = 1.f / (m_film_ior * m_film_ior);
            Float sin2_f = sin2_a * inv_nf_sq;
            Float cos_f = dr::safe_sqrt(1.f - sin2_f);

            // Snell into substrate, per wavelength.
            UnpolarizedSpectrum inv_ns_sq = dr::rcp(dr::square(eta_substrate));
            UnpolarizedSpectrum sin2_s = sin2_a * inv_ns_sq;
            auto tir = sin2_s >= 1.f;
            sin2_s = dr::minimum(sin2_s, UnpolarizedSpectrum(1.f - 1e-7f));
            UnpolarizedSpectrum cos_s = dr::safe_sqrt(1.f - sin2_s);

            // Top interface (ambient → film): n_a = 1.
            Float r01_s = (cos_a - m_film_ior * cos_f) /
                          (cos_a + m_film_ior * cos_f);
            Float r01_p = (m_film_ior * cos_a - cos_f) /
                          (m_film_ior * cos_a + cos_f);
            // Bottom (film → substrate), per wavelength.
            UnpolarizedSpectrum r12_s = (m_film_ior * cos_f - eta_substrate * cos_s) /
                                        (m_film_ior * cos_f + eta_substrate * cos_s);
            UnpolarizedSpectrum r12_p = (eta_substrate * cos_f - m_film_ior * cos_s) /
                                        (eta_substrate * cos_f + m_film_ior * cos_s);

            UnpolarizedSpectrum phi =
                (4.f * dr::Pi<ScalarFloat>) * m_film_ior * m_film_thickness *
                cos_f * dr::rcp(wavelengths);
            UnpolarizedSpectrum cos_phi = dr::cos(phi);

            auto airy = [&](Float r01, const UnpolarizedSpectrum &r12) {
                UnpolarizedSpectrum r01_sq(r01 * r01);
                UnpolarizedSpectrum r12_sq = dr::square(r12);
                UnpolarizedSpectrum two_r = 2.f * r01 * r12 * cos_phi;
                return (r01_sq + r12_sq + two_r) /
                       (1.f + r01_sq * r12_sq + two_r);
            };

            UnpolarizedSpectrum R = 0.5f * (airy(r01_s, r12_s) + airy(r01_p, r12_p));
            return dr::select(tir, UnpolarizedSpectrum(1.f), R);
        } else {
            DRJIT_MARK_USED(cos_theta_i);
            DRJIT_MARK_USED(eta_substrate);
            DRJIT_MARK_USED(wavelengths);
            return UnpolarizedSpectrum(0.f);
        }
    }

    std::pair<BSDFSample3f, Spectrum> sample(const BSDFContext &ctx,
                                             const SurfaceInteraction3f &si,
                                             Float sample1,
                                             const Point2f &sample2,
                                             Mask active) const override {
        MI_MASKED_FUNCTION(ProfilerPhase::BSDFSample, active);

        bool has_specular = ctx.is_enabled(BSDFFlags::DeltaReflection, 0),
             has_diffuse  = ctx.is_enabled(BSDFFlags::DiffuseReflection, 1);

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        active &= cos_theta_i > 0.f;

        BSDFSample3f bs = dr::zeros<BSDFSample3f>();
        UnpolarizedSpectrum result(0.f);
        if (unlikely((!has_specular && !has_diffuse) || dr::none_or<false>(active)))
            return { bs, result };

        // Determine which component should be sampled. With dispersion or
        // thin-film, the hero wavelength (lane 0) drives the sampling
        // probability and the per-wavelength reflectance feeds the
        // specular/diffuse weights.
        UnpolarizedSpectrum eta_spec = eval_eta(si.wavelengths);
        Float eta_hero = has_dispersion() ? Float(eta_spec[0]) : Float(m_eta);

        Float f_i = std::get<0>(fresnel(cos_theta_i, eta_hero));
        UnpolarizedSpectrum r_in_spec(f_i);
        if (has_dispersion()) {
            auto [r_s, ct_s, eit_s, eti_s] =
                fresnel(UnpolarizedSpectrum(cos_theta_i), eta_spec);
            (void) ct_s; (void) eit_s; (void) eti_s;
            r_in_spec = r_s;
        }
        if (has_thin_film()) {
            r_in_spec = thin_film_reflectance(cos_theta_i, eta_spec, si.wavelengths);
            f_i = Float(r_in_spec[0]);
        }
        Float prob_specular = f_i * m_specular_sampling_weight,
              prob_diffuse  = (1.f - f_i) * (1.f - m_specular_sampling_weight);

        if (unlikely(has_specular != has_diffuse))
            prob_specular = has_specular ? 1.f : 0.f;
        else
            prob_specular = prob_specular / (prob_specular + prob_diffuse);

        prob_diffuse = 1.f - prob_specular;

        Mask sample_specular = active && (sample1 < prob_specular),
             sample_diffuse  = active && !sample_specular;

        bs.eta = 1.f;
        bs.pdf = 0.f;

        if (dr::any_or<true>(sample_specular)) {
            dr::masked(bs.wo, sample_specular) = reflect(si.wi);
            dr::masked(bs.pdf, sample_specular) = prob_specular;
            dr::masked(bs.sampled_component, sample_specular) = 0;
            dr::masked(bs.sampled_type, sample_specular) = +BSDFFlags::DeltaReflection;

            UnpolarizedSpectrum value = r_in_spec / bs.pdf;
            if (m_specular_reflectance)
                value *= m_specular_reflectance->eval(si, sample_specular);
            result[sample_specular] = value;
        }

        if (dr::any_or<true>(sample_diffuse)) {
            dr::masked(bs.wo, sample_diffuse) = warp::square_to_cosine_hemisphere(sample2);
            dr::masked(bs.pdf, sample_diffuse) = prob_diffuse * warp::square_to_cosine_hemisphere_pdf(bs.wo);
            dr::masked(bs.sampled_component, sample_diffuse) = 1;
            dr::masked(bs.sampled_type, sample_diffuse) = +BSDFFlags::DiffuseReflection;

            Float cos_theta_o = Frame3f::cos_theta(bs.wo);
            Float f_o = std::get<0>(fresnel(cos_theta_o, eta_hero));
            UnpolarizedSpectrum r_out_spec(f_o);
            if (has_dispersion()) {
                auto [r_s, ct_s, eit_s, eti_s] =
                    fresnel(UnpolarizedSpectrum(cos_theta_o), eta_spec);
                (void) ct_s; (void) eit_s; (void) eti_s;
                r_out_spec = r_s;
            }
            if (has_thin_film())
                r_out_spec = thin_film_reflectance(cos_theta_o, eta_spec, si.wavelengths);

            // 1/η² term also varies with wavelength under dispersion.
            UnpolarizedSpectrum inv_eta_2_spec = has_dispersion()
                ? dr::rcp(dr::square(eta_spec))
                : UnpolarizedSpectrum(m_inv_eta_2);

            UnpolarizedSpectrum value = m_diffuse_reflectance->eval(si, sample_diffuse);
            value /= 1.f - (m_nonlinear ? (value * m_fdr_int) : m_fdr_int);
            value *= inv_eta_2_spec * (1.f - r_in_spec) * (1.f - r_out_spec) / prob_diffuse;
            result[sample_diffuse] = value;
        }

        return { bs, depolarizer<Spectrum>(result) };
    }

    Spectrum eval(const BSDFContext &ctx, const SurfaceInteraction3f &si,
                  const Vector3f &wo, Mask active) const override {
        MI_MASKED_FUNCTION(ProfilerPhase::BSDFEvaluate, active);

        bool has_diffuse = ctx.is_enabled(BSDFFlags::DiffuseReflection, 1);

        Float cos_theta_i = Frame3f::cos_theta(si.wi),
              cos_theta_o = Frame3f::cos_theta(wo);

        active &= cos_theta_i > 0.f && cos_theta_o > 0.f;

        if (unlikely(!has_diffuse || dr::none_or<false>(active)))
            return 0.f;

        UnpolarizedSpectrum eta_spec = eval_eta(si.wavelengths);
        Float eta_hero = has_dispersion() ? Float(eta_spec[0]) : Float(m_eta);

        Float f_i = std::get<0>(fresnel(cos_theta_i, eta_hero)),
              f_o = std::get<0>(fresnel(cos_theta_o, eta_hero));

        UnpolarizedSpectrum r_in_spec(f_i), r_out_spec(f_o);
        if (has_dispersion()) {
            auto [r_s_i, ct_i, eit_i, eti_i] =
                fresnel(UnpolarizedSpectrum(cos_theta_i), eta_spec);
            auto [r_s_o, ct_o, eit_o, eti_o] =
                fresnel(UnpolarizedSpectrum(cos_theta_o), eta_spec);
            (void) ct_i; (void) eit_i; (void) eti_i;
            (void) ct_o; (void) eit_o; (void) eti_o;
            r_in_spec  = r_s_i;
            r_out_spec = r_s_o;
        }
        if (has_thin_film()) {
            r_in_spec  = thin_film_reflectance(cos_theta_i, eta_spec, si.wavelengths);
            r_out_spec = thin_film_reflectance(cos_theta_o, eta_spec, si.wavelengths);
        }

        UnpolarizedSpectrum inv_eta_2_spec = has_dispersion()
            ? dr::rcp(dr::square(eta_spec))
            : UnpolarizedSpectrum(m_inv_eta_2);

        UnpolarizedSpectrum diff = m_diffuse_reflectance->eval(si, active);
        diff /= 1.f - (m_nonlinear ? (diff * m_fdr_int) : m_fdr_int);

        diff *= warp::square_to_cosine_hemisphere_pdf(wo) *
                inv_eta_2_spec * (1.f - r_in_spec) * (1.f - r_out_spec);

        return depolarizer<Spectrum>(diff) & active;
    }

    Float pdf(const BSDFContext &ctx, const SurfaceInteraction3f &si,
              const Vector3f &wo, Mask active) const override {
        MI_MASKED_FUNCTION(ProfilerPhase::BSDFEvaluate, active);

        Float cos_theta_i = Frame3f::cos_theta(si.wi),
              cos_theta_o = Frame3f::cos_theta(wo);

        active &= cos_theta_i > 0.f && cos_theta_o > 0.f;

        if (unlikely(!ctx.is_enabled(BSDFFlags::DiffuseReflection, 1) || dr::none_or<false>(active)))
            return 0.f;

        Float prob_diffuse = 1.f;

        if (ctx.is_enabled(BSDFFlags::DeltaReflection, 0)) {
            // Hero wavelength governs the sampling probability (matches
            // what sample() uses).
            UnpolarizedSpectrum eta_spec = eval_eta(si.wavelengths);
            Float eta_hero = has_dispersion() ? Float(eta_spec[0]) : Float(m_eta);
            Float f_i = std::get<0>(fresnel(cos_theta_i, eta_hero));
            if (has_thin_film())
                f_i = Float(thin_film_reflectance(
                    cos_theta_i, eta_spec, si.wavelengths)[0]);
            Float prob_specular = f_i * m_specular_sampling_weight;
            prob_diffuse  = (1.f - f_i) * (1.f - m_specular_sampling_weight);
            prob_diffuse = prob_diffuse / (prob_specular + prob_diffuse);
        }

        Float pdf = warp::square_to_cosine_hemisphere_pdf(wo) * prob_diffuse;

        return dr::select(active, pdf, 0.f);
    }

    std::pair<Spectrum, Float> eval_pdf(const BSDFContext &ctx,
                                        const SurfaceInteraction3f &si,
                                        const Vector3f &wo,
                                        Mask active) const override {
        MI_MASKED_FUNCTION(ProfilerPhase::BSDFEvaluate, active);

        bool has_diffuse = ctx.is_enabled(BSDFFlags::DiffuseReflection, 1);

        Float cos_theta_i = Frame3f::cos_theta(si.wi),
              cos_theta_o = Frame3f::cos_theta(wo);

        active &= cos_theta_i > 0.f && cos_theta_o > 0.f;

        if (unlikely(!has_diffuse || dr::none_or<false>(active)))
            return { 0.f, 0.f };

        UnpolarizedSpectrum eta_spec = eval_eta(si.wavelengths);
        Float eta_hero = has_dispersion() ? Float(eta_spec[0]) : Float(m_eta);

        Float f_i = std::get<0>(fresnel(cos_theta_i, eta_hero)),
              f_o = std::get<0>(fresnel(cos_theta_o, eta_hero));

        UnpolarizedSpectrum r_in_spec(f_i), r_out_spec(f_o);
        if (has_dispersion()) {
            auto [r_s_i, ct_i, eit_i, eti_i] =
                fresnel(UnpolarizedSpectrum(cos_theta_i), eta_spec);
            auto [r_s_o, ct_o, eit_o, eti_o] =
                fresnel(UnpolarizedSpectrum(cos_theta_o), eta_spec);
            (void) ct_i; (void) eit_i; (void) eti_i;
            (void) ct_o; (void) eit_o; (void) eti_o;
            r_in_spec  = r_s_i;
            r_out_spec = r_s_o;
        }
        if (has_thin_film()) {
            r_in_spec  = thin_film_reflectance(cos_theta_i, eta_spec, si.wavelengths);
            r_out_spec = thin_film_reflectance(cos_theta_o, eta_spec, si.wavelengths);
            f_i = Float(r_in_spec[0]);  // hero drives sampling probability
        }

        UnpolarizedSpectrum inv_eta_2_spec = has_dispersion()
            ? dr::rcp(dr::square(eta_spec))
            : UnpolarizedSpectrum(m_inv_eta_2);

        UnpolarizedSpectrum diff = m_diffuse_reflectance->eval(si, active);
        diff /= 1.f - (m_nonlinear ? (diff * m_fdr_int) : m_fdr_int);

        Float hemi_pdf = warp::square_to_cosine_hemisphere_pdf(wo);

        diff *= hemi_pdf * inv_eta_2_spec * (1.f - r_in_spec) * (1.f - r_out_spec);

        Float prob_diffuse = 1.f;
        if (ctx.is_enabled(BSDFFlags::DeltaReflection, 0)) {
            Float prob_specular = f_i * m_specular_sampling_weight;
            prob_diffuse  = (1.f - f_i) * (1.f - m_specular_sampling_weight);
            prob_diffuse = prob_diffuse / (prob_specular + prob_diffuse);
        }

        return { dr::select(active, depolarizer<Spectrum>(diff), 0.f),
                 dr::select(active, hemi_pdf * prob_diffuse, 0.f) };
    }

    Spectrum eval_diffuse_reflectance(const SurfaceInteraction3f &si,
                                      Mask active) const override {
        return m_diffuse_reflectance->eval(si, active);
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "SmoothPlastic[" << std::endl
            << "  diffuse_reflectance = "      << m_diffuse_reflectance      << "," << std::endl;
        if (m_specular_reflectance)
            oss << "  specular_reflectance = " << m_specular_reflectance     << "," << std::endl;

        oss << "  specular_sampling_weight = " << m_specular_sampling_weight << "," << std::endl
            << "  nonlinear = "                << (int) m_nonlinear          << "," << std::endl
            << "  eta = "                      << m_eta                      << "," << std::endl
            << "  fdr_int = "                  << m_fdr_int                  << "," << std::endl
            << "  fdr_ext = "                  << m_fdr_ext                  << "," << std::endl;
        if (m_cauchy_b != 0.f)
            oss << "  cauchy_b = " << m_cauchy_b << "," << std::endl;
        if (m_film_thickness > 0.f)
            oss << "  film_thickness = " << m_film_thickness << " nm," << std::endl
                << "  film_ior = "       << m_film_ior       << ","    << std::endl;
        oss << "]";
        return oss.str();
    }

    MI_DECLARE_CLASS(SmoothPlastic)
private:
    ref<Texture> m_diffuse_reflectance;
    ref<Texture> m_specular_reflectance;
    ScalarFloat m_eta;
    ScalarFloat m_inv_eta_2;
    ScalarFloat m_fdr_int;
    ScalarFloat m_fdr_ext;
    ScalarFloat m_cauchy_b;
    ScalarFloat m_film_thickness;
    ScalarFloat m_film_ior;
    Float m_specular_sampling_weight;
    bool m_nonlinear;

    MI_TRAVERSE_CB(Base, m_diffuse_reflectance, m_specular_reflectance,
                   m_specular_sampling_weight, m_cauchy_b,
                   m_film_thickness, m_film_ior)
};

MI_EXPORT_PLUGIN(SmoothPlastic)
NAMESPACE_END(mitsuba)
