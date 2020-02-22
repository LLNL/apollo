
* Applications are compiled with "-DENABLE\_APOLLO=On"
    * This causes RAJA to use the Apollo forall template
    * That template contains all other templates and can switch between them

# Application and Apollo
1. [JOB] Job script launches
2. [SOS] SOS runtime is launched before Applications
3. [APOLLO] Controller is launched (see it's list below)
4. [APPLICATION] Application starts
5. [APOLLO] Static singleton Apollo::Apollo() class is initialized.
    1. Apollo initializes with SOS
    2. SOS launches a thread to monitor a local socket for feedback
    3. Apollo's Controller will deliver new models using this thread
    4. Apollo client library accesses singleton with Apollo.instance();
6. [APPLICATION] Application runs until it hits RAJA loops.
7. [RAJA] FIRST time any RAJA loop is encountered, the following happens:
    1. Static instance of Apollo::Region() is constructed.
    2. Region is identified by using Callpath library to find module offset.
    3. Default model is attached to Region
        * Can be overridden by environment variable
        * Otherwise uses the baked-in default, Static pol:0
    4. Thread counts are calculated (at present, other things later)
    5. Code now continues into the normal RAJA template activity, below
8. [RAJA] Template body of that region executes:
    1. region.begin() is called
        * Gets start time
    2. Model is evaluated to get policy\_index
    3. apolloPolicySwitcher(body, policy\_index) is called to actually run body
    4. region.end() is called
        * Stores the time for this combination of independent features
9. [APPLICATION] At the end of the simulation step call flushAllRegionMeasurements()
10. [APOLLO] When models are received from the controller, attach them to Regions
11. [APPLICATION] Loop until shutdown.

# Controller (Python, running async w/Application)
1. [CONTROLLER] Register with SOS
2. [CONTROLLER] Sit in a loop waiting for N new steps worth of information
3. [CONTROLLER] IF APOLLO REGION is TRAINING:
    1. Generate DecisionTree classifier
    2. Generate RegressionTree predictor (+ std.dev)
    4. Dispatch models to SOS for delivery to local nodes and application ranks
    5. Update internal database of RegressionTrees to evaluate retrain needs
4. [CONTROLLER] IF APOLLO REGION is NOT TRAINING (is using a learned model):
    1. Plug its data (all or sample) into its RegressionTree predictor.
    2. IF OBSERVED times are w/in X std.dev of prediction, continue.
    3. IF OBSERVED times are NOT w/in X std.dev of PREDICTION:
        * TBD: Make a determination about sending all/some ranks into NEW TRAINING
        * This might mean things go significantly slower, so we want to manage frequency
        * If it keeps happening, possibilities:
            * We are missing an essential independent variable like simulation phase
            * There is some asymptotic aspect to the simulation and this ML technique is not fit
            * Our model is not having an impact on the performance, for some reason
            * Regrids are having major impacts, might want to add ind.feat for distance from regrid
5. Loop back around to step 2
