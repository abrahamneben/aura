#define BOOST_TEST_MODULE library
#include <boost/test/unit_test.hpp>

#include <boost/aura/device.hpp>
#include <boost/aura/environment.hpp>
#include <boost/aura/library.hpp>

#include <test/test.hpp>

#include <iostream>


// _____________________________________________________________________________

BOOST_AUTO_TEST_CASE(basic_library)
{
        boost::aura::initialize();
        {
                boost::aura::device d(AURA_UNIT_TEST_DEVICE);
                boost::aura::library l;
        }
        boost::aura::finalize();
}

BOOST_AUTO_TEST_CASE(basic_library_from_string)
{
        boost::aura::initialize();
        {
                boost::aura::device d(AURA_UNIT_TEST_DEVICE);

                std::string kernelstring(R"(#ifdef AURA_BASE_CUDA
                __global__ void  add(int *a, int *b, int *c )
                {
                        int tid = blockIdx.x;
                        c[tid] = a[tid] + b[tid];
                }
                #endif
                #ifdef AURA_BASE_OPENCL
                __kernel void  add
                        (__global int *a, __global int *b, __global int *c)
                {
                        int tid = get_global_id(0);
                        c[tid] = a[tid] + b[tid];
                }
                #endif)"
                                         "\n");
                boost::aura::library l(kernelstring, d);
        }
        boost::aura::finalize();
}

BOOST_AUTO_TEST_CASE(basic_library_from_file)
{
        boost::aura::initialize();
        {
                boost::aura::device d(AURA_UNIT_TEST_DEVICE);
                boost::aura::library l(
                        boost::aura::path(boost::aura::test::get_test_dir() +
                                "/kernels.al"),
                        d);
        }
        boost::aura::finalize();
}
