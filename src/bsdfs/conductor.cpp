#include <mitsuba/core/properties.h>
#include <mitsuba/core/warp.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/fresnel.h>
#include <mitsuba/render/ior.h>
#include <mitsuba/render/texture.h>
#include <drjit/complex.h>

NAMESPACE_BEGIN(mitsuba)

/**!

.. _bsdf-conductor:

Smooth conductor (:monosp:`conductor`)
-------------------------------------------

.. pluginparameters::

 * - material
   - |string|
   - Name of the material preset, see :num:`conductor-ior-list`. (Default: none)

 * - eta, k
   - |spectrum| or |texture|
   - Real and imaginary components of the material's index of refraction. (Default: based on the value of :paramtype:`material`)
   - |exposed|, |differentiable|, |discontinuous|

 * - specular_reflectance
   - |spectrum| or |texture|
   - Optional factor that can be used to modulate the specular reflection component.
     Note that for physical realism, this parameter should never be touched. (Default: 1.0)
   - |exposed|, |differentiable|

 * - film_thickness
   - |float|
   - Optional thickness of a non-absorbing dielectric film coating the metal,
     in nanometers. When nonzero in a spectral, non-polarized variant, the
     reflectance becomes wavelength-dependent via the Airy formula for an
     ambient → film → conductor stack — the standard model for oil films on
     metal, anti-reflection coatings on telescope mirrors, anodized aluminium,
     and the structural color of beetle shells / peacock feathers. Not yet
     supported in polarized variants — instantiation throws if both are
     requested. Ignored in RGB / monochromatic variants.
     (Default: 0, no film)

 * - film_ior
   - |float| or |string|
   - Refractive index of the thin film, specified numerically or via a known
     material name. Treated as absolute (the conductor BSDF assumes ambient
     IOR = 1). Only consulted when ``film_thickness > 0``.
     (Default: air / 1.000277)

.. subfigstart::
.. subfigure:: ../../resources/data/docs/images/render/bsdf_conductor_gold.jpg
   :caption: Gold
.. subfigure:: ../../resources/data/docs/images/render/bsdf_conductor_aluminium.jpg
   :caption: Aluminium
.. subfigend::
    :label: fig-bsdf-conductor

This plugin implements a perfectly smooth interface to a conducting material,
such as a metal that is described using a Dirac delta distribution. For a
similar model that instead describes a rough surface microstructure, take a look
at the separately available :ref:`roughconductor <bsdf-roughconductor>` plugin.
In contrast to dielectric materials, conductors do not transmit
any light. Their index of refraction is complex-valued and tends to undergo
considerable changes throughout the visible color spectrum.

When using this plugin, you should ideally enable one of the :monosp:`spectral`
modes of the renderer to get the most accurate results. While it also works
in RGB mode, the computations will be more approximate in nature.
Also note that this material is one-sided---that is, observed from the
back side, it will be completely black. If this is undesirable,
consider using the :ref:`twosided <bsdf-twosided>` BRDF adapter plugin.

The following XML snippet describes a material definition for gold:

.. tabs::
    .. code-tab:: xml
        :name: lst-conductor-gold

        <bsdf type="conductor">
            <string name="material" value="Au"/>
        </bsdf>

    .. code-tab:: python

        'type': 'conductor',
        'material': 'Au'

It is also possible to load spectrally varying index of refraction data from
two external files containing the real and imaginary components,
respectively (see :ref:`Scene format <sec-file-format>` for details on the file format):

.. tabs::
    .. code-tab:: xml
        :name: lst-conductor-files

        <bsdf type="conductor">
            <spectrum name="eta" filename="conductorIOR.eta.spd"/>
            <spectrum name="k" filename="conductorIOR.k.spd"/>
        </bsdf>


    .. code-tab:: python

        'type': 'conductor',
        'eta':  {
            'type': 'spectrum',
            'filename': 'conductorIOR.eta.spd'
        },
        'k':  {
            'type': 'spectrum',
            'filename': 'conductorIOR.k.spd'
        }

In *polarized* rendering modes, the material automatically switches to a polarized
implementation of the underlying Fresnel equations.

Thin-film interference (spectral variants only)
***********************************************

Setting ``film_thickness`` (in nanometers) coats the metal surface with a
single non-absorbing dielectric film of refractive index ``film_ior``. In a
spectral variant the conductor's reflectance becomes wavelength-dependent
via the Airy formula for an ambient → film → substrate stack, where the
substrate has the conductor's complex IOR. This produces the iridescent
sheen of oil films on metal, anti-reflection coatings on telescope mirrors,
anodized aluminium, and the structural color of beetle shells and peacock
feathers. See :ref:`dielectric <bsdf-dielectric>` for the full Airy
derivation and the typical-thickness reference table. Polarized variants
are not yet supported — instantiation throws if both are requested.

To facilitate the tedious task of specifying spectrally-varying index of
refraction information, Mitsuba 3 ships with a set of measured data for several
materials, where visible-spectrum information was publicly
available:

.. figtable::
    :label: conductor-ior-list
    :caption: This table lists all supported materials that can be passed into the
       :ref:`conductor <bsdf-conductor>` and :ref:`roughconductor <bsdf-roughconductor>`
       plugins. Note that some of them are not actually conductors---this is not a
       problem, they can be used regardless (though only the reflection component
       and no transmission will be simulated). In most cases, there are multiple
       entries for each material, which represent measurements by different
       authors.
    :alt: List table

    .. list-table::
        :widths: 15 30 15 30
        :header-rows: 1

        * - Preset(s)
          - Description
          - Preset(s)
          - Description
        * - :paramtype:`a-C`
          - Amorphous carbon
          - :paramtype:`Na_palik`
          - Sodium
        * - :paramtype:`Ag`
          - Silver
          - :paramtype:`Nb`, :paramtype:`Nb_palik`
          - Niobium
        * - :paramtype:`Al`
          - Aluminium
          - :paramtype:`Ni_palik`
          - Nickel
        * - :paramtype:`AlAs`, :paramtype:`AlAs_palik`
          - Cubic aluminium arsenide
          - :paramtype:`Rh`, :paramtype:`Rh_palik`
          - Rhodium
        * - :paramtype:`AlSb`, :paramtype:`AlSb_palik`
          - Cubic aluminium antimonide
          - :paramtype:`Se`, :paramtype:`Se_palik`
          - Selenium
        * - :paramtype:`Au`
          - Gold
          - :paramtype:`SiC`, :paramtype:`SiC_palik`
          - Hexagonal silicon carbide
        * - :paramtype:`Be`, :paramtype:`Be_palik`
          - Polycrystalline beryllium
          - :paramtype:`SnTe`, :paramtype:`SnTe_palik`
          - Tin telluride
        * - :paramtype:`Cr`
          - Chromium
          - :paramtype:`Ta`, :paramtype:`Ta_palik`
          - Tantalum
        * - :paramtype:`CsI`, :paramtype:`CsI_palik`
          - Cubic caesium iodide
          - :paramtype:`Te`, :paramtype:`Te_palik`
          - Trigonal tellurium
        * - :paramtype:`Cu`, :paramtype:`Cu_palik`
          - Copper
          - :paramtype:`ThF4`, :paramtype:`ThF4_palik`
          - Polycryst. thorium (IV) fluoride
        * - :paramtype:`Cu2O`, :paramtype:`Cu2O_palik`
          - Copper (I) oxide
          - :paramtype:`TiC`, :paramtype:`TiC_palik`
          - Polycrystalline titanium carbide
        * - :paramtype:`CuO`, :paramtype:`CuO_palik`
          - Copper (II) oxide
          - :paramtype:`TiN`, :paramtype:`TiN_palik`
          - Titanium nitride
        * - :paramtype:`d-C`, :paramtype:`d-C_palik`
          - Cubic diamond
          - :paramtype:`TiO2`, :paramtype:`TiO2_palik`
          - Tetragonal titan. dioxide
        * - :paramtype:`Hg`, :paramtype:`Hg_palik`
          - Mercury
          - :paramtype:`VC`, :paramtype:`VC_palik`
          - Vanadium carbide
        * - :paramtype:`HgTe`, :paramtype:`HgTe_palik`
          - Mercury telluride
          - :paramtype:`V_palik`
          - Vanadium
        * - :paramtype:`Ir`, :paramtype:`Ir_palik`
          - Iridium
          - :paramtype:`VN`, :paramtype:`VN_palik`
          - Vanadium nitride
        * - :paramtype:`K`, :paramtype:`K_palik`
          - Polycrystalline potassium
          - :paramtype:`W`
          - Tungsten
        * - :paramtype:`Li`, :paramtype:`Li_palik`
          - Lithium
          -
          -
        * - :paramtype:`MgO`, :paramtype:`MgO_palik`
          - Magnesium oxide
          -
          -
        * - :paramtype:`Mo`, :paramtype:`Mo_palik`
          - Molybdenum
          - :paramtype:`none`
          - No mat. profile (100% reflecting mirror)

These index of refraction values are identical to the data distributed
with PBRT. They are originally from the `Luxpop database <http://www.luxpop.com>`_
and are based on data by Palik et al. :cite:`Palik1998Handbook` and measurements
of atomic scattering factors made by the Center For X-Ray Optics (CXRO) at
Berkeley and the Lawrence Livermore National Laboratory (LLNL).

There is also a special material profile named :paramtype:`none`, which disables
the computation of Fresnel reflectances and produces an idealized
100% reflecting mirror.

 */

