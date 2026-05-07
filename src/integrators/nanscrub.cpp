#include <mitsuba/render/integrator.h>
#include <mitsuba/render/records.h>

NAMESPACE_BEGIN(mitsuba)

/**!

.. _integrator-nanscrub:

NaN-scrubbing integrator (:monosp:`nanscrub`)
---------------------------------------------

.. pluginparameters::

 * - (Nested plugin)
   - :paramtype:`integrator`
   - The wrapped integrator that actually traces rays.

This wrapper integrator forwards every ``sample()`` call to a nested
integrator and intercepts the returned spectrum (and AOVs) before they
reach the film. Any non-finite component (NaN or Inf) is replaced with
zero. The mask returned by the nested integrator is also forced to
:monosp:`false` for any sample whose spectrum contained a non-finite
value, so the film accumulator does not even count those samples.

This addresses a class of artifacts in mitsuba 3 caused by Dr.Jit's
wavefront execution: when one ray within a SIMD wave produces a
numerical edge case (e.g., division by a near-zero PDF, sampling near a
singular phase function), the resulting NaN can propagate through
masking-incomplete operations to other lanes in the same wave. The
characteristic visual signature is small square blocks of corrupted
pixels (matching the SIMD width) in long renders. Pixels that carry a
single NaN sample also become NaN, since the running average is
poisoned for the rest of the integration.

By scrubbing at the integrator/film boundary, the wrapper localizes
errors: a NaN sample contributes 0 instead of poisoning the pixel.
This is mathematically a tiny bias toward zero (proportional to the
NaN-sample probability, typically ``< 1e-5``), in exchange for clean
animations.

.. tabs::
    .. code-tab:: xml

        <integrator type="nanscrub">
            <integrator type="volpathmis">
                <integer name="max_depth" value="16"/>
            </integrator>
        </integrator>

    .. code-tab:: python

        'type': 'nanscrub',
        'nested': {
            'type': 'volpathmis',
            'max_depth': 16,
        }

The wrapper has no parameters of its own — its only effect is to scrub
the nested integrator's output.

 */

template <typename Float, typename Spectrum>
class NaNScrubIntegrator final : public SamplingIntegrator<Float, Spectrum> {
public:
    MI_IMPORT_BASE(SamplingIntegrator)
    MI_IMPORT_TYPES(Scene, Sampler, Medium)

    NaNScrubIntegrator(const Properties &props) : Base(props) {
        for (auto &prop : props.objects()) {
            Base *integrator = prop.try_get<Base>();
            if (!integrator)
                continue;
            if (m_integrator)
                Throw("nanscrub: only one nested integrator may be specified.");
            m_integrator = integrator;
        }
        if (!m_integrator)
            Throw("nanscrub: a nested integrator must be specified.");
    }

    std::pair<Spectrum, Mask> sample(const Scene *scene,
                                     Sampler *sampler,
                                     const RayDifferential3f &ray,
                                     const Medium *medium,
                                     Float *aovs,
                                     Mask active) const override {
        MI_MASKED_FUNCTION(ProfilerPhase::SamplingIntegratorSample, active);

        auto [spec, mask] =
            m_integrator->sample(scene, sampler, ray, medium, aovs, active);

        /* Detect any non-finite component in the spectrum lane-wise. A
           single NaN in any wavelength of a sample is enough to corrupt
           the film when accumulated; we treat the whole sample as zero
           rather than letting it propagate. */
        UnpolarizedSpectrum spec_u = unpolarized_spectrum(spec);
        Mask is_bad = Mask(false);
        for (size_t i = 0; i < UnpolarizedSpectrum::Size; ++i)
            is_bad |= !dr::isfinite(spec_u.entry(i));

        spec  = dr::select(is_bad, Spectrum(0.f), spec);
        mask  = mask & !is_bad;

        /* Scrub AOVs as well — they're written into the film just like
           the radiance and a NaN in a normal/albedo channel will poison
           that pixel's AOV output similarly. We do not know the AOV
           count at compile time, so the wrapped integrator is queried
           via aov_names(). */
        size_t n_aov = m_integrator->aov_names().size();
        for (size_t i = 0; i < n_aov; ++i)
            aovs[i] = dr::select(dr::isfinite(aovs[i]), aovs[i], Float(0.f));

        return { spec, mask };
    }

    std::vector<std::string> aov_names() const override {
        return m_integrator->aov_names();
    }

    void traverse(TraversalCallback *cb) override {
        cb->put("integrator", m_integrator.get(), ParamFlags::Differentiable);
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "NaNScrubIntegrator[" << std::endl
            << "  integrator = " << string::indent(m_integrator) << std::endl
            << "]";
        return oss.str();
    }

    MI_DECLARE_CLASS(NaNScrubIntegrator)
private:
    ref<Base> m_integrator;

    MI_TRAVERSE_CB(Base, m_integrator)
};

MI_EXPORT_PLUGIN(NaNScrubIntegrator)
NAMESPACE_END(mitsuba)
