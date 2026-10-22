r"""
Crossover Operators
===================

This example shows where each crossover operator places the children of
fixed parents.  Each figure mates the same parents many times and plots all
the children.  Operators that use 2 parents per operation mate rows ``2i``
and ``2i + 1``, so stacking copies of one pair of parents gives many
independent draws in a single call.

First we define a few functions useful for plotting.
"""

import matplotlib.pyplot as plt
import numpy as np
import moocore

LOWER, UPPER = 0.0, 10.0
BOUNDS = dict(lower=LOWER, upper=UPPER)


def mate(fun, a, b, n_pairs=50_000, **kwargs):
    """Mate parents ``a`` and ``b`` ``n_pairs`` times."""
    parents = np.tile(np.array([a, b], dtype=float), (n_pairs, 1))
    return fun(parents, seed=42, **kwargs)


def plot_density(ax, values, **kwargs):
    """Plot the distribution of ``values`` within the bounds."""
    ax.hist(
        values,
        bins=200,
        range=(LOWER, UPPER),
        density=True,
        histtype="step",
        **kwargs,
    )


def mark_parents(ax, positions):
    """Show the parents as dotted vertical lines."""
    for x in positions:
        ax.axvline(x, color="gray", linestyle=":", linewidth=1)


# %%
#
# Where the children of two parents end up
# ----------------------------------------
#
# The parents are :math:`(3, 3)` and :math:`(7, 7)` within :math:`[0, 10]^2`.
# Each panel shows the distribution of the first variable of the children for
# the default parameters of each operator.  Uniform and single arithmetic
# crossover only produce a few distinct values, which appear as spikes.

a, b = [3.0, 3.0], [7.0, 7.0]
operators = {
    "Bounded exponential": (moocore.bounded_exponential_crossover, BOUNDS),
    "Blend alpha": (moocore.blend_alpha_crossover, BOUNDS),
    "Local": (moocore.local_crossover, {}),
    "Uniform": (moocore.uniform_crossover, {}),
    "Single arithmetic": (moocore.single_arithmetic_crossover, {}),
}
fig, axes = plt.subplots(len(operators), 1, figsize=(6, 8), sharex=True)
for i, (ax, (name, (fun, kwargs))) in enumerate(zip(axes, operators.items())):
    plot_density(ax, mate(fun, a, b, **kwargs)[:, 0], color=f"C{i}")
    mark_parents(ax, [a[0], b[0]])
    ax.set_title(name, loc="left", fontsize="medium")
    ax.set_yticks([])
axes[-1].set_xlabel("$x_1$")
fig.tight_layout()
plt.show()

# %%
#
# Bounded exponential crossover: the scale parameter
# --------------------------------------------------
#
# The parameter ``scale`` (:math:`\lambda`) controls how far the children move
# from their parents.  Each child stays close to one parent, and the bounds
# truncate the distribution.  On a logarithmic scale, the exponential tails
# become straight lines.

fig, ax = plt.subplots(figsize=(6, 3.5))
for i, scale in enumerate([0.01, 0.05, 0.1, 0.5]):
    children = mate(
        moocore.bounded_exponential_crossover, a, b, scale=scale, **BOUNDS
    )
    plot_density(
        ax,
        children[:, 0],
        color=plt.cm.viridis(i / 4),
        label=rf"$\lambda = {scale}$",
    )
mark_parents(ax, [a[0], b[0]])
ax.set_yscale("log")
ax.set_ylim(1e-3, None)
ax.set_xlabel("$x_1$")
ax.set_ylabel("Density")
ax.legend()
fig.tight_layout()
plt.show()

# %%
#
# Blend crossover: the alpha parameter
# ------------------------------------
#
# BLX-:math:`\alpha` samples uniformly from the interval between the parents,
# widened by ``alpha`` times their distance on each side.

fig, ax = plt.subplots(figsize=(6, 3.5))
for i, alpha in enumerate([0.0, 0.25, 0.5, 1.0]):
    children = mate(moocore.blend_alpha_crossover, a, b, alpha=alpha, **BOUNDS)
    plot_density(
        ax,
        children[:, 0],
        color=plt.cm.viridis(i / 4),
        label=rf"$\alpha = {alpha}$",
    )
mark_parents(ax, [a[0], b[0]])
ax.set_xlabel("$x_1$")
ax.set_ylabel("Density")
ax.legend()
fig.tight_layout()
plt.show()

