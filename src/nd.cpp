#include "nd.h"

#include <godot_cpp/godot.hpp>
#include "xtensor/xtensor.hpp"
#include "xtensor/xadapt.hpp"

#include "asarray.h"
#include "asshape.h"
#include "xtv.h"

using namespace godot;

void nd::_bind_methods() {
	godot::ClassDB::bind_static_method("nd", D_METHOD("dtype", "array"), &nd::dtype);
	godot::ClassDB::bind_static_method("nd", D_METHOD("shape", "array"), &nd::shape);
	godot::ClassDB::bind_static_method("nd", D_METHOD("size", "array"), &nd::size);
	godot::ClassDB::bind_static_method("nd", D_METHOD("ndim", "array"), &nd::ndim);

	godot::ClassDB::bind_static_method("nd", D_METHOD("as_type", "array"), &nd::as_type);

	godot::ClassDB::bind_static_method("nd", D_METHOD("as_array", "array", "dtype"), &nd::as_array, DEFVAL(nullptr), DEFVAL(NDArray::DType::DTypeMax));
	godot::ClassDB::bind_static_method("nd", D_METHOD("array", "array", "dtype"), &nd::array, DEFVAL(nullptr), DEFVAL(NDArray::DType::DTypeMax));
	godot::ClassDB::bind_static_method("nd", D_METHOD("zeros", "shape", "dtype"), &nd::zeros, DEFVAL(nullptr), DEFVAL(NDArray::DType::Float64));
	godot::ClassDB::bind_static_method("nd", D_METHOD("ones", "shape", "dtype"), &nd::ones, DEFVAL(nullptr), DEFVAL(NDArray::DType::Float64));

	godot::ClassDB::bind_static_method("nd", D_METHOD("add", "a", "b"), &nd::add);
	godot::ClassDB::bind_static_method("nd", D_METHOD("subtract", "a", "b"), &nd::subtract);
	godot::ClassDB::bind_static_method("nd", D_METHOD("multiply", "a", "b"), &nd::multiply);
	godot::ClassDB::bind_static_method("nd", D_METHOD("divide", "a", "b"), &nd::divide);
}

nd::nd() {
}

nd::~nd() {
	// Add your cleanup here.
}

NDArray::DType nd::dtype(Variant array) {
	// TODO We can totally do this without constructing an array. More code though.
	std::shared_ptr<xtv::XTVariant> existing_array;
	if (!variant_as_array(array, existing_array)) {
		ERR_FAIL_V_MSG(NDArray::DType::DTypeMax, "Not an array.");
	}

	return xtv::dtype(*existing_array);
}

PackedInt64Array nd::shape(Variant array) {
	// TODO We can totally do this without constructing an array. More code though.
	std::shared_ptr<xtv::XTVariant> existing_array;
	if (!variant_as_array(array, existing_array)) {
		ERR_FAIL_V_MSG(PackedInt64Array(), "Not an array.");
	}

	auto shape = xtv::shape(*existing_array);
	// TODO This seems a bit weird, but it works for now.
	auto packed = PackedInt64Array();
	for (auto d : shape) {
		packed.append(d);
	}
	return packed;
}

uint64_t nd::size(Variant array) {
	// TODO We can totally do this without constructing an array. More code though.
	std::shared_ptr<xtv::XTVariant> existing_array;
	if (!variant_as_array(array, existing_array)) {
		ERR_FAIL_V_MSG(NDArray::DType::DTypeMax, "Not an array.");
	}

	return xtv::size(*existing_array);
}

uint64_t nd::ndim(Variant array) {
	// TODO We can totally do this without constructing an array. More code though.
	std::shared_ptr<xtv::XTVariant> existing_array;
	if (!variant_as_array(array, existing_array)) {
		ERR_FAIL_V_MSG(NDArray::DType::DTypeMax, "Not an array.");
	}

	return xtv::dimension(*existing_array);
}

Variant nd::as_type(Variant array, NDArray::DType dtype) {
	return nd::array(array, dtype);
}

Variant nd::as_array(Variant array, xtv::DType dtype) {
	auto type = array.get_type();

	// Can we take a view?
	if (type == Variant::OBJECT) {
		if (auto ndarray = dynamic_cast<NDArray*>((Object*)(array))) {
			if (dtype == xtv::DType::DTypeMax || ndarray->dtype() == dtype) {
				return array;
			}
		}
	}

	// Ok, we need a copy of the data.
	return nd::array(array, dtype);
}

Variant nd::array(Variant array, xtv::DType dtype) {
	auto type = array.get_type();

	std::shared_ptr<xtv::XTVariant> existing_array;
	if (!variant_as_array(array, existing_array)) {
		return nullptr;
	}

	if (dtype == xtv::DType::DTypeMax) {
		dtype = xtv::DType((*existing_array).index());
	}

	auto result = xtv::array(*existing_array, dtype);
	if (result == nullptr) {
		ERR_FAIL_V_MSG(nullptr, "Dtype must be set for this operation.");\
	}
	
	return Variant(memnew(NDArray(result)));
}

Variant nd::zeros(Variant shape, xtv::DType dtype) {
	std::vector<size_t> shape_array;
	if (!variant_as_shape<size_t>(shape, shape_array)) {
		return nullptr;
	}

	auto result = xtv::zeros(shape_array, dtype);
	if (result == nullptr) {
		ERR_FAIL_V_MSG(nullptr, "Dtype must be set for this operation.");\
	}
	
	return Variant(memnew(NDArray(result)));
}

Variant nd::ones(Variant shape, xtv::DType dtype) {
	std::vector<size_t> shape_array;
	if (!variant_as_shape<size_t>(shape, shape_array)) {
		return nullptr;
	}

	auto result = xtv::ones(shape_array, dtype);
	if (result == nullptr) {
		ERR_FAIL_V_MSG(nullptr, "Dtype must be set for this operation.");\
	}
	
	return Variant(memnew(NDArray(result)));
}

template <typename operation>
inline Variant binOp(Variant a, Variant b) {
	std::shared_ptr<xtv::XTVariant> a_;
	if (!variant_as_array(a, a_)) {
		return nullptr;
	}
	std::shared_ptr<xtv::XTVariant> b_;
	if (!variant_as_array(b, b_)) {
		return nullptr;
	}

	return Variant(memnew(NDArray(xtv::operation<operation>(*a_, *b_))));
}

Variant nd::add(Variant a, Variant b) {
	// godot::UtilityFunctions::print(xt::has_simd_interface<xt::xarray<int64_t>>::value);
	// godot::UtilityFunctions::print(xt::has_simd_type<xt::xarray<int64_t>>::value);
	return binOp<xtv::Add>(a, b);
}

Variant nd::subtract(Variant a, Variant b) {
	return binOp<xtv::Subtract>(a, b);
}

Variant nd::multiply(Variant a, Variant b) {
	return binOp<xtv::Multiply>(a, b);
}

Variant nd::divide(Variant a, Variant b) {
	return binOp<xtv::Divide>(a, b);
}