template <typename Float, typename Spectrum>
class SmoothConductor final : public BSDF<Float, Spectrum> {
public:
    MI_IMPORT_BASE(BSDF, m_flags, m_components)
    MI_IMPORT_TYPES(Texture)

    SmoothConductor(const Properties &props) : Base(props) {
        m_flags = BSDFFlags::DeltaReflection | BSDFFlags::FrontSide;
        m_components.push_back(m_flags);

        m_specular_reflectance = props.get_texture<Texture>("specular_reflectance", 1.f);

        std::string_view material = props.get<std::string_view>("material", "none");
        if (props.has_property("eta") || material == "none") {
            m_eta = props.get_unbounded_texture<Texture>("eta", 0.f);
            m_k   = props.get_unbounded_texture<Texture>("k",   1.f);
            if (material != "none")
                Throw("Should specify either (eta, k) or material, not both.");
        } else {
            std::tie(m_eta, m_k) = complex_ior_from_file<Spectrum, Texture>(props.get<std::string_view>("material", "Cu"));
        }

        /* Optional thin-film interference (see SmoothDielectric for the full
           write-up). Film is a non-absorbing dielectric; substrate is the
           conductor with complex IOR (m_eta + i·m_k). film_ior is parsed
           like in the dielectric BSDF, but here ambient is implicitly vacuum
           (n_a = 1) since conductor has no ext_ior parameter. The polarized
           Mueller path doesn't yet handle thin-film phase shifts; we throw
           rather than silently produce wrong polarization output. */
        ScalarFloat film_thickness = props.get<ScalarFloat>("film_thickness", 0.f);
        ScalarFloat film_ior_abs   = lookup_ior(props, "film_ior", "air");

        if (film_thickness < 0.f)
            Throw("'film_thickness' must be non-negative (got %f).", film_thickness);
        if (film_ior_abs <= 0.f)
            Throw("'film_ior' must be positive (got %f).", film_ior_abs);

        if constexpr (is_polarized_v<Spectrum>) {
            if (film_thickness > 0.f)
                Throw("'film_thickness' is not yet supported in polarized "
                      "variants of conductor. Use a non-polarized spectral "
                      "variant for thin-film effects.");
        }

        m_film_thickness = film_thickness;
        m_film_ior       = film_ior_abs;  // ambient n_a = 1, so already relative
    }

