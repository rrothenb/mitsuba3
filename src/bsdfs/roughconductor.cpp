#include <mitsuba/core/string.h>
#include <mitsuba/core/fwd.h>
#include <mitsuba/core/plugin.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/fresnel.h>
#include <mitsuba/render/ior.h>
#include <mitsuba/render/microfacet.h>
#include <mitsuba/render/texture.h>
#include <drjit/complex.h>

NAMESPACE_BEGIN(mitsuba)

/**!
.. _bsdf-roughconductor:

Rough conductor material (:monosp:`roughconductor`)
---------------------------------------------------

.. pluginparameters::

 * - material
   - |string|
   - Name of the material preset, see :num:`conductor-ior-list`. (Default: none)

 * - eta, k
   - |spectrum| or |texture|
   - Real and imaginary components of the material's index of refraction. (Default: based on the value of :monosp:`material`)
   - |exposed|, |differentiable|, |discontinuous|

 * - specular_reflectance
   - |spectrum| or |texture|
   - Optional factor that can be used to modulate the specular reflection component.
     Note that for physical realism, this parameter should never be touched. (Default: 1.0)
   - |exposed|, |differentiable|

 * - distribution
   - |string|
   - Specifies the type of microfacet normal distribution used to model the surface roughness.

     - :monosp:`beckmann`: Physically-based distribution derived from Gaussian random surfaces.
       This is the default.
     - :monosp:`ggx`: The GGX :cite:`Walter07Microfacet` distribution (also known as Trowbridge-Reitz
       :cite:`Trowbridge19975Average` distribution) was designed to better approximate the long
       tails observed in measurements of ground surfaces, which are not modeled by the Beckmann
       distribution.

 * - alpha, alpha_u, alpha_v
   - |texture| or |float|
   - Specifies the roughness of the unresolved surface micro-geometry along the tangent and
     bitangent directions. When the Beckmann distribution is used, this parameter is equal to the
     **root mean square** (RMS) slope of the microfacets. :monosp:`alpha` is a convenience
     parameter to initialize both :monosp:`alpha_u` and :monosp:`alpha_v` to the same value. (Default: 0.1)
   - |exposed|, |differentiable|, |discontinuous|

 * - sample_visible
   - |bool|
   - Enables a sampling technique proposed by Heitz and D'Eon :cite:`Heitz1014Importance`, which
     focuses computation on the visible parts of the microfacet normal distribution, considerably
     reducing variance in some cases. (Default: |true|, i.e. use visible normal sampling)

 * - film_thickness
   - |float|
   - Optional thickness of a non-absorbing dielectric film coating the metal
     microfacets, in nanometers. When nonzero in a spectral, non-polarized
     variant, each microfacet's reflectance is given by the Airy formula for
     an ambient → film → conductor stack — producing iridescent metals such
     as oil-on-chrome, anodized aluminium, AR-coated mirrors, and the
     structural color of beetle shells / peacock feathers (which are
     effectively rough conductive substrates with thin layers). Not yet
     supported in polarized variants. Ignored in RGB / monochromatic
     variants. (Default: 0, no film)

 * - film_ior
   - |float| or |string|
   - Refractive index of the thin film, specified numerically or via a
     known material name. Treated as absolute (the conductor BSDF assumes
     ambient IOR = 1). Only consulted when ``film_thickness > 0``.
     (Default: air / 1.000277)

This plugin implements a realistic microfacet scattering model for rendering
rough conducting materials, such as metals.

.. subfigstart::
.. subfigure:: ../../resources/data/docs/images/render/bsdf_roughconductor_copper.jpg
   :caption: Rough copper (Beckmann, :math:`\alpha=0.1`)
.. subfigure:: ../../resources/data/docs/images/render/bsdf_roughconductor_anisotropic_aluminium.jpg
   :caption: Vertically brushed aluminium (Anisotropic Beckmann, :math:`\alpha_u=0.05,\ \alpha_v=0.3`)
.. subfigure:: ../../resources/data/docs/images/render/bsdf_roughconductor_textured_carbon.jpg
   :caption: Carbon fiber using two inverted checkerboard textures for ``alpha_u`` and ``alpha_v``
.. subfigend::
    :label: fig-bsdf-roughconductor


Microfacet theory describes rough surfaces as an arrangement of unresolved
and ideally specular facets, whose normal directions are given by a
specially chosen *microfacet distribution*. By accounting for shadowing
and masking effects between these facets, it is possible to reproduce the
important off-specular reflections peaks observed in real-world measurements
of such materials.

This plugin is essentially the *roughened* equivalent of the (smooth) plugin
:ref:`conductor <bsdf-conductor>`. For very low values of :math:`\alpha`, the two will
be identical, though scenes using this plugin will take longer to render
due to the additional computational burden of tracking surface roughness.

The implementation is based on the paper *Microfacet Models
for Refraction through Rough Surfaces* by Walter et al.
:cite:`Walter07Microfacet` and it supports two different types of microfacet
distributions.

To facilitate the tedious task of specifying spectrally-varying index of
refraction information, this plugin can access a set of measured materials
for which visible-spectrum information was publicly available
(see the corresponding table in the :ref:`conductor <bsdf-conductor>` reference).

When no parameters are given, the plugin activates the default settings,
which describe a 100% reflective mirror with a medium amount of roughness modeled
using a Beckmann distribution.

To get an intuition about the effect of the surface roughness parameter
:math:`\alpha`, consider the following approximate classification: a value of
:math:`\alpha=0.001-0.01` corresponds to a material with slight imperfections
on an otherwise smooth surface finish, :math:`\alpha=0.1` is relatively rough,
and :math:`\alpha=0.3-0.7` is **extremely** rough (e.g. an etched or ground
finish). Values significantly above that are probably not too realistic.


The following XML snippet describes a material definition for brushed aluminium:

.. tabs::
    .. code-tab:: xml
        :name: lst-roughconductor-aluminium

        <bsdf type="roughconductor">
            <string name="material" value="Al"/>
            <string name="distribution" value="ggx"/>
            <float name="alpha_u" value="0.05"/>
            <float name="alpha_v" value="0.3"/>
        </bsdf>

    .. code-tab:: python

        'type': 'roughconductor',
        'material': 'Al',
        'distribution': 'ggx',
        'alpha_u': 0.05,
        'alpha_v': 0.3

Technical details
*****************

All microfacet distributions allow the specification of two distinct
roughness values along the tangent and bitangent directions. This can be
used to provide a material with a *brushed* appearance. The alignment
of the anisotropy will follow the UV parameterization of the underlying
mesh. This means that such an anisotropic material cannot be applied to
triangle meshes that are missing texture coordinates.

Since Mitsuba 0.5.1, this plugin uses a new importance sampling technique
contributed by Eric Heitz and Eugene D'Eon, which restricts the sampling
domain to the set of visible (unmasked) microfacet normals. The previous
approach of sampling all normals is still available and can be enabled
by setting :monosp:`sample_visible` to :monosp:`false`. However this will lead
to significantly slower convergence.

When using this plugin, you should ideally compile Mitsuba with support for
spectral rendering to get the most accurate results. While it also works
in RGB mode, the computations will be more approximate in nature.
Also note that this material is one-sided---that is, observed from the
back side, it will be completely black. If this is undesirable,
consider using the :ref:`twosided <bsdf-twosided>` BRDF adapter.

In *polarized* rendering modes, the material automatically switches to a polarized
implementation of the underlying Fresnel equations.

Thin-film interference (spectral variants only)
***********************************************

Setting ``film_thickness`` (in nanometers) coats each microfacet with a
single non-absorbing dielectric film of refractive index ``film_ior``.
The microfacet's reflectance becomes wavelength-dependent via the Airy
formula evaluated at its local incidence angle, with the conductor's
complex IOR as the substrate. This is the standard model for iridescent
metals (oil-on-chrome, anodized aluminium, AR-coated optics) and for
the structural color of beetle shells / peacock feathers — which are
effectively rough metallic substrates with thin biological films. See
:ref:`conductor <bsdf-conductor>` for the full Airy derivation and the
typical-thickness reference table.

 */

