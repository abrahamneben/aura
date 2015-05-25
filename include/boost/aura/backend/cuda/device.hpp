#ifndef AURA_BACKEND_CUDA_DEVICE_HPP
#define AURA_BACKEND_CUDA_DEVICE_HPP

#include <fstream>
#include <cstddef>
#include <cstring>
#include <unordered_map>
#include <boost/move/move.hpp>
#include <vector>
#include <cuda.h>
#include <boost/aura/backend/cuda/call.hpp>
#include <boost/aura/backend/shared/call.hpp>
#include <boost/aura/backend/cuda/module.hpp>
#include <boost/aura/backend/cuda/context.hpp>
#include <boost/aura/misc/deprecate.hpp>
#include <boost/aura/device_lock.hpp>

namespace boost {
namespace aura {
namespace backend_detail {
namespace cuda {

class device;
class module;

/// kernel handle
typedef CUfunction kernel;

module create_module_from_string(const char * str, device & d,
		const char * build_options);

module create_module_from_file(const char * filename, device & d,
		const char * build_options);

kernel create_kernel(module m, const char * kernel_name);

void print_module_build_log(module & m, const device & d);


/**
 * device class
 *
 * every interaction with devices starts from this class
 */
class device
{

private:
	BOOST_MOVABLE_BUT_NOT_COPYABLE(device)

public:

	/// create empty device object without device and context
	inline explicit device() : context_(nullptr) {}

	/**
	 * create device form ordinal, also creates a context
	 *
	 * @param ordinal device number
	 */
	inline explicit device(std::size_t ordinal) :
		context_(new detail::context(ordinal)),
		ordinal_(ordinal)
	{}

	/**
	 * create device form ordinal with lock
	 *
	 * @param ordinal device number
	 * @param device_lock a device lock object
	 */
	inline explicit device(std::size_t ordinal, device_lock& dl) :
		context_(new detail::context(ordinal)),
		ordinal_(ordinal),
		device_lock_(dl)
	{}

	/// destroy device
	inline ~device()
	{
		finalize();
	}

	/**
	 * move constructor, move device information here, invalidate other
	 *
	 * @param d device to move here
	 */
	device(BOOST_RV_REF(device) d) :
		context_(d.context_),
		ordinal_(d.ordinal_),
		modules_(std::move(d.modules_)),
		device_lock_(std::move(d.device_lock_))
	{
		d.context_ = nullptr;
		d.modules_.clear();
	}

	/**
	 * move assignment, move device information here, invalidate other
	 *
	 * @param d device to move here
	 */
	device& operator=(BOOST_RV_REF(device) d)
	{
		finalize();
		context_ = d.context_;
		ordinal_ = d.ordinal_;
		modules_ = std::move(d.modules_);
		device_lock_ = std::move(d.device_lock_);
		d.context_ = nullptr;
		d.modules_.clear();
		return *this;
	}

	/// load a kernel from a file
	kernel load_from_file(const char* kernel_name,
			const char* file_name,
			const char* build_options=NULL)
	{
		auto it = modules_.find(file_name);
		if (modules_.end() == it) {
			auto it2 = modules_.insert(std::make_pair(file_name,
				create_module_from_file(file_name,
						*this, build_options)));
			it = it2.first;
		}
		return it->second.get_kernel(kernel_name);
	}

	/// load a kernel from a string
	kernel load_from_string(const char* kernel_name,
			const char* kernel_string,
			const char* build_options=NULL)
	{
		auto it = modules_.find(kernel_string);
		if (modules_.end() == it)
		{
			auto it2 = modules_.insert(
					std::move(std::make_pair(kernel_string,
						create_module_from_string(
							kernel_string, *this,
							build_options))));
			it = it2.first;
		}

		return it->second.get_kernel(kernel_name);
	}

	/// make device active
	inline void set()
	{
		context_->set();
	}

	/// undo make device active
	inline void unset()
	{
		context_->unset();
	}

	/**
	 * pin
	 *
	 * disable unset, device context stays associated with current thread
	 * usefull for interoperability with other libraries that use a context
	 * explicitly
	 */
	inline void pin()
	{
		context_->pin();
	}

	/// unpin (reenable unset)
	inline void unpin()
	{
		context_->unpin();
	}

	/// access the device handle
	inline const CUdevice & get_backend_device() const
	{
		return context_->get_backend_device();
	}

	/// access the context handle
	inline const CUcontext & get_backend_context() const
	{
		return context_->get_backend_context();
	}

	/// access the context handle
	inline detail::context* get_context()
	{
		return context_;
	}

