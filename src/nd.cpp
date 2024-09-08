#include "nd.h"

#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "xtensor/xtensor.hpp"
#include "xtensor/xadapt.hpp"
#include "xtensor/xoperation.hpp"

#include "conversion_array.h"
#include "conversion_shape.h"
#include "conversion_slice.h"
#include "conversion_axes.h"
#include "xtv.h"
#include "ndarray.h"
#include "ndrange.h"

using namespace godot;
using namespace xtv;

void nd::_bind_methods() {
	BIND_ENUM_CONSTANT(Float64);
	BIND_ENUM_CONSTANT(Float32);
	BIND_ENUM_CONSTANT(Int8);
	BIND_ENUM_CONSTANT(Int16);
	BIND_ENUM_CONSTANT(Int32);
	BIND_ENUM_CONSTANT(Int64);
	BIND_ENUM_CONSTANT(UInt8);
	BIND_ENUM_CONSTANT(UInt16);
	BIND_ENUM_CONSTANT(UInt32);
	BIND_ENUM_CONSTANT(UInt64);

	godot::ClassDB::bind_static_method("nd", D_METHOD("newaxis"), &nd::newaxis);
	godot::ClassDB::bind_static_method("nd", D_METHOD("from", "start"), &nd::from);
	godot::ClassDB::bind_static_method("nd", D_METHOD("to", "stop"), &nd::to);
	godot::ClassDB::bind_static_method("nd", D_METHOD("range", "start_or_stop", "stop", "step"), &nd::range, static_cast<int64_t>(0), DEFVAL(nullptr), static_cast<int64_t>(1));

	godot::ClassDB::bind_static_method("nd", D_METHOD("dtype", "array"), &nd::dtype);
	godot::ClassDB::bind_static_method("nd", D_METHOD("size_of_dtype_in_bytes", "dtype"), &nd::size_of_dtype_in_bytes);
	godot::ClassDB::bind_static_method("nd", D_METHOD("shape", "array"), &nd::shape);
	godot::ClassDB::bind_static_method("nd", D_METHOD("size", "array"), &nd::size);
	godot::ClassDB::bind_static_method("nd", D_METHOD("ndim", "array"), &nd::ndim);

	godot::ClassDB::bind_static_method("nd", D_METHOD("as_type", "array", "dtype"), &nd::as_type);

	godot::ClassDB::bind_static_method("nd", D_METHOD("as_array", "array", "dtype"), &nd::as_array, DEFVAL(nullptr), DEFVAL(nd::DType::DTypeMax));
	godot::ClassDB::bind_static_method("nd", D_METHOD("array", "array", "dtype"), &nd::array, DEFVAL(nullptr), DEFVAL(nd::DType::DTypeMax));

	godot::ClassDB::bind_static_method("nd", D_METHOD("empty", "shape", "dtype"), &nd::empty, DEFVAL(nullptr), DEFVAL(nd::DType::Float64));
	godot::ClassDB::bind_static_method("nd", D_METHOD("full", "shape", "fill_value", "dtype"), &nd::full, DEFVAL(nullptr), DEFVAL(nullptr), DEFVAL(nd::DType::Float64));
	godot::ClassDB::bind_static_method("nd", D_METHOD("zeros", "shape", "dtype"), &nd::zeros, DEFVAL(nullptr), DEFVAL(nd::DType::Float64));
	godot::ClassDB::bind_static_method("nd", D_METHOD("ones", "shape", "dtype"), &nd::ones, DEFVAL(nullptr), DEFVAL(nd::DType::Float64));

	godot::ClassDB::bind_static_method("nd", D_METHOD("add", "a", "b"), &nd::add);
	godot::ClassDB::bind_static_method("nd", D_METHOD("subtract", "a", "b"), &nd::subtract);
	godot::ClassDB::bind_static_method("nd", D_METHOD("multiply", "a", "b"), &nd::multiply);
	godot::ClassDB::bind_static_method("nd", D_METHOD("divide", "a", "b"), &nd::divide);
	godot::ClassDB::bind_static_method("nd", D_METHOD("remainder", "a", "b"), &nd::remainder);
	godot::ClassDB::bind_static_method("nd", D_METHOD("pow", "a", "b"), &nd::pow);

	godot::ClassDB::bind_static_method("nd", D_METHOD("abs", "a"), &nd::abs);
	godot::ClassDB::bind_static_method("nd", D_METHOD("sqrt", "a"), &nd::sqrt);

	godot::ClassDB::bind_static_method("nd", D_METHOD("exp", "a"), &nd::exp);
	godot::ClassDB::bind_static_method("nd", D_METHOD("log", "a"), &nd::log);

	godot::ClassDB::bind_static_method("nd", D_METHOD("sin", "a"), &nd::sin);
	godot::ClassDB::bind_static_method("nd", D_METHOD("cos", "a"), &nd::cos);
	godot::ClassDB::bind_static_method("nd", D_METHOD("tan", "a"), &nd::tan);

	godot::ClassDB::bind_static_method("nd", D_METHOD("sum", "a"), &nd::sum, DEFVAL(nullptr), DEFVAL(nullptr));
	godot::ClassDB::bind_static_method("nd", D_METHOD("prod", "a"), &nd::sum, DEFVAL(nullptr), DEFVAL(nullptr));
	godot::ClassDB::bind_static_method("nd", D_METHOD("mean", "a"), &nd::mean, DEFVAL(nullptr), DEFVAL(nullptr));
	godot::ClassDB::bind_static_method("nd", D_METHOD("var", "a"), &nd::std, DEFVAL(nullptr), DEFVAL(nullptr));
	godot::ClassDB::bind_static_method("nd", D_METHOD("std", "a"), &nd::std, DEFVAL(nullptr), DEFVAL(nullptr));
	godot::ClassDB::bind_static_method("nd", D_METHOD("max", "a"), &nd::std, DEFVAL(nullptr), DEFVAL(nullptr));
	godot::ClassDB::bind_static_method("nd", D_METHOD("min", "a"), &nd::std, DEFVAL(nullptr), DEFVAL(nullptr));
}

