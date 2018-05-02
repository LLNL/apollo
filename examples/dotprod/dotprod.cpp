  RAJA::ReduceSum<RAJA::seq_reduce, double> seqdot(0.0);

  RAJA::forall<RAJA::seq_exec>(RAJA::RangeSegment(0, N), [=] (int i) { 
    seqdot += a[i] * b[i]; 
  });

  dot = seqdot.get();
  std::cout << "\t (a, b) = " << dot << std::endl;
