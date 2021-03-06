// Brunel 10,000 Neuron Network with Plasticity
// Author: Nasir Ahmad (Created: 03/05/2018)


/*
  This network has been created to benchmark Spike

  Publications:

*/


#include "Spike/Spike.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <math.h>
#include <getopt.h>
#include <time.h>
#include <iomanip>
#include <vector>
#include <stdlib.h>

void connect_with_sparsity(
    int input_layer,
    int output_layer,
    spiking_neuron_parameters_struct* input_layer_params,
    spiking_neuron_parameters_struct* output_layer_params,
    voltage_spiking_synapse_parameters_struct* SYN_PARAMS,
    float sparseness,
    SpikingModel* Model
    ){
  // Change the connectivity type
  int num_post_neurons = 
    output_layer_params->group_shape[0]*output_layer_params->group_shape[1];
  int num_pre_neurons = 
    input_layer_params->group_shape[0]*input_layer_params->group_shape[1];
  int num_syns_per_post = 
    sparseness*num_pre_neurons;
  
  std::vector<int> prevec, postvec;
  for (int outid = 0; outid < num_post_neurons; outid++){
    for (int inid = 0; inid < num_syns_per_post; inid++){
      postvec.push_back(outid);
      prevec.push_back(rand() % num_pre_neurons);
    }
  }

  SYN_PARAMS->pairwise_connect_presynaptic = prevec;
  SYN_PARAMS->pairwise_connect_postsynaptic = postvec;
  SYN_PARAMS->connectivity_type = CONNECTIVITY_TYPE_PAIRWISE;

  Model->AddSynapseGroup(input_layer, output_layer, SYN_PARAMS);

}
int connect_from_mat(
    int layer1,
    int layer2,
    voltage_spiking_synapse_parameters_struct* SYN_PARAMS, 
    std::string filename,
    SpikingModel* Model,
    float timestep,
    int numskipgroups=1){
  int synapse_group_index = -1;

  ifstream weightfile;
  string line;
  stringstream ss;
  std::vector<int> prevec, postvec;
  std::vector<float> weightvec;
  std::vector<float> delayvec;
  int pre, post;
  float weight;
  int linecount = 0;
  weightfile.open(filename.c_str());

  if (weightfile.is_open()){
    printf("Loading weights from mat file: %s\n", filename.c_str());
    while (getline(weightfile, line)){
      if (line.c_str()[0] == '%'){
        continue;
      } else {
        linecount++;
        if (linecount == 1) continue;
        //printf("%s\n", line.c_str());
        ss.clear();
        ss << line;
        ss >> pre >> post >> weight;
        prevec.push_back(pre - 1);
        postvec.push_back(post - 1);
        weightvec.push_back(weight);
        delayvec.push_back(SYN_PARAMS->delay_range[0] - (linecount % numskipgroups)*timestep);
        //printf("%d\n", (linecount % numskipgroups));

        //printf("%d, %d, %f\n", pre, post, weight);
      }
    }
    SYN_PARAMS->pairwise_connect_presynaptic = prevec;
    SYN_PARAMS->pairwise_connect_postsynaptic = postvec;
    SYN_PARAMS->pairwise_connect_weight = weightvec;
    SYN_PARAMS->pairwise_connect_delay = delayvec;
    SYN_PARAMS->connectivity_type = CONNECTIVITY_TYPE_PAIRWISE;
    synapse_group_index = Model->AddSynapseGroup(layer1, layer2, SYN_PARAMS);
  } else {
    printf("Could not find weight matrices for loading! Have you created these as instructed in the README.md??\n");
    exit(-1);
  }

  return(synapse_group_index);
}