nd::nd() {
}

nd::~nd() {
	// Add your cleanup here.
}

StringName nd::newaxis() {
	return ::newaxis();
}

range_part to_range_part(const Variant& variant) {
	switch (variant.get_type()) {
		case Variant::INT:
			return int64_t(variant);
		case NULL:
			return xt::placeholders::xtuph{};
		default:
			throw std::runtime_error("Invalid type for range.");
	}
}

Ref<NDRange> nd::from(int64_t start) {
	return {memnew(NDRange(start, xt::placeholders::xtuph{}, xt::placeholders::xtuph{}))};
}

Ref<NDRange> nd::to(int64_t stop) {
	return {memnew(NDRange(xt::placeholders::xtuph{}, stop, xt::placeholders::xtuph{}))};
}

Ref<NDRange> nd::range(Variant start_or_stop, Variant stop, Variant step) {
	try {
		if (stop.get_type() == Variant::Type::NIL && step.get_type() == Variant::Type::NIL) {
			return {memnew(NDRange(0, to_range_part(start_or_stop), xt::placeholders::xtuph{}))};
		}
		else if (step.get_type() == Variant::Type::NIL) {
			return {memnew(NDRange(to_range_part(start_or_stop), to_range_part(stop), xt::placeholders::xtuph{}))};
		}
		else {
			return {memnew(NDRange(to_range_part(start_or_stop), to_range_part(stop), to_range_part(step)))};
		}
	}
	catch (std::runtime_error& error) {
		ERR_FAIL_V_MSG(nullptr, error.what());
	}
}

nd::DType nd::dtype(Variant array) {
	// TODO We can totally do this without constructing an array. More code though.
	try {
		std::shared_ptr<xtv::XTVariant> existing_array = variant_as_array(array);
		return xtv::dtype(*existing_array);
	}
	catch (std::runtime_error& error) {
		ERR_FAIL_V_MSG(DTypeMax, error.what());
	}
}

uint64_t nd::size_of_dtype_in_bytes(DType dtype) {
	return xtv::size_of_dtype_in_bytes(dtype);
}

PackedInt64Array nd::shape(Variant array) {
	try {
		// TODO We can totally do this without constructing an array. More code though.
		std::shared_ptr<xtv::XTVariant> existing_array = variant_as_array(array);

		auto shape = xtv::shape(*existing_array);
		// TODO This seems a bit weird, but it works for now.
		auto packed = PackedInt64Array();
		for (auto d : shape) {
			packed.append(d);
		}
		return packed;
	}
	catch (std::runtime_error& error) {
		ERR_FAIL_V_MSG(PackedInt64Array(), error.what());
	}
}

uint64_t nd::size(Variant array) {
	try {
		// TODO We can totally do this without constructing an array. More code though.
		std::shared_ptr<xtv::XTVariant> existing_array = variant_as_array(array);

		return xtv::size(*existing_array);
	}
	catch (std::runtime_error& error) {
		ERR_FAIL_V_MSG(0, error.what());
	}
}