template <typename Float, typename Spectrum>
class RoughConductor final : public BSDF<Float, Spectrum> {
public:
    MI_IMPORT_BASE(BSDF, m_flags, m_components)
    MI_IMPORT_TYPES(Texture, MicrofacetDistribution)

    RoughConductor(const Properties &props) : Base(props) {
        std::string_view material = props.get<std::string_view>("material", "none");
        if (props.has_property("eta") || material == "none") {
            m_eta = props.get_unbounded_texture<Texture>("eta", 0.f);
            m_k   = props.get_unbounded_texture<Texture>("k",   1.f);
            if (material != "none")
                Throw("Should specify either (eta, k) or material, not both.");
        } else {
            std::tie(m_eta, m_k) = complex_ior_from_file<Spectrum, Texture>(props.get<std::string_view>("material", "Cu"));
        }

        if (props.has_property("distribution")) {
            std::string distr = string::to_lower(props.get<std::string_view>("distribution"));
            if (distr == "beckmann")
                m_type = MicrofacetType::Beckmann;
            else if (distr == "ggx")
                m_type = MicrofacetType::GGX;
            else
                Throw("Specified an invalid distribution \"%s\", must be "
                      "\"beckmann\" or \"ggx\"!", distr.c_str());
        } else {
            m_type = MicrofacetType::Beckmann;
        }

        m_sample_visible = props.get<bool>("sample_visible", true);

        if (props.has_property("alpha_u") || props.has_property("alpha_v")) {
            if (!props.has_property("alpha_u") || !props.has_property("alpha_v"))
                Throw("Microfacet model: both 'alpha_u' and 'alpha_v' must be specified.");
            if (props.has_property("alpha"))
                Throw("Microfacet model: please specify"
                      "either 'alpha' or 'alpha_u'/'alpha_v'.");
            m_alpha_u = props.get_unbounded_texture<Texture>("alpha_u");
            m_alpha_v = props.get_unbounded_texture<Texture>("alpha_v");
        } else {
            m_alpha_u = m_alpha_v = props.get_unbounded_texture<Texture>("alpha", 0.1f);
        }

        if (props.has_property("specular_reflectance"))
            m_specular_reflectance = props.get_texture<Texture>("specular_reflectance", 1.f);

        /* Optional thin-film interference (see SmoothConductor for the
           full write-up). The film coats each microfacet; reflectance is
           given by the Airy formula evaluated per wavelength at the
           microfacet's local incidence angle. The polarized Mueller path
           does not yet handle thin-film phase shifts; we throw rather
           than silently produce wrong polarization output. */
        ScalarFloat film_thickness = props.get<ScalarFloat>("film_thickness", 0.f);
        ScalarFloat film_ior_abs   = lookup_ior(props, "film_ior", "air");

        if (film_thickness < 0.f)
            Throw("'film_thickness' must be non-negative (got %f).", film_thickness);
        if (film_ior_abs <= 0.f)
            Throw("'film_ior' must be positive (got %f).", film_ior_abs);

        if constexpr (is_polarized_v<Spectrum>) {
            if (film_thickness > 0.f)
                Throw("'film_thickness' is not yet supported in polarized "
                      "variants of roughconductor.");
        }

        m_film_thickness = film_thickness;
        m_film_ior       = film_ior_abs;  // ambient n_a = 1, already relative

        m_flags = BSDFFlags::GlossyReflection | BSDFFlags::FrontSide;
        if (m_alpha_u != m_alpha_v)
            m_flags = m_flags | BSDFFlags::Anisotropic;

        m_components.clear();
        m_components.push_back(m_flags);
    }

