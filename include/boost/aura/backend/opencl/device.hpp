
#ifndef AURA_BACKEND_OPENCL_DEVICE_HPP
#define AURA_BACKEND_OPENCL_DEVICE_HPP

#include <fstream>
#include <cstddef>
#include <cstring>
#include <unordered_map>
#include <boost/move/move.hpp>
#include <vector>
#ifdef __APPLE__
	#include "OpenCL/opencl.h"
#else
	#include "CL/cl.h"
#endif
#include <boost/aura/backend/shared/call.hpp>
#include <boost/aura/backend/opencl/call.hpp>
#include <boost/aura/backend/opencl/context.hpp>
#include <boost/aura/misc/deprecate.hpp>
#include <boost/aura/device_lock.hpp>
#include <boost/aura/backend/opencl/module.hpp>
namespace boost {
namespace aura {
namespace backend_detail {
namespace opencl {

class device;
class module;

/// kernel handle
typedef cl_kernel kernel;



module create_module_from_file(const char * filename, device & d,
		const char * build_options);

kernel create_kernel(module& m, const char * kernel_name);


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
	inline device(int ordinal) :
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
				create_module_from_string(fcontent.c_str(),
						*this, build_options)));
			it = it2.first;
		}
		return it->second.get_kernel(kernel_name);
	}

	/// load a kernel from a string
	kernel load_from_string(const char* kernel_name,
			const char* kernel_string,
			const char* build_options=NULL, bool debug=false)
	{
		auto it = modules_.find(kernel_string);
		if (modules_.end() == it) {
			auto it2 = modules_.insert(
					std::move(std::make_pair(kernel_string,
						std::move(module(kernel_string, *this,
							build_options)))));
			it = it2.first;
		}

		return it->second.get_kernel(kernel_name);
	}

	/// make device active
	inline void set() const {}

	/// undo make device active
	inline void unset() const {}

	/// pin
	inline void pin() {}

	/// unpin
	inline void unpin() {}

	/// access the backend device handle
	inline const cl_device_id & get_backend_device() const
	{
		return context_->get_backend_device();
	}

	/// access the backend context handle
	inline const cl_context & get_backend_context() const
	{
		return context_->get_backend_context();
	}

	/// access the context handle
	inline detail::context * get_context()
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
inline int device_get_count()
{
	// get platforms
	unsigned int num_platforms = 0;
	AURA_OPENCL_SAFE_CALL(clGetPlatformIDs(0, 0, &num_platforms));
	std::vector<cl_platform_id> platforms(num_platforms);
	AURA_OPENCL_SAFE_CALL(clGetPlatformIDs(num_platforms,
				&platforms[0], 0));

	// find device
	int num_devices = 0;
	for(unsigned int i=0; i<num_platforms; i++) {
		unsigned int num_devices_platform = 0;
		AURA_OPENCL_SAFE_CALL(clGetDeviceIDs(platforms[i],
					CL_DEVICE_TYPE_ALL, 0, 0,
					&num_devices_platform));
		num_devices += num_devices_platform;
	}
	return num_devices;
}

/**
 * print system info to stdout
 */
inline void print_system_info()
{
	// get platforms
	unsigned int num_platforms = 0;
	AURA_OPENCL_SAFE_CALL(clGetPlatformIDs(0, 0, &num_platforms));
	std::vector<cl_platform_id> platforms(num_platforms);
	AURA_OPENCL_SAFE_CALL(clGetPlatformIDs(
				num_platforms, &platforms[0], 0));

	unsigned int num = 0;

	// find device
	for(unsigned int i=0; i<num_platforms; i++) {
		unsigned int num_devices_platform = 0;
		AURA_OPENCL_SAFE_CALL(
				clGetDeviceIDs(platforms[i],
					CL_DEVICE_TYPE_ALL,
					0, 0, &num_devices_platform));
		std::vector<cl_device_id> devices;
		devices.reserve(num_devices_platform);

		AURA_OPENCL_SAFE_CALL(
				clGetDeviceIDs(platforms[i],
					CL_DEVICE_TYPE_ALL,
					num_devices_platform,
					&devices[0], NULL
					)
				);

		for(unsigned int j=0; j<num_devices_platform; j++) {
			std::size_t namelen;
			AURA_OPENCL_SAFE_CALL(clGetDeviceInfo(devices[j],
						CL_DEVICE_NAME, 0, NULL,
						&namelen));
			std::vector<char> name;
			name.reserve(namelen);
			AURA_OPENCL_SAFE_CALL(clGetDeviceInfo(
						devices[j], CL_DEVICE_NAME,
						namelen, &name[0], NULL));
			printf("platform %u device %u (ordinal %u): %s\n",
					i, j, num, &name[0]);
			num++;
		}
	}
}
inline void print_device_info() {
print_system_info();
}
DEPRECATED(void print_device_info());


#include <boost/aura/backend/shared/device_info.hpp>

/// return the device info
inline device_info device_get_info(device & d)
{
	device_info di;

	// name and vendor
	std::size_t len;
	std::vector<char> buf;
	AURA_OPENCL_SAFE_CALL(clGetDeviceInfo(d.get_backend_device(),
	CL_DEVICE_NAME, 0, NULL, &len));
	buf.reserve(len);
	AURA_OPENCL_SAFE_CALL(clGetDeviceInfo(d.get_backend_device(),
	CL_DEVICE_NAME, len, &buf[0], NULL));
	strncpy(di.name, &buf[0], sizeof(di.name)-1);
	AURA_OPENCL_SAFE_CALL(clGetDeviceInfo(d.get_backend_device(),
	CL_DEVICE_VENDOR, 0, NULL, &len));
	buf.reserve(len);
	AURA_OPENCL_SAFE_CALL(clGetDeviceInfo(d.get_backend_device(),
	CL_DEVICE_VENDOR, len, &buf[0], NULL));
	strncpy(di.vendor, &buf[0], sizeof(di.name)-1);

	// mesh
	cl_uint dims;
	AURA_OPENCL_SAFE_CALL(clGetDeviceInfo(d.get_backend_device(),
	CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(dims), &dims, NULL));
	size_t sizes[dims];
	AURA_OPENCL_SAFE_CALL(clGetDeviceInfo(d.get_backend_device(),
	CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t)*dims, sizes, NULL));
	assert(dims <= AURA_MAX_BUNDLE_DIMS);
	for(cl_uint i=0; i<dims; i++) {
		di.max_mesh.push_back(sizes[i]);
	}

	// bundle
	di.max_bundle = di.max_mesh;

	// fibers in bundle
	size_t wgs;
	AURA_OPENCL_SAFE_CALL(clGetDeviceInfo(d.get_backend_device(),
	CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(wgs), &wgs, NULL));
	di.max_fibers_per_bundle = wgs;
	return di;
}

inline device create_device_exclusive()
{
	int num = device_get_count();
	for (int n=0; n<device_get_count(); n++) {
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

inline const cl_device_id & get_backend_device(const device & d)
{
	return d.get_backend_device();
}
inline const cl_context & get_backend_context(const device & d)
{
	return d.get_backend_context();
}

} // opencl
} // backend_detail
} // aura
} // boost

#endif // AURA_BACKEND_OPENCL_DEVICE_HPP