uint64_t nd::ndim(Variant array) {
	try {
		// TODO We can totally do this without constructing an array. More code though.
		std::shared_ptr<xtv::XTVariant> existing_array = variant_as_array(array);

		return xtv::dimension(*existing_array);
	}
	catch (std::runtime_error& error) {
		ERR_FAIL_V_MSG(0, error.what());
	}
}

Ref<NDArray> nd::as_type(Variant array, nd::DType dtype) {
	try {
		return nd::as_array(array, dtype);
	}
	catch (std::runtime_error& error) {
		ERR_FAIL_V_MSG(nullptr, error.what());
	}
}

Ref<NDArray> nd::as_array(Variant array, nd::DType dtype) {
	auto type = array.get_type();

	// Can we take a view?
	if (type == Variant::OBJECT) {
		if (auto ndarray = dynamic_cast<NDArray*>(static_cast<Object *>(array))) {
			if (dtype == nd::DType::DTypeMax || ndarray->dtype() == dtype) {
				return array;
			}
		}
	}

	// Ok, we need a copy of the data.
	return nd::array(array, dtype);
}

Ref<NDArray> nd::array(Variant array, nd::DType dtype) {
	try {
		std::shared_ptr<xtv::XTVariant> existing_array = variant_as_array(array);

		// Default value.
		if (dtype == nd::DType::DTypeMax) {
			dtype = xtv::dtype(*existing_array);
		}

		auto result = xtv::make_xarray(dtype, *existing_array);
		return Ref(memnew(NDArray(result)));
	}
	catch (std::runtime_error& error) {
		ERR_FAIL_V_MSG(Ref<NDArray>(), error.what());
	}
}

Ref<NDArray> nd::empty(Variant shape, nd::DType dtype) {
	try {
		std::vector<size_t> shape_array = variant_as_shape<size_t, std::vector<size_t>>(shape);

		return Ref(memnew(NDArray(xtv::empty(dtype, shape_array))));
	}
	catch (std::runtime_error& error) {
		ERR_FAIL_V_MSG(Ref<NDArray>(), error.what());
	}
}

template <typename V>
Ref<NDArray> _full(Variant shape, V value, nd::DType dtype) {
	try {
		std::vector<size_t> shape_array = variant_as_shape<size_t, std::vector<size_t>>(shape);

		return Ref(memnew(NDArray(xtv::full(dtype, value, shape_array))));
	}
	catch (std::runtime_error& error) {
		ERR_FAIL_V_MSG(Ref<NDArray>(), error.what());
	}
}

Ref<NDArray> nd::full(Variant shape, Variant fill_value, nd::DType dtype) {
	switch (fill_value.get_type()) {
		case Variant::INT:
			if (dtype == nd::DType::DTypeMax) dtype = nd::DType::Int64;
			return _full(shape, int64_t(fill_value), dtype);
		case Variant::FLOAT:
			if (dtype == nd::DType::DTypeMax) dtype = nd::DType::Float64;
			return _full(shape, double_t(fill_value), dtype);
		default:
			ERR_FAIL_V_MSG(Ref<NDArray>(), "The fill value must be a number literal (for now).");
	}
}

Ref<NDArray> nd::zeros(Variant shape, nd::DType dtype) {
	return _full(shape, 0, dtype);
}

Ref<NDArray> nd::ones(Variant shape, nd::DType dtype) {
	return _full(shape, 1, dtype);
}

// The first parameter is the one used by the xarray operation, while the second is used for type deduction.
// It's ok if they're the same.
template <typename FX, typename PromotionRule>
inline Ref<NDArray> binary_operation(Variant a, Variant b) {
	try {
		std::shared_ptr<xtv::XTVariant> a_ = variant_as_array(a);
		std::shared_ptr<xtv::XTVariant> b_ = variant_as_array(b);

		auto result = xtv::xoperation<PromotionRule>(XFunction<FX> {}, *a_, *b_);
		return { memnew(NDArray(result)) };
	}
	catch (std::runtime_error& error) {
		ERR_FAIL_V_MSG({}, error.what());
	}
}

Ref<NDArray> nd::add(Variant a, Variant b) {
	// godot::UtilityFunctions::print(value);
	return binary_operation<xt::detail::plus, xtv::promote::function_result<xt::detail::plus>>(a, b);
}

Ref<NDArray> nd::subtract(Variant a, Variant b) {
	return binary_operation<xt::detail::minus, xtv::promote::function_result<xt::detail::minus>>(a, b);
}