    void traverse(TraversalCallback *cb) override {
        if (m_specular_reflectance)
            cb->put("specular_reflectance", m_specular_reflectance, ParamFlags::Differentiable);

        if (!has_flag(m_flags, BSDFFlags::Anisotropic)) {
            cb->put("alpha",   m_alpha_u, ParamFlags::Differentiable | ParamFlags::Discontinuous);
        } else {
            cb->put("alpha_u", m_alpha_u, ParamFlags::Differentiable | ParamFlags::Discontinuous);
            cb->put("alpha_v", m_alpha_v, ParamFlags::Differentiable | ParamFlags::Discontinuous);
        }

        cb->put("eta", m_eta, ParamFlags::Differentiable | ParamFlags::Discontinuous);
        cb->put("k",   m_k,   ParamFlags::Differentiable | ParamFlags::Discontinuous);
    }

    bool has_thin_film() const {
        return is_spectral_v<Spectrum> && !is_polarized_v<Spectrum> &&
               m_film_thickness > 0.f;
    }

    /// Per-microfacet thin-film intensity reflectance for a non-absorbing
    /// dielectric film over a conductor. Bottom (film → conductor) Fresnel
    /// amplitudes are complex; the Airy combine is done in complex
    /// arithmetic per polarization. See SmoothConductor for the derivation.
    UnpolarizedSpectrum thin_film_reflectance(
            Float cos_h,
            const dr::Complex<UnpolarizedSpectrum> &eta_substrate,
            const Wavelength &wavelengths) const {
        if constexpr (is_spectral_v<Spectrum> && !is_polarized_v<Spectrum>) {
            using CSpec = dr::Complex<UnpolarizedSpectrum>;

            // Mitsuba's internal Fresnel convention has Im(eta) <= 0; the
            // BSDF stores k as positive, so conjugate to match.
            CSpec eta = dr::conj(eta_substrate);

            Float cos_a = dr::abs(cos_h);
            Float sin2_a = 1.f - dr::square(cos_a);

            // Snell into the (real) film.
            ScalarFloat inv_nf_sq = 1.f / (m_film_ior * m_film_ior);
            Float sin2_f = sin2_a * inv_nf_sq;
            Float cos_f  = dr::safe_sqrt(1.f - sin2_f);

            // Snell into the (complex) conductor.
            CSpec sin2_c = CSpec(UnpolarizedSpectrum(sin2_a),
                                 UnpolarizedSpectrum(0.f)) / (eta * eta);
            CSpec cos_c = dr::sqrt(CSpec(UnpolarizedSpectrum(1.f),
                                         UnpolarizedSpectrum(0.f)) - sin2_c);

            // Top interface (ambient → film), real amplitudes.
            Float r01_s = (cos_a - m_film_ior * cos_f) /
                          (cos_a + m_film_ior * cos_f);
            Float r01_p = (m_film_ior * cos_a - cos_f) /
                          (m_film_ior * cos_a + cos_f);

            // Bottom interface (film → conductor), complex amplitudes.
            UnpolarizedSpectrum nf_cf(m_film_ior * cos_f);
            CSpec eta_cos_c = eta * cos_c;
            CSpec eta_cos_f = eta * UnpolarizedSpectrum(cos_f);
            CSpec nf_cos_c  = cos_c * m_film_ior;

            CSpec r12_s = (CSpec(nf_cf, UnpolarizedSpectrum(0.f)) - eta_cos_c) /
                          (CSpec(nf_cf, UnpolarizedSpectrum(0.f)) + eta_cos_c);
            CSpec r12_p = (eta_cos_f - nf_cos_c) /
                          (eta_cos_f + nf_cos_c);

            UnpolarizedSpectrum phi =
                (4.f * dr::Pi<ScalarFloat>) * m_film_ior * m_film_thickness *
                cos_f * dr::rcp(wavelengths);
            CSpec e_iphi(dr::cos(phi), dr::sin(phi));

            auto airy = [&](Float r01, const CSpec &r12) {
                CSpec r12_e = r12 * e_iphi;
                CSpec num   = CSpec(UnpolarizedSpectrum(r01),
                                    UnpolarizedSpectrum(0.f)) + r12_e;
                CSpec denom = CSpec(UnpolarizedSpectrum(1.f),
                                    UnpolarizedSpectrum(0.f)) +
                              r12_e * r01;
                return dr::squared_norm(num / denom);
            };

            return 0.5f * (airy(r01_s, r12_s) + airy(r01_p, r12_p));
        } else {
            DRJIT_MARK_USED(cos_h);
            DRJIT_MARK_USED(eta_substrate);
            DRJIT_MARK_USED(wavelengths);
            return UnpolarizedSpectrum(0.f);
        }
    }

