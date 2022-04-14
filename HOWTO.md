# Instructions

This documentation provides brief instructions on how to build and run with Apollo,
and how to build RAJA with Apollo support.


## Building Apollo
Apollo uses cmake and has only optional dependencies on other libraries. Available cmake parameters are:

`ENABLE_MPI` enables Apollo distributed training over MPI (default: OFF)

`ENABLE_OpenCV` enables using OpenCV as the ML library backend (default: OFF, Apollo uses its own builtin ML library)

`ENABLE_CUDA` enables event-based timing of CUDA kernels (default OFF: used for timing async launched CUDA kernels)

`ENABLE_HIP` enables event-based timing of HIP kernels in Apollo (default OFF: used for timing async launched HIP kernels)

`BUILD_SHARED_LIBS` builds Apollo as a shared library instead of static (default OFF)

`ENABLE_JIT_DTREE` enables JIT compilation of decision tree evaluation (default OFF: **experimental**,
available only with Apollo builtin ML Library, i.e., when `ENABLE_OPENCV=OFF`)

By default all those flags are OFF and without setting them builds a fully working Apollo runtime library
that can time synchronously executing code regions. When asynchronous CUDA/HIP kernels are used it is required
to set `ENABLE_CUDA` or `ENABLE_HIP` to ON. It is recommended to use the builtin ML library instead of OpenCV
(so keep `ENABLE_OPENCV=OFF`)

Example build commands for HIP async timing:

```
cd <apollo repo root>
mkdir -p build
pushd build
cmake .. -DENABLE_HIP=on (enables HIP async timing) -DCMAKE_PREFIX_PATH=./install
make -j install
popd
```


## Building RAJA

The current version of Apollo-enabled RAJA is at:

https://github.com/ggeorgakoudis/RAJA/tree/feature/apollo

Buiding RAJA with Apollo support requires to provide the following cmake parameters:

`-DENABLE_APOLLO=ON -DAPOLLO_DIR=<apollo repo root>/build/install`

## Running with Apollo

### Instrumentation

Users must instrument tunable code regions using the Apollo API (TODO documentation) to enable tuning through Apollo.

Users of Apollo through RAJA avoid most of the instrumentation as it is is implemented internally to RAJA using Apollo multi-policy interfaces.
For tuning, Apollo requires collecting of training data through exploration on available policies and instructing Apollo to _train_ a tuning model
once training data are sufficient. What is sufficient training data is application-specific. There are two ways to instruct Apollo to
train a model:

1. Programmatically by calling the `Apollo::train(int epoch)` method in the application source code (encouraged), which will use collected training data
up to that point and create a tuning model through Apollo.
The parameter `epoch` is used for tracing in Apollo, hence it carries no special meaning for the application.
If tracing is not desired setting it always to 0 is recommended.

#### Example (RAJA)
```
...
using PolicyList = camp::list<
    RAJA::hip_exec<64>,
    RAJA::hip_exec<256>,
    RAJA::hip_exec<1024>>;

// Get the handle to the apollo runtime
Apollo *apollo = Apollo::instance()
for(int i=1; i<=ITERS; i++) {
...
  RAJA::forall<RAJA::apollo_multi_exec<PolicyList>>(
      RAJA::RangeSegment(0, test_size),
    [=] RAJA_DEVICE (int i) {
    a[i] += b[i] * c;
  });

  // Train every 100 iterations.
  if(i%100)
    apollo->train(0);
...
}
```

2. At runtime using the `APOLLO_GLOBAL_TRAIN_PERIOD=<#>` env var that instructs Apollo to train a tuning model every `<#>` tunable region executions.
#### Example
`$ APOLLO_GLOBAL_TRAIN_PERIOD=100 APOLLO_POLICY_MODEL=DecisionTree,explore=RoundRobin,max_depth=4 <executable>`

This instructs Apollo train every 100 region executions using a DecisionTree model. The next section describes how
to set policy selection models using the `APOLLO_POLICY_MODEL` env var.

