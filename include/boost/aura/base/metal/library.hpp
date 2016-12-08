#pragma once

#include <boost/aura/base/alang.hpp>
#include <boost/aura/base/metal/alang.hpp>
#include <boost/aura/base/metal/device.hpp>
#include <boost/aura/base/metal/safecall.hpp>
#include <boost/aura/io.hpp>

#include <iostream>

namespace boost
{
namespace aura
{
namespace base_detail
{
namespace metal
{


class library
{
public:
        /// Create empty library.
        inline explicit library()
                : device_(nullptr)
        {
        }

        /// Prevent copies.
        library(const library&) = delete;
        void operator=(const library&) = delete;

        /// Create library from string.
        inline explicit library(
                const std::string& kernelstring,
                device& d,
                bool inject_aura_preamble = true,
                const std::string& options = "")
                : device_(&d)
        {
                create_from_string(kernelstring, options, inject_aura_preamble);
        }

        /// Create library from file.
        inline explicit library(
                boost::aura::path p,
                device& d,
                bool inject_aura_preamble = true,
                const std::string& options = "")
                : device_(&d)
        {
                auto kernelstring = boost::aura::read_all(p);
                create_from_string(kernelstring, options, inject_aura_preamble);
        }


        /// Access device.
        const device& get_device() { return *device_; }

        /// Access library.
        id<MTLLibrary> get_base_library() { return library_; }

        const id<MTLLibrary> get_base_library() const { return library_; }

        ~library() { finalize(); }

private:
        /// Create a library from a string.
        void create_from_string(
                const std::string& kernelstring,
                const std::string& opt,
                bool inject_aura_preamble)
        {
                shared_alang_header salh;
                alang_header alh;

                std::string kernelstring_with_preamble = kernelstring;
                if (inject_aura_preamble)
                {
                        // Prepend AURA define to kernel.
                        kernelstring_with_preamble =
                                std::string("#define AURA_BASE_METAL\n") +
                                salh.get() +
                                std::string("\n") +
                                alh.get() +
                                std::string("\n") +
                                kernelstring_with_preamble;
                }

                NSError* err;
                library_ = [device_->get_base_device()
                        newLibraryWithSource:
                                [NSString stringWithUTF8String:
                                                  kernelstring_with_preamble
                                                          .c_str()]
                                     options:nil
                                       error:&err];

                if (!library_)
                {
                        NSLog(@"Error: %@ %@", err, [err userInfo]);
                }
                AURA_METAL_CHECK_ERROR(library_);
        }

        /// Finalize object.
        void finalize()
        {
                if (device_ != nullptr)
                {
                        library_ = nil;
                }
        }

        /// Pointer to device the feed was created for
        device* device_;

        /// Library
        id<MTLLibrary> library_;

        /// Library compile log
        std::string log_;
};


} // metal
} // base_detail
} // aura
} // boost