# %%
#
# Offspring geometry
# ------------------
#
# With the parents at opposite corners, :math:`(2, 8)` and :math:`(8, 2)`, the
# operators differ in which regions of the decision space their children
# reach.  Uniform and single-point crossover recombine whole variables, so
# their children land on the corners of the box spanned by the parents.

a, b = [2.0, 8.0], [8.0, 2.0]
operators = {
    "Bounded exponential": (moocore.bounded_exponential_crossover, BOUNDS),
    "Blend alpha": (moocore.blend_alpha_crossover, BOUNDS),
    "Local": (moocore.local_crossover, {}),
    "Uniform": (moocore.uniform_crossover, {}),
    "Single arithmetic": (moocore.single_arithmetic_crossover, {}),
    "Single point": (moocore.single_point_crossover, {}),
}
fig, axes = plt.subplots(2, 3, figsize=(9, 6), sharex=True, sharey=True)
for i, (ax, (name, (fun, kwargs))) in enumerate(
    zip(axes.flat, operators.items())
):
    children = mate(fun, a, b, n_pairs=1000, **kwargs)
    ax.scatter(children[:, 0], children[:, 1], s=4, alpha=0.3, color=f"C{i}")
    ax.scatter(*np.array([a, b]).T, marker="D", s=40, color="black")
    ax.set_title(name, fontsize="medium")
    ax.set_xlim(LOWER, UPPER)
    ax.set_ylim(LOWER, UPPER)
    ax.set_aspect("equal")
for ax in axes[1]:
    ax.set_xlabel("$x_1$")
for ax in axes[:, 0]:
    ax.set_ylabel("$x_2$")
fig.tight_layout()
plt.show()

# %%
#
# Which parent each variable comes from
# -------------------------------------
#
# Uniform and single-point crossover only copy values, so they also work for
# binary and integer variables.  With one parent of all zeros and one of all
# ones, the share of ones in the first child shows how likely each variable is
# to come from the second parent.  Uniform crossover treats all variables
# alike, whereas single-point crossover keeps neighbouring variables together.

nvar = 10
zeros, ones = np.zeros(nvar, dtype=int), np.ones(nvar, dtype=int)
fig, ax = plt.subplots(figsize=(6, 3.5))
for fun, name in [
    (moocore.uniform_crossover, "Uniform"),
    (moocore.single_point_crossover, "Single point"),
]:
    first_children = mate(fun, zeros, ones, n_pairs=4000)[0::2]
    ax.plot(
        np.arange(1, nvar + 1), first_children.mean(axis=0), "o-", label=name
    )
ax.axhline(0.5, color="gray", linestyle=":", linewidth=1)
ax.set_xticks(np.arange(1, nvar + 1))
ax.set_ylim(-0.05, 1.05)
ax.set_xlabel("Variable")
ax.set_ylabel("Share taken from the second parent")
ax.legend()
fig.tight_layout()
plt.show()

# %%
#
# Differential evolution and parent-centric crossover
# ---------------------------------------------------
#
# These operators produce one child per parent and choose the other parents at
# random from the population.  Here, every member of a population of 60 points
# (circles) is a target, and each operator is called 40 times.  Passing the
# same :class:`numpy.random.Generator` as ``seed`` gives new children on each
# call.  Both DE variants spread their children over the region covered by
# the population, whereas PCX keeps them close to their index parents.

rng = np.random.default_rng(42)
population = np.clip(rng.normal([4.0, 6.0], 1.2, size=(60, 2)), 0.2, 9.8)
operators = {
    "DE/rand/1/bin": moocore.de_rand_1_bin_crossover,
    "DE/current/1": moocore.de_current_1_crossover,
    "Parent-centric": moocore.parent_centric_crossover,
}
fig, axes = plt.subplots(1, 3, figsize=(10, 3.8), sharex=True, sharey=True)
for i, (ax, (name, fun)) in enumerate(zip(axes, operators.items())):
    children = np.vstack(
        [fun(population, seed=rng, **BOUNDS) for _ in range(40)]
    )
    ax.scatter(children[:, 0], children[:, 1], s=3, alpha=0.3, color=f"C{i}")
    ax.scatter(
        population[:, 0],
        population[:, 1],
        s=20,
        facecolors="none",
        edgecolors="black",
    )
    ax.set_title(name, fontsize="medium")
    ax.set_xlim(LOWER, UPPER)
    ax.set_ylim(LOWER, UPPER)
    ax.set_aspect("equal")
    ax.set_xlabel("$x_1$")
axes[0].set_ylabel("$x_2$")
fig.tight_layout()
plt.show()
