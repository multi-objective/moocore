r"""Sampling Random Nondominated Sets
=================================

This example illustrates how to sample random sets with
mutually nondominated points.

"""

import moocore
import numpy as np
import matplotlib.pyplot as plt
import plotly.graph_objects as go
from plotly.subplots import make_subplots


def plot_3d(what, x, title, plotly=False):
    """Scatter plot of 3D points."""
    if not plotly:
        fig = plt.figure()
        ax = fig.add_subplot(projection="3d")

    match what:
        case "simplex":
            # Standard 2-simplex vertices in 3D
            x_s, y_s, z_s = np.eye(3, dtype=int)
            if plotly:
                surface = go.Mesh3d(
                    x=x_s,
                    y=y_s,
                    z=z_s,
                    i=[0],
                    j=[1],
                    k=[2],
                    color="cyan",
                    opacity=0.2,
                    flatshading=True,
                    name="Simplex",
                    hoverinfo="none",
                )
            else:
                ax.plot_trisurf(
                    x_s,
                    y_s,
                    z_s,
                    triangles=[[0, 1, 2]],
                    color="cyan",
                    alpha=0.2,
                    edgecolor="gray",
                )

        case "concave" | "convex":
            # Generate points on the positive orthant of the sphere.
            phi = np.linspace(0, np.pi / 2, 50)
            theta = np.linspace(0, np.pi / 2, 50)
            phi, theta = np.meshgrid(phi, theta)
            # Convert spherical to Cartesian coordinates (unit sphere)
            x_s = np.sin(phi) * np.cos(theta)
            y_s = np.sin(phi) * np.sin(theta)
            z_s = np.cos(phi)

            if what == "convex":
                x_s = 1 - x_s
                y_s = 1 - y_s
                z_s = 1 - z_s

            if plotly:
                surface = go.Surface(
                    x=x_s,
                    y=y_s,
                    z=z_s,
                    colorscale=[[0, "cyan"], [1, "cyan"]],
                    opacity=0.2,
                    showscale=False,
                    name="Surface",
                )
            else:
                ax.plot_surface(
                    x_s, y_s, z_s, color="cyan", alpha=0.2, edgecolor="gray"
                )

        case _:
            raise ValueError(f"Unknown plot type {what}")

    if plotly:
        scatter = go.Scatter3d(
            x=x[:, 0],
            y=x[:, 1],
            z=x[:, 2],
            mode="markers",
            marker=dict(size=2, color="blue"),
            name="Data points",
        )

        layout = go.Layout(
            title=title,
            scene=dict(
                xaxis=dict(title="X", range=[0, 1]),
                yaxis=dict(title="Y", range=[0, 1]),
                zaxis=dict(title="Z", range=[0, 1]),
                # Approx. elev=30, azim=25
                camera=dict(eye=dict(x=1.2, y=1.2, z=0.8)),
            ),
            margin=dict(l=0, r=0, b=0, t=40),
            showlegend=False,
        )

        fig = go.Figure(data=[surface, scatter], layout=layout)

    else:
        ax.scatter(
            x[:, 0],
            x[:, 1],
            x[:, 2],
            color="blue",
            s=20,
            marker="o",
            depthshade=False,
        )

        ax.set_xlabel("X")
        ax.set_ylabel("Y")
        ax.set_zlabel("Z")
        ax.set_xlim(0, 1)
        ax.set_ylim(0, 1)
        ax.set_zlim(0, 1)
        ax.set_title(title)
        ax.view_init(elev=30, azim=25)

    return fig


def plotly_3d(what, x, title):
    """Scatter plot of 3D points using plotly."""
    return plot_3d(what=what, x=x, title=title, plotly=True)


def plotly_3d_side_by_side(fig1, fig2):
    """Show two plotly 3D figures side-by-side."""
    fig = make_subplots(
        rows=1,
        cols=2,
        specs=[[{"type": "scene"}, {"type": "scene"}]],
        subplot_titles=(fig1.layout.title.text, fig2.layout.title.text),
    )
    fig.add_traces(
        fig1.data,
        rows=[1] * len(fig1.data),
        cols=[1] * len(fig1.data),
    )
    fig.add_traces(
        fig2.data,
        rows=[1] * len(fig2.data),
        cols=[2] * len(fig2.data),
    )
    fig.update_layout(
        height=400, title="", margin=dict(l=0, r=0, b=0, t=40), showlegend=False
    )
    fig.update_scenes(fig1.layout.scene.to_plotly_json())
    return fig