    std::pair<BSDFSample3f, Spectrum> sample(const BSDFContext &ctx,
                                             const SurfaceInteraction3f &si,
                                             Float /* sample1 */,
                                             const Point2f &sample2,
                                             Mask active) const override {
        MI_MASKED_FUNCTION(ProfilerPhase::BSDFSample, active);

        BSDFSample3f bs = dr::zeros<BSDFSample3f>();
        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        active &= cos_theta_i > 0.f;

        if (unlikely(!ctx.is_enabled(BSDFFlags::GlossyReflection) || dr::none_or<false>(active)))
            return { bs, 0.f };

        /* Construct a microfacet distribution matching the
           roughness values at the current surface position. */
        MicrofacetDistribution distr(m_type,
                                     m_alpha_u->eval_1(si, active),
                                     m_alpha_v->eval_1(si, active),
                                     m_sample_visible);

        // Sample M, the microfacet normal
        Normal3f m;
        std::tie(m, bs.pdf) = distr.sample(si.wi, sample2);

        // Perfect specular reflection based on the microfacet normal
        bs.wo = reflect(si.wi, m);
        bs.eta = 1.f;
        bs.sampled_component = 0;
        bs.sampled_type = +BSDFFlags::GlossyReflection;

        // Ensure that this is a valid sample
        active &= (bs.pdf != 0.f) && Frame3f::cos_theta(bs.wo) > 0.f;

        UnpolarizedSpectrum weight;
        if (likely(m_sample_visible))
            weight = distr.smith_g1(bs.wo, m);
        else
            weight = distr.G(si.wi, bs.wo, m) * dr::dot(si.wi, m) /
                     (cos_theta_i * Frame3f::cos_theta(m));

        // Jacobian of the half-direction mapping
        bs.pdf /= 4.f * dr::dot(bs.wo, m);

        // Evaluate the Fresnel factor
        dr::Complex<UnpolarizedSpectrum> eta_c(m_eta->eval(si, active),
                                           m_k->eval(si, active));

        Spectrum F;
        if constexpr (is_polarized_v<Spectrum>) {
            /* Due to the coordinate system rotations for polarization-aware
               pBSDFs below we need to know the propagation direction of light.
               In the following, light arrives along `-wo_hat` and leaves along
               `+wi_hat`. */
            Vector3f wo_hat = ctx.mode == TransportMode::Radiance ? bs.wo : si.wi,
                     wi_hat = ctx.mode == TransportMode::Radiance ? si.wi : bs.wo;

            // Mueller matrix for specular reflection.
            F = mueller::specular_reflection(UnpolarizedSpectrum(dot(wo_hat, m)), eta_c);

            /* The Stokes reference frame vector of this matrix lies perpendicular
               to the plane of reflection. */
            Vector3f s_axis_in  = dr::cross(m, -wo_hat);
            Vector3f s_axis_out = dr::cross(m, wi_hat);

            // Singularity when the input & output are collinear with the normal
            Mask collinear = dr::all(s_axis_in ==  Vector3f(0));
            s_axis_in  = dr::select(collinear, Vector3f(1, 0, 0),
                                               dr::normalize(s_axis_in));
            s_axis_out = dr::select(collinear, Vector3f(1, 0, 0),
                                               dr::normalize(s_axis_out));

            /* Rotate in/out reference vector of F s.t. it aligns with the implicit
               Stokes bases of -wo_hat & wi_hat. */
            F = mueller::rotate_mueller_basis(F,
                                              -wo_hat, s_axis_in, mueller::stokes_basis(-wo_hat),
                                               wi_hat, s_axis_out, mueller::stokes_basis(wi_hat));
        } else {
            if (has_thin_film())
                F = thin_film_reflectance(dr::dot(si.wi, m), eta_c, si.wavelengths);
            else
                F = fresnel_conductor(UnpolarizedSpectrum(dr::dot(si.wi, m)), eta_c);
        }

        /* If requested, include the specular reflectance component */
        if (m_specular_reflectance)
            weight *= m_specular_reflectance->eval(si, active);

        return { bs, (F * weight) & active };
    }

