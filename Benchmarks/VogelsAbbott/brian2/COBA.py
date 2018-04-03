'''
This is a Brian script implementing a benchmark described
in the following review paper:

Simulation of networks of spiking neurons: A review of tools and strategies
(2007). Brette, Rudolph, Carnevale, Hines, Beeman, Bower, Diesmann, Goodman,
Harris, Zirpe, Natschlager, Pecevski, Ermentrout, Djurfeldt, Lansner, Rochel,
Vibert, Alvarez, Muller, Davison, El Boustani and Destexhe.
Journal of Computational Neuroscience 23(3):349-98

Benchmark 2: random network of integrate-and-fire neurons with exponential
synaptic currents.

Clock-driven implementation with exact subthreshold integration
(but spike times are aligned to the grid).
'''
import getopt, sys, timeit
try:
    optlist, args = getopt.getopt(sys.argv[1:], '', ['fast', 'simtime=', 'num_timesteps_delay='])
except getopt.GetoptError as err:
    print(str(err))
    usage()
    sys.exit(2)

simtime = 1.0
fast = False
num_timesteps_delay = 1
for o, a in optlist:
    if (o == "--fast"):
        fast = True
        print("Running without Monitoring Spikes (fast mode)\n")
    elif (o == "--simtime"):
        simtime=float(a)
        print("Simulation Time: " + a)
    elif (o == "--num_timesteps_delay"):
        num_timesteps_delay=int(a)
        print("Delay (in number of timesteps): " + a)

from brian2 import *
from scipy.io import mmread

defaultclock.dt = 0.1*ms

NE = 3200
NI = NE/4

scale = 1
we = scale*0.4*nS
wi = scale*5.1*nS
tau_ampa  = 5.0*ms
tau_gaba = 10.0*ms

gl = 10.0*nS
el = -60.*mV
er = -80.*mV
vt = -50.*mV
memc = 200.0*pfarad
bgcurrent = 200.*pA

eqs = '''
dv/dt  = (-gl*(v-el)-(g_ampa*v+g_gaba*(v-er))+bgcurrent)/memc : volt
dg_ampa/dt = -g_ampa/tau_ampa : siemens
dg_gaba/dt = -g_gaba/tau_gaba : siemens
'''

P = NeuronGroup(NE+NI, model=eqs, threshold='v>vt', reset='v = el', refractory=5*ms, method='euler')
P.v = er
P.g_ampa = 0.*nS
P.g_gaba = 0.*nS
Pe = P[:NE]
Pi = P[NE:]

#Ce = Synapses(P, P, on_pre='ge += we')
conn_ee = Synapses(Pe,Pe,on_pre='g_ampa += we', delay=num_timesteps_delay*defaultclock.dt)
conn_ei = Synapses(Pe,Pi,on_pre='g_ampa += we', delay=num_timesteps_delay*defaultclock.dt)
conn_ie = Synapses(Pi,Pe,on_pre='g_gaba += wi', delay=num_timesteps_delay*defaultclock.dt)
conn_ii = Synapses(Pi,Pi,on_pre='g_gaba += wi', delay=num_timesteps_delay*defaultclock.dt)



ee_mat = mmread('../pynn.ee.wmat')
ei_mat = mmread('../pynn.ei.wmat')
ie_mat = mmread('../pynn.ie.wmat')
ii_mat = mmread('../pynn.ii.wmat')
conn_ee.connect(i=ee_mat.row, j=ee_mat.col)
conn_ei.connect(i=ei_mat.row, j=ei_mat.col)
conn_ie.connect(i=ie_mat.row, j=ie_mat.col)
conn_ii.connect(i=ii_mat.row, j=ii_mat.col)

#conn_ei.connect(Pe, Pi, scale*array(mmread('../pynn.ei.wmat').todense(),dtype=float))
#conn_ie.connect(Pi, Pe, scale*array(mmread('../pynn.ie.wmat').todense(),dtype=float))
#conn_ii.connect(Pi, Pi, scale*array(mmread('../pynn.ii.wmat').todense(),dtype=float))


print("Simulating for " + str(simtime) + "s")
if (fast):
    starttime = timeit.default_timer()
    run(simtime * second)
    totaltime = timeit.default_timer() - starttime
    f = open("timefile.dat", "w")
    f.write("%f" % totaltime)
    f.close()
    print("Real Time Sim: " + str(totaltime) + "s")
else:
    s_mon = SpikeMonitor(P)
    starttime = timeit.default_timer()
    run(simtime * second)
    totaltime = timeit.default_timer() - starttime
    print("Mean Network Firing Rate: " + str(len(s_mon.i) / (4000.0*simtime)) + "Hz")
    print("Real Time Sim: " + str(totaltime) + "s")
    #plot(s_mon.t/ms, s_mon.i, ',k')
    #xlabel('Time (ms)')
print("Complete.")