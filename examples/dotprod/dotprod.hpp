#ifndef APOLLO_EXAMPLES_DOTPROD_HPP
#define APOLLO_EXAMPLES_DOTPROD_HPP

double random_double(double minval, double maxval);


// template<typename Numeric, typename Generator = std::mt19937>
// Numeric random(Numeric from, Numeric to)
// {
//     thread_local static Generator gen(std::random_device{}());
// 
//     using dist_type = typename std::conditional
//     <
//         std::is_integral<Numeric>::value
//         , std::uniform_int_distribution<Numeric>
//         , std::uniform_real_distribution<Numeric>
//     >::type;
// 
//     thread_local static dist_type dist;
// 
//     return dist(gen, typename dist_type::param_type{from, to});
// }
// 

#endif