    Spectrum eval(const BSDFContext &ctx, const SurfaceInteraction3f &si,
                  const Vector3f &wo, Mask active) const override {
        MI_MASKED_FUNCTION(ProfilerPhase::BSDFEvaluate, active);

        Float cos_theta_i = Frame3f::cos_theta(si.wi),
              cos_theta_o = Frame3f::cos_theta(wo);

        active &= cos_theta_i > 0.f && cos_theta_o > 0.f;

        if (unlikely(!ctx.is_enabled(BSDFFlags::GlossyReflection) || dr::none_or<false>(active)))
            return 0.f;

        // Calculate the half-direction vector
        Vector3f H = dr::normalize(wo + si.wi);

        /* Construct a microfacet distribution matching the
           roughness values at the current surface position. */
        MicrofacetDistribution distr(m_type,
                                     m_alpha_u->eval_1(si, active),
                                     m_alpha_v->eval_1(si, active),
                                     m_sample_visible);

        // Evaluate the microfacet normal distribution
        Float D = distr.eval(H);

        active &= D != 0.f;

        // Evaluate Smith's shadow-masking function
        Float G = distr.G(si.wi, wo, H);

        // Evaluate the full microfacet model (except Fresnel)
        UnpolarizedSpectrum result = D * G / (4.f * Frame3f::cos_theta(si.wi));

        // Evaluate the Fresnel factor
        dr::Complex<UnpolarizedSpectrum> eta_c(m_eta->eval(si, active),
                                           m_k->eval(si, active));

        Spectrum F;
        if constexpr (is_polarized_v<Spectrum>) {
            /* Due to the coordinate system rotations for polarization-aware
               pBSDFs below we need to know the propagation direction of light.
               In the following, light arrives along `-wo_hat` and leaves along
               `+wi_hat`. */
            Vector3f wo_hat = ctx.mode == TransportMode::Radiance ? wo : si.wi,
                     wi_hat = ctx.mode == TransportMode::Radiance ? si.wi : wo;

            // Mueller matrix for specular reflection.
            F = mueller::specular_reflection(UnpolarizedSpectrum(dot(wo_hat, H)), eta_c);

            /* The Stokes reference frame vector of this matrix lies perpendicular
               to the plane of reflection. */
            Vector3f s_axis_in  = dr::cross(H, -wo_hat);
            Vector3f s_axis_out = dr::cross(H, wi_hat);

            // Singularity when the input & output are collinear with the normal
            Mask collinear = dr::all(s_axis_in == Vector3f(0));
            s_axis_in  = dr::select(collinear, Vector3f(1, 0, 0),
                                               dr::normalize(s_axis_in));
            s_axis_out = dr::select(collinear, Vector3f(1, 0, 0),
                                               dr::normalize(s_axis_out));

            /* Rotate in/out reference vector of F s.t. it aligns with the implicit
               Stokes bases of -wo_hat & wi_hat. */
            F = mueller::rotate_mueller_basis(F,
                                              -wo_hat, s_axis_in, mueller::stokes_basis(-wo_hat),
                                               wi_hat, s_axis_out, mueller::stokes_basis(wi_hat));
        } else {
            if (has_thin_film())
                F = thin_film_reflectance(dr::dot(si.wi, H), eta_c, si.wavelengths);
            else
                F = fresnel_conductor(UnpolarizedSpectrum(dr::dot(si.wi, H)), eta_c);
        }

        /* If requested, include the specular reflectance component */
        if (m_specular_reflectance)
            result *= m_specular_reflectance->eval(si, active);

        return (F * result) & active;
    }