	/// access the device ordinal
	inline std::size_t get_ordinal() const
	{
		return ordinal_;
	}

	/// compare devices
	inline bool operator==(const device& d) const
	{
		return (context_ == d.context_ && ordinal_ == d.ordinal_);
	}

	/// compare devices
	inline bool operator!=(const device& d) const
	{
		return !(*this == d);
	}

private:
	/// finalize object (called from dtor and move assign)
	void finalize()
	{
		if(nullptr != context_) {
			delete context_;
		}
	}

private:
	/// context handle
	detail::context * context_;

	/// device ordinal
	std::size_t ordinal_;

	/// modules
	std::unordered_map<std::string, module> modules_;

	/// device_lock
	device_lock device_lock_;
};


/**
 * get number of devices available
 *
 * @return number of devices
 */
inline std::size_t device_get_count()
{
	int num_devices;
	AURA_CUDA_SAFE_CALL(cuDeviceGetCount(&num_devices));
	return (std::size_t)num_devices;
}

/// print device info to stdout
inline void print_system_info()
{
	for(std::size_t n=0; n<device_get_count(); n++) {
		CUdevice device;
		AURA_CUDA_SAFE_CALL(cuDeviceGet(&device, n));
		char device_name[400];
		AURA_CUDA_SAFE_CALL(cuDeviceGetName(device_name, 400, device));
		printf("%lu: %s\n", n, device_name);
	}
}

inline void print_device_info()
{
	print_system_info();
}

DEPRECATED(void print_device_info());

/// return the amount of free memory on the device
inline std::size_t device_get_free_memory(device &d)
{
	std::size_t free;
	std::size_t total;
	d.set();
	AURA_CUDA_SAFE_CALL(cuMemGetInfo(&free, &total));
	d.unset();
	return free;
}

/// return the amount of total memory on the device
inline std::size_t device_get_total_memory(device &d)
{
	std::size_t free;
	std::size_t total;
	d.set();
	AURA_CUDA_SAFE_CALL(cuMemGetInfo(&free, &total));
	d.unset();
	return total;
}

#include <boost/aura/backend/shared/device_info.hpp>

/// return the device info
inline device_info device_get_info(device & d)
{
	device_info di;
	// name and vendor
	AURA_CUDA_SAFE_CALL(cuDeviceGetName(di.name, sizeof(di.name)-1,
	d.get_backend_device()));
	strncpy(di.vendor, "Nvidia", sizeof(di.vendor)-1);

	// mesh
	int r=0;
	AURA_CUDA_SAFE_CALL(cuDeviceGetAttribute(&r,
	CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_X, d.get_backend_device()));
	di.max_mesh.push_back(r);
	AURA_CUDA_SAFE_CALL(cuDeviceGetAttribute(&r,
	CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_Y, d.get_backend_device()));
	di.max_mesh.push_back(r);
	AURA_CUDA_SAFE_CALL(cuDeviceGetAttribute(&r,
	CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_Z, d.get_backend_device()));
	di.max_mesh.push_back(r);

	// bundle
	AURA_CUDA_SAFE_CALL(cuDeviceGetAttribute(&r,
	CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_X, d.get_backend_device()));
	di.max_bundle.push_back(r);
	AURA_CUDA_SAFE_CALL(cuDeviceGetAttribute(&r,
	CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_Y, d.get_backend_device()));
	di.max_bundle.push_back(r);
	AURA_CUDA_SAFE_CALL(cuDeviceGetAttribute(&r,
	CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_Z, d.get_backend_device()));
	di.max_bundle.push_back(r);

	// fibers in bundle
	AURA_CUDA_SAFE_CALL(cuDeviceGetAttribute(&r,
	CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_BLOCK, d.get_backend_device()));
	di.max_fibers_per_bundle = r;
	return di;
}

inline device create_device_exclusive()
{
	int num = device_get_count();
	for (std::size_t n=0; n<device_get_count(); n++) {
		auto dl = create_device_lock(num);
		if (dl) {
			return device(n, dl);
		}
	}
	throw "no device available!";
}


inline kernel create_kernel(module& m, const char * kernel_name)
{
	return m.get_kernel(kernel_name);
}

inline const CUdevice& get_backend_device(const device & d)
{
	return d.get_backend_device();
}
inline const CUdevice& get_backend_context(device & d)
{
	return d.get_context()->get_backend_device();
}

inline context* get_contex(device& d)
{
	return d.context;
}

inline void set(device& d)
{
	d.set();
}

inline void unset(device& d)
{
	d.unset();
}

} // cuda
} // backend_detail
} // aura
} // boost

#endif // AURA_BACKEND_CUDA_DEVICE_HPP

