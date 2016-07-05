#define BOOST_TEST_MODULE multi_comp_units
#include <boost/test/unit_test.hpp>

#include <boost/aura/copy.hpp>
#include <boost/aura/device.hpp>
#include <boost/aura/device_ptr.hpp>
#include <boost/aura/environment.hpp>
#include <boost/aura/feed.hpp>
#include <boost/aura/invoke.hpp>
#include <boost/aura/kernel.hpp>
#include <boost/aura/library.hpp>

#include <test/test.hpp>


BOOST_AUTO_TEST_CASE(use_in_this_file)
{
        boost::aura::initialize();
        {
                boost::aura::device d(AURA_UNIT_TEST_DEVICE);
                boost::aura::feed f(d);
                boost::aura::library l(
                        boost::aura::path(boost::aura::test::get_test_dir() +
                                "/kernels.al"),
                        d);
                boost::aura::kernel k("add", l);
                const std::size_t num_el = 128;

                std::vector<float> a(num_el, 2.0f);
                std::vector<float> b(num_el, 3.0f);
                std::vector<float> c(num_el, 0.0f);
                std::vector<float> expected(num_el, 2.0f + 3.0f);

                boost::aura::device_array<float> a_device(num_el, d);
                boost::aura::device_array<float> b_device(num_el, d);
                boost::aura::device_array<float> c_device(num_el, d);

                boost::aura::copy(a.begin(), a.end(), a_device.begin(), f);
                boost::aura::copy(b.begin(), b.end(), b_device.begin(), f);
                boost::aura::copy(c.begin(), c.end(), c_device.begin(), f);

                boost::aura::invoke(k, boost::aura::mesh({128, 1, 1}),
                        boost::aura::bundle({
                                1, 1, 1,
                        }),
                        boost::aura::args(a_device.get_base_ptr(),
                                            b_device.get_base_ptr(),
                                            c_device.get_base_ptr()),
                        f);

                boost::aura::copy(
                        c_device.begin(), c_device.end(), c.begin(), f);
                boost::aura::wait_for(f);

                BOOST_CHECK(std::equal(
                        expected.begin(), expected.end(), c.begin()));

                if (c_device.is_shared_memory())
                {
                        BOOST_CHECK(std::equal(expected.begin(), expected.end(),
                                c_device.get_host_ptr()));
                }
        }
        boost::aura::finalize();
}

void test_multi_comp_units2();
BOOST_AUTO_TEST_CASE(use_in_other_file) { test_multi_comp_units2(); }