    Float pdf(const BSDFContext &ctx, const SurfaceInteraction3f &si,
              const Vector3f &wo, Mask active) const override {
        MI_MASKED_FUNCTION(ProfilerPhase::BSDFEvaluate, active);

        Float cos_theta_i = Frame3f::cos_theta(si.wi),
              cos_theta_o = Frame3f::cos_theta(wo);

        // Calculate the half-direction vector
        Vector3f m = dr::normalize(wo + si.wi);

        /* Filter cases where the micro/macro-surface don't agree on the side.
           This logic is evaluated in smith_g1() called as part of the eval()
           and sample() methods and needs to be replicated in the probability
           density computation as well. */
        active &= cos_theta_i > 0.f && cos_theta_o > 0.f &&
                  dr::dot(si.wi, m) > 0.f && dr::dot(wo, m) > 0.f;

        if (unlikely(!ctx.is_enabled(BSDFFlags::GlossyReflection) || dr::none_or<false>(active)))
            return 0.f;

        /* Construct a microfacet distribution matching the
           roughness values at the current surface position. */
        MicrofacetDistribution distr(m_type,
                                     m_alpha_u->eval_1(si, active),
                                     m_alpha_v->eval_1(si, active),
                                     m_sample_visible);

        Float result;
        if (likely(m_sample_visible))
            result = distr.eval(m) * distr.smith_g1(si.wi, m) /
                     (4.f * cos_theta_i);
        else
            result = distr.pdf(si.wi, m) / (4.f * dr::dot(wo, m));

        return dr::select(active, result, 0.f);
    }

