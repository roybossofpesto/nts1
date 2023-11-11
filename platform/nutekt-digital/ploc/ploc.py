#!/usr/bin/env python3

from pylab import *
from matplotlib.collections import LineCollection
from matplotlib.animation import FuncAnimation

def make_laplace_ope(pps):
    minus_laplace_ope = 2 * eye(pps.size, pps.size, 0)
    minus_laplace_ope -= eye(pps.size, pps.size, 1)
    minus_laplace_ope -= eye(pps.size, pps.size, -1)

    # print(minus_laplace_ope)
    # print(matrix_rank(minus_laplace_ope), pps.size)
    assert matrix_rank(minus_laplace_ope) == pps.size

    inv_minus_laplace_ope = inv(minus_laplace_ope)
    # print(inv_minus_laplace_ope)

    dp = pps[1] - pps[0]
    laplace_ope = -minus_laplace_ope / dp /dp
    return laplace_ope

def run_steps(cc, xxs, dxs, nstep, dt_, laplace_ope):
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

def plot_step_sequence(pps, iis, dis, indices, nframe, cc):
    indices = [len(pps) // 4, 2 * len(pps) // 4 , 3 * len(pps) // 4]
    steps = [(iis, dis)]
    laplace_ope = make_laplace_ope(pps)
    samples = []
    def push_samples_at_positions():
        aas, das = steps[-1]
        samples.append([aas[index] for index in indices])
        assert len(steps) == len(samples)
    push_samples_at_positions()
    while len(steps) < nframe:
        aas, das = steps[-1]
        print(f"seq {len(steps)}/{nframe} {aas.shape} {das.shape}")
        bbs, dbs = run_steps(cc, aas, das, 256, 100e-3, laplace_ope)
        steps.append((bbs, dbs))
        push_samples_at_positions()
    samples = array(samples)
    print(f"got {samples.shape[0]} samples {samples.shape}")

    fig = figure(figsize=(6.4, 6.8))
    axe, axe_ = fig.subplots(2, 1)
    for kk, (aas, das) in enumerate(steps):
        plot_string(axe, pps, aas, das) #, label=f"step{kk}")
    line, sca = plot_string(axe, pps, zzs, dzs, line_color="k", label="init")
    colorbar(sca, ax=axe)
    axe.set_xlim(-.2, 1.2)
    axe.set_ylim(-1.2, 1.2)

    colors = []
    for kk, index in enumerate(indices):
        line_, = axe_.plot(linspace(0., 1., samples.shape[0]), samples[:, kk], ls="-", marker="+", label=f"p={pps[index]:.02f}")
        colors.append(line_.get_color())
    axe_.legend()
    axe_.set_xlim(0, 1)
    axe_.set_ylim(-1.2, 1.2)

    axe.vlines([pps[index] for index in indices], -1, 1, colors, "dashed")
    axe.legend()


def energy_stretch(xxs, dxs, dp, dt, cc):
    xxs_ = concatenate([(0.,), xxs, (0.,)])
    foo = xxs[:-1] - xxs[1:]
    return pow(abs(foo), 2).sum() / dp

def energy_speed(xxs, dxs, dp, dt, cc):
    # print(1/dt)
    foo = dxs / dt / cc
    return pow(abs(foo), 2).sum() * dp * 4000. / 64 * 200

yys = array([])
dys = array([])
samples = []

energy_datas = [
    ("en stretch", energy_stretch),
    ("en speed", energy_speed),
    ("en tot", lambda aas, das, dp, dt, cc: energy_stretch(aas, das, dp, dt, cc) + energy_speed(aas, das, dp, dt, cc))
]
energies = []
def push_samples_at_positions(aas, das, dp, dt, cc):
    samples.append([aas[index] for index in indices])
    energies.append([make_energy(aas, das, dp, dt, cc) for label_energy, make_energy in energy_datas])

def run_animation(pps, iis, dis, dt, cc, cmap):
    global yys, dys
    dp = pps[1] - pps[0]
    yys = iis.copy()
    dys = dis.copy()
    samples.clear()
    fig = figure(figsize=(6.4, 8.8))
    axe, axe_, axe__ = fig.subplots(3, 1)
    line, sca = plot_string(axe, pps, yys, dys, cmap=cmap)
    push_samples_at_positions(yys, dys, dp, dt, cc)

    colorbar(sca, ax=axe)
    axe.set_xlim(-.2, 1.2)
    axe.set_ylim(-1.2, 1.2)
    axe_.set_xlim(0, 1)
    axe_.set_ylim(-1.2, 1.2)
    axe__.set_xlim(0, 1)
    axe__.set_ylim(0, 20)

    lines_ = []
    for kk, index in enumerate(indices):
        values = array([sample[kk] for sample in samples])
        line_, = axe_.plot(linspace(0., 1., len(values)), values, label=f"p={pps[index]:.02f}")
        lines_.append(line_)
    axe_.legend()

    lines__ = []
    for kk, (label_energy, make_energy) in enumerate(energy_datas):
        values = array([energy[kk] for energy in energies])
        line__, = axe__.plot(linspace(0., 1., len(values)), values, label=label_energy)
        lines__.append(line__)
    axe__.legend()

    axe.vlines([pps[index] for index in indices], -1, 1, [line_.get_color() for line_ in lines_], "dashed")
    # axe.legend()

    laplace_ope = make_laplace_ope(pps)

    def init():
        return line, sca

    def update(frame):
        global yys, dys
        euc_norm_yys = norm(yys, 2.)
        inf_norm_yys = norm(yys, np.inf)
        zero_norm_yys = norm(yys, 0)
        print(f"frame {frame:04d} norm_yys euc {euc_norm_yys:.4f} inf {inf_norm_yys:.4f} zero {zero_norm_yys:.4f}")
        yys, dys = run_steps(cc, yys, dys, 200, dt, laplace_ope)
        pps_, yys_, dys_ = make_padded(pps, yys, dys)
        line.set_data(pps_, yys_)
        sca = axe.scatter(pps_, yys_, c=dys_, cmap=cmap)
        push_samples_at_positions(yys, dys, dp, dt, cc)

        for kk, line_ in enumerate(lines_):
            values = array([sample[kk] for sample in samples])
            line_.set_data(linspace(0., 1., len(values)), values)

        for kk, line__ in enumerate(lines__):
            values = array([energy[kk] for energy in energies])
            line__.set_data(linspace(0., 1., len(values)), values)

        return [line, sca] + lines_ + lines__

    return FuncAnimation(fig, \
        update, \
        init_func = init, \
        interval = dt * 1000,
        blit = True)

positions = linspace(0., 1., 256)

mu = .3
sigma = .05
zzs = exp(pow((positions - mu) / sigma, 2) / -2)
dzs = zeros_like(zzs, dtype=float)

indices = [len(positions) // 4, 2 * len(positions) // 4 , 3 * len(positions) // 4]
plot_step_sequence(positions, zzs, dzs, indices, cc=.25, nframe=20)
plot_step_sequence(positions, zzs, dzs, indices, cc=.5, nframe=10)

# ani = run_animation(positions, zzs, dzs, dt=20e-3, cc=.25, cmap="coolwarm")
# ani = run_animation(positions, zzs, dzs, dt=20e-3, cc=1., cmap="coolwarm")
ani_ = run_animation(positions, zzs, dzs, dt=20e-3, cc=2., cmap="coolwarm")
# ani_ = run_animation(positions, zzs, dzs, dt=20e-3, cc=.5, cmap="coolwarm")

show()
