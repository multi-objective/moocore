r"""
Sampling Sequences of Dominated and Nondominated Points
=======================================================

This example illustrates how to sample sequences of multi-dimensional points with various dominance properties using :func:`~moocore.generate_sequence`.

First we define a few functions useful for plotting.
"""

# sphinx_gallery_multi_image = "single"
import moocore
import numpy as np
import matplotlib.pyplot as plt

from _utils import plotly_3d, plotly_3d_side_by_side, plot_sequence_2d


def plotly_generate_sequence_3d(what, n, method, seed, show_index=False):
    """Generate points with moocore.generate_sequence() and plot them with plotly_3d()."""
    return plotly_3d(
        what,
        moocore.generate_sequence(n, 3, method, seed=seed),
        title=f'method="{method}"',
        rank=True,
        show_index=show_index,
    )


# %%
#
# Sequences in 2D
# ---------------
#
# First, we plot completely ordered sequences. We use different seeds to get
# different sequences.  Points are colored according to their Pareto rank, with
# lower ranks having darker colors. In these two sequences, each point has a
# different color (rank) from the rest.
#
n = 10
fig, axes = plt.subplots(1, 2, figsize=(8, 4), sharex=True, sharey=True)
for ax, method, seed in zip(
    axes, ["each-dominates-previous", "each-dominates-next"], [42, 43]
):
    points = moocore.generate_sequence(n, 2, method=method, seed=seed)
    plot_sequence_2d(points, title=f'method="{method}"', ax=ax)

fig.tight_layout()
plt.show()

# %%
# Random sequences sampled in the unit hypercube or the unit hypersphere.
#
n = 10
for method in ["cube", "sphere"]:
    fig, axes = plt.subplots(1, 3, figsize=(12, 4), sharex=True, sharey=True)
    show_circle = method == "sphere"
    for ax, ndsort in zip(axes, [1, 0, -1]):
        points = moocore.generate_sequence(
            n, 2, method=method, ndsort=ndsort, seed=42
        )
        plot_sequence_2d(
            points,
            title=f'method="{method}", ndsort={ndsort}',
            ax=ax,
            show_circle=show_circle,
        )

    fig.tight_layout()
    plt.show()

# %%
# Naive sampling within the hypersphere produces a bias towards the origin.
#
n = 200
method = "sphere"
fig, axes = plt.subplots(1, 2, figsize=(8, 4), sharex=True, sharey=True)
points = moocore.generate_sequence(n, 2, method=method, seed=42)
plot_sequence_2d(
    points,
    title=f'method="{method}"',
    ax=axes[0],
    show_circle=True,
    show_index=False,
)

# Naive
rng = np.random.default_rng(42)
points = np.abs(rng.normal(size=(n, 2)))
points /= np.linalg.norm(points, axis=1, keepdims=True)
points *= rng.uniform(0, 1, size=(n, 1))
plot_sequence_2d(
    points, title="Naive", ax=axes[1], show_circle=True, show_index=False
)
fig.tight_layout()
plt.show()


# %%
# Generate the analytic sequence proposed by :footcite:t:`Gla2017fast`.
#

n = 15
method = "glas2017"
fig, axes = plt.subplots(1, 3, figsize=(12, 4), sharex=True, sharey=True)
for ax, c_value in zip(axes, [0.5, 1, 2]):
    points = moocore.generate_sequence(
        n, 2, method=method, c_value=c_value, seed=42
    )
    plot_sequence_2d(
        points, title=f'method="{method}", c_value={c_value}', ax=ax
    )

fig.tight_layout()
plt.show()


# %%
#
# Sequences in integer space
# -----------------------------------------
#
# We can also generate points in integer space.

n = 10
fig, axes = plt.subplots(1, 2, figsize=(8, 4), sharex=True, sharey=True)
for ax, method, seed in zip(
    axes, ["each-dominates-previous", "each-dominates-next"], [42, 43]
):
    points = moocore.generate_sequence(
        n, 2, method=method, seed=seed, integer=True
    )
    plot_sequence_2d(points, title=f'method="{method}"', ax=ax)

fig.tight_layout()
plt.show()

for method in ["cube", "sphere"]:
    fig, axes = plt.subplots(1, 3, figsize=(12, 4), sharex=True, sharey=True)
    show_circle = method == "sphere"
    for ax, ndsort in zip(axes, [1, 0, -1]):
        points = moocore.generate_sequence(
            n, 2, method=method, ndsort=ndsort, seed=42, integer=True
        )
        plot_sequence_2d(
            points,
            title=f'method="{method}", ndsort={ndsort}',
            ax=ax,
            show_circle=show_circle,
        )

    fig.tight_layout()
    plt.show()

n = 15
method = "glas2017"
fig, axes = plt.subplots(1, 3, figsize=(12, 4), sharex=True, sharey=True)
for ax, c_value in zip(axes, [0.5, 1, 2]):
    points = moocore.generate_sequence(
        n, 2, method=method, c_value=c_value, seed=42, integer=True
    )
    plot_sequence_2d(
        points, title=f'method="{method}", c_value={c_value}', ax=ax
    )

fig.tight_layout()
plt.show()


# %%
#
# Sequences in 3D
# ---------------
#
# Completely ordered sequences. We use different seeds to get different
# sequences.  Points are colored according to their Pareto rank, with lower
# ranks having darker colors. In these two sequences, each point has a
# different color (rank) from the rest.
#
n = 10
fig1 = plotly_generate_sequence_3d(
    "simplex", n, method="each-dominates-previous", seed=42, show_index=True
)
fig2 = plotly_generate_sequence_3d(
    "simplex", n, method="each-dominates-next", seed=43, show_index=True
)
plotly_3d_side_by_side(fig1, fig2)


# %%
#
# Random sequences sampled in the unit hypercube or the unit hypersphere.  If
# you hover over a point, a tooltip shows the coordinates and the index of the
# point in the sequence.
#
n = 100
fig1 = plotly_generate_sequence_3d("concave", n, method="cube", seed=42)
fig2 = plotly_generate_sequence_3d("concave", n, method="sphere", seed=42)
plotly_3d_side_by_side(fig1, fig2)


# %%
#
# Generate the analytic sequence proposed by :footcite:t:`Gla2017fast`.
#

n = 100
method = "glas2017"
c_value = 0.5
fig1 = plotly_3d(
    "simplex",
    moocore.generate_sequence(n, 3, method, seed=42, c_value=c_value),
    title=f'method="{method}", c_value={c_value}',
    rank=True,
)
c_value = 1.5
fig2 = plotly_3d(
    "simplex",
    moocore.generate_sequence(n, 3, method, seed=42, c_value=c_value),
    title=f'method="{method}", c_value={c_value}',
    rank=True,
)

plotly_3d_side_by_side(fig1, fig2)

# %%
# References
# ----------
# .. footbibliography::
#

# %%
# .. rubric:: Related examples
# .. minigallery:: ../../examples/plot_generate.py