    bool has_thin_film() const {
        return is_spectral_v<Spectrum> && !is_polarized_v<Spectrum> &&
               m_film_thickness > 0.f;
    }

    /// Intensity reflectance per wavelength for a non-absorbing dielectric
    /// film coating a conductor. The bottom (film → conductor) Fresnel
    /// amplitudes are complex, so the Airy combine is done in complex
    /// arithmetic per polarization.
    UnpolarizedSpectrum thin_film_reflectance(
            Float cos_theta_i,
            const dr::Complex<UnpolarizedSpectrum> &eta_substrate,
            const Wavelength &wavelengths) const {
        if constexpr (is_spectral_v<Spectrum> && !is_polarized_v<Spectrum>) {
            using CSpec = dr::Complex<UnpolarizedSpectrum>;

            // Mitsuba's internal Fresnel convention has Im(eta) <= 0 (see
            // fresnel_polarized in include/mitsuba/render/fresnel.h). The
            // conductor BSDF stores k as positive, so we conjugate here to
            // match the convention used by the rest of the renderer.
            CSpec eta = dr::conj(eta_substrate);

            Float cos_a = dr::abs(cos_theta_i);
            Float sin2_a = 1.f - dr::square(cos_a);

            // Snell into the (real) film.
            ScalarFloat inv_nf_sq = 1.f / (m_film_ior * m_film_ior);
            Float sin2_f = sin2_a * inv_nf_sq;
            Float cos_f  = dr::safe_sqrt(1.f - sin2_f);

            // Snell into the (complex) conductor. sin²(θ_c) = sin²(θ_a) / n_c²
            // (n_a = 1 by convention).
            CSpec sin2_c = CSpec(UnpolarizedSpectrum(sin2_a),
                                 UnpolarizedSpectrum(0.f)) / (eta * eta);
            CSpec cos_c = dr::sqrt(CSpec(UnpolarizedSpectrum(1.f),
                                         UnpolarizedSpectrum(0.f)) - sin2_c);

            // Top interface (ambient → film): real Fresnel amplitudes.
            Float r01_s = (cos_a - m_film_ior * cos_f) /
                          (cos_a + m_film_ior * cos_f);
            Float r01_p = (m_film_ior * cos_a - cos_f) /
                          (m_film_ior * cos_a + cos_f);

            // Bottom interface (film → conductor): complex amplitudes.
            UnpolarizedSpectrum nf_cf(m_film_ior * cos_f);  // real promoted
            CSpec eta_cos_c = eta * cos_c;
            CSpec eta_cos_f = eta * UnpolarizedSpectrum(cos_f);
            CSpec nf_cos_c  = cos_c * m_film_ior;  // scalar * complex spectrum

            CSpec r12_s = (CSpec(nf_cf, UnpolarizedSpectrum(0.f)) - eta_cos_c) /
                          (CSpec(nf_cf, UnpolarizedSpectrum(0.f)) + eta_cos_c);
            CSpec r12_p = (eta_cos_f - nf_cos_c) /
                          (eta_cos_f + nf_cos_c);

            // Phase accumulated per round-trip through the (non-absorbing) film.
            UnpolarizedSpectrum phi =
                (4.f * dr::Pi<ScalarFloat>) * m_film_ior * m_film_thickness *
                cos_f * dr::rcp(wavelengths);
            CSpec e_iphi(dr::cos(phi), dr::sin(phi));

            // Airy: r = (r01 + r12·e^iφ) / (1 + r01·r12·e^iφ).  R = |r|².
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
            DRJIT_MARK_USED(cos_theta_i);
            DRJIT_MARK_USED(eta_substrate);
            DRJIT_MARK_USED(wavelengths);
            return UnpolarizedSpectrum(0.f);
        }
    }

