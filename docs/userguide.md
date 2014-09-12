User guide {#ug}
==========

@brief _cf4ocl_ user guide.

[TOC]

# Overview {#ug_overview}

The C Framework for OpenCL, _cf4ocl_, is a cross-platform pure C99
object-oriented framework for developing and benchmarking OpenCL
projects in C/C++. It aims to:

* Promote the rapid development of OpenCL programs in C/C++.
* Assist in the benchmarking of OpenCL events, such as kernel execution
and data transfers.
* Simplify the analysis of the OpenCL environment and of kernel
requirements.

## Features {#ug_features}

* Object-oriented interface to the OpenCL API
  * New/destroy functions, no direct memory alloc/free
  * Easy (and extensible) device selection
  * Simple event dependency mechanism
  * User-friendly error management
* OpenCL version independent
* Integrated profiling

# Using the library {#ug_libray}

## Basics {#ug_basics}

### Library organization {#ug_forg}

The _cf4ocl_ library offers an object-oriented interface to the OpenCL
API using wrapper classes and methods (or structs and functions, in C
terms), grouped into modules of the same name, as shown in the following
table:

| _cf4ocl_ module                         | _cf4ocl_ wrapper class | Wrapped OpenCL type |
| --------------------------------------- | ---------------------- | ------------------- |
| @ref PLATFORM_WRAPPER "Platform module" | ::CCLPlatform*         | cl_platform_id      |
| @ref DEVICE_WRAPPER "Device module"     | ::CCLDevice*           | cl_device_id        |
| @ref CONTEXT_WRAPPER "Context module"   | ::CCLContext*          | cl_context          |
| @ref QUEUE_WRAPPER "Queue module"       | ::CCLQueue*            | cl_command_queue    |
| @ref PROGRAM_WRAPPER "Program module"   | ::CCLProgram*          | cl_program          |
| @ref KERNEL_WRAPPER "Kernel module"     | ::CCLKernel*           | cl_kernel           |
| @ref EVENT_WRAPPER "Event module"       | ::CCLEvent*            | cl_event            |
| @ref MEMOBJ_WRAPPER "MemObj module"     | ::CCLMemObj*           | cl_mem              |
| @ref BUFFER_WRAPPER "Buffer module"     | ::CCLBuffer*           | cl_mem              |
| @ref IMAGE_WRAPPER "Image module"       | ::CCLImage*            | cl_mem              |
| @ref SAMPLER_WRAPPER "Sampler module"   | ::CCLSampler*          | cl_sampler          |

Some of the provided methods directly wrap OpenCL functions (e.g.
::ccl_buffer_enqueue_copy()), while others perform a number of OpenCL
operations in one function call
(e.g. ::ccl_kernel_set_args_and_enqueue_ndrange()). The wrapper classes
are organized in a hierarchical @ref ug_architecture "inheritance tree".

Additional modules are also available:

| _cf4ocl_ module                                | Description                                                                                        |
| ---------------------------------------------- | -------------------------------------------------------------------------------------------------- |
| @ref DEVICE_SELECTOR "Device selector module"  | Automatically select devices using filters.                                                        |
| @ref DEVICE_QUERY "Device query module"        | Helpers for querying device information, mainly used by the @ref ug_devinfo "ccl_devinfo" program. |
| @ref ERRORS "Errors module"                    | Convert OpenCL error codes into human-readable strings.                                            |
| @ref PLATFORMS "Platforms module"              | Management of the OpencL platforms available in the system.                                        |
| @ref PROFILER "Profiler module"                | Simple, convenient and thorough profiling of OpenCL events.                                        |

### The new/destroy rule {#ug_new_destroy}

The _cf4ocl_ constructors and destructors have `new` and `destroy` in
their name, respectively. In _cf4ocl_, the new/destroy rule states
the following:

_For each invoked constructor, the respective destructor must also be
invoked._

This might seem obvious, but in many instances several objects are
obtained using other (non-constructor) methods during the course of a
program. These objects are automatically released and should not be
destroyed by client code.

For example, it is possible to get a kernel belonging to a program
using the ::ccl_program_get_kernel() function:

~~~~~~~~~~~~~~~{.c}
CCLProgram* prg;
CCLKernel* krnl;
~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~{.c}
prg = ccl_program_new_from_source_file(ctx, "myprog.cl", NULL);
~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~{.c}
krnl = ccl_program_get_kernel(prg, "someKernel", NULL);
~~~~~~~~~~~~~~~

The returned kernel wrapper object will be freed when the program
is destroyed; as such, there is no need to free it. Destroying the program will
suffice:

~~~~~~~~~~~~~~~{.c}
ccl_program_destroy(prg);
~~~~~~~~~~~~~~~

### Getting info about OpenCL objects {#ug_getinfo}

The `ccl_<class>_get_info_<scalar|array>()` macros can be used to get
information about OpenCL objects. Use the `array` version when the
expected return value is a pointer or array, or the `scalar` version
otherwise (e.g. when the expected return value is primitive or scalar
type).

For example, to get the name and the number of compute cores on a
device:

~~~~~~~~~~~~~~~{.c}
CCLDevice* dev;
char* name;
cl_uint n_cores;
~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~{.c}
name = ccl_device_get_info_array(dev, CL_DEVICE_NAME, char*, NULL);
n_cores = ccl_device_get_info_scalar(dev, CL_DEVICE_MAX_COMPUTE_UNITS, cl_uint, NULL);
~~~~~~~~~~~~~~~

The `ccl_<class>_get_info()` macros serve more specific scenarios, and
are likely to be used less often. These macros return a
::CCLWrapperInfo* object, which contains two public fields:

* `value` - A pointer to the requested value as returned by the OpenCL
`clGet*Info()` functions.
* `size` - The size in bytes of the value pointed to by the `value`
field.

To use the value, a cast should be performed on the `value` field to
convert it to the required type (which is what the
`ccl_<class>_get_info_<scalar|array>()` macros automatically do).

The values and objects returned by these macros are automatically
released when the respective wrapper object is destroyed and should
never be directly freed by client code.

### Error handling {#ug_errorhandle}

Error-reporting _cf4ocl_ functions provide two methods for client-side
error handling:

1. The return value.
2. [GError](https://developer.gnome.org/glib/stable/glib-Error-Reporting.html#GError)-based
error-reporting.

The first method consists of analysing the return value of a function.
Error-throwing functions which return a pointer will return `NULL` if
an error occurs. The remaining error-reporting functions return
`CL_FALSE` if an error occurs (or `CL_TRUE` otherwise). Client code can
check for errors by looking for `NULL` or `CL_FALSE` return values,
depending on the function. This error handling method does not provide
additional information about the reported error. For example:

~~~~~~~~~~~~~~~{.c}
CCLContext* ctx;
CCLProgram* prg;
~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~{.c}
prg = ccl_program_new_from_source_file(ctx, "program.cl", NULL);
if (!prg) {
    fprintf(stderr, "An error ocurred");
    exit(-1);
}
~~~~~~~~~~~~~~~

The second method uses the [GLib Error Reporting](https://developer.gnome.org/glib/stable/glib-Error-Reporting.html)
approach and is more flexible. A `GError` object is initialized to
`NULL`, and a pointer to it is passed as the last argument to the
function being called. If the `GError` object is still `NULL` after the
function call, no error has occurred. Otherwise, an error occurred
and it is possible to get a user-friendly error message:

~~~~~~~~~~~~~~~{.c}
CCLContext* ctx;
CCLProgram* prg;
GError* err = NULL;
~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~{.c}
prg = ccl_program_new_from_source_file(ctx, "program.cl", &err);
if (err) {
    fprintf(stderr, "%s", err->message);
    exit(-1);
}
~~~~~~~~~~~~~~~

An error domain and error code are also available in the `GError`
object. The domain indicates the module the error-reporting function is
located in, while the code indicates the specific error that occurred.
Three kinds of domain can be returned by error-reporting _cf4ocl_
functions, each of them associated with distinct error codes:

| Domain          | Codes                                                             | Description                                           |
| --------------- | ----------------------------------------------------------------- | ----------------------------------------------------- |
| ::CCL_ERROR     | ::ccl_error_code enum                                             | Error in _cf4ocl_ not related with external libraries |
| ::CCL_OCL_ERROR | [cl.h](http://www.khronos.org/registry/cl/api/2.0/cl.h)           | Error in OpenCL function calls                        |
| A GLib domain   | [GLib-module](https://developer.gnome.org/glib/stable/) dependent | Error in GLib function call (file open/save, etc)     |

For example, it is possible for client code to act on different OpenCL
errors:

~~~~~~~~~~~~~~~{.c}
CCLContext* ctx;
CCLBuffer* buf;
GError* err = NULL;
~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~{.c}
buf = ccl_buffer_new(ctx, flags, size, host_ptr, &err);
if (err) {
    if (err->domain == CCL_OCL_ERROR) {
        /* Check if it's OpenCL error. */
        switch (err->code) {
            /* Do different things depending on OpenCL error code. */
            case CL_INVALID_VALUE:
                /* Handle invalid values */
            case CL_INVALID_BUFFER_SIZE:
                /* Handle invalid buffer sizes */
            case CL_INVALID_HOST_PTR:
                /* Handle invalid host pointer */

            /* Handle other OpenCL errors */

        }
    } else {
        /* Handle other errors */
    }
}
~~~~~~~~~~~~~~~

Finally, if client code wants to continue execution after an error was
caught, it is mandatory to use the [g_clear_error()] function to free
the error object and reset its value to `NULL`. Not doing so is a bug,
especially if more error-reporting functions are to be called moving
forward. For example:

~~~~~~~~~~~~~~~{.c}
CCLContext* ctx;
CCLProgram* prg;
GError* err = NULL;
~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~{.c}
prg = ccl_program_new_from_source_file(ctx, "program.cl", &err);
if (err) {
    /* Print the error message, but don't terminate program. */
    fprintf(stderr, "%s", err->message);
    g_clear_error(&err);
}
~~~~~~~~~~~~~~~

Even if the program terminates due to an error, the [g_clear_error()]
can be still be called to destroy the error object.

### Log messages {#ug_log}

_cf4ocl_ uses the [GLib message logging framework](https://developer.gnome.org/glib/stable/glib-Message-Logging.html)
to log messages and warnings. _cf4ocl_ log output is handled by
[GLib's default log handler](https://developer.gnome.org/glib/stable/glib-Message-Logging.html#g-log-default-handler),
which outputs warnings and messages to `stderr`. If client code wishes
to redirect this output, it can do so by specifying another
[log function](https://developer.gnome.org/glib/stable/glib-Message-Logging.html#GLogFunc)
for the `cf4ocl2` log domain with
[g_log_set_handler()](https://developer.gnome.org/glib/stable/glib-Message-Logging.html#g-log-set-handler).
For example:

@code{.c}
/* Log function which outputs messages to a stream specified in user_data. */
void my_log_function(const gchar *log_domain, GLogLevelFlags log_level,
	const gchar *message, gpointer user_data) {

	g_fprintf((FILE*) user_data, "[%s](%d)>%s\n",
		log_domain, log_level, message);

}
@endcode
@code{.c}
FILE* my_file;
@endcode
@code{.c}
/* Add log handler for all messages from cf4ocl. */
g_log_set_handler("cf4ocl2", G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
	my_log_function, my_file);
@endcode

## Wrapper modules {#ug_wrappers}

Each [OpenCL class](http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/classDiagram.html)
is associated with a _cf4ocl_ module. At the most basic level, each
module offers a wrapper class and functions which wrap or map their
OpenCL equivalents. This, in itself, already simplifies working with
OpenCL in C, because the _cf4ocl_ wrapper classes internally manages
directly related objects, which may be created during the course of a
program. As such, client code just needs to follow the
@ref ug_new_destroy "new/destroy" rule for directly created objects,
thus not having to worry with memory allocation/deallocation of
intermediate objects.

In most cases, however, each _cf4ocl_ module also provides methods for
other common and not-so-common OpenCL host code patterns, allowing the
programmer to avoid their verbosity and focus on OpenCL device code.

All _cf4ocl_ wrapper classes extend the ::CCLWrapper* abstract
wrapper class. The properties and methods of this class, which are
concerned with reference counts, wrapping/unwrapping of OpenCL objects
and getting object information, are essentially of internal use by other
_cf4ocl_ classes. This functionality is also available to client code
which requires a more advanced integration with _cf4ocl_, as explained
in the @ref ug_cclwrapper "advanced" section.

Several OpenCL objects, namely `cl_platform_id`, `cl_context` and
`cl_program`, have a direct relationship with a set of `cl_device_id`
objects. In order to map this relationship, _cf4ocl_ provides the
::CCLDevContainer* class, which is an intermediate class between the
::CCLWrapper* parent class and the ::CCLPlatform*, ::CCLContext* and
::CCLProgram* wrappers. The ::CCLDevContainer* class implements
functionality for managing a set of ::CCLDevice* wrapper instances.
This functionality is exposed to client code through concrete wrapper
methods. For example, the ::CCLContext* class provides the
::ccl_context_get_all_devices(), ::ccl_context_get_device() and
::ccl_context_get_num_devices() methods for this purpose.

### Platform module {#ug_platform}

@copydoc PLATFORM_WRAPPER

### Device module {#ug_device}

@copydoc DEVICE_WRAPPER

### Context module {#ug_context}

@copydoc CONTEXT_WRAPPER

### Command queue module {#ug_queue}

@copydoc QUEUE_WRAPPER

### Memory object module {#ug_memobj}

@copydoc MEMOBJ_WRAPPER

### Buffer module {#ug_buffer}

@copydoc BUFFER_WRAPPER

### Image module {#ug_image}

@copydoc IMAGE_WRAPPER

### Sampler module {#ug_sampler}

@copydoc SAMPLER_WRAPPER

### Program module {#ug_program}

@copydoc PROGRAM_WRAPPER

### Kernel module {#ug_kernel}

@copydoc KERNEL_WRAPPER

#### Kernel arguments module {#ug_kernel_args}

@copydoc KERNEL_ARG

### Event module {#ug_event}

@copydoc EVENT_WRAPPER

#### Event wait lists module {#ug_event_wait_lists}

@copydoc EVENT_WAIT_LIST

## Other modules {#ug_othermodules}

### Device selector module {#ug_devsel}

@copydoc DEVICE_SELECTOR

### Device query module {#ug_devquery}

@copydoc DEVICE_QUERY

### Errors module {#ug_errors}

@copydoc ERRORS

### Platforms module {#ug_platforms}

@copydoc PLATFORMS

### Profiler module {#ug_profiling}

@copydoc PROFILER

# Using the utilities {#ug_utils}

## ccl_devinfo {#ug_devinfo}

@copydetails devinfo

## ccl_kerninfo {#ug_kerninfo}

@copydetails kerninfo

## ccl_plot_events.py {#ug_plot_events}

@copybrief plot_events.py

@copydetails plot_events.py

# Advanced {#ug_advanced}

## Wrapper architecture {#ug_architecture}

The wrapper classes, which wrap OpenCL types, are implemented using an
object-oriented (OO) approach in C. While C does not directly provide
OO constructs, it is possible to implement features such as inheritance,
polymorphism or encapsulation. Using this approach, _cf4ocl_ is able to
offer a clean and logical class system, while being available in a form
(C) which can be directly or indirectly invoked from other programming
languages.

Each _cf4ocl_ wrapper class is defined by a source (.c) file and a
header (.h) file. The former contains the private class properties and
the method implementations, while the later defines its public API. The
class body is implemented in the source file as a C `struct`; the header
file provides an opaque pointer to it, which is the public side of the
class from a client code perspective. The only exceptions are the
::CCLWrapper* and ::CCLDevContainer* abstract classes, which have the
respective `struct` body publicly available in their header files. This
is required due to the way inheritance is implemented in _cf4ocl_,
i.e., by including a member representing the parent class `struct` in
the body of the child class `struct`. This way, instances of the child
class can be cast to its parent type when required. The child class
`struct` effectively extends the parent class `struct`. An example of
this approach can be shown with the definitions of the abstract
::CCLWrapper* class and of the concrete ::CCLEvent* class, which extends
::CCLWrapper*:

_In abstract_wrapper.h:_
@code{.c}
/* Base class for all OpenCL wrappers. */
typedef struct ccl_wrapper {

	/* The wrapped OpenCL object. */
	void* cl_object;

	/* Information about the wrapped OpenCL object. */
	GHashTable* info;

	/* Reference count. */
	int ref_count;

} CCLWrapper;
@endcode

_In event_wrapper.h:_

@code{.c}
/* Event wrapper class type declaration. */
typedef struct ccl_event CCLEvent;
@endcode

_In event_wrapper.c:_

@code{.c}
/* Event wrapper class, extends CCLWrapper */
struct ccl_event {

	/* Parent wrapper object. */
	CCLWrapper base;

	/* Event name, for profiling purposes only. */
	const char* name;

};
@endcode

The cost to this flexibility is that encapsulation for the two abstract
classes is lost. However, the properties and functionality of these
classes are private, in the sense that they should not be directly
handled by client code. They are not part of the _de jure_ public API,
which minimizes the issue of lost encapsulation, keeping the
architecture simple while guaranteeing an implementation of inheritance
in C.

Methods are implemented as functions which accept the object on which
they operate as the first parameter. When useful, function-like macros
are also used as object methods, such as the case of the
@ref ug_getinfo "info macros". Polymorphism is not used, as the so
called "abstract" methods are just functions which provide common
operations to concrete methods, named differently for each concrete
class. For example, the ::ccl_dev_container_get_device() abstract method
is called by the ::ccl_context_get_device(), ::ccl_platform_get_device()
and ::ccl_program_get_device() concrete methods, for which it provides
common functionality.

The _cf4ocl_ class hierarchy is shown in the following inheritance
diagram:

@dot
digraph cf4ocl {
	rankdir=RL;
	node [shape=record, fontname=Helvetica, fontsize=10];
	wrapper [ label="CCLWrapper*" URL="@ref ccl_wrapper"];
	devcon [ label="CCLDevContainer*" URL="@ref ccl_dev_container"];
	ctx [ label="CCLContext*" URL="@ref ccl_context"];
	prg [ label="CCLProgram*" URL="@ref ccl_program"];
	platf [ label="CCLPlatform*" URL="@ref ccl_platform"];
	memobj [ label="CCLMemObj*" URL="@ref ccl_memobj"];
	buf [ label="CCLBuffer*" URL="@ref ccl_buffer"];
	img [ label="CCLImage*" URL="@ref ccl_image"];
	dev [ label="CCLDevice*" URL="@ref ccl_device"];
	evt [ label="CCLEvent*" URL="@ref ccl_event"];
	krnl [ label="CCLKernel*" URL="@ref ccl_kernel"];
	queue [ label="CCLQueue*" URL="@ref ccl_queue"];
	smplr [ label="CCLSampler*" URL="@ref ccl_sampler"];
	devcon -> wrapper;
	ctx -> devcon;
	prg -> devcon;
	platf -> devcon;
	memobj->wrapper;
	buf -> memobj;
	img -> memobj;
	dev -> wrapper;
	evt -> wrapper;
	krnl -> wrapper;
	queue -> wrapper;
	smplr -> wrapper;
}
@enddot

## The CCLWrapper base class {#ug_cclwrapper}

The ::CCLWrapper* base class holds several low-level responsibilities
for wrapper objects:

* Wrapping/unwrapping of OpenCL objects and maintaining a one-to-one
relationship between wrapped objects and wrapper objects
* Low-level memory management (allocation and deallocation)
* Reference counting
* Information handling (i.e., handling of data returned by the several
`clGet*Info()` OpenCL functions)

Wrapper constructors create the OpenCL object to be wrapped, but
delegate memory allocation to the special `ccl_<class>_new_wrap()`
functions. These accept the OpenCL object, and in turn call the
::ccl_wrapper_new() function, passing it not only the object, but also
the size in bytes of the wrapper to be created. The ::ccl_wrapper_new()
function allocates memory for the wrapper (initializing this memory
to zero), and keeps the OpenCL object (wrapping it) in the created
wrapper instance. For example, the ::ccl_kernel_new() creates the
`cl_kernel` object with the clCreateKernel() OpenCL function, but then
relies on the ::ccl_kernel_new_wrap() function (and thus, on
::ccl_wrapper_new()) for allocation and initialization of the new
::CCLKernel* wrapper object memory.

The destruction of wrapper objects and respective memory deallocation
is performed in a similar fashion. Each wrapper class has its own
`ccl_<class>_destroy()` method, but this method delegates actual
object release to the "abstract" ::ccl_wrapper_unref() function. This
function accepts the wrapper to be destroyed, its size in bytes, and
two function pointers: the first, with prototype defined by
::ccl_wrapper_release_fields(), is a wrapper specific function for
releasing internal wrapper objects, which the super class has no
knowledge of; the second is the OpenCL object destructor function, with
prototype defined by ::ccl_wrapper_release_cl_object(). Continuing on
the kernel example, the ::ccl_kernel_destroy() method delegates kernel
wrapper destruction to ::ccl_wrapper_unref(), passing it the kernel
wrapper object, its size (i.e. `sizeof(` ::CCLKernel `)`), the "private"
(static in C) `ccl_kernel_release_fields()` function for destroying
kernel internal objects, and the clReleaseKernel() OpenCL kernel
destructor function.

As such, all _cf4ocl_ wrapper objects use a common memory allocation and
deallocation strategy, implemented in the ::CCLWrapper* super class.

The `ccl_<class>_new_wrap()` special constructors respect the
@ref ug_new_destroy "new/destroy" rule. Wrappers created with their
special constructor must be released with the respective
`ccl_<class>_destroy()` function. This allows client code to create
OpenCL objects directly with OpenCL functions, and then wrap the objects
to take advantage of _cf4ocl_ functionality and features. The OpenCL
object can be retrieved from its wrapper at all times with the
respective `ccl_<class>_unwrap` method.

If `ccl_<class>_new_wrap()` functions are passed an OpenCL object which
is already wrapped, a new wrapper will not be created. Instead, the
existing wrapper is returned, with its reference count increased by 1.
Thus, there is always a one-to-one relationship between wrapped OpenCL
objects and their respective wrappers. In reality, the
`ccl_<class>_destroy()` functions decreases the reference count of the
respective wrapper, only destroying it if the reference count reaches
zero. Client code can increase and decrease the reference count of a
wrapper object using the associated `ccl_<class>_ref()` and
`ccl_<class>_unref()` macros. The `ccl_<class>_ref()` macros call the
::ccl_wrapper_ref() "abstract" function, casting the wrapper to its
base class (::CCLWrapper*), while the `ccl_<class>_unref()` macros are
just aliases for the respective `ccl_<class>_destroy()` functions.

The ::CCLWrapper* class maintains a static hash table which associates
OpenCL objects (keys) to _cf4ocl_ wrappers (values). Access to this
table is thread-safe and performed by the ::ccl_wrapper_new() and
::ccl_wrapper_unref() functions.

The management of OpenCL object information is also handled by the
::CCLWrapper* class. The ::ccl_wrapper_get_info() method accepts
two wrapper objects, the first being the object to query; the second is
an auxiliary object required by some lower-level OpenCL info functions,
such clGetKernelWorkGroupInfo(), which requires a device object besides
the kernel object. ::ccl_wrapper_get_info() also requires a pointer to
a OpenCL `clGet*Info()` function (with type defined as
::ccl_wrapper_info_fp) in order to perform the desired query.
::ccl_wrapper_get_info() returns a ::CCLWrapperInfo* object, which
contains two public properties: the queried value and its size. To be
useful, the value must be cast to the correct type. The
::ccl_wrapper_get_info_value() and ::ccl_wrapper_get_info_size() methods
call ::ccl_wrapper_get_info(), but directly return the value and size of
the ::CCLWrapper* object, respectively.

The requested information is kept in the information table of the
respective wrapper object. When the wrapper object is destroyed, all the
information objects are also released. As such, client code does not
need to worry about freeing objects returned by the
`ccl_wrapper_get_info*()` methods. These also accept a `use_cache`
boolean argument, which if true, causes the methods to first search
for the information in the wrappers information table, in case it has
already been requested; if not, they proceed with the query as normal.

Client code will usually use the @ref ug_getinfo "info macros" of each
wrapper in order to fetch information about the underlying OpenCL
object. These macros expand into the `ccl_wrapper_get_info*()` methods,
automatically casting objects and values to the appropriate type,
selecting the correct `clGet*Info()` function and setting the cache flag
as appropriate for the object being queried. For example, platform
object queries can always be cached, as the returned information will
never change.

## The CCLDevContainer class {#ug_ccldevcontainer}

The intermediate ::CCLDevContainer* class provides functionality for
managing a set of ::CCLDevice* wrapper instances, abstracting code
common to the ::CCLPlatform*, ::CCLContext* and ::CCLProgram* classes,
all of which internally keep a set of devices. The ::CCLDevContainer*
class contains three "abstract" methods for accessing the associated set
of ::CCLDevice* wrappers, namely:

* ::ccl_dev_container_get_all_devices(): get all ::CCLDevice* wrappers
in device container object.
* ::ccl_dev_container_get_device(): get ::CCLDevice* wrapper at given
index.
* ::ccl_dev_container_get_num_devices(): return number of devices in
device container object.

Client code should use the respective wrapper implementations. For
example, in the case of ::CCLProgram* objects, client code should use
the ::ccl_program_get_all_devices(), ::ccl_program_get_device() and
::ccl_program_get_num_devices() methods to manage the set of devices
associated with the program.

## The CCLMemObj class {#ug_cclmemobj}

The relationship between the ::CCLMemObj* class and the ::CCLBuffer* and
::CCLImage* classes follows that of the respective [OpenCL types](http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/classDiagram.html).
In other words, both OpenCL images and buffers are memory objects with
common functionality, and _cf4ocl_ directly maps this relationship with
the respective wrappers.


[GLib]: https://developer.gnome.org/glib/ "GLib"
[g_clear_error()]: https://developer.gnome.org/glib/stable/glib-Error-Reporting.html#g-clear-error "g_clear_error()"
[OpenCL type]: http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/abstractDataTypes.html "OpenCL types"