---

Apollo uses an env var to set the policy selection model:

`APOLLO_POLICY_MODEL`

It sets the policy selection model given as a string of the policy name followed by comma separated key-value pair parameters that specify the model:

### Static
Always select the same fixed policy without exploration or tuning

#### Parameters
`policy=<#>` set the fixed policy number to select, default 0

#### Example
`$ APOLLO_POLICY_MODEL=Static,policy=1 <executable>` (runs selecting always policy 1)

### RoundRobin
Selects policies cyclically
#### Parameters
None

#### Example
`$ APOLLO_POLICY_MODEL=RoundRobin <executable>` (runs cycling through policies)

### Random
Selects policies uniformly randomly
#### Parameters
None

#### Example
`$ APOLLO_POLICY_MODEL=Random <executable>` (runs selecting policy at random)

### DecisionTree
Select policies by training a decision tree classification tuning model
### Parameters
`explore=(RoundRobin|Random)` set the model used for exploration collecting training data

`max_depth=<#>` set the maximum depth of created decision trees (default: 2)

`load` load a previously trained model (see later on Apollo model storing)
#### Example
`$ APOLLO_POLICY_MODEL=DecisionTree,explore=RoundRobin,max_depth=4 <executable>`
(runs using RoundRobin for exploration and trains a DecisionTree model of depth 4)

### RandomForest
Select policies using a random forest classification tuning model

#### Parameters
`explore=(RoundRobin|Random)` set the model used for exploration collecting training data

`num_tress=<#>` the number of trees in the forest (default: 10)

`max_depth=<#>` set the maximum depth of created decision trees (default: 2)

`load` load a previously trained model (see later on Apollo model storing)

#### Example
`$ APOLLO_POLICY_MODEL=RandomForest,explore=RoundRobin,num_trees=20,max_depth=4 <executable>`
(runs using RoundRobin for exploration and trains a RandomForest model of 20 decision trees each of maximum depth 4)

### PolicyNet
Select policies using a reinforced learning policy network model
#### Parameters
`lr=<#.#>` learning rate for the policy network (default: 0.01)

`beta=<#.#>` reward moving average damping factor (default: 0.5)

`beta1=<#.#>` Adam optimizer beta1 first moment damping factor (default: 0.5)

`beta2=<#.#>` Adam optimizer beta2 second moment damping factor (default: 0.9)

`load` loads a previously trained model (see later on Apollo model storing)

By default Apollo executes an applications with a Static model always choosing policy 0 so no
exploration or tuning is performed. The `APOLLO_POLICY_MODEL` env var must be set to enable tuning.
An example of overriding that for a DecisionTree tuning model of depth 4 with RoundRobin exploration is:

`$ APOLLO_POLICY_MODEL=DecisionTree,explore=RoundRobin,max_depth=4 <executable>`

---

### Storing and loading models
Apollo can save trained models and load them to bootstrap tuned execution without exploration by setting this env var:

`APOLLO_STORE_MODELS=1`

Apollo stores model files under the path `.apollo/models` in the current executing directory and will load those models when a tuning policy
(DecisionTree, RandomForest, PolicyNet) is given the parameter `load` through `APOLLO_POLICY_MODEL` and
the application executable is ran from the directory containing the previously stored model files.

### Example
(First run: create models)
```
$ APOLLO_STORE_MODELS=1 APOLLO_POLICY_MODEL=DecisionTree,explore=RoundRobin,max_depth=4 <executable>`
```
(Second run: load previously stored models)
```
$ APOLLO_POLICY_MODEL=DecisionTree,load <executable>`
```

---

### Tracing

Apollo provides a CSV trace of execution (not intended to be enabled for production runs) capturing region execution and timing information setting this env var:

`APOLLO_TRACE_CSV=1`

Trace files are store under the path `.apollo/traces` in the current executing directory.


