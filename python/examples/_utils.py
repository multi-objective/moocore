import numpy as np
import moocore
import matplotlib.pyplot as plt
from matplotlib.patches import Arc
from matplotlib.colors import to_hex
import plotly.graph_objects as go
from plotly.subplots import make_subplots


def plot_3d(what, x, title, plotly=False, rank=False, show_index=False):
    """Scatter plot of 3D points."""
    if not plotly:
        fig = plt.figure()
        ax = fig.add_subplot(projection="3d")

    if rank:
        ranks = moocore.pareto_rank(x)
        n_ranks = ranks.max() + 1
        # One distinct color for each rank
        cmap = plt.get_cmap("viridis", n_ranks)
        rank_colors = [to_hex(cmap(i)) for i in range(n_ranks)]

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
        color = [rank_colors[r] for r in ranks] if rank else "blue"
        text = [str(i + 1) for i in range(len(x))] if rank else []
        scatter = go.Scatter3d(
            x=x[:, 0],
            y=x[:, 1],
            z=x[:, 2],
            mode="markers+text" if show_index else "markers",
            text=text,
            textposition="top center",
            marker=dict(size=2, color=color),
        )
        if x.max() <= 1:
            limits = [0, 1]
        else:
            limits = [x.min(), x.max()]

        layout = go.Layout(
            title=title,
            scene=dict(
                xaxis=dict(title="X", range=limits),
                yaxis=dict(title="Y", range=limits),
                zaxis=dict(title="Z", range=limits),
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
        ax.set(
            xlabel="X",
            ylabel="Y",
            zlabel="Z",
            xlim=(0, 1),
            ylim=(0, 1),
            zlim=(0, 1),
            title=title,
        )
        ax.view_init(elev=30, azim=25)

    return fig


def plotly_3d(what, x, title, rank=False, show_index=False):
    """Scatter plot of 3D points using plotly."""
    return plot_3d(
        what=what,
        x=x,
        title=title,
        plotly=True,
        rank=rank,
        show_index=show_index,
    )


def plotly_3d_side_by_side(fig1, fig2):
    """Show two plotly 3D figures side-by-side."""
    fig = make_subplots(
        rows=1,
        cols=2,
        specs=[[{"type": "scene"}, {"type": "scene"}]],
        subplot_titles=(fig1.layout.title.text, fig2.layout.title.text),
    )
    len1 = len(fig1.data)
    fig.add_traces(fig1.data, rows=[1] * len1, cols=[1] * len1)
    len2 = len(fig2.data)
    fig.add_traces(fig2.data, rows=[1] * len2, cols=[2] * len2)
    fig.update_layout(
        height=400, title="", margin=dict(l=0, r=0, b=0, t=40), showlegend=False
    )
    fig.update_scenes(fig1.layout.scene.to_plotly_json())
    return fig


def plot_sequence_2d(points, title, ax, show_index=True, show_circle=False):
    """Plot a sequence in 2D"""
    ranks = moocore.pareto_rank(points)
    n_ranks = ranks.max() + 1

    ax.scatter(
        points[:, 0],
        points[:, 1],
        c=ranks,
        cmap=plt.get_cmap("viridis", n_ranks),
        vmin=-0.5,
        vmax=n_ranks - 0.5,
    )

    if show_index:
        for i, (x, y) in enumerate(points):
            ax.text(x, y, str(i + 1), fontsize=10, ha="left", va="bottom")

    if show_circle:
        radius = 1 if points.max() <= 1 else 2**31
        ax.add_patch(
            Arc(
                (0, 0),
                2 * radius,
                2 * radius,
                theta1=0,
                theta2=90,
                fill=False,
                linewidth=1.5,
                linestyle="--",
            )
        )

    if points.min() >= 0 and points.max() <= 1:
        ax.set_xlim(0, 1.05)
        ax.set_ylim(0, 1.05)

    ax.set_title(title)
    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.grid(True)