    std::pair<Spectrum, Float> eval_pdf(const BSDFContext &ctx,
                                        const SurfaceInteraction3f &si,
                                        const Vector3f &wo,
                                        Mask active) const override {
        MI_MASKED_FUNCTION(ProfilerPhase::BSDFEvaluate, active);

        Float cos_theta_i = Frame3f::cos_theta(si.wi),
              cos_theta_o = Frame3f::cos_theta(wo);

        // Calculate the half-direction vector
        Vector3f H = dr::normalize(wo + si.wi);

        /* Filter cases where the micro/macro-surface don't agree on the side.
           This logic is evaluated in smith_g1() called as part of the eval()
           and sample() methods and needs to be replicated in the probability
           density computation as well. */
        active &= cos_theta_i > 0.f && cos_theta_o > 0.f &&
                  dr::dot(si.wi, H) > 0.f && dr::dot(wo, H) > 0.f;

        if (unlikely(!ctx.is_enabled(BSDFFlags::GlossyReflection) || dr::none_or<false>(active)))
            return { 0.f, 0.f };

        /* Construct a microfacet distribution matching the
           roughness values at the current surface position. */
        MicrofacetDistribution distr(m_type,
                                     m_alpha_u->eval_1(si, active),
                                     m_alpha_v->eval_1(si, active),
                                     m_sample_visible);

        // Evaluate the microfacet normal distribution
        Float D = distr.eval(H);

        active &= D != 0.f;

        // Evaluate Smith's shadow-masking function
        Float smith_g1_wi = distr.smith_g1(si.wi, H);
        Float G = smith_g1_wi * distr.smith_g1(wo, H);

        // Evaluate the full microfacet model (except Fresnel)
        UnpolarizedSpectrum value = D * G / (4.f * Frame3f::cos_theta(si.wi));

        // Evaluate the Fresnel factor
        dr::Complex<UnpolarizedSpectrum> eta_c(m_eta->eval(si, active),
                                           m_k->eval(si, active));

        Spectrum F;
        if constexpr (is_polarized_v<Spectrum>) {
            /* Due to the coordinate system rotations for polarization-aware
               pBSDFs below we need to know the propagation direction of light.
               In the following, light arrives along `-wo_hat` and leaves along
               `+wi_hat`. */
            Vector3f wo_hat = ctx.mode == TransportMode::Radiance ? wo : si.wi,
                     wi_hat = ctx.mode == TransportMode::Radiance ? si.wi : wo;

            // Mueller matrix for specular reflection.
            F = mueller::specular_reflection(UnpolarizedSpectrum(dot(wo_hat, H)), eta_c);

            /* The Stokes reference frame vector of this matrix lies perpendicular
               to the plane of reflection. */
            Vector3f s_axis_in  = dr::cross(H, -wo_hat);
            Vector3f s_axis_out = dr::cross(H, wi_hat);

            // Singularity when the input & output are collinear with the normal
            Mask collinear = dr::all(s_axis_in == Vector3f(0));
            s_axis_in  = dr::select(collinear, Vector3f(1, 0, 0),
                                               dr::normalize(s_axis_in));
            s_axis_out = dr::select(collinear, Vector3f(1, 0, 0),
                                               dr::normalize(s_axis_out));

            /* Rotate in/out reference vector of F s.t. it aligns with the implicit
               Stokes bases of -wo_hat & wi_hat. */
            F = mueller::rotate_mueller_basis(F,
                                              -wo_hat, s_axis_in, mueller::stokes_basis(-wo_hat),
                                               wi_hat, s_axis_out, mueller::stokes_basis(wi_hat));
        } else {
            if (has_thin_film())
                F = thin_film_reflectance(dr::dot(si.wi, H), eta_c, si.wavelengths);
            else
                F = fresnel_conductor(UnpolarizedSpectrum(dr::dot(si.wi, H)), eta_c);
        }

        // If requested, include the specular reflectance component
        if (m_specular_reflectance)
            value *= m_specular_reflectance->eval(si, active);

        Float pdf;
        if (likely(m_sample_visible))
            pdf = D * smith_g1_wi / (4.f * cos_theta_i);
        else
            pdf = distr.pdf(si.wi, H) / (4.f * dr::dot(wo, H));

        return { F * value & active, dr::select(active, pdf, 0.f) };
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "RoughConductor[" << std::endl
            << "  distribution = " << m_type << "," << std::endl
            << "  sample_visible = " << m_sample_visible << "," << std::endl
            << "  alpha_u = " << string::indent(m_alpha_u) << "," << std::endl
            << "  alpha_v = " << string::indent(m_alpha_v) << "," << std::endl;
        if (m_specular_reflectance)
           oss << "  specular_reflectance = " << string::indent(m_specular_reflectance) << "," << std::endl;
        oss << "  eta = " << string::indent(m_eta) << "," << std::endl
            << "  k = " << string::indent(m_k);
        if (m_film_thickness > 0.f)
            oss << "," << std::endl
                << "  film_thickness = " << m_film_thickness << " nm," << std::endl
                << "  film_ior = "       << m_film_ior;
        oss << std::endl << "]";
        return oss.str();
    }

    MI_DECLARE_CLASS(RoughConductor)
private:
    /// Specifies the type of microfacet distribution
    MicrofacetType m_type;
    /// Anisotropic roughness values
    ref<Texture> m_alpha_u, m_alpha_v;
    /// Importance sample the distribution of visible normals?
    bool m_sample_visible;
    /// Relative refractive index (real component)
    ref<Texture> m_eta;
    /// Relative refractive index (imaginary component).
    ref<Texture> m_k;
    /// Specular reflectance component
    ref<Texture> m_specular_reflectance;
    /// Thin-film thickness (nm) and IOR
    ScalarFloat m_film_thickness;
    ScalarFloat m_film_ior;

    MI_TRAVERSE_CB(Base, m_alpha_u, m_alpha_v, m_eta, m_k,
                   m_specular_reflectance, m_film_thickness, m_film_ior)
};

MI_EXPORT_PLUGIN(RoughConductor)
NAMESPACE_END(mitsuba)
