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

def run_steps(cc, xxs, dxs, nstep, dt_):
    # cc propagation speed in unit / second
    # dt_ in second
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

def plot_string(ax, pps, xxs, dxs, line_color=None, cmap=None, label=None):
    pps_, xxs_, dxs_ = make_padded(pps, xxs, dxs)
    ln, = ax.plot(pps_, xxs_, color=line_color, label=label, zorder=0)
    sc = ax.scatter(pps_, xxs_, c=dxs_, cmap=cmap)
    return ln, sc

positions = [len(pps) // 4, 2 * len(pps) // 4 , 3 * len(pps) // 4]
def plot_step_sequence(nframe=10):
    steps = [(zzs, dzs)]

    samples = []
    def push_samples_at_positions():
        aas, das = steps[-1]
        samples.append([aas[index] for index in positions])
        assert len(steps) == len(samples)
    push_samples_at_positions()
    while len(steps) < nframe:
        aas, das = steps[-1]
        print(f"** {len(steps)} {aas.shape} {das.shape}")
        bbs, dbs = run_steps(.2, aas, das, 256, 100e-3)
        steps.append((bbs, dbs))
        push_samples_at_positions()
    samples = array(samples)

    fig = figure(figsize=(6.4, 6.8))
    axe, axe_ = fig.subplots(2, 1)
    for kk, (aas, das) in enumerate(steps):
        plot_string(axe, pps, aas, das) #, label=f"step{kk}")
    line, sca = plot_string(axe, pps, zzs, dzs, line_color="k", label="init")
    colorbar(sca, ax=axe)
    axe.set_xlim(-.2, 1.2)
    axe.set_ylim(-1.2, 1.2)

    colors = []
    for kk, index in enumerate(positions):
        line_, = axe_.plot(samples[:, kk], label=f"p={pps[index]:.02f}")
        colors.append(line_.get_color())
    axe_.legend()
    axe_.set_xlim(-.2, 1.2)
    axe_.set_ylim(-1.2, 1.2)

    axe.vlines([pps[index] for index in positions], -1, 1, colors, "dashed")
    axe.legend()
plot_step_sequence()

yys = zzs.copy()
dys = dzs.copy()
samples = []
def push_samples_at_positions(aas, das):
    samples.append([aas[index] for index in positions])

def run_animation(dt=20e-3, cmap="coolwarm"):
    fig = figure(figsize=(6.4, 6.8))
    axe, axe_ = fig.subplots(2, 1)
    line, sca = plot_string(axe, pps, yys, dys, cmap=cmap)
    push_samples_at_positions(yys, dys)

    colorbar(sca, ax=axe)
    axe.set_xlim(-.2, 1.2)
    axe.set_ylim(-1.2, 1.2)
    axe_.set_xlim(0, 1)
    axe_.set_ylim(-1.2, 1.2)

    lines_ = []
    for kk, index in enumerate(positions):
        values = array([sample[kk] for sample in samples])
        line_, = axe_.plot(linspace(0., 1., len(values)), values, label=f"p={pps[index]:.02f}")
        lines_.append(line_)
    axe_.legend()

    axe.vlines([pps[index] for index in positions], -1, 1, [line_.get_color() for line_ in lines_], "dashed")
    # axe.legend()

    def init():
        return line, sca

    def update(frame):
        global yys, dys
        log_euc_norm_yys = log10(norm(yys, 2.))
        log_inf_norm_yys = log10(norm(yys, np.inf))
        log_zero_norm_yys = log10(norm(yys, 0))
        print(f"frame {frame:04d} log_norm_yys euc {log_euc_norm_yys:.4f} inf {log_inf_norm_yys:.4f} zero {log_zero_norm_yys:.4f}")
        yys, dys = run_steps(.8, yys, dys, 64, dt)
        pps_, yys_, dys_ = make_padded(pps, yys, dys)
        line.set_data(pps_, yys_)
        sca = axe.scatter(pps_, yys_, c=dys_, cmap=cmap)
        push_samples_at_positions(yys, dys)

        for kk, line_ in enumerate(lines_):
            values = array([sample[kk] for sample in samples])
            line_.set_data(linspace(0., 1., len(values)), values)

        return [line, sca] + lines_

    return FuncAnimation(fig, \
        update, \
        init_func = init, \
        interval = dt * 1000,
        blit = True)
ani = run_animation()

show()