    void traverse(TraversalCallback *cb) override {
        cb->put("eta",                  m_eta,                  ParamFlags::Differentiable | ParamFlags::Discontinuous);
        cb->put("k",                    m_k,                    ParamFlags::Differentiable | ParamFlags::Discontinuous);
        cb->put("specular_reflectance", m_specular_reflectance, ParamFlags::Differentiable);
    }

    std::pair<BSDFSample3f, Spectrum> sample(const BSDFContext &ctx,
                                             const SurfaceInteraction3f &si,
                                             Float /* sample1 */,
                                             const Point2f &/* sample2 */,
                                             Mask active) const override {
        MI_MASKED_FUNCTION(ProfilerPhase::BSDFSample, active);

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        active &= cos_theta_i > 0.f;

        BSDFSample3f bs = dr::zeros<BSDFSample3f>();
        Spectrum value(0.f);
        if (unlikely(dr::none_or<false>(active) || !ctx.is_enabled(BSDFFlags::DeltaReflection)))
            return { bs, value };

        bs.sampled_component = 0;
        bs.sampled_type = +BSDFFlags::DeltaReflection;
        bs.wo  = reflect(si.wi);
        bs.eta = 1.f;
        bs.pdf = 1.f;

        dr::Complex<UnpolarizedSpectrum> eta(m_eta->eval(si, active),
                                             m_k->eval(si, active));
        UnpolarizedSpectrum reflectance = m_specular_reflectance->eval(si, active);

        if constexpr (is_polarized_v<Spectrum>) {
            /* Due to the coordinate system rotations for polarization-aware
               pBSDFs below we need to know the propagation direction of light.
               In the following, light arrives along `-wo_hat` and leaves along
               `+wi_hat`. */
            Vector3f wo_hat = ctx.mode == TransportMode::Radiance ? bs.wo : si.wi,
                     wi_hat = ctx.mode == TransportMode::Radiance ? si.wi : bs.wo;

            // Mueller matrix for specular reflection.
            value = mueller::specular_reflection(UnpolarizedSpectrum(Frame3f::cos_theta(wo_hat)), eta);

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

            /* Rotate in/out reference vector of `value` s.t. it aligns with the
               implicit Stokes bases of -wo_hat & wi_hat. */
            value = mueller::rotate_mueller_basis(value,
                                                  -wo_hat, s_axis_in, mueller::stokes_basis(-wo_hat),
                                                   wi_hat, s_axis_out, mueller::stokes_basis(wi_hat));
            value *= mueller::absorber(reflectance);
        } else {
            if (has_thin_film()) {
                UnpolarizedSpectrum r_film = thin_film_reflectance(
                    cos_theta_i, eta, si.wavelengths);
                value = reflectance * r_film;
            } else {
                value = reflectance * fresnel_conductor(
                    UnpolarizedSpectrum(cos_theta_i), eta);
            }
        }

        return { bs, value & active };
    }