int main (int argc, char *argv[]){
  // Getting options:
  float simtime = 20.0;
  float sparseness = 0.1;
  bool fast = false;
  bool no_TG = false;
  bool plastic = false;
  int numsyngroups = 1;
  const char* const short_opts = "";
  const option long_opts[] = {
    {"simtime", 1, nullptr, 0},
    {"fast", 0, nullptr, 1},
    {"plastic", 0, nullptr, 4},
    {"NOTG", 0, nullptr, 5},
    {"num_synapse_groups", 1, nullptr, 6}
  };
  // Check the set of options
  while (true) {
    const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

    // If none
    if (-1 == opt) break;

    switch (opt){
      case 0:
        printf("Running with a simulation time of: %ss\n", optarg);
        simtime = std::stof(optarg);
        break;
      case 1:
        printf("Running in fast mode (no spike collection)\n");
        fast = true;
        break;
      case 4:
        printf("Running with plasticity ON\n");
        plastic = true;
        break;
      case 5:
        printf("TURNING OFF TIMESTEP GROUPING\n");
        no_TG = true;
        break;
      case 6:
        printf("Number of synapse groups; %s\n", optarg);
        numsyngroups = std::stoi(optarg);
        break;
    }
  };
  
  // TIMESTEP MUST BE SET BEFORE DATA IS IMPORTED. USED FOR ROUNDING.
  // The details below shall be used in a SpikingModel
  SpikingModel * BenchModel = new SpikingModel();
  float timestep = 0.0001f; // 50us for now
  BenchModel->SetTimestep(timestep);
  float delayval = 1.5f*powf(10.0, -3.0); // 1.5ms

  // Create neuron, synapse and stdp types for this model
  LIFSpikingNeurons * lif_spiking_neurons = new LIFSpikingNeurons();
  PoissonInputSpikingNeurons * poisson_input_spiking_neurons = new PoissonInputSpikingNeurons();
  VoltageSpikingSynapses * voltage_spiking_synapses = new VoltageSpikingSynapses(42);

  weightdependent_stdp_plasticity_parameters_struct * WDSTDP_PARAMS = new weightdependent_stdp_plasticity_parameters_struct;
  WDSTDP_PARAMS->a_plus = 1.0;
  WDSTDP_PARAMS->a_minus = 1.0;
  WDSTDP_PARAMS->tau_plus = 0.02;
  WDSTDP_PARAMS->tau_minus = 0.02;
  WDSTDP_PARAMS->lambda = 1.0f*powf(10.0, -2);
  WDSTDP_PARAMS->alpha = 2.02;
  WDSTDP_PARAMS->w_max = 0.3*powf(10.0, -3);
  WDSTDP_PARAMS->nearest_spike_only = false;
  WeightDependentSTDPPlasticity * weightdependent_stdp = new WeightDependentSTDPPlasticity((SpikingSynapses *) voltage_spiking_synapses, (SpikingNeurons *)lif_spiking_neurons, (SpikingNeurons *) poisson_input_spiking_neurons, (stdp_plasticity_parameters_struct *) WDSTDP_PARAMS);

  if (plastic)
    BenchModel->AddPlasticityRule(weightdependent_stdp);


  // Add my populations to the SpikingModel
  BenchModel->spiking_neurons = lif_spiking_neurons;
  BenchModel->input_spiking_neurons = poisson_input_spiking_neurons;
  BenchModel->spiking_synapses = voltage_spiking_synapses;

  SpikingActivityMonitor* spike_monitor = new SpikingActivityMonitor(lif_spiking_neurons);
  SpikingActivityMonitor* input_spike_monitor = new SpikingActivityMonitor(poisson_input_spiking_neurons);
  if (!fast){
    BenchModel->AddActivityMonitor(spike_monitor);
    BenchModel->AddActivityMonitor(input_spike_monitor);
  }


  // Set up Neuron Parameters
  lif_spiking_neuron_parameters_struct * EXC_NEURON_PARAMS = new lif_spiking_neuron_parameters_struct();
  lif_spiking_neuron_parameters_struct * INH_NEURON_PARAMS = new lif_spiking_neuron_parameters_struct();

  EXC_NEURON_PARAMS->somatic_capacitance_Cm = 200.0f*pow(10.0, -12);  // pF
  INH_NEURON_PARAMS->somatic_capacitance_Cm = 200.0f*pow(10.0, -12);  // pF

  EXC_NEURON_PARAMS->somatic_leakage_conductance_g0 = 10.0f*pow(10.0, -9);  // nS
  INH_NEURON_PARAMS->somatic_leakage_conductance_g0 = 10.0f*pow(10.0, -9);  // nS

  EXC_NEURON_PARAMS->resting_potential_v0 = 0.0f*pow(10.0, -3); // -74mV
  INH_NEURON_PARAMS->resting_potential_v0 = 0.0f*pow(10.0, -3); // -82mV
  
  EXC_NEURON_PARAMS->after_spike_reset_potential_vreset = 0.0f*pow(10.0, -3);
  INH_NEURON_PARAMS->after_spike_reset_potential_vreset = 0.0f*pow(10.0, -3);

  EXC_NEURON_PARAMS->absolute_refractory_period = 2.0f*pow(10, -3);  // ms
  INH_NEURON_PARAMS->absolute_refractory_period = 2.0f*pow(10, -3);  // ms

  EXC_NEURON_PARAMS->threshold_for_action_potential_spike = 20.0f*pow(10.0, -3);
  INH_NEURON_PARAMS->threshold_for_action_potential_spike = 20.0f*pow(10.0, -3);

  EXC_NEURON_PARAMS->background_current = 0.0f*pow(10.0, -2); //
  INH_NEURON_PARAMS->background_current = 0.0f*pow(10.0, -2); //

  /*
    Setting up INPUT NEURONS
  */
  // Creating an input neuron parameter structure
  poisson_input_spiking_neuron_parameters_struct* input_neuron_params = new poisson_input_spiking_neuron_parameters_struct();
  // Setting the dimensions of the input neuron layer
  input_neuron_params->group_shape[0] = 1;    // x-dimension of the input neuron layer
  input_neuron_params->group_shape[1] = 10000;    // y-dimension of the input neuron layer
  input_neuron_params->rate = 20.0f; // Hz
  int input_layer_ID = BenchModel->AddInputNeuronGroup(input_neuron_params);
  poisson_input_spiking_neurons->set_up_rates();

  /*
    Setting up NEURON POPULATION
  */
  vector<int> EXCITATORY_NEURONS;
  vector<int> INHIBITORY_NEURONS;
  // Creating a single exc and inh population for now
  EXC_NEURON_PARAMS->group_shape[0] = 1;
  EXC_NEURON_PARAMS->group_shape[1] = 8000;
  INH_NEURON_PARAMS->group_shape[0] = 1;
  INH_NEURON_PARAMS->group_shape[1] = 2000;
  EXCITATORY_NEURONS.push_back(BenchModel->AddNeuronGroup(EXC_NEURON_PARAMS));
  INHIBITORY_NEURONS.push_back(BenchModel->AddNeuronGroup(INH_NEURON_PARAMS));

  /*
    Setting up SYNAPSES
  */
  voltage_spiking_synapse_parameters_struct * EXC_OUT_SYN_PARAMS = new voltage_spiking_synapse_parameters_struct();
  voltage_spiking_synapse_parameters_struct * INH_OUT_SYN_PARAMS = new voltage_spiking_synapse_parameters_struct();
  voltage_spiking_synapse_parameters_struct * INPUT_SYN_PARAMS = new voltage_spiking_synapse_parameters_struct();
  // Setting delays
  EXC_OUT_SYN_PARAMS->delay_range[0] = delayval;
  EXC_OUT_SYN_PARAMS->delay_range[1] = delayval;
  INH_OUT_SYN_PARAMS->delay_range[0] = delayval;
  INH_OUT_SYN_PARAMS->delay_range[1] = delayval;
  INPUT_SYN_PARAMS->delay_range[0] = delayval;
  INPUT_SYN_PARAMS->delay_range[1] = delayval;

  // Set Weight Range (in mVs)
  float weight_val = 0.1f*powf(10.0, -3.0);
  float gamma = 5.0f;
  /*
  EXC_OUT_SYN_PARAMS->weight_range[0] = weight_val;
  EXC_OUT_SYN_PARAMS->weight_range[1] = weight_val;
  INH_OUT_SYN_PARAMS->weight_range[0] = -gamma * weight_val;
  INH_OUT_SYN_PARAMS->weight_range[1] = -gamma * weight_val;
  */
  INPUT_SYN_PARAMS->weight_range[0] = weight_val;
  INPUT_SYN_PARAMS->weight_range[1] = weight_val;

  // Biological Scaling factors (ensures that voltage is in mV)
  float weight_multiplier = 1.0f; //powf(10.0, -3.0);
  EXC_OUT_SYN_PARAMS->weight_scaling_constant = weight_multiplier;
  INH_OUT_SYN_PARAMS->weight_scaling_constant = weight_multiplier;
  INPUT_SYN_PARAMS->weight_scaling_constant = weight_multiplier;


  connect_from_mat(
    INHIBITORY_NEURONS[0], EXCITATORY_NEURONS[0],
    INH_OUT_SYN_PARAMS, 
    "../../ie.wmat",
    BenchModel,
    timestep);
  connect_from_mat(
    INHIBITORY_NEURONS[0], INHIBITORY_NEURONS[0],
    INH_OUT_SYN_PARAMS, 
    "../../ii.wmat",
    BenchModel,
    timestep);
  connect_from_mat(
    EXCITATORY_NEURONS[0], INHIBITORY_NEURONS[0],
    EXC_OUT_SYN_PARAMS, 
    "../../ei.wmat",
    BenchModel,
    timestep);

  connect_with_sparsity(
      input_layer_ID, EXCITATORY_NEURONS[0],
      input_neuron_params, EXC_NEURON_PARAMS,
      INPUT_SYN_PARAMS, sparseness,
      BenchModel);
  connect_with_sparsity(
      input_layer_ID, INHIBITORY_NEURONS[0],
      input_neuron_params, INH_NEURON_PARAMS,
      INPUT_SYN_PARAMS, sparseness,
      BenchModel);

  if (plastic)
    EXC_OUT_SYN_PARAMS->plasticity_vec.push_back(weightdependent_stdp);


  int ee_syns = connect_from_mat(
    EXCITATORY_NEURONS[0], EXCITATORY_NEURONS[0],
    EXC_OUT_SYN_PARAMS, 
    "../../ee.wmat",
    BenchModel,
    timestep,
    numsyngroups);
  
  /*
  // Creating Synapse Populations
  connect_with_sparsity(
      EXCITATORY_NEURONS[0], INHIBITORY_NEURONS[0],
      EXC_NEURON_PARAMS, INH_NEURON_PARAMS,
      EXC_OUT_SYN_PARAMS, sparseness,
      BenchModel);
  if (plastic)
    EXC_OUT_SYN_PARAMS->plasticity_vec.push_back(weightdependent_stdp);
  connect_with_sparsity(
      EXCITATORY_NEURONS[0], EXCITATORY_NEURONS[0],
      EXC_NEURON_PARAMS, EXC_NEURON_PARAMS,
      EXC_OUT_SYN_PARAMS, sparseness,
      BenchModel);
  connect_with_sparsity(
      INHIBITORY_NEURONS[0], EXCITATORY_NEURONS[0],
      INH_NEURON_PARAMS, EXC_NEURON_PARAMS,
      INH_OUT_SYN_PARAMS, sparseness,
      BenchModel);
  connect_with_sparsity(
      INHIBITORY_NEURONS[0], INHIBITORY_NEURONS[0],
      INH_NEURON_PARAMS, INH_NEURON_PARAMS,
      INH_OUT_SYN_PARAMS, sparseness,
      BenchModel);
  connect_with_sparsity(
      input_layer_ID, EXCITATORY_NEURONS[0],
      input_neuron_params, EXC_NEURON_PARAMS,
      INPUT_SYN_PARAMS, sparseness,
      BenchModel);
  connect_with_sparsity(
      input_layer_ID, INHIBITORY_NEURONS[0],
      input_neuron_params, INH_NEURON_PARAMS,
      INPUT_SYN_PARAMS, sparseness,
      BenchModel);
    */


  /*
    COMPLETE NETWORK SETUP
  */
  BenchModel->finalise_model();
  if (no_TG)
    BenchModel->timestep_grouping = 1;

  clock_t starttime = clock();
  BenchModel->run(simtime);
  clock_t totaltime = clock() - starttime;
  if ( fast ){
    std::ofstream timefile;
    std::string filename = "timefile.dat";
    timefile.open(filename);
    timefile << std::setprecision(10) << ((float)totaltime / CLOCKS_PER_SEC);
    timefile.close();
  }
  // Dump the weights if we are running in plasticity mode
  if (plastic)
    BenchModel->spiking_synapses->save_connectivity_as_binary("./", "BRUNELPLASTIC_", ee_syns);
  if (!fast){
    spike_monitor->save_spikes_as_binary("./", "BR");
    input_spike_monitor->save_spikes_as_binary("./", "INPUT_BR");
  }
  return(0);
}