Ref<NDArray> nd::multiply(Variant a, Variant b) {
	return binary_operation<xt::detail::multiplies, xtv::promote::function_result<xt::detail::multiplies>>(a, b);
}

Ref<NDArray> nd::divide(Variant a, Variant b) {
	return binary_operation<xt::detail::divides, xtv::promote::function_result<xt::detail::divides>>(a, b);
}

Ref<NDArray> nd::remainder(Variant a, Variant b) {
	return binary_operation<xt::math::remainder_fun, xtv::promote::function_result<xt::math::remainder_fun>>(a, b);
}

Ref<NDArray> nd::pow(Variant a, Variant b) {
	return binary_operation<xt::math::pow_fun, xtv::promote::function_result<xt::math::pow_fun>>(a, b);
}


template <typename FX, typename PromotionRule>
inline Ref<NDArray> unary_operation(Variant a) {
	try {
		auto a_ = variant_as_array(a);

		auto result = xtv::xoperation<PromotionRule>(XFunction<FX> {}, *a_);
		return { memnew(NDArray(result)) };
	}
	catch (std::runtime_error& error) {
		ERR_FAIL_V_MSG({}, error.what());
	}
}

Ref<NDArray> nd::abs(Variant a) {
	return unary_operation<xt::math::abs_fun, xtv::promote::function_result<xt::math::abs_fun>>(a);
}

Ref<NDArray> nd::sqrt(Variant a) {
	return unary_operation<xt::math::sqrt_fun, xtv::promote::function_result<xt::math::sqrt_fun>>(a);
}

Ref<NDArray> nd::exp(Variant a) {
	return unary_operation<xt::math::exp_fun, xtv::promote::function_result<xt::math::exp_fun>>(a);
}

Ref<NDArray> nd::log(Variant a) {
	return unary_operation<xt::math::log_fun, xtv::promote::function_result<xt::math::log_fun>>(a);
}

Ref<NDArray> nd::sin(Variant a) {
	return unary_operation<xt::math::sin_fun, xtv::promote::function_result<xt::math::sin_fun>>(a);
}

Ref<NDArray> nd::cos(Variant a) {
	return unary_operation<xt::math::cos_fun, xtv::promote::function_result<xt::math::cos_fun>>(a);
}

Ref<NDArray> nd::tan(Variant a) {
	return unary_operation<xt::math::tan_fun, xtv::promote::function_result<xt::math::tan_fun>>(a);
}

template <typename FX, typename PromotionRule>
inline Ref<NDArray> reduction(Variant a, Variant axes) {
	try {
		auto axes_ = variant_to_axes(axes);
		auto a_ = variant_as_array(a);

		auto result = xtv::xreduction<PromotionRule>(
			FX{}, axes_, *a_
		);

		return {memnew(NDArray(result))};
	}
	catch (std::runtime_error& error) {
		ERR_FAIL_V_MSG({}, error.what());
	}
}

#define Reducer(fun_name)\
	template <typename A>\
	auto operator()(xtv::GivenAxes& axes, A&& a) const {\
		return xt::fun_name(std::forward<A>(a), axes);\
	}\
\
	template <typename A>\
	auto operator()(A&& a) const {\
		return xt::fun_name(std::forward<A>(a));\
	}

struct Sum { Reducer(sum) };

Ref<NDArray> nd::sum(Variant a, Variant axes) {
	return reduction<Sum, xtv::promote::common_type>(a, axes);
}

struct Prod { Reducer(prod) };

Ref<NDArray> nd::prod(Variant a, Variant axes) {
	return reduction<Prod, xtv::promote::at_least_int32>(a, axes);
}

struct Mean { Reducer(mean) };

Ref<NDArray> nd::mean(Variant a, Variant axes) {
	return reduction<Mean, xtv::promote::matching_float_or_default<double_t>>(a, axes);
}

struct Variance { Reducer(variance) };

Ref<NDArray> nd::var(Variant a, Variant axes) {
	return reduction<Variance, xtv::promote::matching_float_or_default<double_t>>(a, axes);
}

struct Std { Reducer(stddev) };

Ref<NDArray> nd::std(Variant a, Variant axes) {
	return reduction<Std, xtv::promote::matching_float_or_default<double_t>>(a, axes);
}

struct Amax { Reducer(amax) };

Ref<NDArray> nd::max(Variant a, Variant axes) {
	return reduction<Amax, xtv::promote::common_type>(a, axes);
}

struct Amin { Reducer(amin) };

Ref<NDArray> nd::min(Variant a, Variant axes) {
	return reduction<Amin, xtv::promote::common_type>(a, axes);
}