# %%
#
# Random nondominated sets in 2D
# ------------------------------

n = 100
rng = np.random.default_rng(42)

methods = ["simplex", "concave-sphere", "convex-sphere", "convex-simplex"]
colors = ["red", "blue", "green", "purple"]
markers = ["o", "s", "^", "d"]  # circle, square, triangle_up, diamond

plt.figure(figsize=(4, 4))
for i, method in enumerate(methods):
    points = moocore.generate_ndset(n, 2, method, seed=rng)
    plt.scatter(
        points[:, 0],
        points[:, 1],
        color=colors[i],
        marker=markers[i],
        label=method,
    )

plt.xlabel("X")
plt.ylabel("Y")
plt.legend()
plt.tight_layout()
plt.grid(True)
plt.show()

# %%
#
# Random nondominated sets in integer space
# -----------------------------------------
#
# We can also generate points in integer space.

n = 100
rng = np.random.default_rng(42)

maximum = 0
points = []
for method in methods:
    x = moocore.generate_ndset(n, 2, method, seed=rng, integer=True)
    points += [x]
    maximum = max(maximum, x.max())

plt.figure(figsize=(4, 4))
for i, method in enumerate(methods):
    x = points[i]
    # Normalise so we can plot all sets within the same range.
    if x.max() < maximum:
        x = moocore.normalise(x, lower=0, upper=x.max(), to_range=(0, maximum))
    plt.scatter(
        x[:, 0], x[:, 1], color=colors[i], marker=markers[i], label=method
    )
plt.xlabel("X")
plt.ylabel("Y")
plt.legend()
plt.tight_layout()
plt.grid(True)
plt.show()

# %%
#
# Variants of convex nondominated sets
# ------------------------------------
#
# A popular way to generate a convex nondominated set is to translate the
# negative orthant of the hyper-sphere to the unit hyper-cube
# (``convex-sphere``).  However, there are other possible convex sets with
# different properties. For example the method ``convex-simplex`` applies a
# non-linear transformation to a ``concave-sphere`` set, resulting in a convex
# transformation of the standard simplex.
#

n = 2000
rng = np.random.default_rng(42)

points = moocore.generate_ndset(n, 3, "convex-sphere", seed=rng)
fig1 = plotly_3d("simplex", points, title="Convex-sphere")

points = moocore.generate_ndset(n, 3, "convex-simplex", seed=rng)
fig2 = plotly_3d("simplex", points, title="Convex-simplex")

plotly_3d_side_by_side(fig1, fig2)

# %%
#
# Uniform sampling (moocore) vs projections of uniform samples (naive)
# --------------------------------------------------------------------
#
# Naive methods for sampling such sets usually sample points uniformly in the
# hypercube and project them into a lower dimensional manifold
# :cite:p:`LacKlaFon2017box`, e.g., the standard simplex or the positive orthant
# of the hypersphere.  However, such projections do not preserve the uniformity
# of the sampling, that is, not all points in the manifold have the same
# probability of being sampled.
#
# The function :func:`~moocore.generate_ndset` produces a uniform sampling
# on the manifold, as shown in the following examples:

n = 2000
rng = np.random.default_rng(42)

points = moocore.generate_ndset(n, 3, "simplex", seed=rng)
fig1 = plotly_3d("simplex", points, title="Simplex (moocore)")

points = rng.uniform(size=(n, 3))
points /= points.sum(axis=1, keepdims=True)
fig2 = plotly_3d("simplex", points, title="Simplex (naive)")

plotly_3d_side_by_side(fig1, fig2)

# %%

points = moocore.generate_ndset(n, 3, "concave-sphere", seed=rng)
fig1 = plotly_3d("concave", points, title="Concave-sphere (moocore)")

points = rng.uniform(size=(n, 3))
points /= np.linalg.norm(points, axis=1, keepdims=True)
fig2 = plotly_3d("concave", points, title="Concave-sphere (naive)")

plotly_3d_side_by_side(fig1, fig2)

# %%

points = moocore.generate_ndset(n, 3, "convex-sphere", seed=rng)
fig1 = plotly_3d("convex", points, title="Convex-sphere (moocore)")

points = rng.uniform(size=(n, 3))
points /= np.linalg.norm(points, axis=1, keepdims=True)
fig2 = plotly_3d("convex", 1.0 - points, title="Convex-sphere (naive)")

plotly_3d_side_by_side(fig1, fig2)
