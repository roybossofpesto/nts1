#!/bin/env python

from pylab import *
from matplotlib.collections import LineCollection
from matplotlib.animation import FuncAnimation

pps = linspace(0., 1., 32)
# ys = zeros_like(pps, dtype=float)
# ys[3 * xs.size // 4] = 1

mu = .3
sigma = .05
zzs = exp(pow((pps - mu) / sigma, 2) / -2)
dzs = zeros_like(zzs, dtype=float)

minus_laplace_ope = 2 * eye(pps.size, pps.size, 0)
minus_laplace_ope -= eye(pps.size, pps.size, 1)
minus_laplace_ope -= eye(pps.size, pps.size, -1)

print(minus_laplace_ope)
print(matrix_rank(minus_laplace_ope), pps.size)

inv_minus_laplace_ope = inv(minus_laplace_ope)
print(inv_minus_laplace_ope)

dx = pps[1] - pps[0]
laplace_ope = -minus_laplace_ope / dx / dx

cc = .2 # propagation speed in unit / second
def run_steps(xxs, dxs, nstep, dt_):
    assert nstep > 0
    ccs = xxs.copy()
    dcs = dxs.copy()
    dt = dt_ / nstep
    cc_dt_squared = cc * cc * dt * dt
    for step in range(nstep):
        ddcs = cc_dt_squared * laplace_ope @ ccs
        dcs += ddcs
        ccs += dcs
    return ccs, dcs

def make_padded(pps, xxs, dxs):
    assert pps.size == xxs.size
    assert pps.size
    dp = pps[1] - pps[0]
    pps_ = concatenate([(-dp,), pps, (pps[-1] + dp,)])
    xxs_ = concatenate([(0.,), xxs, (0.,)])
    dxs_ = concatenate([(0.,), dxs, (0.,)])
    return pps_, xxs_, dxs_
    # segment for segment in make_padded_pairs()]

def plot_string(ax, pps, xxs, dxs, line_color=None, label=None):
    pps_, xxs_, dxs_ = make_padded(pps, xxs, dxs)
    ln, = ax.plot(pps_, xxs_, color=line_color, label=label, zorder=0)
    sc = ax.scatter(pps_, xxs_, c=dxs_, cmap="viridis")
    return ln, sc

def plot_step_sequence(nframe=10):
    steps = [(zzs, dzs)]

    while len(steps) < nframe:
        aas, das = steps[-1]
        print(f"** {len(steps)} {aas.shape} {das.shape}")
        bbs, dbs = run_steps(aas, das, 256, 100e-3)
        steps.append((bbs, dbs))

    fig = figure()
    axe = fig.subplots()
    for kk, (aas, das) in enumerate(steps):
        plot_string(axe, pps, aas, das, label=f"step{kk}")
    line, sca = plot_string(axe, pps, zzs, dzs, line_color="k", label="init")
    colorbar(sca, ax=axe)
    axe.set_xlim(-.2, 1.2)
    axe.set_ylim(-1.2, 1.2)
    legend()
plot_step_sequence()

yys = zzs.copy()
dys = dzs.copy()
def run_animation(dt=20e-3):
    fig = figure()
    axe = fig.subplots()
    line, sca = plot_string(axe, pps, yys, dys)
    colorbar(sca, ax=axe)
    axe.set_xlim(-.2, 1.2)
    axe.set_ylim(-1.2, 1.2)
    def init():
        return line, sca

    def update(frame):
        print(f"frame {frame}")
        global yys, dys
        yys, dys = run_steps(yys, dys, 64, dt)
        pps_, yys_, dys_ = make_padded(pps, yys, dys)
        line.set_data(pps_, yys_)
        sca = axe.scatter(pps_, yys_, c=dys_, cmap="coolwarm")
        return line, sca

    return FuncAnimation(fig, \
        update, \
        init_func = init, \
        interval = dt * 1000,
        blit = True)
ani = run_animation()

show()