    Spectrum eval(const BSDFContext & /*ctx*/, const SurfaceInteraction3f & /*si*/,
                  const Vector3f & /*wo*/, Mask /*active*/) const override {
        return 0.f;
    }

    Float pdf(const BSDFContext & /*ctx*/, const SurfaceInteraction3f & /*si*/,
              const Vector3f & /*wo*/, Mask /*active*/) const override {
        return 0.f;
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "SmoothConductor[" << std::endl
            << "  eta = " << string::indent(m_eta) << "," << std::endl
            << "  k = "   << string::indent(m_k)   << "," << std::endl
            << "  specular_reflectance = " << string::indent(m_specular_reflectance);
        if (m_film_thickness > 0.f)
            oss << "," << std::endl
                << "  film_thickness = " << m_film_thickness << " nm," << std::endl
                << "  film_ior = "       << m_film_ior;
        oss << std::endl << "]";
        return oss.str();
    }

    MI_DECLARE_CLASS(SmoothConductor)
private:
    ref<Texture> m_specular_reflectance;
    ref<Texture> m_eta, m_k;
    ScalarFloat m_film_thickness;
    ScalarFloat m_film_ior;

    MI_TRAVERSE_CB(Base, m_specular_reflectance, m_eta, m_k,
                   m_film_thickness, m_film_ior)
};

MI_EXPORT_PLUGIN(SmoothConductor)
NAMESPACE_END(mitsuba)
