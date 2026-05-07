import pytest
import drjit as dr
import mitsuba as mi


def _simple_scene(integrator):
    """Cornell-box-like scene with the given integrator."""
    return mi.load_dict({
        'type': 'scene',
        'integrator': integrator,
        'sensor': {
            'type': 'perspective', 'fov': 45,
            'to_world': mi.ScalarTransform4f.look_at(
                origin=[0, 0, 4], target=[0, 0, 0], up=[0, 1, 0]),
            'sampler': {'type': 'independent', 'sample_count': 4},
            'film': {'type': 'hdrfilm', 'width': 16, 'height': 16,
                     'rfilter': {'type': 'box'}},
        },
        'sphere': {
            'type': 'sphere',
            'bsdf': {'type': 'diffuse', 'reflectance': 0.5},
        },
        'light': {
            'type': 'constant',
            'radiance': {'type': 'rgb', 'value': [1.0, 1.0, 1.0]},
        },
    })


def test01_construct(variants_all):
    """Wrapper accepts a single nested integrator and rejects bad config."""
    scrub = mi.load_dict({
        'type': 'nanscrub',
        'nested': {'type': 'path'},
    })
    assert scrub is not None

    # No nested integrator should error.
    with pytest.raises(RuntimeError):
        mi.load_dict({'type': 'nanscrub'})


def test02_passthrough_when_clean(variants_all_rgb):
    """When the inner integrator never produces non-finite output, the
    wrapper must produce an image identical to running the inner alone."""
    plain = _simple_scene({'type': 'path', 'max_depth': 4})
    wrapped = _simple_scene({'type': 'nanscrub',
                             'nested': {'type': 'path', 'max_depth': 4}})

    img_plain   = mi.render(plain,   spp=4, seed=42)
    img_wrapped = mi.render(wrapped, spp=4, seed=42)

    # Identical seeds + identical sampling order should give identical pixels.
    assert dr.allclose(img_plain, img_wrapped, atol=1e-5), \
        f'wrapper changed output on clean integrand'


def test03_no_nan_in_clean_render(variants_all_rgb):
    """The wrapped output must be entirely finite for a normal scene."""
    import numpy as np
    scene = _simple_scene({'type': 'nanscrub',
                           'nested': {'type': 'path', 'max_depth': 4}})
    img = mi.render(scene, spp=4)
    arr = np.array(img)
    assert np.all(np.isfinite(arr)), \
        f'nanscrub-wrapped render produced non-finite output: ' \
        f'NaN={np.isnan(arr).sum()}, Inf={np.isinf(arr).sum()}'
